//THis is for loging

#ifndef WIN32
#include <sys/time.h>
#else
#include <windows.h>
#endif 

#include <time.h>
#include <stdarg.h>

#pragma warning( disable : 4390 )
#define MAX_LOG_STR_SIZE 1024
#define LOGNAME "App.log"

// Log levels
#define ALWAYS	-1 
#define ERROR	0
#define EVENT	2
#define API		4
#define INFO	6
#define ENTRY	8
#define EXIT	10
#define DEBUG	12
#define LOGALL	20

#define NOINDEX -1

#ifndef WIN32
#define DWORD int
#endif
static int dindex=NOINDEX;  // this is a global so that if there is no index in scope
						   // logging macros will not fail
static int LogLevel = LOGALL;

static void Log(int level, int dindex, const char* Format, ...);
static char *LogLevel2Str(int level);
class CAppLog  
{
public:
	CAppLog();
	virtual ~CAppLog();
	void Write(char *LogStr);
	void Open(char *logname,int level,FILE *fh);
	void Close();

private:
	int mLogLevel;
	char mLogName[MAX_LOG_STR_SIZE];
	FILE *mLogfh;
};


CAppLog::CAppLog():
mLogfh(NULL)
{
	
}
void CAppLog::Close(){
	Log(DEBUG,NOINDEX,"Closing log file %s",mLogName);
	fclose(mLogfh);
	mLogfh=NULL;
}
void CAppLog::Open(char *logname=LOGNAME,int level=LOGALL,FILE *fh=NULL )
{
	mLogLevel=level;
	strcpy(mLogName,logname);

	if (mLogfh==NULL){

		mLogfh = fopen(mLogName, "wt");
		if (!mLogfh)
		{
			printf("Error opening log file %s\n",mLogName);
			exit(1);
		}

	}

	Log(DEBUG,NOINDEX,"Log file %s opened, Level=%s",
		mLogName,
		LogLevel2Str(mLogLevel));
	

}

void CAppLog::Write(char *LogStr){
		fprintf(mLogfh, "%s\n", LogStr);
		fflush(mLogfh);
}

CAppLog::~CAppLog()
{
	
	
	if(mLogfh){
		Close();
	}

}

static CAppLog *gAppLog=NULL;
static CAppLog AppLog;
static int log_crc;
#define CRC(rc,logstr)\
	log_crc=rc;\
	if(log_crc == -1){ Log(ERROR,dindex,"Error in %s",logstr);} \
	else{ Log(API,index,"%s issued successfully",logstr);}\
	if(log_crc == -1)

#define LOG_FILE_INIT(szLogName)\
	gAppLog=&AppLog;\
	gAppLog->Open(szLogName,LogLevel);\
	
#define LOG_SETLOGLEVEL(level)\
	LogLevel=level;\
	Log(ALWAYS,NOINDEX,"Setting LogLevel to %s",LogLevel2Str(LogLevel));

#define LOG_ENTER(szFunc)\
	 char FuncName[MAX_LOG_STR_SIZE] = szFunc;\
	 Log(ENTRY,dindex,"Entering %s",FuncName);

#define LOG_EXIT()\
 Log(EXIT,dindex,"Exiting %s",FuncName);


#define LOG_ENTRYEXIT(szFunc)\
	 CFuncLogger flogger(szFunc,dindex);

#ifndef WIN32
void Log(int level, int dindex, const char* Format, ...)
{

	char LogStr[MAX_LOG_STR_SIZE] = {0};
	va_list ArgList;
	struct timeval  tv;
	struct tm      *tm;

	uint32_t         start;
 
 

	if(level > LogLevel){
		return;
	}

	gettimeofday(&tv, NULL);
	tm = localtime(&tv.tv_sec);
	sprintf(LogStr+strlen(LogStr), 
					"%02d:%02d:%02d.%03d ",
					tm->tm_hour,
					tm->tm_min,
					tm->tm_sec,
					tv.tv_usec/1000);


	sprintf(LogStr+strlen(LogStr),"%s   ",LogLevel2Str(level));
	
	if(dindex == NOINDEX){
		sprintf(LogStr+strlen(LogStr),"XXX| ");
	}
	else{
		sprintf(LogStr+strlen(LogStr),"%03d| ",dindex);
	}
	va_start(ArgList, Format);
	vsprintf(LogStr+strlen(LogStr), Format, ArgList);

	
	printf("%s\n", LogStr);
	fflush(stdout);

	
	if (gAppLog!=NULL){
		gAppLog->Write(LogStr);

	}

#else
void Log(int level, int index, const char* Format, ...)
{
char LogStr[MAX_LOG_STR_SIZE] = {0};
	va_list ArgList;
	SYSTEMTIME SystemTime;


	if(level > LogLevel){
		return;
	}

	::GetLocalTime(&SystemTime);
	sprintf(LogStr+strlen(LogStr), 
					"%02d:%02d:%02d.%03d ",
					SystemTime.wHour,
					SystemTime.wMinute,
					SystemTime.wSecond,
					SystemTime.wMilliseconds);


	sprintf(LogStr+strlen(LogStr),"%s   ",LogLevel2Str(level));
	
	if(index == NOINDEX){
		sprintf(LogStr+strlen(LogStr),"XXX| ");
	}
	else{
		sprintf(LogStr+strlen(LogStr),"%03d| ",index);
	}
	va_start(ArgList, Format);
	vsprintf(LogStr+strlen(LogStr), Format, ArgList);

	
	printf("%s\n", LogStr);
	fflush(stdout);

	
	if (gAppLog!=NULL){
		gAppLog->Write(LogStr);

	}
#endif
}

char *DXTerm2Str(long lTermMask)
	{
	if(lTermMask & TM_DIGIT) 		 return "TM_DIGIT";
	else if(lTermMask & TM_MAXDTMF)	 return "TM_MAXDTMF";
	else if(lTermMask & TM_MAXSIL)	 return "TM_MAXSIL";
	else if(lTermMask & TM_MAXNOSIL) return "TM_MAXNOSIL";
	else if(lTermMask & TM_LCOFF)	 return "TM_LCOFF";
	else if(lTermMask & TM_IDDTIME)	 return "TM_IDDTIME";
	else if(lTermMask & TM_MAXTIME)	 return "TM_MAXTIME";
	else if(lTermMask & TM_USRSTOP)	 return "TM_USRSTOP";
	else if(lTermMask & TM_TONE)	 return "TM_TONE";
	else if(lTermMask & TM_EOD)		 return "TM_EOD";
	else if(lTermMask & TM_ERROR)	 return "TM_ERROR";
	else if(lTermMask & TM_PATTERN)	 return "TM_PATTERN";
	if (lTermMask == 0) return "Normal Termination";
	return "";
	}

char *SRLMode2Str(int mode){
	switch(mode)
		{
#ifdef SR_STASYNC
	case  SR_STASYNC    : return "SR_STASYNC / SR_POLLMODE";
	case  SR_MTASYNC      : return "SR_MTASYNC / SR_SIGMODE";
	case  SR_MTSYNC       : return "SR_MTSYNC";
#else
	case SR_POLLMODE    : return "SR_STASYNC / SR_POLLMODE";
#endif
	
	default				: return "Unknown SRLMODE";
		}
	}

char *CPTerm2Str(long lCPTermMask)
	{
	switch(lCPTermMask)
		{
	case  CR_BUSY       : return "CR_BUSY";
	case  CR_NOANS      : return "CR_NOANS";
	case  CR_NORB       : return "CR_NORB";
	case  CR_CNCT       : return "CR_CNCT";
	case  CR_CEPT       : return "CR_CEPT";
	case  CR_STOPD      : return "CR_STOPD";
	case  CR_NODIALTONE : return "CR_NODIALTONE";
	case  CR_FAXTONE    : return "CR_FAXTONE";
	case  CR_ERROR      : return "CR_ERROR";
	default				: return "Unknown CP termination";
		}
	}

char *CSTEvt2Str(unsigned short ushCSTEvt)
	{
	switch(ushCSTEvt)
		{
	case  DE_RINGS     : return "DE_RINGS";
	case  DE_SILON     : return "DE_SILON";
	case  DE_SILOF     : return "DE_SILOF";
	case  DE_LCON      : return "DE_LCON";
	case  DE_LCOF      : return "DE_LCOF";
	case  DE_WINK      : return "DE_WINK";
	case  DE_RNGOFF    : return "DE_RNGOFF";
	case  DE_DIGITS    : return "DE_DIGITS";
	case  DE_DIGOFF    : return "DE_DIGOFF";
	case  DE_LCREV     : return "DE_LCREV";
	case  DE_TONEON    : return "DE_TONEON";
	case  DE_TONEOFF   : return "DE_TONEOFF";
	case  DE_STOPRINGS : return "DE_STOPRINGS";
	default			   : return "Unknown CST event";
		}
	}

char *CnfEventToStr(unsigned int iEvent)
{
	switch(iEvent)
	{
	case CNFEV_OPEN                    : return "CNFEV_OPEN";
	case CNFEV_OPEN_CONF               : return "CNFEV_OPEN_CONF";
	case CNFEV_OPEN_PARTY              : return "CNFEV_OPEN_PARTY";
	case CNFEV_ADD_PARTY               : return "CNFEV_ADD_PARTY";
	case CNFEV_REMOVE_PARTY            : return "CNFEV_REMOVE_PARTY";
	case CNFEV_GET_ATTRIBUTE           : return "CNFEV_GET_ATTRIBUTE";
	case CNFEV_SET_ATTRIBUTE           : return "CNFEV_SET_ATTRIBUTE";
	case CNFEV_ENABLE_EVENT            : return "CNFEV_ENABLE_EVENT";
	case CNFEV_DISABLE_EVENT           : return "CNFEV_DISABLE_EVENT";
	case CNFEV_GET_DTMF_CONTROL        : return "CNFEV_GET_DTMF_CONTROL";
	case CNFEV_SET_DTMF_CONTROL        : return "CNFEV_SET_DTMF_CONTROL";
	case CNFEV_GET_ACTIVE_TALKER       : return "CNFEV_GET_ACTIVE_TALKER";
	case CNFEV_GET_PARTY_LIST          : return "CNFEV_GET_PARTY_LIST";
	case CNFEV_GET_DEVICE_COUNT        : return "CNFEV_GET_DEVICE_COUNT";
	case CNFEV_CONF_OPENED             : return "CNFEV_CONF_OPENED";
	case CNFEV_CONF_CLOSED             : return "CNFEV_CONF_CLOSED";
	case CNFEV_PARTY_ADDED             : return "CNFEV_PARTY_ADDED";
	case CNFEV_PARTY_REMOVED           : return "CNFEV_PARTY_REMOVED";
	case CNFEV_DTMF_DETECTED           : return "CNFEV_DTMF_DETECTED";
	case CNFEV_ACTIVE_TALKER           : return "CNFEV_ACTIVE_TALKER";
	case CNFEV_ERROR                   : return "CNFEV_ERROR";
	case CNFEV_OPEN_FAIL               : return "CNFEV_OPEN_FAIL";
	case CNFEV_OPEN_CONF_FAIL          : return "CNFEV_OPEN_CONF_FAIL";
	case CNFEV_OPEN_PARTY_FAIL         : return "CNFEV_OPEN_PARTY_FAIL";
	case CNFEV_ADD_PARTY_FAIL          : return "CNFEV_ADD_PARTY_FAIL";
	case CNFEV_REMOVE_PARTY_FAIL       : return "CNFEV_REMOVE_PARTY_FAIL";
	case CNFEV_GET_ATTRIBUTE_FAIL      : return "CNFEV_GET_ATTRIBUTE_FAIL";
	case CNFEV_SET_ATTRIBUTE_FAIL      : return "CNFEV_SET_ATTRIBUTE_FAIL";
	case CNFEV_ENABLE_EVENT_FAIL       : return "CNFEV_ENABLE_EVENT_FAIL";
	case CNFEV_DISABLE_EVENT_FAIL      : return "CNFEV_DISABLE_EVENT_FAIL";
	case CNFEV_GET_DTMF_CONTROL_FAIL   : return "CNFEV_GET_DTMF_CONTROL_FAIL";
	case CNFEV_SET_DTMF_CONTROL_FAIL   : return "CNFEV_SET_DTMF_CONTROL_FAIL";
	case CNFEV_GET_ACTIVE_TALKER_FAIL  : return "CNFEV_GET_ACTIVE_TALKER_FAIL";
	case CNFEV_GET_PARTY_LIST_FAIL     : return "CNFEV_GET_PARTY_LIST_FAIL";
	case CNFEV_GET_DEVICE_COUNT_FAIL   : return "CNFEV_GET_DEVICE_COUNT_FAIL";
	case CNFEV_RESET_DEVICES		   : return "CNFEV_RESET_DEVICES";
	default							   : return "Unknown CNF event";
	}
}

int Evt2Str(int hDev, long lEvtType, long *lEvtDatap, char *Str)
	{
	DX_CST *pCST;
	long lTermMask=0, lCPTermMask=0;

	if ((lEvtType >= TDX_PLAY) && (lEvtType <= TDX_GETR2MF))
		{
		lTermMask = ATDX_TERMMSK(hDev);
		}

	switch(lEvtType)
		{
	// dxxxlib events
		case TDX_PLAY:
			strcpy (Str,  "TDX_PLAY");
			sprintf(Str + strlen(Str), " Data 0x%lx %s",
									  lTermMask, DXTerm2Str(lTermMask));
			break;
		case TDX_RECORD:
			strcpy (Str,  "TDX_RECORD");
			sprintf(Str + strlen(Str), " Data 0x%lx %s",
									  lTermMask, DXTerm2Str(lTermMask));
			break;
		case TDX_GETDIG:
			strcpy (Str,  "TDX_GETDIG");
			sprintf(Str + strlen(Str), " Data 0x%lx %s",
									  lTermMask, DXTerm2Str(lTermMask));
			break;
		case TDX_DIAL:
			strcpy (Str,  "TDX_DIAL");
			sprintf(Str + strlen(Str), " Data 0x%lx %s",
									  lTermMask, DXTerm2Str(lTermMask));
			break;
		case TDX_CALLP:
			strcpy (Str,  "TDX_CALLP");
			lCPTermMask = ATDX_CPTERM(hDev);
			sprintf(Str + strlen(Str), " Data 0x%lx %s", 
								lCPTermMask, CPTerm2Str(lCPTermMask));
			if (lCPTermMask == CR_ERROR)
				sprintf(Str + strlen(Str), " 0x%lx", ATDX_CPERROR(hDev));
			break;
		case TDX_CST:
			strcpy (Str, "TDX_CST");
			pCST = (DX_CST *) lEvtDatap;
			if(pCST == NULL)
			break;
			sprintf(Str + strlen(Str), "Data 0x%x %s", 
			  	(BYTE) (pCST->cst_data & 0xff), CSTEvt2Str(pCST->cst_event));
			break;
		case TDX_SETHOOK:
			strcpy (Str,  "TDX_SETHOOK");
			pCST = (DX_CST *) lEvtDatap;
			if(pCST == NULL)
				break;
			switch(pCST->cst_event)
			{
			case DX_OFFHOOK:
				sprintf(Str + strlen(Str), "Data 0x%x Offhook", pCST->cst_event);
				break;
			case DX_ONHOOK:
				sprintf(Str + strlen(Str), "Data 0x%x Onhook", pCST->cst_event);
				break;
			} // End switch(pCST->cst_event)
			break;
		case TDX_WINK:
			strcpy (Str,  "TDX_WINK");
			break;
		case TDX_PLAYTONE:
			strcpy (Str,  "TDX_PLAYTONE");
			sprintf(Str + strlen(Str), " Data 0x%lx %s",
									  lTermMask, DXTerm2Str(lTermMask));
			break;
		case TDX_GETR2MF:
			strcpy (Str,  "TDX_GETR2MF");
			break;
		case TDX_ERROR:
			strcpy (Str,  "TDX_ERROR");
			sprintf(Str + strlen(Str), " %s", ATDV_ERRMSGP(hDev));
			break;
		case TDX_BARGEIN:
			strcpy (Str,  "TDX_BARGEIN");
			break;
		case DX_ATOMIC_ERR:
			strcpy (Str,  "DX_ATOMIC_ERR");
			break;

	// ISDN events
		case CCEV_TASKFAIL      :
			strcpy (Str,  "CCEV_TASKFAIL");
			break;
		case CCEV_ANSWERED      :
			strcpy (Str,  "CCEV_ANSWERED");
			break;
		case CCEV_CALLPROGRESS  :
			strcpy (Str,  "CCEV_CALLPROGRESS");
			break;
		case CCEV_ACCEPT        :
			strcpy (Str,  "CCEV_ACCEPT");
			break;
		case CCEV_DROPCALL      :
			strcpy (Str,  "CCEV_DROPCALL");
			break;
		case CCEV_RESTART       :
			strcpy (Str,  "CCEV_RESTART");
			break;
		case CCEV_CALLINFO      :
			strcpy (Str,  "CCEV_CALLINFO");
			break;
		case CCEV_REQANI        :
			strcpy (Str,  "CCEV_REQANI");
			break;
		case CCEV_SETCHANSTATE  :
			strcpy (Str,  "CCEV_SETCHANSTATE");
			break;
		case CCEV_FACILITY_ACK  :
			strcpy (Str,  "CCEV_FACILITY_ACK");
			break;
		case CCEV_FACILITY_REJ  :
			strcpy (Str,  "CCEV_FACILITY_REJ");
			break;
		case CCEV_MOREDIGITS    :
			strcpy (Str,  "CCEV_MOREDIGITS");
			break;
		case CCEV_SETBILLING    :
			strcpy (Str,  "CCEV_SETBILLING");
			break;
		case CCEV_ALERTING      :
			strcpy (Str,  "CCEV_ALERTING");
			break;
		case CCEV_CONNECTED     :
			strcpy (Str,  "CCEV_CONNECTED");
			break;
		case CCEV_ERROR         :
			strcpy (Str,  "CCEV_ERROR");
			break;
		case CCEV_OFFERED       :
			strcpy (Str,  "CCEV_OFFERED");
			break;
		case CCEV_DISCONNECTED  :
			strcpy (Str,  "CCEV_DISCONNECTED");
			break;
		case CCEV_PROCEEDING    :
			strcpy (Str,  "CCEV_PROCEEDING");
			break;

		case CCEV_USRINFO       :
			strcpy (Str,  "CCEV_USRINFO");
			break;
		case CCEV_FACILITY      :
			strcpy (Str,  "CCEV_FACILITY");
			break;
		case CCEV_CONGESTION    :
			strcpy (Str,  "CCEV_CONGESTION");
			break;
		case CCEV_D_CHAN_STATUS :
			strcpy (Str,  "CCEV_D_CHAN_STATUS");
			break;
		case CCEV_NOUSRINFOBUF  :
			strcpy (Str,  "CCEV_NOUSRINFOBUF");
			break;
		case CCEV_NOFACILITYBUF :
			strcpy (Str,  "CCEV_NOFACILITYBUF");
			break;
		case CCEV_BLOCKED       :
			strcpy (Str,  "CCEV_BLOCKED");
			break;
		case CCEV_UNBLOCKED     :
			strcpy (Str,  "CCEV_UNBLOCKED");
			break;
		case CCEV_ISDNMSG       :
			strcpy (Str,  "CCEV_ISDNMSG");
			break;
		case CCEV_NOTIFY        :
			strcpy (Str,  "CCEV_NOTIFY");
			break;
		case CCEV_L2FRAME       :
			strcpy (Str,  "CCEV_L2FRAME");
			break;
		case CCEV_L2BFFRFULL    :
			strcpy (Str,  "CCEV_L2BFFRFULL");
			break;
		case CCEV_L2NOBFFR      :
			strcpy (Str,  "CCEV_L2NOBFFR");
			break;

		case CCEV_SETUP_ACK:
			strcpy (Str,  "CCEV_SETUP_ACK");
			break;

		case CCEV_DIVERTED:
			strcpy (Str,  "CCEV_DIVERTED");
			break;
		case CCEV_HOLDCALL:
			strcpy (Str,  "CCEV_HOLDCALL");
			break;
		case CCEV_HOLDACK:
			strcpy (Str,  "CCEV_HOLDACK");
			break;
		case CCEV_HOLDREJ:
			strcpy (Str,  "CCEV_HOLDREJ");
			break;
		case CCEV_NSI:
			strcpy (Str,  "CCEV_NSI");
			break;
		case CCEV_RETRIEVECALL:
			strcpy (Str,  "CCEV_RETRIEVECALL");
			break;
		case CCEV_RETRIEVEACK:
			strcpy (Str,  "CCEV_RETRIEVEACK");
			break;
		case CCEV_RETRIEVEREJ:
			strcpy (Str,  "CCEV_RETRIEVEREJ");
			break;
		case CCEV_TRANSFERACK:
			strcpy (Str,  "CCEV_TRANSFERACK");
			break;
		case CCEV_TRANSFERREJ:
			strcpy (Str,  "CCEV_TRANSFERREJ");
			break;
		case CCEV_TRANSIT:
			strcpy (Str,  "CCEV_TRANSIT");
			break;
		case CCEV_RESTARTFAIL:
			strcpy (Str,  "CCEV_RESTARTFAIL");
			break;
		case CCEV_TERM_REGISTER :
			strcpy(Str,"CCEV_TERM_REGISTER");
			break;
		case CCEV_RCVTERMREG_ACK :
			strcpy(Str,"CCEV_RCVTERMREG_ACK");
			break;
		case CCEV_RCVTERMREG_NACK :
			strcpy(Str,"CCEV_RCVTERMREG_NACK");
			break;


		case CCEV_FACILITYNULL   :
			strcpy(Str,"NULL (Dummy) CRN Facility IE");
			break;
		case CCEV_INFOGLOBAL  :
			strcpy(Str,"GLOBAL CRN Information IE");
			break;
		case CCEV_NOTIFYGLOBAL  :
			strcpy(Str,"GLOBAL CRN Notify IE");
			break;
		case CCEV_FACILITYGLOBAL :
			strcpy(Str,"GLOBAL CRN Facility IE");
			break;
		case CCEV_DROPACK  :
			strcpy(Str,"DROP Request Acknowledgement message");
			break;

		//these events are from gclib.h
		case GCEV_ATTACH:
			strcpy(Str,"media device successfully attached");
			break;
		case GCEV_ATTACH_FAIL:
			strcpy(Str,"failed to attach media device");
			break;
		case GCEV_DETACH:
			strcpy(Str,"media device successfully detached");
			break;
		case GCEV_DETACH_FAIL:
			strcpy(Str,"failed to detach media device");
			break;
		case GCEV_MEDIA_REQ:
			strcpy(Str,"Remote end is requesting media channel");
			break;
		case GCEV_STOPMEDIA_REQ:
			strcpy(Str,"Remote end is requesting media streaming stop");
			break;
		case GCEV_MEDIA_ACCEPT:
			strcpy(Str,"Media channel established with peer");
			break;
		case GCEV_MEDIA_REJ:
			strcpy(Str,"Failed to established media channel with peer");
			break;
		case GCEV_OPENEX:
			strcpy(Str,"Device Opened successfully");
			break;
		case GCEV_OPENEX_FAIL:
			strcpy(Str,"Device failed to Open");
			break;

		case GCEV_CALLSTATUS :
			strcpy(Str,"End of call status");
			break;
		case GCEV_MEDIADETECTED :
			strcpy(Str,"CPA: Media detected");
			break;

		case GCEV_ACKCALL      :
			strcpy (Str,  "Termination event for callack");
			break;
		case GCEV_SETUPTRANSFER:
			strcpy(Str,"Ready for making consultation call");
			break;
		case GCEV_COMPLETETRANSFER:
			strcpy(Str,"Transfer completed successfully");
			break;
		case GCEV_SWAPHOLD: 
			strcpy(Str,"Call on hold swapped with active call");
			break;
		case GCEV_BLINDTRANSFER:
			strcpy(Str,"Call transferred to consultation call");
			break;
		case GCEV_LISTEN:
			strcpy(Str,"Channel (listen) connected to SCbus timeslot");
			break;
		case GCEV_UNLISTEN:
			strcpy(Str,"Channel (unlisten) disconnected from SCbus timeslot");
			break;
		case GCEV_DETECTED:
			strcpy(Str,"ISDN NOTIFYNULL message received/GC Incoming call detected");
			break;
		case GCEV_FATALERROR:
			strcpy(Str,"Fatal error has occurred");
			break;
		case GCEV_RELEASECALL :
			strcpy(Str,"GCEV_RELEASECALL (Call released)");
			break;
		case GCEV_RELEASECALL_FAIL :
			strcpy(Str,"Failed to release call");
			break;
		case GCEV_DIALTONE:
			strcpy(Str,"The call has transitioned to GCST_DialTone state");
			break;
		case GCEV_DIALING:
			strcpy(Str,"The call has transitioned to GCST_Dialing state");
			break;
		case GCEV_ALARM:/* NOTE: this alarm is disabled by default */
			strcpy(Str,"An alarm occurred");
			break;
		case GCEV_MOREINFO:
            strcpy(Str, "Status of information requested\received");
			break;
		case GCEV_SENDMOREINFO:
			strcpy(Str, "More information sent to the network");
			break;
		case GCEV_CALLPROC:
            strcpy(Str, "Call acknowledged to indicate that it is now proceeding");
			break;
		case GCEV_NODYNMEM:
			strcpy(Str,"No dynamic memory available");
			break;
		case GCEV_EXTENSION:
			strcpy(Str,"Extension event");
			break;
		case GCEV_EXTENSIONCMPLT:
			strcpy(Str,"Termination event for gc_Extension()");
			break;
		case GCEV_GETCONFIGDATA:
			strcpy(Str,"Configuration data successfully retrieved");
			break;
		case GCEV_GETCONFIGDATA_FAIL:
			strcpy(Str,"Failed to retrieve configuration data");
			break;
		case GCEV_SETCONFIGDATA:
			strcpy(Str,"Configuration data successfully set");
			break;
		case GCEV_SETCONFIGDATA_FAIL:
			strcpy(Str,"Failed to set configuration data");
			break;
		case GCEV_SERVICEREQ:
			strcpy(Str,"Service Request received");
			break;
		case GCEV_SERVICERESP:
			strcpy(Str,"Service Response received");
			break;
		case GCEV_SERVICERESPCMPLT:
			strcpy(Str,"Service Response sent");
			break;

		case GCEV_INVOKE_XFER_ACCEPTED:
			strcpy(Str,"Invoke transfer accepted by the remote party");
			break;
		case GCEV_INVOKE_XFER_REJ:
			strcpy(Str,"Invoke transfer rejected by the remote party");
			break;
		case GCEV_INVOKE_XFER:
			strcpy(Str,"Successful completion of invoke transfer");
			break;
		case GCEV_INVOKE_XFER_FAIL:
			strcpy(Str,"Failure in invoke transfer");
			break;
		case GCEV_REQ_XFER:
			strcpy(Str,"Receiving a call transfer request");
			break;
		case GCEV_ACCEPT_XFER:
			strcpy(Str,"Successfully accept the transfer request from remote party");
			break;
		case GCEV_ACCEPT_XFER_FAIL:
			strcpy(Str,"Failure to accept the transfer request from remote party");
			break;
		case GCEV_REJ_XFER:
			strcpy(Str,"Successfully reject the transfer request from remote party");
			break;
		case GCEV_REJ_XFER_FAIL:
			strcpy(Str,"Failure to reject the transfer request");
			break;
		case GCEV_XFER_CMPLT:
			strcpy(Str,"Successful completion of call transfer at the party receiving the request");
			break;
		case GCEV_XFER_FAIL:
			strcpy(Str,"Failure to reroute a transferred call");
			break;
		case GCEV_INIT_XFER:
			strcpy(Str,"Successful completion of transfer initiate");
			break;
		case GCEV_INIT_XFER_REJ:
			strcpy(Str,"Transfer initiate rejected by the remote party");
			break;
		case GCEV_INIT_XFER_FAIL:
			strcpy(Str,"Failure in transfer initiate");
			break;
		case GCEV_REQ_INIT_XFER:
			strcpy(Str,"Receiving a transfer initiate request");
			break;
		case GCEV_ACCEPT_INIT_XFER:
			strcpy(Str,"Successfully accept the transfer initiate request");
			break;
		case GCEV_ACCEPT_INIT_XFER_FAIL:
			strcpy(Str,"Failure to accept the transfer initiate request");
			break;
		case GCEV_REJ_INIT_XFER:
			strcpy(Str,"Successfully reject the transfer initiate request");
			break;
		case GCEV_REJ_INIT_XFER_FAIL:
			strcpy(Str,"Failure to reject the transfer initiate request");
			break;
		case GCEV_TIMEOUT:
			strcpy(Str,"Notification of generic time out");
			break;

		case GCEV_FACILITYREQ:
			strcpy(Str,"A facility request is made by CO");
			break;

		case GCEV_TRACEDATA:
			strcpy(Str,"Tracing Data");
			break;

		//CNF EVENTS
		case CNFEV_OPEN:                    
		case CNFEV_OPEN_CONF  :
		case CNFEV_OPEN_PARTY :
		case CNFEV_ADD_PARTY:
		case CNFEV_REMOVE_PARTY:
		case CNFEV_GET_ATTRIBUTE:
		case CNFEV_SET_ATTRIBUTE:
		case CNFEV_ENABLE_EVENT:
		case CNFEV_DISABLE_EVENT:
		case CNFEV_GET_DTMF_CONTROL:
		case CNFEV_SET_DTMF_CONTROL:
		case CNFEV_GET_ACTIVE_TALKER :
		case CNFEV_GET_PARTY_LIST:
		case CNFEV_GET_DEVICE_COUNT:
		case CNFEV_CONF_OPENED:
		case CNFEV_CONF_CLOSED :
		case CNFEV_PARTY_ADDED:
		case CNFEV_PARTY_REMOVED:
		case CNFEV_DTMF_DETECTED:
		case CNFEV_ACTIVE_TALKER:
		case CNFEV_ERROR:
		case CNFEV_OPEN_FAIL  :
		case CNFEV_OPEN_CONF_FAIL :
		case CNFEV_OPEN_PARTY_FAIL:
		case CNFEV_ADD_PARTY_FAIL:
		case CNFEV_REMOVE_PARTY_FAIL :
		case CNFEV_GET_ATTRIBUTE_FAIL:
		case CNFEV_SET_ATTRIBUTE_FAIL:
		case CNFEV_ENABLE_EVENT_FAIL :
		case CNFEV_DISABLE_EVENT_FAIL:
		case CNFEV_GET_DTMF_CONTROL_FAIL:
		case CNFEV_SET_DTMF_CONTROL_FAIL :
		case CNFEV_GET_ACTIVE_TALKER_FAIL:
		case CNFEV_GET_PARTY_LIST_FAIL:
		case CNFEV_GET_DEVICE_COUNT_FAIL:
		case CNFEV_RESET_DEVICES:
			strcpy (Str,  CnfEventToStr((unsigned int)lEvtType));
			break;

		// Device management
		case DMEV_CONNECT:
			strcpy (Str,  "DMEV_CONNECT");
			break;
		case DMEV_CONNECT_FAIL:                      
			strcpy (Str,  "DMEV_CONNECT_FAIL");
			break;
		case DMEV_DISCONNECT:                        
			strcpy (Str,  "DMEV_DISCONNECT");
			break;
		case DMEV_DISCONNECT_FAIL:                   
			strcpy (Str,  "DMEV_DISCONNECT_FAIL");
			break;
		case DMEV_GET_RESOURCE_RESERVATIONINFO:      
			strcpy (Str,  "DMEV_GET_RESOURCE_RESERVATIONINFO");
			break;
		case DMEV_GET_RESOURCE_RESERVATIONINFO_FAIL: 
			strcpy (Str,  "DMEV_GET_RESOURCE_RESERVATIONINFO_FAIL");
			break;

		default:
			strcpy(Str, "Unknown Event");
			return -1;
			break;
		}	// switch

		return 0;
}
int Str2LogLevel( char *str){

	if(strcmp("ERROR",str) == 0)	return(ERROR);
	else if(strcmp("EVENT",str) == 0)	return(EVENT);
	else if(strcmp("API",str) == 0)	return(API);
	else if(strcmp("INFO",str) == 0)	return(INFO);
	else if(strcmp("ENTRY",str) == 0)	return(ENTRY);
	else if(strcmp("EXIT",str) == 0)	return(EXIT);
	else if(strcmp("DEBUG",str) == 0)	return(DEBUG);
	else if(strcmp("ALL",str) == 0)	return(LOGALL);

	return(LOGALL);
	
}
char *LogLevel2Str(int level){

switch(level){
	case ERROR:
		return("[ERROR]");
		break;
	case EVENT:
		return("[EVENT]");
		break;
	case API:
		return("[ API ]");
		break;
	case INFO:
		return("[INFO ]");
		break;
	case ENTRY:
		return("[ENTRY]");
		break;
	case EXIT:
		return("[EXIT ]");
		break;
	case DEBUG:
		return("[DEBUG]");
		break;
	case LOGALL:
		return("[ ALL ]");
		break;
	default:
		return("[     ]");
		break;

	}

}


class CFuncLogger  
{
public:
	CFuncLogger(char *funcname,int dindex);
	virtual ~CFuncLogger();

private:
	char mFuncName[MAX_LOG_STR_SIZE];
	int mIndex;
	DWORD mEnterTime;
};


CFuncLogger::CFuncLogger(char *funcname="FUNCTION",int dindex=NOINDEX):
mIndex(dindex),
mEnterTime(0)
{
#ifdef WIN32
	mEnterTime=GetTickCount();
#endif
	strcpy(mFuncName,funcname);
	Log(ENTRY,mIndex,"Entering %s",mFuncName);
}

CFuncLogger::~CFuncLogger()
{
#ifdef WIN32
	Log(EXIT,mIndex,"Exiting %s, function time=%d",mFuncName,GetTickCount()-mEnterTime);
#else
	Log(EXIT,mIndex,"Exiting %s",mFuncName);
#endif
}
