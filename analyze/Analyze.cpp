
#include "Analyze.h"
#include "interface.h"
#include "TcpCom.h"
#include "human.h"
#include "region.h"
#include "smoke.h"
#include "VideoHandler.h"
#include "flame.h"


extern T_ServerMess  	t_SerMess;
extern T_ClientMess    	t_McuMess;   
extern T_Camera           t_Camera;

Canalyze::Canalyze(T_CAM_FIXED_PARAM t_Camfiexedparam)
	:m_singlecam(t_Camfiexedparam.URL,t_Camfiexedparam.index)
{

	AlarmThreadEnFlag  = 1;
	
	HumanTimeFlag	=  	0;
	SmokeTimeFlag 	=	0;
	RegionTimeFlag 	=	0;
	FireTimeFlag 		=	0;
	
	humanAlarm 		= 	0;
	regionAlarm 		= 	0;
	smokeAlarm 		=	0;
	fireAlarm			=	0;

	humanAlarmFlag  = 0;
	humanAlarmFlagFlag= 0;
	regionAlarmFlag= 0;
	regionAlarmFlagFlag= 0;
	smokeAlarmFlag= 0;
	smokeAlarmFlagFlag= 0;
	fireAlarmFlag= 0;
	fireAlarmFlagFlag= 0;

	humanAlarmFrameNum= 0;
	regionAlarmFrameNum= 0;
	smokeAlarmFrameNum= 0;
	fireAlarmFrameNum = 0;

	humanAlarmFlagFrame= 0;
	regionAlarmFlagFrame= 0;
	smokeAlarmFlagFrame= 0;
	fireAlarmFlagFrame = 0;

	MCUAlarmFlag= 0;
	TmpFlag= 0;
	readFrameNum= 0;

	humanALL= 0;
	humanIN= 0;
	humanOUT= 0;

	memset(doorIN, 0, LINENUM);
	memset(doorOUT, 0, LINENUM);

	sprintf(window,"%s-%d","camera",t_Camfiexedparam.index);
	
}
Canalyze::~Canalyze()
{

}

int Canalyze::turnon_mcu_alarm()
{
	T_PacketHead                 t_PackHeadMcuOpr;
	ST_SM_VDCS_MCU_OPERATE_TERM  t_OprMcu;
	char   ControlBuff[28 + 45]={0};
	int	 iRet = -1;
	
	memset(&t_PackHeadMcuOpr,0,sizeof(T_PacketHead));
	memset(&t_OprMcu,0,sizeof(ST_SM_VDCS_MCU_OPERATE_TERM));
	memcpy(t_OprMcu.MCUAddr,t_McuMess.cMac,MCU_MAC_LEN_20);
	t_OprMcu.port      		=	0x01;
	t_OprMcu.TermType  	=	DeviceTypeWarn;
	t_OprMcu.OpFlag    	=	0x00;
	
	t_PackHeadMcuOpr.magic		  = T_PACKETHEAD_MAGIC;
	t_PackHeadMcuOpr.cmd		  = SM_VDCS_MCU_OPERATE_TERM;
	t_PackHeadMcuOpr.UnEncryptLen = sizeof(ST_SM_VDCS_MCU_OPERATE_TERM);
	
	memcpy(ControlBuff,&t_PackHeadMcuOpr,PACKET_HEAD_LEN);
	memcpy(ControlBuff+PACKET_HEAD_LEN,&t_OprMcu,sizeof(ST_SM_VDCS_MCU_OPERATE_TERM));

	iRet = send(t_McuMess.iFd,ControlBuff,sizeof(ControlBuff),0);
	if(iRet <0){
		printf("send error!\n");
		return -1;
	}
	printf("send %d types to mcu!turn on alarm!\n",iRet);
	
	return iRet;
}

int Canalyze::turnoff_mcu_alarm()
{
	T_PacketHead                 t_PackHeadMcuOpr;
	ST_SM_VDCS_MCU_OPERATE_TERM  t_OprMcu;
	char   ControlBuff[28 + 45]={0};
	int	 iRet = -1;
	
	memset(&t_PackHeadMcuOpr,0,sizeof(T_PacketHead));
	memset(&t_OprMcu,0,sizeof(ST_SM_VDCS_MCU_OPERATE_TERM));
	memcpy(t_OprMcu.MCUAddr,t_McuMess.cMac,MCU_MAC_LEN_20);
	t_OprMcu.port      		=	0x01;
	t_OprMcu.TermType  	=	DeviceTypeWarn;
	t_OprMcu.OpFlag    	=	0x01;
	
	t_PackHeadMcuOpr.magic		  = T_PACKETHEAD_MAGIC;
	t_PackHeadMcuOpr.cmd		  = SM_VDCS_MCU_OPERATE_TERM;
	t_PackHeadMcuOpr.UnEncryptLen = sizeof(ST_SM_VDCS_MCU_OPERATE_TERM);
	
	memcpy(ControlBuff,&t_PackHeadMcuOpr,PACKET_HEAD_LEN);
	memcpy(ControlBuff+PACKET_HEAD_LEN,&t_OprMcu,sizeof(ST_SM_VDCS_MCU_OPERATE_TERM));

	iRet = send(t_McuMess.iFd,ControlBuff,sizeof(ControlBuff),0);
	if(iRet <0){
		printf("send error!\n");
		return -1;
	}
	printf("send %d types to mcu!turn off alarm!\n",iRet);
	
	return iRet;
}

int Canalyze::send_HumAlarm_to_server(T_CAM_HUM_ALARM* pt_Alarm)
{
	T_PacketHead               t_PackHeadWarn;
	ST_SM_ANAY_VDCS_WARN_INFO  t_WarnInfo;
	char AlarmBuff[28+168]={0};
	int 	iRet = -1;
	char url[SINGLE_URL_LEN_128] ={0}; 
	
	strcpy(url, m_singlecam.m_videoStream.c_str());

	memset(&t_PackHeadWarn,0,sizeof(T_PacketHead));
	memset(&t_WarnInfo,0,sizeof(ST_SM_ANAY_VDCS_WARN_INFO));

	t_PackHeadWarn.cmd          		= SM_ANAY_VDCS_WARN_INFO;
	t_PackHeadWarn.magic        		= T_PACKETHEAD_MAGIC;
	t_PackHeadWarn.UnEncryptLen 	= sizeof(ST_SM_ANAY_VDCS_WARN_INFO);
	memcpy(AlarmBuff,&t_PackHeadWarn,PACKET_HEAD_LEN);
	
	memcpy(t_WarnInfo.MCUAddr ,t_McuMess.cMac,MCU_MAC_LEN_20);
	memcpy(t_WarnInfo.CameUrl,url,SINGLE_URL_LEN_128);
	
	t_WarnInfo.WarnType  	= 	pt_Alarm->AlarmType;
	t_WarnInfo.AutoWarn     =      pt_Alarm->AutoWarn;
	t_WarnInfo.InCount         =      humanIN;
	t_WarnInfo.OutCount      = 	humanOUT;
	t_WarnInfo.MaxCount      = 	humanALL;

	dbgprint("t_WarnInfo.MaxCount is %d\n",t_WarnInfo.MaxCount ); 
	dbgprint("pt_Alarm->MaxNum is %d \n",pt_Alarm->MaxNum);
	dbgprint("door1:in=%d,out=%d  door2:in=%d,out=%d\n",doorIN[0],doorOUT[0],doorIN[1],doorOUT[1]);
	
	memcpy(AlarmBuff+PACKET_HEAD_LEN,&t_WarnInfo,sizeof(ST_SM_ANAY_VDCS_WARN_INFO));

	iRet = send(t_SerMess.iClientFd,AlarmBuff,sizeof(AlarmBuff),0);
	if(iRet <0){
		printf("send error!\n");
		return -1;
	}
	printf("warn info send %d types to server!\n",iRet);
	
	return iRet;
}
int Canalyze::send_OthAlarm_to_server(T_CAM_OTH_ALARM* pt_Alarm)
{
	T_PacketHead               t_PackHeadWarn;
	ST_SM_ANAY_VDCS_WARN_INFO  t_WarnInfo;
	char AlarmBuff[28+168]={0};
	int 	iRet = -1;
	char url[SINGLE_URL_LEN_128] ={0}; 

	strcpy(url, m_singlecam.m_videoStream.c_str());

	memset(&t_PackHeadWarn,0,sizeof(T_PacketHead));
	memset(&t_WarnInfo,0,sizeof(ST_SM_ANAY_VDCS_WARN_INFO));

	t_PackHeadWarn.cmd          		= SM_ANAY_VDCS_WARN_INFO;
	t_PackHeadWarn.magic        		= T_PACKETHEAD_MAGIC;
	t_PackHeadWarn.UnEncryptLen 	= sizeof(ST_SM_ANAY_VDCS_WARN_INFO);
	memcpy(AlarmBuff,&t_PackHeadWarn,PACKET_HEAD_LEN);
	
	memcpy(t_WarnInfo.MCUAddr ,t_McuMess.cMac,MCU_MAC_LEN_20);
	memcpy(t_WarnInfo.CameUrl,url,SINGLE_URL_LEN_128);
	
	t_WarnInfo.WarnType  	= 	pt_Alarm->AlarmType;
	t_WarnInfo.AutoWarn     =      pt_Alarm->AutoWarn;

	memcpy(AlarmBuff+PACKET_HEAD_LEN,&t_WarnInfo,sizeof(ST_SM_ANAY_VDCS_WARN_INFO));

	iRet = send(t_SerMess.iClientFd,AlarmBuff,sizeof(AlarmBuff),0);
	if(iRet <0){
		printf("send error!\n");
		return -1;
	}
	printf("warn info send %d types to server!\n",iRet);
	
	return iRet;

}

///////////////////////////////////////////     HumanAlarm     Start    ///////////////////////////////////////////

void Canalyze::HumanAlarmThread()
{
	Mat  HumandispalyFrame;
	uint8 index =m_singlecam.m_index;
	char HumanAlarmWindow[30]={0};
	
	CHuman * human = new CHuman(index);
	sprintf(HumanAlarmWindow,"%s-%s",window,"HumanAlarm");
	
	while(t_Camera.CameraFlag){
		//printf("index = %d\n",index);
		if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamHumAlarm.HumanDetectEn){
			//printf("huam\n");
			if( HumanTimeFlag){
					if(!displayFrame.empty()){
						displayFrame.copyTo(HumandispalyFrame);
						human->HumanAlarmRun(HumandispalyFrame);
						humanAlarm  	=  human->alarm;
						humanALL  	= human->humanstatis.numAll;
						humanIN		= MAX(human->humanstatis.doorin[0],human->humanstatis.doorin[1]);//human->humanstatis.inAll;
						humanOUT	= MAX(human->humanstatis.doorout[0],human->humanstatis.doorout[1]);//human->humanstatis.outAll;
						for(int i=0; i<LINENUM;i++){
							doorIN[i]		= human->humanstatis.doorin[i];
							doorOUT[i]	= human->humanstatis.doorout[i];
						}
						//imshow(HumanAlarmWindow,HumandispalyFrame);
						usleep(10*1000);	
						continue;
					}
					humanAlarm =0;
					human->frameindex  = 0;
					usleep(40*1000);	
			}else{
				human->frameindex  = 0;
				sleep(1);
			}
		}else{
				human->frameindex  = 0;
				sleep(1);
			}
	}
	delete human;
	printf("huamn  exit\n");
	pthread_exit(NULL);
}

int Canalyze::CreateHumanAlarmThread()
{
	int iRet = -1;
	pthread_t HumanAlarmThread;
	iRet = pthread_create(&HumanAlarmThread,NULL,RunHumanAlarmThread,this);

	if(iRet != 0){
		 printf("Create HumanAlarm Thread error!\n");
		 return -1;
	} 
	pthread_detach(HumanAlarmThread);
	return  0;	
}
///////////////////////////////////////////     HumanAlarm     Start    ///////////////////////////////////////////

///////////////////////////////////////////     AlarmRegionAlarm     Start    ///////////////////////////////////////////
void Canalyze::AlarmRegionAlarmTHread()
{	
	uint8 index =m_singlecam.m_index;
	CRegion * region = new CRegion (index);
	Mat tmp;
	
	while(t_Camera.CameraFlag&&AlarmThreadEnFlag){
		if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamAlarmRegionAlarm.DetectEn){
			if( RegionTimeFlag){
					if(!displayFrame.empty()){
						displayFrame.copyTo(tmp);				
						region->alarmRegionDetectRun(tmp);
						regionAlarm = region->alarm;
						//imshow("alarmregion",tmp);
						usleep(10*1000);	
						continue;
					}	
					regionAlarm  =0;
					usleep(40*1000);	
					region->frameindex  = 0;
			}else{
				region->frameindex  = 0;
				sleep(1);
			}
		}else{
			region->frameindex  = 0;
			sleep(1);
		}
	}
	delete region;
	printf("region  exit\n");
	pthread_exit(NULL);
}
int   Canalyze::CreateAlarmRegionAlarmThread()
{
	int iRet = -1;
	pthread_t AlarmRegionAlarmThread;
	iRet = pthread_create(&AlarmRegionAlarmThread,NULL,RunAlarmRegionAlarmTHread,this);

	if(iRet != 0){
		 printf("Create RegionAlarm Thread error!\n");
		 return -1;
	} 
	pthread_detach(AlarmRegionAlarmThread);
	return  0;
}
///////////////////////////////////////////     AlarmRegionAlarm     End     ///////////////////////////////////////////
///////////////////////////////////////////     SmokeAlarm         Start     ///////////////////////////////////////////
void Canalyze::SmokeAlarmThread()
{
	uint8 index =m_singlecam.m_index;
	CSmoke * smoke = new CSmoke (index);
	Mat tmp;
	
	while(t_Camera.CameraFlag&&AlarmThreadEnFlag){
	if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamSmkAlarm.DetectEn){
			if( SmokeTimeFlag){	
					if(!displayFrame.empty()){
						displayFrame.copyTo(tmp);
						smoke->SmokeAlarmDetectRun(tmp);
						smokeAlarm = smoke->alarm;
						//imshow("smokealarm",tmp);
						//waitKey(1);					
						usleep(10*1000);					
						continue;
					}
					smokeAlarm = 0;
					usleep(40*1000);
			}else{
				sleep(1);
			}
		}else{
			sleep(1);
		}
	}
	delete smoke;
	printf("Smoke exit\n");
	pthread_exit(NULL);
}

int Canalyze::CreateSmokeAlarmThread()
{
	int iRet = -1;
	pthread_t SmokeAlarmThread;
	iRet = pthread_create(&SmokeAlarmThread,NULL,RunSmokeAlarmThread,this);

	if(iRet != 0){
		 printf("Create SmokeAlarm Thread error!\n");
		 return -1;
	} 
	pthread_detach(SmokeAlarmThread);
	return  0;
}
//////////////////////////////////////////     SmokeAlarm         End    ///////////////////////////////////////////
////////////////////////////////////////////   FireALarm           start////////////////////////////////////////////

void Canalyze::FireAlarmThread()
{
	uint8 index =m_singlecam.m_index;
	uint8 alarmframenum =0;
	VideoHandler* videoHandler = NULL;
	
	CFlame * flame = new CFlame (index ,&videoHandler);
	Mat tmp;
	
	while(t_Camera.CameraFlag&&AlarmThreadEnFlag){
	if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamFireAlarm.DetectEn){
			if( FireTimeFlag){
					if(!displayFrame.empty()){
						displayFrame.copyTo(tmp);
						fireAlarmRect.clear();
						flame->FlameAlarmRun(tmp,(void *)videoHandler);
						if(flame->alarm == 1) {
							alarmframenum++;
							if(alarmframenum > 5){
									fireAlarm = 1;
									alarmframenum= 0;
							 }
						}else {
							fireAlarm = 0;
							alarmframenum	= 0;
						}	
						/*
						fireAlarmRect  = flame->FlameRect;
						if(fireAlarmRect.size() >0 && fireAlarm){	
							for(uint16 i=0;i<fireAlarmRect.size() ;i++)
							rectangle(tmp, fireAlarmRect[i], Scalar( 0, 0, 255 ), 2, 8, 0);
						}*/
						//imshow("firealarm",tmp);
						//waitKey(1);
						//usleep(10*1000);
						continue;
					}
					fireAlarm =0;
					alarmframenum = 0;
					usleep(40*1000);
			}else{
				sleep(1);
			}
		}else{
			sleep(1);
		}
	}
	delete flame;
	videoHandler = NULL;
	printf("Fire exit\n");
	pthread_exit(NULL);
}

int Canalyze::CreateFireAlarmThread()
{
	int iRet = -1;
	pthread_t FireAlarmThread;
	iRet = pthread_create(&FireAlarmThread,NULL,RunFireAlarmThread,this);

	if(iRet != 0){
		 printf("Create FireAlarm Thread error!\n");
		 return -1;
	} 
	pthread_detach(FireAlarmThread);
	return  0;
}

///////////////////////////////////////////   FireAlarm   End ////////////////////////////////////////////////////////
//////////////////////////////////////////           Time start              ///////////////////////////////////////////
void Canalyze::AlarmClockTimeThread()
{
	time_t timep;
	struct tm * pTM=NULL;
	uint8 index =m_singlecam.m_index;
	int day = -1;
	T_AlarmTime tmpStart;
	T_AlarmTime tmpEnd;
	
	while(t_Camera.CameraFlag&&AlarmThreadEnFlag){
		time (&timep);  
		
		pTM = localtime(&timep);
		
		if(pTM->tm_wday == 0) pTM->tm_wday =7;
		day =  pTM->tm_wday -1;

		if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamHumAlarm.HumanDetectEn ){	
			if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamHumAlarm.AlarmTime[day].Day  ==  (day+1)){
				if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamHumAlarm.AlarmTime[day].Enable  ==  1){
					tmpStart =t_Camera.t_SinCam[index].t_Camvarparam.t_CamHumAlarm.AlarmTime[day].Start;
					tmpEnd  = t_Camera.t_SinCam[index].t_Camvarparam.t_CamHumAlarm.AlarmTime[day].End;
					if((tmpStart.hour == 0&&tmpStart.min == 0)&&(tmpEnd.hour== 23 &&tmpEnd.min == 59)){
							HumanTimeFlag =1;
					}else{
						if(pTM->tm_hour ==tmpStart.hour ){
							if(pTM->tm_min >=tmpStart.min)  HumanTimeFlag =1;
						}else if(pTM->tm_hour == tmpEnd.hour){
							if(pTM->tm_min <=tmpEnd.min) HumanTimeFlag =1;
						}else if(pTM->tm_hour >tmpStart.hour && pTM->tm_hour< tmpEnd.hour){
							HumanTimeFlag =1;
						}else{
							HumanTimeFlag = 0;
						}
					}
				}else{
					HumanTimeFlag = 0;
				}

			}
		}
		if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamSmkAlarm.DetectEn ){
			if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamSmkAlarm.AlarmTime[day].Day	==	(day+1)){
				if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamSmkAlarm.AlarmTime[day].Enable  ==  1){
					tmpStart =t_Camera.t_SinCam[index].t_Camvarparam.t_CamSmkAlarm.AlarmTime[day].Start;
					tmpEnd	= t_Camera.t_SinCam[index].t_Camvarparam.t_CamSmkAlarm.AlarmTime[day].End;
					if((tmpStart.hour == 0&&tmpStart.min == 0)&&(tmpEnd.hour== 23 &&tmpEnd.min == 59)){
							SmokeTimeFlag =1;
					}else{
						if(pTM->tm_hour ==tmpStart.hour ){
							if(pTM->tm_min >=tmpStart.min)	SmokeTimeFlag =1;
						}else if(pTM->tm_hour == tmpEnd.hour){
							if(pTM->tm_min <=tmpEnd.min) SmokeTimeFlag =1;
						}else if(pTM->tm_hour >tmpStart.hour && pTM->tm_hour< tmpEnd.hour){
							SmokeTimeFlag =1;
						}else{
							SmokeTimeFlag = 0;
						}
					}
				}else{
					SmokeTimeFlag = 0;
				}
			}
		}

		if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamAlarmRegionAlarm.DetectEn ){
			if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamAlarmRegionAlarm.AlarmTime[day].Day	==	(day+1)){
				if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamAlarmRegionAlarm.AlarmTime[day].Enable  ==  1){
					tmpStart =t_Camera.t_SinCam[index].t_Camvarparam.t_CamAlarmRegionAlarm.AlarmTime[day].Start;
					tmpEnd	= t_Camera.t_SinCam[index].t_Camvarparam.t_CamAlarmRegionAlarm.AlarmTime[day].End;
					if((tmpStart.hour == 0&&tmpStart.min == 0)&&(tmpEnd.hour== 23 &&tmpEnd.min == 59)){
							RegionTimeFlag =1;
					}else{
						if(pTM->tm_hour ==tmpStart.hour ){
							if(pTM->tm_min >=tmpStart.min)	RegionTimeFlag =1;
						}else if(pTM->tm_hour == tmpEnd.hour){
							if(pTM->tm_min <=tmpEnd.min) RegionTimeFlag =1;
						}else if(pTM->tm_hour >tmpStart.hour && pTM->tm_hour< tmpEnd.hour){
							RegionTimeFlag =1;
						}else{
							RegionTimeFlag = 0;
						}
					}
				}else{
					RegionTimeFlag = 0;
				}
			}

		
		}
		
		if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamFireAlarm.DetectEn ){
			if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamFireAlarm.AlarmTime[day].Day	==	(day+1)){
				if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamFireAlarm.AlarmTime[day].Enable  ==  1){
					tmpStart =t_Camera.t_SinCam[index].t_Camvarparam.t_CamFireAlarm.AlarmTime[day].Start;
					tmpEnd	= t_Camera.t_SinCam[index].t_Camvarparam.t_CamFireAlarm.AlarmTime[day].End;
					if((tmpStart.hour == 0&&tmpStart.min == 0)&&(tmpEnd.hour== 23 &&tmpEnd.min == 59)){
							FireTimeFlag =1;
					}else{
						if(pTM->tm_hour ==tmpStart.hour ){
							if(pTM->tm_min >=tmpStart.min)	FireTimeFlag =1;
						}else if(pTM->tm_hour == tmpEnd.hour){
							if(pTM->tm_min <=tmpEnd.min) FireTimeFlag =1;
						}else if(pTM->tm_hour >tmpStart.hour && pTM->tm_hour< tmpEnd.hour){
							FireTimeFlag =1;
						}else{
							FireTimeFlag = 0;
						}
					}
				}else{
					FireTimeFlag = 0;
				}
			}
		}


		if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamFixedObjAlarm.DetectEn ){
		
		}
		sleep(1);
	}
	printf("time exit\n");
	pthread_exit(NULL);
}

int   Canalyze::CreateAlarmClockTimeThread()
{
	int iRet = -1;
	pthread_t AlarmClockTimeThread;
	iRet = pthread_create(&AlarmClockTimeThread,NULL,RunAlarmClockTimeThread,this);

	if(iRet != 0){
		 printf("Create ClockTime Thread error!\n");
		 return -1;
	} 
	pthread_detach(AlarmClockTimeThread);
	return  0;
}
////////////////////////////////////////// Time   End /////////////////////////////////////////////////
int Canalyze::AnalyzeInit()
{

	int iRet = -1;	
	iRet = m_singlecam.GetCamParam();	 /* 长、宽、帧速 */
	if(iRet < 0) return  -1;
	
	m_rows			 =	m_singlecam.m_rows;
	m_cols			 =	m_singlecam.m_cols;
	
	displayFrame		 =	Mat(m_rows,m_cols,CV_8UC3); 

	
	/* 创建各个报警线程 */
	CreateHumanAlarmThread();
	CreateAlarmRegionAlarmThread();   
	CreateSmokeAlarmThread();
	CreateFireAlarmThread();
	
	/* 时间线程 */
	CreateAlarmClockTimeThread();

	// namedWindow(window);
	return 0;
}

int Canalyze::AlarmStrategy()
{
	uint8 index =m_singlecam.m_index;

	if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamHumAlarm.HumanDetectEn &&HumanTimeFlag){
		if((humanAlarm == 1 )&&(humanAlarmFlag == 0)){
			humanAlarmFrameNum++;
		}
		
		if((humanAlarmFrameNum >  5) &&   !humanAlarmFlag){
			printf("camera %d human alarm\n" ,index);
			send_HumAlarm_to_server(&(t_Camera.t_SinCam[index].t_Camvarparam.t_CamHumAlarm));
			if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamHumAlarm.AutoWarn  ==  1) {
				turnon_mcu_alarm();   
				MCUAlarmFlag++;
			}	
			humanAlarmFrameNum =0;
			humanAlarmFlag = 1;
			humanAlarmFlagFlag =1;
		}
	}else{
		humanAlarmFrameNum =0;
		humanAlarmFlag    =0;
		humanAlarmFlagFlag =0;
	}
	
	if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamAlarmRegionAlarm.DetectEn &&RegionTimeFlag){
		if((regionAlarm == 1 )&&(regionAlarmFlag == 0)){
			regionAlarmFrameNum++;
		}
		
		if((regionAlarmFrameNum >  5) &&   !regionAlarmFlag){
			printf("camer %d region alarm\n" ,index);
			send_OthAlarm_to_server(&(t_Camera.t_SinCam[index].t_Camvarparam.t_CamAlarmRegionAlarm));
			if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamAlarmRegionAlarm.AutoWarn  ==  1) {
				turnon_mcu_alarm();   
				MCUAlarmFlag++;
			}	
			regionAlarmFrameNum =0;
			regionAlarmFlag = 1;
			regionAlarmFlagFlag =1;
		}
	}else{
		regionAlarmFrameNum =0;
		regionAlarmFlag    =0;
		regionAlarmFlagFlag =0;
	}
	
	if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamSmkAlarm.DetectEn&&SmokeTimeFlag){
		if((smokeAlarm == 1 )&&(smokeAlarmFlag == 0)){
			smokeAlarmFrameNum++;
		}

		if((smokeAlarmFrameNum >  5) &&   !smokeAlarmFlag){
			printf("camer %d smoke alarm\n" ,index);
			send_OthAlarm_to_server(&(t_Camera.t_SinCam[index].t_Camvarparam.t_CamSmkAlarm));
			if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamSmkAlarm.AutoWarn  ==  1) {
				turnon_mcu_alarm();   
				MCUAlarmFlag++;
			}	
			smokeAlarmFrameNum =0;
			smokeAlarmFlag = 1;
			smokeAlarmFlagFlag =1;
		}
	}else{
		smokeAlarmFrameNum =0;
		smokeAlarmFlag    =0;
		smokeAlarmFlagFlag =0;
	}

	if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamFireAlarm.DetectEn&&FireTimeFlag){
		if((fireAlarm == 1 )&&(fireAlarmFlag == 0)){
			fireAlarmFrameNum++;
		}

		if((fireAlarmFrameNum >  1) &&   !fireAlarmFlag){
			printf("camer %d fire alarm\n",index);
			send_OthAlarm_to_server(&(t_Camera.t_SinCam[index].t_Camvarparam.t_CamFireAlarm));
			if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamFireAlarm.AutoWarn  ==  1) {
				turnon_mcu_alarm();   
				MCUAlarmFlag++;
			}	
			fireAlarmFrameNum =0;
			fireAlarmFlag = 1;
			fireAlarmFlagFlag =1;
		}
	}else{
		fireAlarmFrameNum =0;
		fireAlarmFlag    =0;
		fireAlarmFlagFlag =0;
	}



}

void Canalyze::reStartAlarmOn()
{
	if(humanAlarmFlagFlag == 1){
		humanAlarmFlagFrame++;
		if(humanAlarmFlagFrame  >100){
			humanAlarmFlag = 0;
			humanAlarmFlagFlag = 0;
			humanAlarmFlagFrame =0;
		}
	}

	if(regionAlarmFlagFlag == 1){
		regionAlarmFlagFrame++;
		if(regionAlarmFlagFrame  >100){
			regionAlarmFlag = 0;
			regionAlarmFlagFlag = 0;
			regionAlarmFlagFrame =0;
		}
	}
	
	if(smokeAlarmFlagFlag == 1){
		smokeAlarmFlagFrame++;
		if(smokeAlarmFlagFrame  >100){
			smokeAlarmFlag = 0;
			smokeAlarmFlagFlag = 0;
			smokeAlarmFlagFrame =0;
		}
	}

	if(fireAlarmFlagFlag == 1){
		fireAlarmFlagFrame++;
		if(fireAlarmFlagFrame  >100){
			fireAlarmFlag = 0;
			fireAlarmFlagFlag = 0;
			fireAlarmFlagFrame =0;
		}
	}
}

void Canalyze::Rect_in_display(Mat & display)
{
	uint8 index =m_singlecam.m_index;

	if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamHumAlarm.HumanDetectEn &&HumanTimeFlag){  //人 绿 
		if((t_Camera.t_SinCam[index].t_Camvarparam.t_CamHumAlarm.Flag & 0x02)  == 0x02){  
			
			for(int i=0;i<t_Camera.t_SinCam[index].t_Camvarparam.t_CamHumAlarm.DirectionLines.size();i++)
			{
				line(display,t_Camera.t_SinCam[index].t_Camvarparam.t_CamHumAlarm.DirectionLines[i].Start,t_Camera.t_SinCam[index].t_Camvarparam.t_CamHumAlarm.DirectionLines[i].End,Scalar( 64, 128, 0 ), 2, 8, 0);
			}
		}

		if((t_Camera.t_SinCam[index].t_Camvarparam.t_CamHumAlarm.Flag & 0x01)  == 1){  

			for(int ii=0;ii<t_Camera.t_SinCam[index].t_Camvarparam.t_CamHumAlarm.MonitorZoneRects.size();ii++)
			{
				rectangle(display, t_Camera.t_SinCam[index].t_Camvarparam.t_CamHumAlarm.MonitorZoneRects[ii], Scalar( 64, 128, 0 ), 2, 8, 0);
			}
		}
		
		char FrmNum[100]; 
		sprintf(FrmNum,  "Num:%d,In:%d,Out:%d",humanALL,humanIN,humanOUT); //linux下为snprintf()
	        putText(display, FrmNum,cvPoint(0,25), CV_FONT_HERSHEY_COMPLEX, 0.5, cvScalar(0,0,255)); //cvPoint(250,25)
	}

	if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamAlarmRegionAlarm.DetectEn &&RegionTimeFlag){ //区域  蓝
		for(unsigned int i = 0;i<t_Camera.t_SinCam[index].t_Camvarparam.t_CamAlarmRegionAlarm.Rects.size();i++){
			rectangle(display, t_Camera.t_SinCam[index].t_Camvarparam.t_CamAlarmRegionAlarm.Rects[i], Scalar( 255, 0, 0 ), 2, 8, 0);
		}
	
	}

	if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamSmkAlarm.DetectEn&&SmokeTimeFlag){//烟  灰
		for(unsigned int j = 0;j<t_Camera.t_SinCam[index].t_Camvarparam.t_CamSmkAlarm.Rects.size();j++){
			rectangle(display, t_Camera.t_SinCam[index].t_Camvarparam.t_CamSmkAlarm.Rects[j], Scalar( 60, 60, 60 ), 2, 8, 0);
		}
	
	}
	
	if(t_Camera.t_SinCam[index].t_Camvarparam.t_CamFireAlarm.DetectEn&&FireTimeFlag){  // 火  红
		for(unsigned int j = 0;j<t_Camera.t_SinCam[index].t_Camvarparam.t_CamFireAlarm.Rects.size();j++){
			rectangle(display, t_Camera.t_SinCam[index].t_Camvarparam.t_CamFireAlarm.Rects[j], Scalar( 0, 0, 255 ), 2, 8, 0);
		}
	
	}

}
int Canalyze::Readframe()
{
	int iRet = -1;
	
	iRet = m_singlecam.GetCamFrame(); 
	if(iRet <0)  {
		if(m_singlecam.errReadNum > 300) { 
			m_singlecam.errReadNum  = 0;
			dbgprint("read frame err!\n");
			return -1;
		}
		return 0;
	}
	
	reStartAlarmOn();

	if(MCUAlarmFlag > 0){
		if(TmpFlag == MCUAlarmFlag){
			readFrameNum++;
		}else{
			TmpFlag = MCUAlarmFlag;
			readFrameNum = 0;
		}
		if(readFrameNum  >=  65){
			turnoff_mcu_alarm();
			MCUAlarmFlag = 0;
			TmpFlag = 0;
			readFrameNum = 0;
		}
	}
	
	if(!m_singlecam.readFrame.empty()){
	
		m_singlecam.readFrame.copyTo(displayFrame);

		AlarmStrategy();
		
		Rect_in_display(displayFrame);
		//imshow(window,displayFrame);
		waitKey(1);
		m_singlecam.readFrame.release();
		return 0;
	}
	usleep(20*1000);
	dbgprint("readFrame empty!\n");
	return 0;
}

