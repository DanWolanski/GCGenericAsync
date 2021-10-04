#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#include <conio.h>
#define _WINSOCKAPI_
#include <windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#endif
#include <fcntl.h>
#include <signal.h>
#include <time.h>

// Dialogic headers
#include "srllib.h"
#include "dtilib.h" 
#include "gclib.h"
#include "gcerr.h"
#include "gcip.h"
#include "cnflib.h"
#include "devmgmt.h"
#include "ipmlib.h"
#include "cclib.h"

#include "Log.h"


#define APPNAME	"GenericAsync.cpp"
#define MAXCHAN		500
#define MAXCONF		500
#define MAX_DEVNAME	100
#define MAX_STR_SIZE 1024



#ifndef WIN32
#include <strings.h>

#define Sleep usleep
#define dx_fileopen open
#define dx_fileclose close

#define stricmp strcasecmp

#define O_BINARY 0x00

#define USREV_TESTEVT 0x00ff

#include <sys/select.h>

int kbhit(void)
{
struct timeval tv;
fd_set read_fd;

tv.tv_sec=0;
tv.tv_usec=0;
FD_ZERO(&read_fd);
FD_SET(0,&read_fd);

if(select(1, &read_fd, NULL, NULL, &tv) == -1)
return 0;

if(FD_ISSET(0,&read_fd))
return 1;

return 0;
}
int getch(void){
	int fd = fileno(stdin);

	char c = '\0';
	int e = read(fd, &c, 1);
	if (e == 0) c = '\0';

	return c;
}
#include <termios.h>
#include <unistd.h>

int getch2(void)
{
struct termios oldt,
newt;
int ch;
tcgetattr( STDIN_FILENO, &oldt );
newt = oldt;
newt.c_lflag &= ~( ICANON | ECHO );
tcsetattr( STDIN_FILENO, TCSANOW, &newt );
ch = getchar();
tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
return ch;
}
#endif

static int keeplooping=1;  //used to signal exit time
static long srlmode=SR_STASYNC;
static int  pollmode=1;
static int numevtsProcessed=0;
static int usedtidevs=0;
static long termtime=-1;
static int gcdevcount=24;
static int IPMdevcount=0;
static int voxdevcount=24;
static DWORD AppStartTickCount=0;
static int NumCh=1;
static char playfile[MAX_STR_SIZE]={"music.pcm"};


int SIPPORT= 5060;
int setsipip = 0;
char sipip[128];
char sipipinbo[128];

int outboundmode = 0;
char outboundaddr[1024];

#define SRWAITTIMEOUT 0


#define USREV_STARTTEST		0x7777
#define USREV_EXITAPP		0x7666
#define USREV_CALLCONNECT	0x7555

static struct channel {
	int					index;
	char				devname[MAX_DEVNAME];	/* Argument to nEx() function						*/
	int					voxh;
	long				gch;
	int					ipmh;
	char				ipmname[MAX_STR_SIZE];
	int					fd; 
	char				recfile[MAX_STR_SIZE];
	DX_IOTT				iott;     /* I/O transfer table */  
	DV_TPT				tpt[3];       /* termination parameter table */  
	DX_XPB				xpb;       /* I/O transfer parameter block */
	DX_CAP				cap;
	DV_DIGIT			digbuf;
    int					voxactive;
	long				ts ;
	SC_TSINFO			scts ;
	char				ldevname[MAX_STR_SIZE];
	DWORD				funcstarttime;
	CRN					crn;
	//3pcc
	IP_CAPABILITY * preferredCap;
	char					medianame[MAX_DEVNAME];	/* Argument to gc_OpenEx() function						*/
	int mediaactive;

	int					state;
	
} port[MAXCHAN];



void StartTest(void);
void AppExit(void);

static void MakeCall(int index,char *destnum);

/////////////
static void StartTest( ){
	LOG_ENTRYEXIT("StartTest");
	static int started=0;
	int index;
	//int starttime;
	
	
	
	if(started != 0){
		return;
	}
	else{
		started=1;
	}

	if (outboundmode) {
		for (int i = 0; i < gcdevcount; i++) {

			MakeCall(i, outboundaddr);
		}
	}
	

}

int GetIndexFromDevh( int devh){
	//first check Vox
	int i;

//	for(i=0;i<NumCh;i++){
//		if(devh == port[i].devh) {
//			Log(DEBUG,i,"GetIndexFromDevh(%d)=%d",devh,i);
//			return i;
//		}
//	}
	for(i=0;i<voxdevcount;i++){
		if(devh == port[i].voxh) {
			Log(DEBUG,i,"GetIndexFromDevh(%d)=%d",devh,i);
			return i;
		}
	}

	for(i=0;i<IPMdevcount;i++){
		if(devh == port[i].ipmh) {
			Log(DEBUG,i,"GetIndexFromDevh(%d)=%d",devh,i);
			return i;
		}
	}
	// check GC
	for(i=0;i<gcdevcount;i++){
		if(devh == port[i].gch) {
			Log(DEBUG,i,"GetIndexFromDevh(%d)=%d",devh,i);
			return i;
		}
	}
// Check MMs
	Log(DEBUG,i,"GetIndexFromDevh(%d)=NOINDEX",devh);

	return NOINDEX;

}


static void ShowHelp( void){
				printf("Usage:\n");
				printf("  %s [-?] [-ip ipaddress]\n",APPNAME);
				printf("Where:\n");
				printf(" -?				prints the help\n");
				printf(" -#				sets the #channels (Default=%s)\n",NumCh);
				printf(" -i 			The IP address to use for the SIP traffic\n");
				
				

}


//
// Function: AppInit
// Description: This state is used to initialize the application.  The 
//     command lines are parsed here and all data structures and global 
//	   variables are initialized
//									
static void AppInit(int argc, char *argv[]){
	LOG_ENTRYEXIT("AppInit");
	int i=0;
	int loglevel;
	int srandseed;


	//memsets the entire channel info struct to 0
	memset(port,0,sizeof(struct channel) * MAXCHAN);

	srandseed=GetTickCount();
	srand(srandseed);
	Log(DEBUG,NOINDEX,"Setting Random Number generator, seed=%d",srandseed);
	//Set numch to default of 1
	NumCh=1;
	srlmode=SR_MTASYNC;
	pollmode=0;
	termtime=-1;

	AppStartTickCount=GetTickCount();

	//Place command line arguments parsing here
	for(i=1; i<argc; i++){

		if(argv[i][0] != '-'){
			printf("Error parsing command line args, arguments must start with -\n");
			printf(" use -? option for usage\n");
			exit(0);
		}
		switch (argv[i][1]){
			case '?':
				ShowHelp();
				exit(0);
				break;
			case 'o':
				outboundmode=1;
				i++;
				strcpy(outboundaddr,argv[i]);
				Log(DEBUG,NOINDEX,"Setting to outbound mode, outbound address = %s",outboundaddr);
				break;
			case 'i':
				setsipip = 1;
				i++;
				strcpy(sipip, argv[i]);
				Log(DEBUG, NOINDEX, "Setting sip IP address to %s", sipip);
				break;
			case '#':
				i++;
				if(i <= argc){  
					NumCh = atoi(argv[i]);
					voxdevcount = NumCh;
					gcdevcount = NumCh;
					}
				Log(DEBUG,NOINDEX,"Setting # channels to %d",NumCh);
				break;
			default:
				printf("Unknown command line argument passed.  Use -? for usage\n");
				break;
		}
	}


}
#ifdef WIN32
void intr_hdlr()
#else
void intr_hdlr(int x)
#endif
{

LOG_ENTRYEXIT("intr_hdlr");

Log(EVENT,NOINDEX,"Ctrl-C pressed, exiting");	
keeplooping=0;	

 	
	
}

void enable_int_handers( void){

LOG_ENTRYEXIT("enable_int_handers");

keeplooping=1;
#ifdef WIN32
	signal(SIGINT, (void (__cdecl*)(int)) intr_hdlr);
	signal(SIGTERM, (void (__cdecl*)(int)) intr_hdlr);
#else
	signal(SIGINT,  intr_hdlr);
	signal(SIGTERM,  intr_hdlr);
#endif
	
	
}
///////////////////////////////// VOICE FUNCTIONS //////////////////////////////
static void RecFile(int index,char *afilename=NULL){
	LOG_ENTRYEXIT("RecFile");
	char *filename;	

	if(afilename == NULL){
		Log(INFO,index,"afilename is NULL using default recfile (%s)",playfile);
		filename=port[index].recfile;
	}
	else{
		Log(INFO,index,"setting filename via arg to %s",afilename);
		filename=afilename;
	}

			

			port[index].iott.io_type= IO_EOT|IO_DEV;      /* Transfer type */
			
			port[index].iott.io_offset=0;    /* File/Buffer offset */
			port[index].iott.io_length=-1;    /* Length of data */
			port[index].iott.io_bufp=NULL;     /* Pointer to base memory */
			port[index].iott.io_nextp=NULL;    /* Pointer to next DX_IOTT if IO_LINK */
			port[index].iott.io_prevp=NULL;    /* (optional) Pointer to previous DX_IOTT */ 	
			
			port[index].xpb.wFileFormat    = FILE_FORMAT_VOX ;
			port[index].xpb.wDataFormat    = DATA_FORMAT_MULAW ;
			port[index].xpb.nSamplesPerSec = DRT_8KHZ ;
			port[index].xpb.wBitsPerSample = 8 ;
			port[index].fd=dx_fileopen(filename,O_RDWR|O_CREAT|O_TRUNC,666);

			CRC(port[index].fd,"dx_fileopen"){
				Log(ERROR,index,"Error opening %s for recording",filename);
			}

			port[index].iott.io_fhandle = port[index].fd;

			Log(API,index,"Starting rec of %s (fd=%d)",filename,port[index].fd);
			CRC( dx_reciottdata(port[index].voxh,&(port[index].iott),NULL,&(port[index].xpb),EV_ASYNC),"dx_reciottdata"){
				Log(ERROR,index,"ERRMSGP = %s",ATDV_ERRMSGP(port[index].voxh));
			}
			port[index].voxactive =1;

}

static void PlayFile(int index, char *afilename=NULL){
	LOG_ENTRYEXIT("PlayFile");
	char *filename;

	if(afilename == NULL){
		Log(INFO,index,"afilename is NULL using default playfile (%s)",playfile);
		filename=playfile;
	}
	else{
		Log(INFO,index,"setting filename via arg to %s",afilename);
		filename=afilename;
	}

			
			
			port[index].iott.io_type= IO_EOT|IO_DEV;      /* Transfer type */
			
			port[index].iott.io_offset=0;    /* File/Buffer offset */
			port[index].iott.io_length=-1;    /* Length of data */
			port[index].iott.io_bufp=NULL;     /* Pointer to base memory */
			port[index].iott.io_nextp=NULL;    /* Pointer to next DX_IOTT if IO_LINK */
			port[index].iott.io_prevp=NULL;    /* (optional) Pointer to previous DX_IOTT */ 	
			
			port[index].xpb.wFileFormat    = FILE_FORMAT_VOX ;
			port[index].xpb.wDataFormat    = DATA_FORMAT_MULAW ;
			port[index].xpb.nSamplesPerSec = DRT_8KHZ ;
			port[index].xpb.wBitsPerSample = 8 ;

		  port[index].tpt[0].tp_type = IO_EOT;
		  port[index].tpt[0].tp_termno = DX_MAXDTMF;
		  port[index].tpt[0].tp_length =  1;
		  port[index].tpt[0].tp_flags = TF_MAXDTMF;

		
			port[index].fd=dx_fileopen(filename,O_RDWR|O_BINARY,666);
			CRC(port[index].fd,"dx_fileopen"){
				AppExit();
			}

			port[index].iott.io_fhandle = port[index].fd;
		  
			Log(API,index, "Clearing Digbuf");
		  	CRC(dx_clrdigbuf(port[index].voxh),"dx_clrdigbuf");


			CRC( dx_playiottdata(port[index].voxh,&(port[index].iott),port[index].tpt,&(port[index].xpb),EV_ASYNC),"dx_playiottdata");

			Log(API,index,"Starting play of %s (fd=%d)",filename,port[index].fd);
			port[index].voxactive =1;
}

static void MRecFile(int index,int outindex, char *afilename=NULL){
	LOG_ENTRYEXIT("MRecFile");
	char *filename;	
	SC_TSINFO tsinfo; 
	long scts1,scts2; 
	long tslots[32];

	if(afilename == NULL){
		Log(INFO,index,"afilename is NULL using default recfile ");
		filename=port[index].recfile;
	}
	else{
		Log(INFO,index,"setting filename via arg to %s",afilename);
		filename=afilename;
	}

			

			port[index].iott.io_type= IO_EOT|IO_DEV;      /* Transfer type */
			
			port[index].iott.io_offset=0;    /* File/Buffer offset */
			port[index].iott.io_length=-1;    /* Length of data */
			port[index].iott.io_bufp=NULL;     /* Pointer to base memory */
			port[index].iott.io_nextp=NULL;    /* Pointer to next DX_IOTT if IO_LINK */
			port[index].iott.io_prevp=NULL;    /* (optional) Pointer to previous DX_IOTT */ 	
			
			port[index].xpb.wFileFormat    = FILE_FORMAT_VOX ;
			port[index].xpb.wDataFormat    = DATA_FORMAT_MULAW ;
			port[index].xpb.nSamplesPerSec = DRT_8KHZ ;
			port[index].xpb.wBitsPerSample = 8 ;
			port[index].fd=dx_fileopen(filename,O_RDWR|O_CREAT|O_TRUNC,666);

			CRC(port[index].fd,"dx_fileopen"){
				Log(ERROR,index,"Error opening %s for recording",filename);
			}

			port[index].iott.io_fhandle = port[index].fd;

			tsinfo.sc_numts = 1;  
		   tsinfo.sc_tsarrayp = &scts1;  
			CRC(gc_GetXmitSlot(port[index].gch, &tsinfo),"gc_GetXmitSlot");
		   Log(API,index,"gc_GetXmitSlot(%s) Success (ts=%d)",port[index].devname,scts1);

			tsinfo.sc_numts = 1;  
		   tsinfo.sc_tsarrayp = &scts2;  
			CRC(gc_GetXmitSlot(port[outindex].gch, &tsinfo),"gc_GetXmitSlot");
		   Log(API,index,"gc_GetXmitSlot(%s) Success (ts=%d)",port[outindex].devname,scts2);
		  
			tslots[0]=scts1;
			tslots[1]=scts2;

			tsinfo.sc_numts = 2;  
		  	tsinfo.sc_tsarrayp = &tslots[0];			

			Log(API,index,"Starting rec of %s (fd=%d)",filename,port[index].fd);
			CRC( dx_mreciottdata(port[index].voxh,&(port[index].iott),NULL,&(port[index].xpb),EV_ASYNC,&tsinfo),"dx_mreciottdata"){
				Log(ERROR,index,"ERRMSGP = %s",ATDV_ERRMSGP(port[index].voxh));
			}
			port[index].voxactive =1;

}

static void GetDTMF(int index){
	LOG_ENTRYEXIT("GetDTMF");
	

		
/* Set to terminate  number of digits */  
			 
		  port[index].tpt[0].tp_type = IO_CONT;
		  port[index].tpt[0].tp_termno = DX_MAXDTMF;
		  port[index].tpt[0].tp_length =  1;
		  port[index].tpt[0].tp_flags = TF_MAXDTMF;
		  
		  port[index].tpt[1].tp_type = IO_EOT;
		  port[index].tpt[1].tp_termno = DX_MAXTIME;
		  port[index].tpt[1].tp_length =  600;
		  port[index].tpt[1].tp_flags = TF_MAXTIME;


		  Log(API,index, "Starting dx_getdig (Tpt = 1dig and Maxtime 60sec)");


		  CRC(dx_getdig(port[index].voxh,port[index].tpt,&(port[index].digbuf),EV_ASYNC),"dx_getdig");


}

void OpenVox(int NumCh){
	LOG_ENTRYEXIT("OpenVox");
	int index;

	for(index=0;index<NumCh;index++){

		port[index].index=index;

			sprintf(port[index].devname,"dxxxB%dC%d",((index)/4)+1,((index)%4)+1);
			Log(API,index,"Opening, dx_open( %s )",port[index].devname);
		port[index].voxh = dx_open(port[index].devname,0);
			CRC(port[index].voxh,"dx_open()");
		
		Log(INFO,index,"%s Opened, voxh=%d",port[index].devname,port[index].voxh);
	
		dx_stopch(port[index].voxh,EV_SYNC);
		sprintf(port[index].recfile,"%s.pcm",port[index].devname);
		Log(INFO,index,"Set recfile to %s",port[index].recfile);

		Log(INFO,index,"Setting event mask to turn on Digit CST evtents");
		CRC(dx_setevtmsk(port[index].voxh,DM_DIGITS),"dx_setevtmsk(DM_DIGITS)");

	}

	

}


void AppExit( void ){

	LOG_ENTRYEXIT("AppExit");
	int index;
	static int stopping=0;
	
	
	Log(ALWAYS,NOINDEX,"Application Exiting....");
	
	if(stopping != 0){
		return;
	}
	else{
		stopping=1;
	}

	keeplooping=0;

	for(index=0;index<NumCh;index++){
		Log(API,index,"Closing VOX Device, dx_close( %s )",port[index].devname);
		dx_close(port[index].voxh);
		
		if(port[index].fd != NULL){
			Log(API,index,"Closing open file, dx_fileclose( %d )",port[index].fd);
			dx_fileclose(port[index].fd);

		}

	
			
	}

	
	Log(ALWAYS,NOINDEX,"All Cleanup Done, exiting in 2 seconds");
	Sleep(2000);
	exit(0);
}
////////////////////////////// GC Functions ///////////////////////////////////////////

void OpenGC(int NumCh){
	LOG_ENTRYEXIT("OpenGC");
	int index=0;


	GC_START_STRUCT gc_start;     // Structure for use with gc_Start()
	CCLIB_START_STRUCT  cc_start[2];
	IPCCLIB_START_DATA ip_libData;
    IP_VIRTBOARD    ip_virtBoard[1];
	GC_PARM_BLK			  *parmblkp = NULL;
	

	memset((void *) &gc_start, 0, sizeof(GC_START_STRUCT));

	cc_start[0].cclib_name = "GC_IPM_LIB";
	cc_start[0].cclib_data = NULL;
	cc_start[1].cclib_name = "GC_H3R_LIB";
	cc_start[1].cclib_data = &ip_libData;

	INIT_IP_VIRTBOARD(ip_virtBoard);
	INIT_IPCCLIB_START_DATA(&ip_libData, 1, ip_virtBoard);

	ip_virtBoard[0].sip_msginfo_mask = IP_SIP_MIME_ENABLE | IP_SIP_MSGINFO_ENABLE;
	ip_virtBoard[0].sup_serv_mask = IP_SUP_SERV_CALL_XFER; 
	ip_virtBoard[0].sip_msginfo_mask = IP_SIP_MSGINFO_ENABLE;
	Log(API,NOINDEX,"SIP PORT SET TO %d",SIPPORT);
	ip_virtBoard[0].sip_signaling_port = SIPPORT;
	if (setsipip) {
		Log(API, NOINDEX, "SIP IP SET TO %s", sipip);
		ip_virtBoard[0].localIP.ip_ver = IPVER4;
		/*For an IPv4 address, the address must be stored in memory using the network byte order (big
		endian) rather than the little-endian byte order of the Dialogic® architecture. A socket API,
		htonl( ), is available to convert from host byte order to network byte order. As an example, to
		specify an IP address of 127.10.20.30, you may use either of the following C statements:
		ipv4 = 0x1e140a7f -oripv4 = htonl(0x7f0a141e)
		For more information on the byte order of IPv4 addresses, see RFC 791 and RFC 792*/
		
		inet_pton(AF_INET, sipip, &ip_virtBoard[0].localIP.u_ipaddr.ipv4);
	}
	ip_virtBoard[0].h323_max_calls = IP_CFG_NO_CALLS;
	ip_virtBoard[0].sip_max_calls = NumCh;
	ip_virtBoard[0].total_max_calls=NumCh;


	ip_libData.board_list = ip_virtBoard;
	gc_start.cclib_list = cc_start;
	gc_start.num_cclibs = 2;

	Log(API,NOINDEX,"Calling GC_START");

	// Start the GC library
	CRC(gc_Start( &gc_start), "gc_Start"){
		GC_INFO gc_error_info;
		gc_ErrorInfo(&gc_error_info);
		Log(ERROR, NOINDEX, " Error: gc_Start Value: 0x%hx - %s", gc_error_info.gcValue, gc_error_info.gcMsg);
		AppExit();
	}


	// Setting the DTMF Transport Mode
	CRC(gc_util_insert_parm_val(&parmblkp, IPSET_DTMF, IPPARM_SUPPORT_DTMF_BITMASK,
													//sizeof(char), IP_DTMF_TYPE_INBAND_RTP);
													//IP_DTMF_TYPE_RFC_2833,
													sizeof(char), IP_DTMF_TYPE_RFC_2833),"gc_util_insert");
	for(index=0;index < NumCh; index++){
	
	
		sprintf(port[index].ldevname,":P_SIP:N_iptB1T%d:M_ipmB1C%d\0",  index+1,index+1);

		Log(API,index,"GCOpening device %s", port[index].ldevname);

		port[index].crn =0;
		CRC(gc_OpenEx(&(port[index].gch),port[index].ldevname,	EV_SYNC,NULL),"gc_OpenEx"){
		//if(gc_OpenEx(&(port[index].gch),openstr,	EV_ASYNC,(void *)&port[index]) != GC_SUCCESS){
			AppExit();
		}
		Log(API,index,"gc_Open Successful... %s is gch %d",port[index].ldevname,port[index].gch);
		sprintf(port[index].ipmname,"ipmB1C%d",index+1);
		CRC(gc_GetResourceH(port[index].gch,&(port[index].ipmh),GC_MEDIADEVICE ),"gc_GetResourceH"){
			AppExit();
		}
		Log(API,index,"gc_GetResourceH... %s is ipmh %d",port[index].ipmname,port[index].ipmh);


		CRC(gc_SetUserInfo(GCTGT_GCLIB_CHAN, port[index].gch, parmblkp, GC_ALLCALLS),"gc_SetUserInfo()RFC2833)");

		Log(API,index, "gc_SetUserInfo(linedev=%ld) Success - DTMF mode is RFC2833", port[index].gch);
	}
	

					gc_util_delete_parm_blk(parmblkp);


}

static void MakeCall(int index,char *destnum){

	LOG_ENTRYEXIT("MakeCall");
	GC_MAKECALL_BLK gcmkbl ={0};
GCLIB_MAKECALL_BLK gclib_mkbl;
   gcmkbl.cclib = NULL;
   gcmkbl.gclib = &gclib_mkbl;
   GC_PARM_BLK *l_datap = NULL;
   memset(&gclib_mkbl, 0, sizeof(GCLIB_MAKECALL_BLK));

    char *pDisplay = "TestApp";


		GC_PARM_BLKP pGcParmBlk = NULL;
	int duration = GC_SINGLECALL;
	IP_CAPABILITY capability = {0};
	char l_callerId[IP_SIP_CALLIDSIZE] = {0};
	
	Log(API,index,"Setting Codecs to G.711U 20ms");
	//Initialize capability information
	capability.type = GCCAPTYPE_AUDIO;
	capability.direction = IP_CAP_DIR_LCLTRANSMIT;
	
	capability.capability = GCCAP_AUDIO_g711Ulaw64k;
	capability.extra.audio.frames_per_pkt = 20;
	capability.payload_type = IP_USE_STANDARD_PAYLOADTYPE;
	capability.extra.audio.VAD = GCPV_DISABLE;

	CRC(gc_util_insert_parm_ref(&pGcParmBlk,GCSET_CHAN_CAPABILITY,
		IPPARM_LOCAL_CAPABILITY, sizeof(IP_CAPABILITY),
		&capability) ,"gc_uitil_insert_parm_ref(IP_CAP)");	
	
	capability.direction = IP_CAP_DIR_LCLRECEIVE;
	CRC(gc_util_insert_parm_ref(&pGcParmBlk,GCSET_CHAN_CAPABILITY,
		IPPARM_LOCAL_CAPABILITY,sizeof(IP_CAPABILITY),
		&capability) ,"gc_uitil_insert_parm_ref(IP_CAP2)");	

	
	CRC(gc_SetUserInfo(GCTGT_GCLIB_CHAN,port[index].gch, pGcParmBlk, duration),"gc_SetUserInfo");
	
 // set GCLIB_ADDRESS_BLK with destination string & type
    strcpy(gcmkbl.gclib->destination.address, destnum);
   gcmkbl.gclib->destination.address_type = GCADDRTYPE_TRANSPARENT;
   gcmkbl.gclib->origination.address_type = GCADDRTYPE_TRANSPARENT;
//  set protocol for the call

   gc_util_insert_parm_val(&l_datap, IPSET_PROTOCOL, IPPARM_PROTOCOL_BITMASK,
                     sizeof(char), IP_PROTOCOL_SIP);

    // insert display string
    gc_util_insert_parm_ref(&l_datap,IPSET_CALLINFO, IPPARM_DISPLAY,
                               (unsigned char)(strlen(pDisplay)+1), pDisplay);

	
	
	Log(API,index," Calling %s",destnum);

	CRC(gc_MakeCall(port[index].gch,&(port[index].crn),NULL,&gcmkbl,90,EV_ASYNC),"gc_MakeCall");
	gc_util_delete_parm_blk(pGcParmBlk);
}
static void AcceptCall(int index){
	LOG_ENTRYEXIT("AcceptCall");


		GC_PARM_BLKP pGcParmBlk = NULL;
	int duration = GC_SINGLECALL;
	IP_CAPABILITY capability = {0};
	char l_callerId[IP_SIP_CALLIDSIZE] = {0};
	
	Log(API,index,"Setting Codecs to G.711U 20ms");
	//Initialize capability information
	capability.type = GCCAPTYPE_AUDIO;
	capability.direction = IP_CAP_DIR_LCLTRANSMIT;
	
	capability.capability = GCCAP_AUDIO_g711Ulaw64k;
	capability.extra.audio.frames_per_pkt = 20;
	capability.payload_type = IP_USE_STANDARD_PAYLOADTYPE;
	capability.extra.audio.VAD = GCPV_DISABLE;

	CRC(gc_util_insert_parm_ref(&pGcParmBlk,GCSET_CHAN_CAPABILITY,
		IPPARM_LOCAL_CAPABILITY, sizeof(IP_CAPABILITY),
		&capability) ,"gc_uitil_insert_parm_ref(IP_CAP)");	
	
	capability.direction = IP_CAP_DIR_LCLRECEIVE;
	CRC(gc_util_insert_parm_ref(&pGcParmBlk,GCSET_CHAN_CAPABILITY,
		IPPARM_LOCAL_CAPABILITY,sizeof(IP_CAPABILITY),
		&capability) ,"gc_uitil_insert_parm_ref(IP_CAP2)");	

	
	CRC(gc_SetUserInfo(GCTGT_GCLIB_CHAN,port[index].gch, pGcParmBlk, duration),"gc_SetUserInfo");
	
	CRC(gc_GetCallInfo(port[index].crn, DESTINATION_ADDRESS, l_callerId),"gc_getCallInfo");

	Log(API,index,"Caller ID is %s",l_callerId);
		
	Log(API,index,"Accepting Call 0x%x",port[index].crn);
	CRC(gc_AcceptCall(port[index].crn,0,EV_ASYNC),"gc_AcceptCall");
	

	gc_util_delete_parm_blk(pGcParmBlk);
}


static void AnswerCall(int index){
	LOG_ENTRYEXIT("AnswerCall");


	CRC(gc_AnswerCall(port[index].crn,0,EV_ASYNC),"gc_AnswerCall");	

	
}

static void WaitCall(int index){
	LOG_ENTRYEXIT("WaitCall");


	CRC(gc_WaitCall(port[index].gch,NULL,NULL,-1,EV_ASYNC),"gc_WaitCall");

	
}
static void DropCall(int index){
	LOG_ENTRYEXIT("DropCall");


	CRC(gc_DropCall(port[index].crn, GC_NORMAL_CLEARING, EV_ASYNC),"gc_DropCall");

	
}
static void ReleaseCall(int index){
	LOG_ENTRYEXIT("ReleaseCall");


	CRC(gc_ReleaseCallEx(port[index].crn,EV_ASYNC),"gc_ReleaseCallEx");

	
}
static void RouteMedia(int index){
	LOG_ENTRYEXIT("RouteMedia");
	 SC_TSINFO      sc_tsinfo;        /* CTbus time slot structure */  
	 SC_TSINFO      tsinfo;        /* CTbus time slot structure */  
    long scts; 

	if(index < 0){
		Log(ERROR,index,"Invalid index passed, %d",index);
		return;
	}
	
	Log(DEBUG,index,"Routing index is %d ( %s and %s)",index,port[index].devname,port[index].ldevname);

	tsinfo.sc_numts = 1;  
    tsinfo.sc_tsarrayp = &scts;  
	CRC(gc_GetXmitSlot(port[index].gch, &tsinfo),"gc_GetXmitSlot");

	Log(API,index,"gc_GetXmitSlot() Success (ts=%d)",scts);
	   
	CRC(dx_listen(port[index].voxh, &tsinfo),"dx_listen");
	Log(API,index,"dx_listen() Success (ts=%d)",scts);

	sc_tsinfo.sc_numts = 1;  
    sc_tsinfo.sc_tsarrayp = &scts;  
    /* Get CTbus time slot connected to transmit of voice  
       channel on board ...1 */  
    CRC(dx_getxmitslot(port[index].voxh, &sc_tsinfo),"dx_getxmitslot")
    {  
		AppExit();
    }
	Log(API,index,"%s is xmitting to %d, attaching %s",port[index].devname,scts,port[index].ldevname);

	port[index].funcstarttime=GetTickCount();
	CRC(gc_Listen(port[index].gch,&sc_tsinfo,EV_ASYNC),"gc_Listen"){
		AppExit();
	}

}
static void UnRouteMedia(int index){
	LOG_ENTRYEXIT("UnRouteMedia");
	 

	if(index < 0){
		Log(ERROR,index,"Invalid index passed, %d",index);
		return;
	}
	
	Log(DEBUG,index,"UnRouting index is %d ( %s and %s)",index,port[index].devname,port[index].ldevname);

	port[index].funcstarttime=GetTickCount();
	CRC(gc_UnListen(port[index].gch,EV_ASYNC),"gc_Unlisten"){
		AppExit();
	}

}


void OnCallConnect(int index){
	LOG_ENTRYEXIT("OnCallConnect");

	PlayFile(index,"music.pcm");

}

#ifdef WIN32
static long int ProcessEvt(unsigned long q){
#else
static long  ProcessEvt(void * parm ){
int q=0;
#endif
	DWORD CBHTime=GetTickCount();
	LOG_ENTRYEXIT("ProcessEvt");
	int devh=sr_getevtdev();
	int evttype=sr_getevttype();
	long evtlen = sr_getevtlen();
	long *datap = (long *)sr_getevtdatap();
	char str[MAX_LOG_STR_SIZE];
	int index=GetIndexFromDevh(devh);
	CNF_PARTY_INFO *pInfo ;
	SRL_DEVICE_HANDLE hParty ;
	CNF_OPEN_PARTY_RESULT *pOpenResult ;
	DWORD	endtime=GetTickCount();
	long	delta;
	int i=0;
	char tmpstr[1024];
	METAEVENT metaevent;
   struct channel		*pline;

	CRC(gc_GetMetaEvent(&metaevent),"gc_GetMetaEvent");

	
	if(!Evt2Str(devh,evttype,datap,str) ){
		//If we don't know the evt str check for customer events
		Log(EVENT,index,"%s RECVD (0x%x), evtlen=%d devh=%d",
				str,evttype,evtlen,devh);
		

	}else{

		Log(EVENT,index,"--%s RECVD (0x%x), evtlen=%d (masked=%d) devh=%d",
				str,evttype,evtlen,evtlen&0x7FFFFFFF,devh);
	}
	if(keeplooping != 1){
		return 0;
	}


	pline=&port[index];
	switch(evttype){
		
		case USREV_EXITAPP:
			AppExit( );
			break;
		case USREV_STARTTEST:
			Log(EVENT,index,"USREV_STARTTEST RECVD, devh=%d, evttype=%d(0x%x)",
				devh,evttype,evttype);
			StartTest( );
			break;
		case USREV_CALLCONNECT:
			OnCallConnect(index);
			break;
// GC EVENTS
		case GCEV_UNBLOCKED:
			Log(API,index,"Putting channel in waitcall state (gc_Waitcall)");
			WaitCall(index);
			break;
		case GCEV_OFFERED:
			port[index].crn=metaevent.crn;
			Log(API,index,"OFFERED call detected, crn=0x%x",port[index].crn);
			AcceptCall(index);
			break;
		case GCEV_ACCEPT:
			Log(API,index,"Call Accepted.. answering");
			AnswerCall(index);
			break;
		case GCEV_ANSWERED:
			Log(API,index,"Call Answered, starting media");
			//Connected now, route in vox dev
			RouteMedia(index);
			break;
		case GCEV_DISCONNECTED:
			Log(API,index,"Dropping call (gc_DropCall) crn=0x%x",port[index].crn);
			DropCall(index);
			break;
		case GCEV_DROPCALL:
			Log(API,index,"Releasing Call (gc_ReleaseCall) crn=0x%x",port[index].crn);
			ReleaseCall(index);
			break;
		case GCEV_RELEASECALL:
			Log(API,index,"Call Released (crn=0x%x), channel is waiting for a new call",port[index].crn);
			UnRouteMedia(index);
			break;
		case GCEV_LISTEN:
			Log(EVENT,index,"GCEV_LISTEN RECVD(0x%x),devh=%d, delta=%dmS",evttype,devh,GetTickCount()-port[index].funcstarttime);
			CRC( sr_putevt(port[index].gch, USREV_CALLCONNECT, 0, NULL, 0) , "sr_putevt(EV_CALLCONNECT)");
			break;
		case GCEV_UNLISTEN:
			Log(EVENT,index,"GCEV_UNLISTEN RECVD(0x%x),devh=%d, delta=%dmS",evttype,devh,GetTickCount()-port[index].funcstarttime);
		  	break;
		case GCEV_CONNECTED:
			Log(API,index,"Call Connected, starting media");
			//Connected now, route in vox dev
			RouteMedia(index);
			break;
		// VOICE events
		case TDX_PLAY:
			Log(INFO,index,"TDX_PLAY Received");
			CRC( dx_fileclose(port[index].fd),"dx_fileclose");
			port[index].fd=NULL;
			port[index].voxactive=0;
			break;
		case TDX_GETDIG:
			//Note here you sould check to see if you got timeout or digit..
			// Timeout or invalid should loop.  
			Log(INFO,index,"TDX_GETDIG Received, DigBuf=%s",port[index].digbuf.dg_value);
			port[index].voxactive=0;
			break;
		case TDX_CST:
			DX_CST *cstp;
			cstp=(DX_CST *)sr_getevtdatap();
			switch(cstp->cst_event){
				case DE_DIGITS:
					Log(EVENT,index,"DE_DIGIT event = Digit pressed = %c",cstp->cst_data);
					break;
				default:
					Log(EVENT,index,"No processing enabled for CST event");
					break;

			}
			
			break;
		case TDX_RECORD:
			Log(INFO,index,"TDX_RECORD Received");
			CRC( dx_fileclose(port[index].fd),"dx_fileclose");
			port[index].fd=NULL;
			port[index].voxactive=0;
			break;
	
		default:
			Log(ERROR,index,"Unhandled event (%d) (0x%x)",evttype,evttype);
			break;
	}

	delta=GetTickCount()-CBHTime;
	Log(INFO,NOINDEX,"Time Spent inside CBhandler = %dmS ",delta );
	
	//consume the event
	return 0;
}

static void LibInit( void ){
	LOG_ENTRYEXIT("LibInit");
	int index=NOINDEX;
	

	//next we have to init the SRL mode. 
	
	CRC(sr_setparm(SRL_DEVICE, SR_MODELTYPE, &srlmode), "sr_setparm"){exit(0);}
	Log(API,NOINDEX,"Setting SRL mode to %s",SRLMode2Str(srlmode));
	

	
	// Lets enable our event handlers for any event 
	// on any device we want to call ProcessEvt
	if(pollmode){
		Log(API,NOINDEX,"IN Pollmode, no event handlers enabled");
	}
	else{
		CRC(sr_enbhdlr( EV_ANYDEV, EV_ANYEVT, ProcessEvt ) , "sr_enbhdlr");
		Log(API,NOINDEX,"Event hander ProcessEvt Enabled (EV_ANYDEV/EV_ANYEVT)");
	}
   

}
void SampleThread( void *targs){
	int rc;
	int index=(int)targs;
	int devh= port[index].gch;
	int lsleeptime;

		srand(GetCurrentThreadId());
		lsleeptime=(rand() % 10000) + 5000;
		
	
		//Note for now don't use LOG functions in these threads bad things happen
		printf("[%d] Thread started ThreadID = 0x%x \n",devh,GetCurrentThreadId());
		printf("[%d] Sleeping for %d ms\n",devh,lsleeptime);
		Sleep(lsleeptime);
		//rc=dx_stopch(devh,EV_ASYNC);
		//printf("[%d] Just issued Stop\n",devh);
		//if(rc==-1){
		//	printf("Error in stop %s\n",ATDV_ERRMSGP(devh));
		//}

		//while(!oktoexit[index]) Sleep(10);
		//printf("[%d] Exiting Thread",devh);
		
}
void StartStopThread(int index){


	//_beginthread(SampleThread,0,(void *)index);

}
void ProcessKey( int key){
	LOG_ENTRYEXIT("ProcessKey");
	int index=NOINDEX;
	char dest[120];

	switch(key){
		//case 'p':
		//case 'P':
		// put Play() func here that calls dx_playiottdata etc
		case 'q':
		case 'Q':
		case 27:  //esc key
			
			keeplooping=0;
			break;
		
		default:
			Log(INFO,index,"%c key pressed",key);
			}
}
int main(int argc, char *argv[]){
	LOG_SETLOGLEVEL(LOGALL);
	LOG_FILE_INIT("GenericAsync.log");
	LOG_ENTRYEXIT("Main");
	char key;
	int index=NOINDEX;


	// process command line args
	AppInit(argc,argv);

		
	LibInit();
	
	// enable Ctrl-C handlers
	enable_int_handers( );
	
	// Open all the Vox
	OpenVox(voxdevcount);
	
	OpenGC(gcdevcount);


   Log(INFO,NOINDEX,"*** Init done- Entering Wait loop  ***");
   Log(INFO,NOINDEX,"*** press 'q' to exit... ***");
   
   //Posting start Event
   Log(ALWAYS,NOINDEX,"Init finished, Starting Test!");
   CRC( sr_putevt(SRL_DEVICE, USREV_STARTTEST, 0, NULL, 0) , "sr_putevt(EV_STARTTEST)");
  


	while(keeplooping){
		if(srlmode == SR_STASYNC){
			if(pollmode){
				if(sr_waitevt(SRWAITTIMEOUT) != -1){
					ProcessEvt(0);
				}
			}
			else{
				while(-1 != sr_waitevt(SRWAITTIMEOUT));
			}
			
		}
			
		if(kbhit()){
			key=getch();
			ProcessKey( key );
		}
		
	}
	// put this here to collect final stats in case of starvation

	AppExit( );
	
	return 0;
}