
#include "TcpCom.h"
#include  "Xmlparser.h"
#include  "Analyze.h"
extern T_ServerMess  	t_SerMess;
extern T_ClientMess    	t_McuMess;   
extern T_DeviceParam 	t_DevParam;
extern T_Camera           t_Camera;

int report_camera_break(uint8 num)
{
	T_PacketHead						t_PackHeadRptCamBrk;
	ST_SM_ANAY_VDCS_DEVICE_STATUS     t_CamStatus;
	char CamStatusBuff[28 + 162] ={0};
	int 	iRet = -1;

	memset(&t_PackHeadRptCamBrk,0,sizeof(T_PacketHead));
	memset(&t_CamStatus,0,sizeof(ST_SM_ANAY_VDCS_DEVICE_STATUS));

	t_PackHeadRptCamBrk.magic			=  T_PACKETHEAD_MAGIC;
	t_PackHeadRptCamBrk.cmd			=  SM_ANAY_VDCS_DEVICE_STATUS;
	t_PackHeadRptCamBrk.UnEncryptLen	=  sizeof(ST_SM_ANAY_VDCS_DEVICE_STATUS);
		
	memcpy(CamStatusBuff,&t_PackHeadRptCamBrk,sizeof(T_PacketHead));
	memcpy(t_CamStatus.MCUAddr ,t_McuMess.cMac,MCU_MAC_LEN_20);
	memcpy(t_CamStatus.CameUrl ,t_Camera.t_SinCam[num].t_Camfixedparam.URL,SINGLE_URL_LEN_128);
	t_CamStatus.DeviceType=  DeviceTypeNetCamera;
	memcpy(CamStatusBuff+sizeof(T_PacketHead),&t_CamStatus,sizeof(ST_SM_ANAY_VDCS_DEVICE_STATUS));

	iRet = send(t_SerMess.iClientFd,CamStatusBuff,sizeof(CamStatusBuff),0);
	if(iRet <=0 ){
		dbgprint("send data to server error !\n");
		return -1;
	}
	dbgprint("send %d byte to server !\n",iRet);	
	return 0;

}

void *Camera_thread(void * arg)
{
	uint8 num = *(uint8*)arg;
	int iRet 	    = -1;
	uint  ReopenNum  =0;
	t_Camera.t_SinCam[num].CamAnalyze   =  new Canalyze(t_Camera.t_SinCam[num].t_Camfixedparam);
	
REOPEN:	 /*考虑摄像头打开但读不到数据的情况*/
	iRet = t_Camera.t_SinCam[num].CamAnalyze->AnalyzeInit();
	if(iRet  <  0){  
		report_camera_break(num);
		goto err;
	}
	t_Camera.t_SinCam[num].CamAnalyze->AlarmThreadEnFlag = 1;
	
	while(t_Camera.CameraFlag){
				iRet = t_Camera.t_SinCam[num].CamAnalyze->Readframe();
				if(iRet < 0) { 
					if(ReopenNum >5){
					     	goto err;
					}
					ReopenNum++;
					goto REOPEN;
				}
	}
err:	
		/* 读摄像头线程退出  要关闭其他报警线程 */
	t_Camera.t_SinCam[num].CamAnalyze->AlarmThreadEnFlag =0;
	sleep(4);
	/*
	t_Camera.t_SinCam[num].CamAnalyze->turnoff_mcu_alarm();
	while(t_Camera.CameraFlag){
	
		goto REOPEN;
	}*/
	
	t_Camera.t_SinCam[num].CamAnalyze->turnoff_mcu_alarm();

	delete t_Camera.t_SinCam[num].CamAnalyze;
	
	memset(&t_Camera.t_SinCam[num],0,sizeof(T_SINGLE_CAMERA));
	
	dbgprint("Camera%d pthread is down!\n",num);
	pthread_exit(NULL);
 	return NULL;
}

int create_camera_thread_old(PT_Camera pt_Camera)
{
	pthread_t pthread_ID;
	int 		iRet = -1;
    	uint8 	iCamNum = 0;
	
	pt_Camera->CameraFlag    = 1;  
	
	while(iCamNum  < pt_Camera->CamNum){
		if(pt_Camera->t_SinCam[iCamNum].t_Camfixedparam.CameStatus== 0){
			iRet = pthread_create(&pthread_ID,NULL,Camera_thread,&pt_Camera->t_SinCam[iCamNum].t_Camfixedparam.index);
			if(iRet != 0){
				 printf("create Camera_thread error!\n");
				 continue;
			}
			dbgprint("Camera%d pthread_create !\n", iCamNum+1);
			pt_Camera->t_SinCam[iCamNum].t_Camfixedparam.CameStatus = 1;
			pthread_detach(pthread_ID);	
		}
		iCamNum++;
	}
	return 0;
}

int search_camera(char * url)
{
	uint8 iNum = 0;
	for(iNum =0;  iNum  <  MAX_CAM_NUM ; iNum++){
				if(strcmp(t_Camera.t_SinCam[iNum].t_Camfixedparam.URL,url)==0)
				                  		return   iNum;
	}
	return -1;
}

void camera_url_process_old(PT_Camera pt_Camera,char *buffer)
{
	uint8 i=0;
	for(i=0;i<pt_Camera->CamNum;i++){
		pt_Camera->t_SinCam[i].t_Camfixedparam.index			= i;
		memcpy(pt_Camera->t_SinCam[i].t_Camfixedparam.URL,buffer+SINGLE_URL_LEN_128*i,SINGLE_URL_LEN_128);
	}
/*
	strcpy((char *)pt_Camera->t_SinCam[0].t_Camfixedparam.URL ,strtok(param->UrlList, ","));
	for(i=1;i<pt_Camera->CamNum;i++){
		  strcpy( pt_Camera->t_SinCam[i].t_Camfixedparam.URL , strtok(NULL, ","));
	}
*/
	
	for(i=0;i<pt_Camera->CamNum;i++)
	{	
		dbgprint("camera%d URL\tis\t%s\n",i,pt_Camera->t_SinCam[i].t_Camfixedparam.URL);
	}
}

int create_camera_thread(PT_Camera pt_Camera,uint8 num)
{
	pthread_t pthread_ID;
	int 		iRet = -1;
	
	pt_Camera->t_SinCam[num].t_Camfixedparam.index = num;
	
	iRet = pthread_create(&pthread_ID,NULL,Camera_thread,&pt_Camera->t_SinCam[num].t_Camfixedparam.index);
	if(iRet != 0){
		 printf("create Camera_thread error!\n");
		 return -1;
	}
	dbgprint("Camera%d pthread_create !\n", num);
	pt_Camera->t_SinCam[num].t_Camfixedparam.CameStatus = 1;
	pthread_detach(pthread_ID);	

	return 0;
}

void camera_url_process(char *buffer,uint8 num)
{
	uint8 i=0 , j=0;
	int iRet = -1;
	char buff[SINGLE_URL_LEN_128] ={0};
	
	t_Camera.CameraFlag    = 1;  
	
	for(i=0;i<num;i++){
		memcpy(buff,buffer+SINGLE_URL_LEN_128*i,SINGLE_URL_LEN_128);
		iRet =search_camera(buff);
		printf("iRet is %d\n",iRet);
		/* 当没有摄像头时返回 -1，增加摄像头 
		*/
		if(iRet == -1){
			for(j=0;j<MAX_CAM_NUM ; j++){
					if(t_Camera.t_SinCam[j].t_Camfixedparam.CameStatus == 0){					
						if(j >= t_Camera.CamNum){
								dbgprint("add  one camera,index is %d ,camera num is %d\n",j,t_Camera.CamNum);
								t_Camera.CamNum++;
						}else{
							dbgprint("replace one camera ,index is %d,camera num is %d\n",j,t_Camera.CamNum);
						}
						printf("URL is %s \n",buff);
						// 创建线程
						memcpy(t_Camera.t_SinCam[j].t_Camfixedparam.URL,buff,SINGLE_URL_LEN_128);
						create_camera_thread(&t_Camera,j);
						break;
					}
			}
			
			if(j == MAX_CAM_NUM) dbgprint("camera is full! %s is  not added \n", buff);
		}

		/*  如果搜索到对应摄像头则查看状态*/

		if(iRet >=0){
			if(t_Camera.t_SinCam[iRet].t_Camfixedparam.CameStatus == 0){
				dbgprint("camera %s is break, renew open\n",buff);
				printf("URL is %s \n",buff);
				/* 重新创建线程 */
				create_camera_thread(&t_Camera,(uint8)iRet);
	
			}else{
				printf("URL is %s \n",buff);
				dbgprint("camera %s is already opened\n",buff);
			}
		}
	}

}
int camera_process(char * buffer,int len,PT_TcpClient instance)
{
	uint8   CamerNum = 0;

	memcpy(&CamerNum,buffer+PACKET_HEAD_LEN,1);
	if(CamerNum  ==0 ||CamerNum  >= (MAX_CAM_NUM -t_Camera.CamNum)){
		dbgprint("wrong CamerNum   %d !\n",CamerNum);
		return -1;
	}	
	dbgprint("add  camera count is %d \n",CamerNum);
	camera_url_process( buffer+PACKET_HEAD_LEN+1,CamerNum);
/*	
	t_Camera.CamNum  =CamerNum;
	dbgprint("camera count is %d \n",t_Camera.CamNum);

	camera_url_process(&t_Camera,buffer+PACKET_HEAD_LEN+1);
	
	create_camera_thread(&t_Camera);
*/

	return 0;
}


int anay_register_ack(char * buffer,PT_TcpClient instance)
{
	ST_SM_VDCS_ANAY_RENEW_REGISTER_MCU_ACK  t_AnayRegAck;
	uint8 	ack;

	memset(&t_AnayRegAck,0,sizeof(ST_SM_VDCS_ANAY_RENEW_REGISTER_MCU_ACK));
	memcpy(&t_AnayRegAck,buffer+PACKET_HEAD_LEN,sizeof(ST_SM_VDCS_ANAY_RENEW_REGISTER_MCU_ACK));

	ack = t_AnayRegAck.Ack;
	if(ack == 0){
			dbgprint("anay register sucess!\n");
			t_SerMess.iAnayRegister = 1;
			return 0;
	}else{
			dbgprint("anay register failed!\n");
			return -1;
	}
	return -1;
}

int device_control_process(char * buffer,PT_TcpClient instance)
{
	T_PacketHead					t_PackHeadMcuOpr;
	ST_SM_VDCS_MCU_OPERATE_TERM     t_McuOpr;

	ST_SM_VDCS_ANAY_DEVICE_CONTROL  t_DeviceControl;
	int McuFd = -1;
	char   ControlBuff[28 + 45]={0};
	int   iRet = -1;

	memset(&t_PackHeadMcuOpr,0,sizeof(T_PacketHead));
	memset(&t_McuOpr,0,sizeof(ST_SM_VDCS_MCU_OPERATE_TERM));
		
	memset(&t_DeviceControl,0,sizeof(ST_SM_VDCS_ANAY_DEVICE_CONTROL));
	memcpy(&t_DeviceControl,buffer+PACKET_HEAD_LEN,sizeof(ST_SM_VDCS_ANAY_DEVICE_CONTROL));

	memcpy((char *)t_McuOpr.MCUAddr,t_DeviceControl.MCUAddr,MCU_MAC_LEN_20);

	McuFd = search_mcu_client((char *)t_McuOpr.MCUAddr);
	if(McuFd <0)
			return -1;	
	dbgprint("have searched mcu !\n");

	//printf("t_DeviceControl.DeviceType is %d\n",t_DeviceControl.DeviceType);
	t_McuOpr.TermType      = t_DeviceControl.DeviceType;
	t_McuOpr.OpFlag        	= t_DeviceControl.OpFlag;
	t_McuOpr.port          	= t_DeviceControl.Port;
	
	memcpy((char *)t_McuOpr.UserName,t_DeviceControl.UserName ,USRNAME_LEN_20);
	memcpy((char *)t_McuOpr.MCUAddr,t_McuMess.cMac,MCU_MAC_LEN_20);

	
	t_PackHeadMcuOpr.magic        		= T_PACKETHEAD_MAGIC;
	t_PackHeadMcuOpr.cmd          		= SM_VDCS_MCU_OPERATE_TERM;
	t_PackHeadMcuOpr.UnEncryptLen 	= sizeof(ST_SM_VDCS_MCU_OPERATE_TERM);
	
	memcpy(ControlBuff,&t_PackHeadMcuOpr,PACKET_HEAD_LEN);
	memcpy(ControlBuff+PACKET_HEAD_LEN,&t_McuOpr,sizeof(ST_SM_VDCS_MCU_OPERATE_TERM));

	iRet = send(McuFd,ControlBuff,sizeof(ControlBuff),0);
	if(iRet <0){
		printf("send error!\n");
		return -1;
	}
	printf("send %d types to mcu!\n",iRet);
	
	return iRet;
}

void mcu_inactive_ack(char * buffer,PT_TcpClient instance)
{
	ST_SM_VDCS_ANAY_DEVICE_STATUS_ACK t_AnayStatusAck;
	uint8 ack;

   	 memset(&t_AnayStatusAck,0,sizeof(ST_SM_VDCS_ANAY_DEVICE_STATUS_ACK));
	memcpy(&t_AnayStatusAck,buffer+PACKET_HEAD_LEN,sizeof(ST_SM_VDCS_ANAY_DEVICE_STATUS_ACK));
	ack = t_AnayStatusAck.Ack;
	if(ack == 0){
			dbgprint("device break report sucess!\n");
	}else{
		dbgprint("device break report error!\n");
	}
}

int warn_info_ack(char * buffer)
{
	ST_SM_VDCS_ANAY_WARN_INFO_ACK  t_AnayWarnAck;
	uint8 ack;
	
    	memset(&t_AnayWarnAck,0,sizeof(ST_SM_VDCS_ANAY_WARN_INFO_ACK));
	memcpy(&t_AnayWarnAck,buffer+PACKET_HEAD_LEN,sizeof(ST_SM_VDCS_ANAY_WARN_INFO_ACK));
		
	ack = t_AnayWarnAck.Ack;
	if(ack == 0){
			dbgprint("ipCamera warninfo report sucess!\n");
	}else{
		dbgprint("ipCamera warninfo report error!\n");
	}
	return 0;
}

int routing_inspection_ack_to_mcu(ST_SM_VDCS_ANAY_ROUTING_INSPECTION_ACK  t_AnayRoutingAck)
{
	T_PacketHead								    t_PackHeadMcuOpr;
	ST_SM_VDCS_MCU_ROUTING_INSPECTION_ACK      t_RoutingAck;
	int McuFd = -1;
	char   RoutingAckBuff[28 + 22]={0};
	int   iRet = -1;

	memset(&t_PackHeadMcuOpr,0,sizeof(T_PacketHead));
	memset(&t_RoutingAck,0,sizeof(ST_SM_VDCS_MCU_ROUTING_INSPECTION_ACK));

	if(strncmp(t_AnayRoutingAck.McuAddr,t_McuMess.cMac,MCU_MAC_LEN_20 ) == 0){
		McuFd =t_McuMess.iFd;
	}else{
		dbgprint("wrong mcu mac!\n");	
		return -1;	
	}		

	memcpy(t_RoutingAck.ReControl,t_AnayRoutingAck.ReControl,RE_CONTROL_LEN_20);
	t_RoutingAck.Ack   =   0;

	t_PackHeadMcuOpr.magic        		= T_PACKETHEAD_MAGIC;
	t_PackHeadMcuOpr.cmd          		= SM_VDCS_MCU_ROUTING_INSPECTION_ACK;
	t_PackHeadMcuOpr.UnEncryptLen 	= sizeof(ST_SM_VDCS_MCU_ROUTING_INSPECTION_ACK);
	
	memcpy(RoutingAckBuff,&t_PackHeadMcuOpr,PACKET_HEAD_LEN);
	memcpy(RoutingAckBuff+PACKET_HEAD_LEN,&t_RoutingAck,sizeof(ST_SM_VDCS_MCU_ROUTING_INSPECTION_ACK));

	iRet = send(McuFd,RoutingAckBuff,sizeof(RoutingAckBuff),0);
	if(iRet <0){
		printf("send error!\n");
		return -1;
	}
	printf("send %d types to mcu!\n",iRet);
	
	return iRet;
}

int routing_inspection_ack(char * buffer)
{
	ST_SM_VDCS_ANAY_ROUTING_INSPECTION_ACK  t_AnayRoutingAck;
	uint8 ack;
	
    	memset(&t_AnayRoutingAck,0,sizeof(ST_SM_VDCS_ANAY_ROUTING_INSPECTION_ACK));
	memcpy(&t_AnayRoutingAck,buffer+PACKET_HEAD_LEN,sizeof(ST_SM_VDCS_ANAY_ROUTING_INSPECTION_ACK));
	
	dbgprint("t_AnayRoutingAck.Action is %c\n",t_AnayRoutingAck.Action);
	
	ack = t_AnayRoutingAck.Ack;
	if(ack == 0){
			routing_inspection_ack_to_mcu(t_AnayRoutingAck);
			dbgprint("routing inspection ack report sucess!\n");
	}else{
		dbgprint("routing inspection ack report error!\n");
	}
	return 0;	
}

int anay_re_register_ack(char * buffer,PT_TcpClient instance)
{
	ST_SM_VDCS_ANAY_REGISTER_ACK t_AnayReRegAck;
	uint8 ack;

	memset(&t_AnayReRegAck,0,sizeof(ST_SM_VDCS_ANAY_REGISTER_ACK));
	memcpy(&t_AnayReRegAck,buffer+PACKET_HEAD_LEN,sizeof(ST_SM_VDCS_ANAY_REGISTER_ACK));

	ack = t_AnayReRegAck.Ack;
	if(ack == 0){
			dbgprint("--anay re_register sucess!--\n");
			t_SerMess.iAnayRegister = 1;
			return 0;
	}else{
			dbgprint("--anay re_register  failed!--\n");
			return -1;
	}
	return -1;
}

/****************************************************************************************************/
//时间段解析
/****************************************************************************************************/

int HumAlarmTime_analyse(T_CAM_HUM_ALARM *alarm,  ST_SM_VDCS_VIDEO_ALARM_TIME *alarmtimer){
	int i=0;
	char hour[3];
	char min[3];
	for(i=0; i<7; i++){
		alarm->AlarmTime[i].Day = alarmtimer[i].Day;
		alarm->AlarmTime[i].Enable = alarmtimer[i].Enable;

		memset(hour, 0, 3);
		memset(min, 0, 3);
		memcpy(hour, alarmtimer[i].StartTime, 2);
		memcpy(min, alarmtimer[i].StartTime+3, 2);
		alarm->AlarmTime[i].Start.hour = atoi(hour);
		alarm->AlarmTime[i].Start.min = atoi(min);

		memset(hour, 0, 3);
		memset(min, 0, 3);
		memcpy(hour, alarmtimer[i].EndTime, 2);
		memcpy(min, alarmtimer[i].EndTime+3, 2);
		alarm->AlarmTime[i].End.hour = atoi(hour);
		alarm->AlarmTime[i].End.min = atoi(min);
	}
	return 0;
}
/****************************************************************************************************/

int push_human_var_data(int num,ST_SM_VDCS_VIDEO_DRAW tmp)
{
	int x=0; 
	int y=0;
	int width =0;
	int height =0;
	Line line;
	
	if(tmp.Type  == 1){//  rect
		x=(int )tmp.StartX;
		y=(int )tmp.StartY;
		width =(int )tmp.EndX;
		height=(int )tmp.EndY;		
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm.MonitorZoneRects.push_back(Rect(x, y, width, height));
	}
	
	if(tmp.Type ==2){ //  Line
		line.Start.x =tmp.StartX;
		line.Start.y=tmp.StartY;
		line.End.x=tmp.EndX;
		line.End.y=tmp.EndY;
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm.DirectionLines.push_back(line);
	}
	
	return 0;
}

int  humandetectParamReset(ST_SM_VDCS_VIDEO_FUNC_PARAM * pt_VideoFuncParam,int num,char *buffer)
{
	uint16 PkgNum =0;
	int i=0;
	vector<ST_SM_VDCS_VIDEO_DRAW>	videodraw;
	ST_SM_VDCS_VIDEO_DRAW  tmpVideoDraw;
	
	if(pt_VideoFuncParam->EnableAlarm == 0){      //  disable
		if(t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm.HumanDetectEn ==  0){
			dbgprint("disable human ,but is already disable\n ");
			return 0;
		}
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm.HumanDetectEn = 0;
		t_Camera.t_SinCam[num].CamAnalyze->turnoff_mcu_alarm();
		memset(&t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm,0,sizeof(T_CAM_HUM_ALARM));
	}else{
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm.HumanDetectEn = 0;
		t_Camera.t_SinCam[num].CamAnalyze->turnoff_mcu_alarm();

		memset(&t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm,0,sizeof(T_CAM_HUM_ALARM));
		usleep(800*1000);
		
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm.MonitorZoneRects.clear();
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm.DirectionLines.clear();

		t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm.AutoWarn  = pt_VideoFuncParam->AutoAlarm;
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm.MaxNum    = pt_VideoFuncParam->MaxHumanNum;
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm.AlarmType = pt_VideoFuncParam->AlarmType;
	
		//todo :时间解析
		HumAlarmTime_analyse(&t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm,pt_VideoFuncParam->AlarmTime);
		
		PkgNum  = pt_VideoFuncParam->PkgNum;
		if(PkgNum  == 0 ||PkgNum  > 20){
			dbgprint("wrong packge number %d!\n",PkgNum);
			memset(&t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm,0,sizeof(T_CAM_HUM_ALARM));
			return -1;
		}
		
		for(i=0; i< PkgNum; i++)
		{
			memcpy(&tmpVideoDraw,buffer+sizeof(ST_SM_VDCS_VIDEO_FUNC_PARAM)+i*(sizeof(ST_SM_VDCS_VIDEO_DRAW)),sizeof(ST_SM_VDCS_VIDEO_DRAW));
			videodraw.push_back(tmpVideoDraw);
		}
		
		for(i =0; i < (int)videodraw.size() ;i++){
			ST_SM_VDCS_VIDEO_DRAW tmp = videodraw[i]; 
			push_human_var_data(num,tmp);
		}
		
		if(t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm.MonitorZoneRects.size() >0 ) t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm.Flag   |=0x01;
		if(t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm.DirectionLines.size() >0 ) t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm.Flag   |=0x02;

		if(t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm.Flag >0){

			t_Camera.t_SinCam[num].CamAnalyze->humanAlarmFrameNum = 0;
			t_Camera.t_SinCam[num].CamAnalyze->humanAlarm = 0;
			t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm.HumanDetectEn = 1;	
			printf("Flag = %d\n", t_Camera.t_SinCam[num].t_Camvarparam.t_CamHumAlarm.Flag );
		}
		videodraw.clear();
	}

	return 0;
}

/****************************************************************************************************/
//时间段解析
/****************************************************************************************************/

int OthAlarmTime_analyse(T_CAM_OTH_ALARM *alarm,  ST_SM_VDCS_VIDEO_ALARM_TIME *alarmtimer){
	int i=0;
	char hour[3];
	char min[3];
	for(i=0; i<7; i++){
		alarm->AlarmTime[i].Day = alarmtimer[i].Day;
		alarm->AlarmTime[i].Enable = alarmtimer[i].Enable;

		memset(hour, 0, 3);
		memset(min, 0, 3);
		memcpy(hour, alarmtimer[i].StartTime, 2);
		memcpy(min, alarmtimer[i].StartTime+3, 2);
		alarm->AlarmTime[i].Start.hour = atoi(hour);
		alarm->AlarmTime[i].Start.min = atoi(min);

		memset(hour, 0, 3);
		memset(min, 0, 3);
		memcpy(hour, alarmtimer[i].EndTime, 2);
		memcpy(min, alarmtimer[i].EndTime+3, 2);
		alarm->AlarmTime[i].End.hour = atoi(hour);
		alarm->AlarmTime[i].End.min = atoi(min);
	}
	return 0;
}
/****************************************************************************************************/

int push_smoke_var_data(int num,ST_SM_VDCS_VIDEO_DRAW tmp)
{
	int x=0; 
	int y=0;
	int width =0;
	int height =0;

	if(tmp.Type  == 1){//  rect
		x=(int )tmp.StartX;
		y=(int )tmp.StartY;
		width =(int )tmp.EndX;
		height=(int )tmp.EndY;		
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamSmkAlarm.Rects.push_back(Rect(x, y, width, height));
	}
	
	return 0;
}

int smokedetectParamReset(ST_SM_VDCS_VIDEO_FUNC_PARAM * pt_VideoFuncParam,int num,char *buffer)
{
	uint16 PkgNum =0;
	int i=0;
	vector<ST_SM_VDCS_VIDEO_DRAW>	videodraw;
	ST_SM_VDCS_VIDEO_DRAW  tmpVideoDraw;

	if(pt_VideoFuncParam->EnableAlarm == 0){      //  disable
		if(t_Camera.t_SinCam[num].t_Camvarparam.t_CamSmkAlarm.DetectEn ==0 ){
				dbgprint("disable smoke ,but is already disable\n ");
				return 0;
		}
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamSmkAlarm.DetectEn = 0;
		t_Camera.t_SinCam[num].CamAnalyze->turnoff_mcu_alarm();
		memset(&t_Camera.t_SinCam[num].t_Camvarparam.t_CamSmkAlarm,0,sizeof(T_CAM_OTH_ALARM));
	}else{
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamSmkAlarm.DetectEn = 0;
		t_Camera.t_SinCam[num].CamAnalyze->turnoff_mcu_alarm();
		usleep(800*1000);
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamSmkAlarm.AutoWarn = pt_VideoFuncParam->AutoAlarm;
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamSmkAlarm.AlarmType = pt_VideoFuncParam->AlarmType;
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamSmkAlarm.Rects.clear();

		//todo :时间解析
		OthAlarmTime_analyse(&t_Camera.t_SinCam[num].t_Camvarparam.t_CamSmkAlarm,pt_VideoFuncParam->AlarmTime);

		PkgNum  = pt_VideoFuncParam->PkgNum;
		if(PkgNum  == 0 ||PkgNum  > 20){
			dbgprint("wrong packge number %d!\n",PkgNum);
			memset(&t_Camera.t_SinCam[num].t_Camvarparam.t_CamSmkAlarm,0,sizeof(T_CAM_OTH_ALARM));
			return -1;
		}

		for(i=0; i< PkgNum; i++)
		{
			memcpy(&tmpVideoDraw,buffer+sizeof(ST_SM_VDCS_VIDEO_FUNC_PARAM)+i*(sizeof(ST_SM_VDCS_VIDEO_DRAW)),sizeof(ST_SM_VDCS_VIDEO_DRAW));
			videodraw.push_back(tmpVideoDraw);
		}
		
		for(i =0; i < (int)videodraw.size() ;i++){
			ST_SM_VDCS_VIDEO_DRAW tmp = videodraw[i]; 
			push_smoke_var_data(num,tmp);
		}
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamSmkAlarm.DetectEn = 1;
		videodraw.clear();
	}
	
	return 0;
}



int push_alarmregion_var_data(int num,ST_SM_VDCS_VIDEO_DRAW tmp)
{
	int x=0; 
	int y=0;
	int width =0;
	int height =0;

	if(tmp.Type  == 1){//  rect
		x=(int )tmp.StartX;
		y=(int )tmp.StartY;
		width =(int )tmp.EndX;
		height=(int )tmp.EndY;		
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamAlarmRegionAlarm.Rects.push_back(Rect(x, y, width, height));
	}
	
	return 0;
}

int alarmregionParamReset(ST_SM_VDCS_VIDEO_FUNC_PARAM * pt_VideoFuncParam,int num,char *buffer)
{
	uint16 PkgNum =0;
	int i=0;
	vector<ST_SM_VDCS_VIDEO_DRAW>	videodraw;
	ST_SM_VDCS_VIDEO_DRAW  tmpVideoDraw;
	
	if(pt_VideoFuncParam->EnableAlarm == 0){	  //  disable
		if(t_Camera.t_SinCam[num].t_Camvarparam.t_CamAlarmRegionAlarm.DetectEn==0 ){
			dbgprint("disable region ,but is already disable \n");
			return 0;
		}
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamAlarmRegionAlarm.DetectEn = 0;
		t_Camera.t_SinCam[num].CamAnalyze->turnoff_mcu_alarm();
		memset(&t_Camera.t_SinCam[num].t_Camvarparam.t_CamAlarmRegionAlarm,0,sizeof(T_CAM_OTH_ALARM));
	}else{
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamAlarmRegionAlarm.DetectEn = 0;
		t_Camera.t_SinCam[num].CamAnalyze->turnoff_mcu_alarm();
		usleep(800*1000);
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamAlarmRegionAlarm.Rects.clear();
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamAlarmRegionAlarm.AutoWarn = pt_VideoFuncParam->AutoAlarm;
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamAlarmRegionAlarm.AlarmType = pt_VideoFuncParam->AlarmType;

		//todo :时间解析
		OthAlarmTime_analyse(&t_Camera.t_SinCam[num].t_Camvarparam.t_CamAlarmRegionAlarm,pt_VideoFuncParam->AlarmTime);

		PkgNum	= pt_VideoFuncParam->PkgNum;
		if(PkgNum  == 0 ||PkgNum  > 20){
			dbgprint("wrong packge number %d!\n",PkgNum);
			memset(&t_Camera.t_SinCam[num].t_Camvarparam.t_CamAlarmRegionAlarm,0,sizeof(T_CAM_OTH_ALARM));
			return -1;
		}

		for(i=0; i< PkgNum; i++)
		{
			memcpy(&tmpVideoDraw,buffer+sizeof(ST_SM_VDCS_VIDEO_FUNC_PARAM)+i*(sizeof(ST_SM_VDCS_VIDEO_DRAW)),sizeof(ST_SM_VDCS_VIDEO_DRAW));
			videodraw.push_back(tmpVideoDraw);
		}
		
		for(i =0; i < (int)videodraw.size() ;i++){
			ST_SM_VDCS_VIDEO_DRAW tmp = videodraw[i]; 
			push_alarmregion_var_data(num,tmp);
		}
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamAlarmRegionAlarm.DetectEn = 1;
		videodraw.clear();
	}


	return 0;
}


int push_alarmfire_var_data(int num,ST_SM_VDCS_VIDEO_DRAW tmp)
{
	int x=0; 
	int y=0;
	int width =0;
	int height =0;

	if(tmp.Type  == 1){//  rect
		x=(int )tmp.StartX;
		y=(int )tmp.StartY;
		width =(int )tmp.EndX;
		height=(int )tmp.EndY;		
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamFireAlarm.Rects.push_back(Rect(x, y, width, height));
	}
	
	return 0;
}

int firedetectParamReset(ST_SM_VDCS_VIDEO_FUNC_PARAM * pt_VideoFuncParam,int num,char *buffer)
{
	uint16 PkgNum =0;
	int i=0;
	vector<ST_SM_VDCS_VIDEO_DRAW>	videodraw;
	ST_SM_VDCS_VIDEO_DRAW  tmpVideoDraw;
	
	if(pt_VideoFuncParam->EnableAlarm == 0){	  //  disable
	
		if(t_Camera.t_SinCam[num].t_Camvarparam.t_CamFireAlarm.DetectEn==0 ){
			dbgprint("disable fire ,but is already disable \n");
			return 0;
		}
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamFireAlarm.DetectEn = 0;
		t_Camera.t_SinCam[num].CamAnalyze->turnoff_mcu_alarm();
		memset(&t_Camera.t_SinCam[num].t_Camvarparam.t_CamFireAlarm,0,sizeof(T_CAM_OTH_ALARM));
	}else{
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamFireAlarm.DetectEn = 0;
		t_Camera.t_SinCam[num].CamAnalyze->turnoff_mcu_alarm();
		usleep(800*1000);
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamFireAlarm.Rects.clear();
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamFireAlarm.AutoWarn = pt_VideoFuncParam->AutoAlarm;
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamFireAlarm.AlarmType = pt_VideoFuncParam->AlarmType;

		//todo :时间解析
		OthAlarmTime_analyse(&t_Camera.t_SinCam[num].t_Camvarparam.t_CamFireAlarm,pt_VideoFuncParam->AlarmTime);

		PkgNum	= pt_VideoFuncParam->PkgNum;
		if(PkgNum  == 0 ||PkgNum  > 20){
			dbgprint("wrong packge number %d!\n",PkgNum);
			memset(&t_Camera.t_SinCam[num].t_Camvarparam.t_CamFireAlarm,0,sizeof(T_CAM_OTH_ALARM));
			return -1;
		}

		for(i=0; i< PkgNum; i++)
		{
			memcpy(&tmpVideoDraw,buffer+sizeof(ST_SM_VDCS_VIDEO_FUNC_PARAM)+i*(sizeof(ST_SM_VDCS_VIDEO_DRAW)),sizeof(ST_SM_VDCS_VIDEO_DRAW));
			videodraw.push_back(tmpVideoDraw);
		}
		
		for(i =0; i < (int)videodraw.size() ;i++){
			ST_SM_VDCS_VIDEO_DRAW tmp = videodraw[i]; 
			push_alarmfire_var_data(num,tmp);
		}
		t_Camera.t_SinCam[num].t_Camvarparam.t_CamFireAlarm.DetectEn = 1;
		videodraw.clear();
	}


	return 0;
}


int set_camera_param(char * buffer)
{
	int num = -1;
	ST_SM_VDCS_VIDEO_FUNC_PARAM   t_VideoFuncParam;
	
	memcpy(&t_VideoFuncParam,buffer+PACKET_HEAD_LEN,sizeof(ST_SM_VDCS_VIDEO_FUNC_PARAM));

	num = search_camera(t_VideoFuncParam.CameUrl);
	if((num <  0)||(num > MAX_CAM_NUM-1)) {
		dbgprint("reset  camera wrong because of wrong url!\n");
		return -1;
	}
	printf("num is %d\n",num);
	switch  (t_VideoFuncParam.AlarmType){
		case  HumanDetect:
			  printf("set human param\n");
			  humandetectParamReset(&t_VideoFuncParam,num,buffer+PACKET_HEAD_LEN);	
			  break;
		case  SmokeDetect:
			  printf("set smoke param\n");
		          smokedetectParamReset(&t_VideoFuncParam,num,buffer+PACKET_HEAD_LEN);
			  break;
		case  FireDetect:
			  printf("set fire param\n");
			  firedetectParamReset(&t_VideoFuncParam,num,buffer+PACKET_HEAD_LEN);
			  break;
		case  FixedObjDetect:
			  break;
		case  AlarmRegionDetect:
			  printf("set region param\n");
			  alarmregionParamReset(&t_VideoFuncParam,num,buffer+PACKET_HEAD_LEN);
			  break;
		default:
			  break;
	}
	return 0;
}


static void client_timer_Heartbeat_task(int fd, short events, void * arg) 
{
	PT_TcpClient instance  =  (PT_TcpClient)arg; 
	struct timeval delay = { 0, 200 };
	
	event_base_loopexit(instance->pt_PServerBase, &delay);

	return ;
}

int process_heart_beat(PT_TcpClient instance)
{
	int iRet =	-1; 
	iRet =	event_del(instance->pt_PServerTimerEvHeartbeat);	
	if(iRet <	0){
		printf("--coudnt del time event--\n");
		return -1;
	}
	
	iRet =	event_add(instance->pt_PServerTimerEvHeartbeat, &(instance->t_PServerHeartbeatTv));	
	if(iRet <	0){
		printf("--coudnt add time event--\n");
		return -1;
	}
	
	return 0;
}

static int server_data_analyse(char * buffer,int len,PT_TcpClient instance)
{
	uint16 cmd;
	T_PacketHead   t_packet_head;
	
	if(len < PACKET_HEAD_LEN){
		 printf("receive error!\n");
		 return SERVER_DATA_ERR;
	}

	memcpy(&t_packet_head,buffer,PACKET_HEAD_LEN);
	cmd = t_packet_head.cmd;
	
	switch (cmd ){
		case SM_VDCS_ANAY_PUSH_CAMERA_PARAM: 
			  printf("SM_VDCS_ANAY_PUSH_CAMERA_PARAM\n");
		      	  camera_process(buffer,len,instance);
			  break;
		case SM_VDCS_ANAY_REGISTER_ACK:      
			  anay_register_ack(buffer,instance);
		      break;
		case SM_VDCS_ANAY_DEVICE_CONTROL:    
			  device_control_process(buffer,instance);
			  break;
		case SM_VDCS_ANAY_DEVICE_STATUS_ACK: 
			  mcu_inactive_ack(buffer,instance);
		      break;
		case SM_VDCS_ANAY_WARN_INFO_ACK:      
			  warn_info_ack(buffer);
			  break;
		case SM_VDCS_ANAY_ROUTING_INSPECTION_ACK:
			  routing_inspection_ack(buffer);
			  break;
		case SM_VDCS_ANAY_RENEW_REGISTER_MCU_ACK:
			  anay_re_register_ack(buffer,instance);
			  break;
		case SM_VDCS_ANAY_SET_CAMERA_PARAM:
		  	printf("SM_VDCS_ANAY_SET_CAMERA_PARAM\n");
                         set_camera_param(buffer);
			 break;
		case  SM_ANAY_HEATBEAT:
			//process_heart_beat(instance);
			break;
		default: break;
	}
	dbgprint("server cmd = %x\n", cmd);
	return 0;
}

static void client_read_cb(struct bufferevent* bev, void* arg)	
{
	char buf[2048];  
	int 	iLen;
	PT_TcpClient instance  =  (PT_TcpClient)arg; 
	
	memset(buf,0,2048);
	struct evbuffer *input = bufferevent_get_input(bev);
	if (evbuffer_get_length(input) == 0) {
		printf("no data read!\n");
		bufferevent_free(bev);
	} 
	if(evbuffer_get_length(input) >2048) return ;
	
	iLen = evbuffer_remove(input, buf, sizeof(buf)); 

	server_data_analyse(buf,iLen,instance);
	
	return ;
}

static int mcu_empty()
{
	int i=0;
	
	for(i;i<6;i++)
	{
		if(t_McuMess.cMac[i] != 0x00)
			                 return 0;
	}
	return 1;
}

static int analyze_register(PT_TcpClient instance)
{

	ST_SM_ANAY_VDCS_REGISTER st_AnayReg;
	T_PacketHead             t_PackHeadAnayReg;
	char AnayRegBuff[60] = {0};
	int iRet = -1;
	memset(&t_PackHeadAnayReg,0 ,sizeof(t_PackHeadAnayReg));
	memset(&st_AnayReg,0 ,sizeof(st_AnayReg));
	t_PackHeadAnayReg.magic          = T_PACKETHEAD_MAGIC;
	t_PackHeadAnayReg.cmd	         = SM_ANAY_VDCS_REGISTER;
	t_PackHeadAnayReg.UnEncryptLen   = sizeof(ST_SM_ANAY_VDCS_REGISTER);

	while(mcu_empty()){
		   sleep(2);
	}
	
	memcpy(st_AnayReg.MCUAddr,t_McuMess.cMac,MCU_MAC_LEN_20);

	memcpy(AnayRegBuff,&t_PackHeadAnayReg,sizeof(t_PackHeadAnayReg));
	memcpy(AnayRegBuff+sizeof(t_PackHeadAnayReg),&st_AnayReg,sizeof(st_AnayReg));
	iRet = send(t_SerMess.iClientFd,AnayRegBuff,sizeof(AnayRegBuff),0);
	dbgprint("send server %d \n",iRet);
	return iRet;
}

static int send_hreatbeat(PT_TcpClient instance)
{
	int iRet = -1;
	T_PacketHead heartbeat;
	memset(&heartbeat, 0 ,sizeof(heartbeat));
	heartbeat.magic        	=  T_PACKETHEAD_MAGIC;
	heartbeat.cmd	       		=  SM_ANAY_HEATBEAT;
	heartbeat.UnEncryptLen =  0;
	iRet = send(t_SerMess.iClientFd,&heartbeat,sizeof(T_PacketHead),0);
	return iRet;
}

static void client_timer_task(int fd, short events, void * arg) 
{
	PT_TcpClient instance  =  (PT_TcpClient)arg; 
	
	if((!t_SerMess.iAnayRegister))
	{
		analyze_register(instance);
		sleep(1);
	}
	if(t_SerMess.iAnayRegister){
		send_hreatbeat(instance);
	}
	event_add(instance->pt_PServerTimerEv, &(instance->t_PServerTv));
	return ;
}

static void client_event_cb(struct bufferevent *bev, short event, void *arg)	
{
	PT_TcpClient instance  =  (PT_TcpClient)arg; 
	struct timeval delay = { 0, 200 };

	if (event & BEV_EVENT_TIMEOUT) {
		printf("--process server connectTimed out--\n"); 
	}
	else if (event & BEV_EVENT_EOF)	{
		printf("connection closed\n");
	}
	else if (event & BEV_EVENT_ERROR) { 
		printf("process server close connection\n"); 
	}
	else if( event & BEV_EVENT_CONNECTED)  {  
		t_SerMess.ialive	 = 1;
		printf("--the client has connected to process server--\n");
		return;
	} 
	event_base_loopexit(instance->pt_PServerBase, &delay);
}


void * tcp_server_thread(void *param)
{
	T_TcpClient   t_ClientInstance;
	int 	 Client_fd   = -1;
	
	t_SerMess.iConnectFlag  =0;

	memset(&t_ClientInstance, 0 ,sizeof(T_TcpClient));

	sleep(2);
	t_ClientInstance.pt_PServerBase  = event_base_new();  
	if (!t_ClientInstance.pt_PServerBase) {
		printf("Could not initialize libevent!\n");
		return NULL;
	}

	t_ClientInstance.pt_PServerBev = bufferevent_socket_new(t_ClientInstance.pt_PServerBase, -1, BEV_OPT_CLOSE_ON_FREE);  
	if (!t_ClientInstance.pt_PServerBev) {
		printf("Error constructing bufferevent!\n");
		return NULL;
	}

	t_ClientInstance.t_PServerTv.tv_sec 	=	5;
	t_ClientInstance.t_PServerTv.tv_usec	=	0;	

	t_ClientInstance.t_PServerHeartbeatTv.tv_sec 	=	15;
	t_ClientInstance.t_PServerHeartbeatTv.tv_usec	=	0;		

	memset(&t_ClientInstance.t_PSeverAddr, 0, sizeof(t_ClientInstance.t_PSeverAddr) );	
	t_ClientInstance.t_PSeverAddr.sin_family	=	AF_INET;  
	t_ClientInstance.t_PSeverAddr.sin_port	=	htons(atoi(t_DevParam.server_port));  
	inet_aton(t_DevParam.server_ip, &t_ClientInstance.t_PSeverAddr.sin_addr);

	bufferevent_socket_connect(t_ClientInstance.pt_PServerBev, (struct sockaddr *)&t_ClientInstance.t_PSeverAddr,  sizeof(t_ClientInstance.t_PSeverAddr));

	Client_fd = bufferevent_getfd(t_ClientInstance.pt_PServerBev); 
	while (connect(Client_fd, (struct sockaddr *)&t_ClientInstance.t_PSeverAddr, sizeof(t_ClientInstance.t_PSeverAddr)) < 0){
		sleep(2);
	}

	t_SerMess.iClientFd	=  Client_fd;

	bufferevent_setcb(t_ClientInstance.pt_PServerBev ,client_read_cb, NULL, client_event_cb, (void*)&t_ClientInstance);  
	bufferevent_enable(t_ClientInstance.pt_PServerBev, EV_READ |EV_WRITE);

	t_ClientInstance.pt_PServerTimerEv = evtimer_new(t_ClientInstance.pt_PServerBase,client_timer_task,(void*)&t_ClientInstance);
	if (!t_ClientInstance.pt_PServerTimerEv || event_add(t_ClientInstance.pt_PServerTimerEv, &t_ClientInstance.t_PServerTv)<0) {
		printf("Could not create/add a timer_event!\n");
		return NULL;
	}

//	t_ClientInstance.pt_PServerTimerEvHeartbeat = evtimer_new(t_ClientInstance.pt_PServerBase,client_timer_Heartbeat_task,(void*)&t_ClientInstance);
//	if (!t_ClientInstance.pt_PServerTimerEvHeartbeat || event_add(t_ClientInstance.pt_PServerTimerEvHeartbeat, &t_ClientInstance.t_PServerHeartbeatTv)<0) {
//		printf("Could not create/add a timer_event  Heartbeat!\n");
//		return NULL;
//	}
		
	event_base_dispatch (t_ClientInstance.pt_PServerBase);	
	// 释放资源
	event_base_free 	(t_ClientInstance.pt_PServerBase);
	bufferevent_free  (t_ClientInstance.pt_PServerBev); 
	evtimer_del 	         (t_ClientInstance.pt_PServerTimerEv);

	
	//对于全局变量的资源处理
	if(t_SerMess.iClientFd > 0){
		close (t_SerMess.iClientFd);
		t_SerMess.iClientFd = -1;
	}
	t_SerMess.ialive	 	   = 0;
	t_SerMess.iConnectFlag  = 1;
	t_SerMess.iAnayRegister = 0;
	
	t_Camera.CameraFlag =0;
	sleep(8);
	memset(&t_Camera,0,sizeof(T_Camera));
	 printf("-- tcp_server_thread is down--\n");
	 pthread_exit(NULL);
	 return NULL;

}


int  re_connect_server(void )
{
	int iRet = -1;
 	
	pthread_t ClientID;
	
	iRet = pthread_create(&ClientID,NULL,tcp_server_thread,NULL);
	if(iRet != 0){
		 printf("create client_thread error!\n");
		 return -1;
	} 
	pthread_detach(ClientID);
	return 0;
}


