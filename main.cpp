
#include  "Common.h"
#include  "CmdDefine.h"
#include  "Xmlparser.h"
#include  "Udptrans.h"
#include 	"TcpCom.h"
#include  "Analyze.h"

char cfg_file[20] = "device.xml";

T_DeviceParam 	t_DevParam;
T_ServerMess  	t_SerMess;
T_ClientMess    	t_McuMess;   
T_Camera              t_Camera;

int main(int argc,char**argv)
{

	int iRet = -1;
	pthread_t mcu_pthread_ID;
	pthread_t server_pthread_ID;

	memset(&t_DevParam,0,sizeof(T_DeviceParam));
	memset(&t_SerMess,0,sizeof(T_ServerMess));
	memset(&t_McuMess,0,sizeof(T_ServerMess));
	
	CXmlparser* xmlparser  = new CXmlparser(cfg_file);
	xmlparser->GetConfigparam(t_DevParam);

	iRet = pthread_create(&mcu_pthread_ID,NULL,tcp_mcu_thread,NULL);
	if(iRet != 0){
		 printf("create client_thread error!\n");
		 return -1;
	} 
	pthread_detach(mcu_pthread_ID);

	iRet = pthread_create(&server_pthread_ID,NULL,tcp_server_thread,NULL);
	if(iRet != 0){
		 printf("create client_thread error!\n");
		 return -1;
	} 
	pthread_detach(server_pthread_ID);

	while(1){	
		sleep(5);
		
		if((t_SerMess.ialive  == 0)&&(t_SerMess.iConnectFlag == 1)){
			re_connect_server();
		}
	}	
	delete  xmlparser;
	return 0;
}

