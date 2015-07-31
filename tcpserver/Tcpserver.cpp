#include "Common.h"
#include "TcpCom.h"

extern T_ClientMess    	t_McuMess;   
extern T_ServerMess	t_SerMess;

int search_mcu_client(char *mac)
{
	int 	fd 	= 	-1;
	int 	i	=	0;
	char MAC[20]={0};
	char tmp[3]={0};

	for(i=0;i<6;i++){	
		tmp[0]= mac[i*2+0];
		tmp[1]= mac[i*2+1];
		MAC[i] =(atoi(tmp)/10)*16+atoi(tmp)%10;
	}
	
	if(strcmp(MAC,t_McuMess.cMac)==0){
		    if(t_McuMess.iAlive){
				return t_McuMess.iFd;
			}else{
				dbgprint("mcu is not alive!\n");
			}
	}else{
		dbgprint("dont exit this mcu!\n");
	}
	return fd;
}

int mcu_register_mac_match(char *mac)
{
	int iNum	;
	int istate  = 0;
	for(iNum=0;iNum<MCU_MAC_LEN_20;iNum++){
	   		if(t_McuMess.cMac[iNum] !=0x00){
	   			istate = 1;
	   			break;
	   		}  					
	 }

	 if(istate == 0)
	 		return MCU_NOT_REGISTER;  

	 if(istate == 1){

	 	for(iNum=0;iNum<MCU_MAC_LEN_20;iNum++){
		 		if(t_McuMess.cMac[iNum] !=mac[iNum]){
		 		return MCU_NEW_REGISTER;
	 		}
	 	}
	 	
	 	return MCU_HAVE_REGISTER;
	 }
	 return 0;
}


int mcu_inactive_report(void)
{
	T_PacketHead                 				  t_PackHeadMcuInactive;
	ST_SM_ANAY_VDCS_DEVICE_STATUS  	  t_AnayDeviceStatus;
	char ReBuff[28+162]={0};
	int    	iRet = -1;
	
	memset(&t_PackHeadMcuInactive,0,sizeof(T_PacketHead));
	t_PackHeadMcuInactive.magic        	=  	T_PACKETHEAD_MAGIC;
	t_PackHeadMcuInactive.cmd          	=  	SM_ANAY_VDCS_DEVICE_STATUS;
	t_PackHeadMcuInactive.UnEncryptLen 	= 	sizeof(ST_SM_ANAY_VDCS_DEVICE_STATUS);

	memset(&t_AnayDeviceStatus,0,sizeof(ST_SM_ANAY_VDCS_DEVICE_STATUS));
	memcpy(t_AnayDeviceStatus.MCUAddr , t_McuMess.cMac,MCU_MAC_LEN_20 );
	t_AnayDeviceStatus.DeviceType   =   DeviceTypeMcu;

	memcpy(ReBuff,&t_PackHeadMcuInactive,sizeof(T_PacketHead));
	memcpy(ReBuff+sizeof(t_PackHeadMcuInactive),&t_AnayDeviceStatus,sizeof(ST_SM_ANAY_VDCS_DEVICE_STATUS));

	iRet = send(t_SerMess.iClientFd,ReBuff,sizeof(ReBuff),0);	
	if(iRet <=0 ){
		dbgprint("send data to server error !\n");
		return MCU_DATA_INAC_REPO_SEND_ERR;
	}
	dbgprint("send %d byte to server !\n",iRet);
	return 0;
}

int mcu_operate_ack(char *buffer)  
{
	ST_SM_MCU_VDCS_OPERATE_TERM_ACK      mcu_operate_ack;
	
	T_PacketHead                         t_PackHeadOprAck;                  
	ST_SM_ANAY_VDCS_DEVICE_CONTROL_ACK   anay_operate_ack;
	uint16 ack;
	char   OprAckBuff[28+88]={0};
	int    iRet =-1;
	
	memset(&mcu_operate_ack, 0 ,sizeof(ST_SM_MCU_VDCS_OPERATE_TERM_ACK));
	memcpy(&mcu_operate_ack ,buffer+PACKET_HEAD_LEN,sizeof(ST_SM_MCU_VDCS_OPERATE_TERM_ACK));
	
	ack = mcu_operate_ack.Ack;		
	if(ack == 0)
			dbgprint("operate success!\n");
	else
			dbgprint("operate failed!\n");

   	if( mcu_operate_ack.UserName[0] ==0)
							 				 return 0;
	
	memset(&t_PackHeadOprAck, 0 ,sizeof(T_PacketHead));
	memset(&anay_operate_ack, 0 ,sizeof(ST_SM_ANAY_VDCS_DEVICE_CONTROL_ACK));

	t_PackHeadOprAck.magic        	 =  T_PACKETHEAD_MAGIC;
	t_PackHeadOprAck.cmd          	 =  SM_ANAY_VDCS_DEVICE_CONTROL_ACK;
	t_PackHeadOprAck.UnEncryptLen =  sizeof(ST_SM_ANAY_VDCS_DEVICE_CONTROL_ACK);
		
	anay_operate_ack.Ack          		=  (uint8)mcu_operate_ack.Ack;
	anay_operate_ack.DeviceType   	=  mcu_operate_ack.TermType;	
	anay_operate_ack.Port         		=  mcu_operate_ack.port;
	memcpy(anay_operate_ack.MCUAddr,(char *)mcu_operate_ack.MCUAddr,MCU_MAC_LEN_20);						
	memcpy(anay_operate_ack.UserName,(char *)mcu_operate_ack.UserName,USRNAME_LEN_20); 
	
	memcpy(OprAckBuff,&t_PackHeadOprAck,sizeof(t_PackHeadOprAck));
	memcpy(OprAckBuff+sizeof(t_PackHeadOprAck),&anay_operate_ack,sizeof(anay_operate_ack));
	iRet = send(t_SerMess.iClientFd,OprAckBuff,sizeof(OprAckBuff),0);	
	if(iRet <=0 ){
		dbgprint("send operate ack error!\n");
		return MCU_DATA_OPR_ACK_SEND_ERR;
	}
	dbgprint("send %d byte to server !\n",iRet);
	return 0;
}


int routing_inspection(char *buffer)
{
	char  	RoutBuff[28+53]={0};

	int   		iRet =-1;

	T_PacketHead						 		t_PackHeadRouting; 
	ST_SM_ANAY_VDCS_ROUTING_INSPECTION   	t_AnyRoutInspection;

	ST_SM_MCU_VDCS_ROUTING_INSPECTION   		 t_McuRoutInspection;
	
	memset(&t_McuRoutInspection, 0 ,sizeof(ST_SM_MCU_VDCS_ROUTING_INSPECTION));
	memcpy(&t_McuRoutInspection ,buffer+PACKET_HEAD_LEN,sizeof(ST_SM_MCU_VDCS_ROUTING_INSPECTION));

	//printf("  ReControl is ");
	//for(uint8 i=0 ; i<RE_CONTROL_LEN_20; i++)
		//printf("%02x  ",t_McuRoutInspection.ReControl[i]);
	//printf("\n");
	
	if(t_McuRoutInspection.Action == 'A'){
		memset(&t_PackHeadRouting, 0 ,sizeof(T_PacketHead));
		memset(&t_AnyRoutInspection, 0 ,sizeof(ST_SM_ANAY_VDCS_ROUTING_INSPECTION));

		t_PackHeadRouting.magic		   =  T_PACKETHEAD_MAGIC;
		t_PackHeadRouting.cmd		   =  SM_ANAY_VDCS_ROUTING_INSPECTION;
		t_PackHeadRouting.UnEncryptLen   =  sizeof(ST_SM_ANAY_VDCS_ROUTING_INSPECTION);

		memcpy(t_AnyRoutInspection.ReControl,t_McuRoutInspection.ReControl, RE_CONTROL_LEN_20);
		memcpy(t_AnyRoutInspection.McuAddr,t_McuMess.cMac,MCU_MAC_LEN_20);
		t_AnyRoutInspection.Action    =   t_McuRoutInspection.Action ;
		
		memcpy(RoutBuff,&t_PackHeadRouting,sizeof(T_PacketHead));
		memcpy(RoutBuff+sizeof(T_PacketHead),&t_AnyRoutInspection,sizeof(ST_SM_ANAY_VDCS_ROUTING_INSPECTION));
		iRet = send(t_SerMess.iClientFd,RoutBuff,sizeof(RoutBuff),0);	
		if(iRet <=0 ){
			dbgprint("send routing inspection error!\n");
			return MCU_DATA_ROUTING_SEND_ERR;
		}
		dbgprint("send %d byte to server !\n",iRet);
		
	}else if(t_McuRoutInspection.Action == 'B'){

	}
	
	return 0;

}

int mcu_register_ack(void)
{
	char RegAckBuf[28 + 32] = {0};
	int 	iRet = -1; 
	T_PacketHead                      				t_PackHeadAck;
	ST_SM_VDCS_MCU_REGISTER_ACK       t_McuRegAck;
	
	memset(&t_PackHeadAck,0,sizeof(T_PacketHead));
	memset(&t_McuRegAck,0,sizeof(t_McuRegAck));
	t_PackHeadAck.magic               =  T_PACKETHEAD_MAGIC;
	t_PackHeadAck.cmd	            =  SM_VDCS_MCU_REGISTER_ACK;
	t_PackHeadAck.UnEncryptLen  =  sizeof(ST_SM_VDCS_MCU_REGISTER_ACK);
	
	memcpy(RegAckBuf,&t_PackHeadAck,sizeof(T_PacketHead));
	memcpy(t_McuRegAck.MCUAddr ,t_McuMess.cMac,MCU_MAC_LEN_20);
	t_McuRegAck.Ack =0;
	memcpy(RegAckBuf+sizeof(t_PackHeadAck),&t_McuRegAck,sizeof(ST_SM_VDCS_MCU_REGISTER_ACK));
	iRet = send(t_McuMess.iFd,RegAckBuf,sizeof(RegAckBuf),0);
	if(iRet <=0 ){
		dbgprint("send data to mcu error !\n");
		return MCU_DATA_REG_ACK_SEND_ERR;
	}
	dbgprint("send %d byte to mcu !\n",iRet);
	
	return 0;

}

int report_mcu_new_register(char *MAC,uint8 type)
{
	T_PacketHead					  			t_PackHeadNewReg;
	ST_SM_VDCS_ANAY_RENEW_REGISTER_MCU  	t_AnayNewReg;
	char ReNewRegBuff[28 + 33] ={0};
	int 	iRet = -1;
	
	memset(&t_PackHeadNewReg,0,sizeof(T_PacketHead));
	memset(&t_AnayNewReg,0,sizeof(ST_SM_VDCS_ANAY_RENEW_REGISTER_MCU));
	
	t_PackHeadNewReg.magic 			=  T_PACKETHEAD_MAGIC;
	t_PackHeadNewReg.cmd			=  SM_VDCS_ANAY_RENEW_REGISTER_MCU;
	t_PackHeadNewReg.UnEncryptLen	=  sizeof(ST_SM_VDCS_ANAY_RENEW_REGISTER_MCU);
		
	memcpy(ReNewRegBuff,&t_PackHeadNewReg,sizeof(T_PacketHead));
	memcpy(t_AnayNewReg.MCUAddr ,t_McuMess.cMac,MCU_MAC_LEN_20);
	t_AnayNewReg.ConnectType    =  type;
	memcpy(ReNewRegBuff+sizeof(T_PacketHead),&t_AnayNewReg,sizeof(ST_SM_VDCS_ANAY_RENEW_REGISTER_MCU));

	if(t_SerMess.iClientFd >0){
		iRet = send(t_SerMess.iClientFd,ReNewRegBuff,sizeof(ReNewRegBuff),0);
		if(iRet <=0 ){
			dbgprint("send data to server error !\n");
			return MCU_RE_REGISTER_REPORT_ERR;
		}
		dbgprint("send %d byte to server !\n",iRet);	
	}else{
		dbgprint("server is not connected!\n");	
	}
	return 0;
}

static int mcu_register_process(char *buffer)
{
 	char MAC[MCU_MAC_LEN_20] = {0};
 	int 	iMatchRet	= 	-1;
 	int 	iRet 		= 	-1;
 	
 	memcpy(MAC,buffer+PACKET_HEAD_LEN,MCU_MAC_LEN_20);	
 	iMatchRet = mcu_register_mac_match(MAC);

 	if(iMatchRet ==MCU_NOT_REGISTER){
 		dbgprint("--mcu start register!--\n");
 		memcpy(t_McuMess.cMac,buffer+PACKET_HEAD_LEN,MCU_MAC_LEN_20);	
 		iRet  = 	mcu_register_ack(); 
 		return iRet;
 	}
 	
 	if(iMatchRet ==MCU_HAVE_REGISTER){
		dbgprint("--mcu restart register!--\n");
		iRet  = 	mcu_register_ack(); 
		report_mcu_new_register(MAC,0);
		return iRet;
	}
	
 	if(iMatchRet ==MCU_NEW_REGISTER){
		dbgprint("--new mcu start register!--\n");
		memset(t_McuMess.cMac,0,MCU_MAC_LEN_20);
		memcpy(t_McuMess.cMac,buffer+PACKET_HEAD_LEN,MCU_MAC_LEN_20);	
		iRet  = 	mcu_register_ack(); 
		report_mcu_new_register(MAC,1);
		return iRet;
	}
	return MCU_REGISTER_ERR;
}

static int mcu_heartbeat_process(PT_TcpServer  pt_TcpServer)
{
	int iRet =	-1; 
	iRet =	event_del(pt_TcpServer->pt_AServerTimerEv);	
	if(iRet <	0){
		printf("--coudnt del time event--\n");
		return MCU_TIME_EVENT_ERR;
	}
	
	iRet =	event_add(pt_TcpServer->pt_AServerTimerEv, &(pt_TcpServer->t_AServerTv));	
	if(iRet <	0){
		printf("--coudnt add time event--\n");
		return MCU_TIME_EVENT_ERR;
	}
	
	t_McuMess.iAlive  = 1;
	return 0;
}

static int mcu_analyze(struct bufferevent *bev,char* buffer, int len,PT_TcpServer  pt_TcpServer)
 {
	uint16 	cmd;
	int    		iRet =0;
	T_PacketHead   t_packet_head;
	
	if(len < PACKET_HEAD_LEN){		
			printf("mcu recive data less!\n");
			return MCU_REC_DATA_NUM_LESS;	
	}
	
	memcpy(&t_packet_head,buffer,PACKET_HEAD_LEN);
	cmd = t_packet_head.cmd;

	switch (cmd ){
	
	case SM_MCU_VDCS_REGISTER:   
			iRet  = 	 mcu_register_process(buffer);
			break;
			
	case SM_MCU_HEARTBEAT: 
			iRet  =	mcu_heartbeat_process(pt_TcpServer);	
			break;
			 
	case SM_MCU_VDCS_OPERATE_TERM_ACK:		
			iRet  =	mcu_operate_ack(buffer);		
		        break;
		        
	case SM_MCU_VDCS_ROUTING_INSPECTION:
	         	iRet  =	routing_inspection(buffer);		
	                break;
	                
	default:  break;
	}
	
	//dbgprint("client cmd = %x\n", cmd);
	return iRet;
 }

static void conn_mcu_readcb(struct bufferevent *bev, void *arg)
{
	char cBuf[1024];  
	int  	iLen =  0;
	int  	iRet = -1;	
	PT_TcpServer  pt_TcpServer = (PT_TcpServer)arg;
	
	memset(cBuf,0,1024); 
	
	struct evbuffer *input = bufferevent_get_input(bev);
	if (evbuffer_get_length(input) == 0) {
		printf("no data read!\n");
		bufferevent_free(bev);
	} 

	iLen = evbuffer_remove(input, cBuf, sizeof(cBuf)); 

	iRet = mcu_analyze(bev,(char *)cBuf,iLen,pt_TcpServer); 
	if(iRet > 0 )
		    printf("mcu analyze data error ,type of error is %d\n",iRet);

}

static void conn_mcu_eventcb(struct bufferevent *bev, short events, void *arg)
{
	PT_TcpServer  pt_TcpServer = (PT_TcpServer)arg;

	if (events & BEV_EVENT_TIMEOUT) {
			printf("--mcu connectTimed out--\n"); 
	}
	else if (events & BEV_EVENT_EOF) {
			printf("--mcu connection closed--\n");
	}
	
	else if (events & BEV_EVENT_ERROR) {
			printf("--mcu some other error--\n");
	}
	
	evtimer_del  (pt_TcpServer->pt_AServerTimerEv);
	bufferevent_free(bev);	
}

static void mcu_timer_task(int fd, short events, void * arg) 
{
	int Client_fd 	= 	-1;
	PT_TcpServer  pt_TcpServer = (PT_TcpServer)arg;

	if( t_McuMess.iAlive )
	{  
	   	Client_fd = bufferevent_getfd(pt_TcpServer->pt_AServerBev);
		printf("--mcu is not alive--\n");
		mcu_inactive_report();
		close(Client_fd);
		memset(&t_McuMess, 0 , sizeof(T_ClientMess));
		t_McuMess.iFd 		=	-1;
	}
}

static void mcu_listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
                                            struct sockaddr *sa, int socklen, void *arg )
{
	PT_TcpServer  pt_TcpServer = (PT_TcpServer)arg;
	
	printf("sa->sa_data is %s \n",inet_ntoa(((struct sockaddr_in*)sa)->sin_addr));
/*
	if(t_McuMess.pt_ClientAddr != NULL){
		printf("inet_ntoa(t_McuMess.pt_ClientAddr->sin_addr) is %s\n",inet_ntoa(t_McuMess.pt_ClientAddr->sin_addr));
		if(strcmp(inet_ntoa(((struct sockaddr_in*)sa)->sin_addr),inet_ntoa(t_McuMess.pt_ClientAddr->sin_addr)) == 0)
			return ;
	}*/
	//if()
	/*
	if(strcmp(inet_ntoa(((struct sockaddr_in*)sa)->sin_addr),inet_ntoa(t_McuMess.pt_ClientAddr->sin_addr)) == 0)
	return ;
	*/

/*
	if(t_McuMess.pt_ClientAddr->sin_addr.s_addr ==sa->sa_data)
		return ;
*/		
	pt_TcpServer->pt_AServerBev = bufferevent_socket_new(pt_TcpServer->pt_AServerBase, fd, BEV_OPT_CLOSE_ON_FREE);
	if (!pt_TcpServer->pt_AServerBev) {
		printf("Error constructing bufferevent!\n");
		return ;
	}	

	dbgprint("--mcu fd listened is %d--\n",fd);
	
	t_McuMess.iFd                    = 	fd;
	t_McuMess.pt_ClientAddr   = 	(struct sockaddr_in*)sa;
	
	
	bufferevent_setcb	(pt_TcpServer->pt_AServerBev, conn_mcu_readcb, NULL, conn_mcu_eventcb, (void *)pt_TcpServer);
	bufferevent_enable	(pt_TcpServer->pt_AServerBev, EV_WRITE|EV_READ);
	
	pt_TcpServer->pt_AServerTimerEv= evtimer_new(pt_TcpServer->pt_AServerBase,mcu_timer_task,(void *)pt_TcpServer);
	if (!pt_TcpServer->pt_AServerTimerEv || event_add(pt_TcpServer->pt_AServerTimerEv, &(pt_TcpServer->t_AServerTv))<0) {
		printf("Could not create/add a timer_event!\n");
		return ;
   	}

}

void *tcp_mcu_thread(void* arg)
{
	T_TcpServer t_Instance;

	memset(&t_Instance, 0 ,sizeof(T_TcpServer));
	t_McuMess.iFd = -1;
	
	t_Instance.pt_AServerBase = event_base_new();
	if (!t_Instance.pt_AServerBase) {  
		printf("Could not initialize libevent!  need to restart program! \n");
		return NULL;
	}

	memset(&t_Instance.t_AServerSin , 0, sizeof(t_Instance.t_AServerSin));
	t_Instance.t_AServerSin.sin_family 	= 	AF_INET;
	t_Instance.t_AServerSin.sin_port 		= 	htons(SERVER_PORT);
	t_Instance.t_AServerTv.tv_sec 		= 	20;
	t_Instance.t_AServerTv.tv_usec 		= 	0;	

	t_Instance.pt_AServerListener 	= 	evconnlistener_new_bind(t_Instance.pt_AServerBase, mcu_listener_cb, (void *)&t_Instance,
								   	 LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,(struct sockaddr*)&t_Instance.t_AServerSin,
	                                				sizeof(t_Instance.t_AServerSin));
	if (!t_Instance.pt_AServerListener) {
		printf("Could not create a listener! need to restart program! \n");
		return NULL;
	}
	
	event_base_dispatch	(t_Instance.pt_AServerBase);

	//如果 MCU 线程退出，释放资源
	evconnlistener_free	(t_Instance.pt_AServerListener);
	event_base_free		(t_Instance.pt_AServerBase);	
	bufferevent_free          (t_Instance.pt_AServerBev);	
	evtimer_del                   (t_Instance.pt_AServerTimerEv);

	if(t_McuMess.iFd>0){
		close (t_McuMess.iFd);
		t_McuMess.iFd =	-1;
	}
	memset(&t_McuMess ,0 ,sizeof(T_ClientMess));
	
	dbgprint("mcu client thread has a breakdown!\n");

	return NULL;
}


