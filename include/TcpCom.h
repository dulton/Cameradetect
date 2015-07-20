#pragma once

#include 	"Common.h"
#include   "Analyze.h"
#include 	"CmdDefine.h"

typedef struct _server_mess{
	
	    int    iClientFd;
	    int    ialive;
	    int    iConnectFlag;
	    int    iAnayRegister;
		
}T_ServerMess,*PT_ServerMess;


enum {	

	SERVER_DATA_ERR =0x001,
};

typedef struct _tcp_client{
	
	struct sockaddr_in       t_PSeverAddr;		   
	struct event_base *     pt_PServerBase;		   
	struct event *               pt_PServerEv;				  
	struct timeval               t_PServerTv; 					  
	struct timeval               t_PServerHeartbeatTv; 	
	struct bufferevent*       pt_PServerBev;
	struct event *               pt_PServerTimerEv;
	struct event *               pt_PServerTimerEvHeartbeat;
	
}T_TcpClient,*PT_TcpClient;

void * tcp_server_thread(void *param);
int  re_connect_server(void );

///////////////////////////////////////////////////////////////////////////////////////

#define SERVER_PORT         8080

//     MCU错误类型        
enum {

	MCU_ACTIVE	                                           =0x001,
	MCU_IN_ACTIVE,
	MCU_REC_DATA_NUM_LESS,
	MCU_DATA_REG_ACK_SEND_ERR,
	MCU_DATA_INAC_REPO_SEND_ERR,
	MCU_DATA_OPR_ACK_SEND_ERR,
	MCU_DATA_ROUTING_SEND_ERR,
	MCU_TIME_EVENT_ERR,
	MCU_REGISTER_ERR,
	MCU_RE_REGISTER_REPORT_ERR,
	MCU_CLOSE_BEEP_ERR,
	MCU_RE_CONTROL_CMD_ERR
};

typedef struct _client_mess{

	int           iFd;	
	char       cMac[20];
	int           iAlive;
	struct sockaddr_in *   pt_ClientAddr;
	
}T_ClientMess,*PT_ClientMess;

// 分析服务器结构体
typedef struct _tcp_server{

	struct event *          	  pt_AServerTimerEv;
	struct timeval         		  t_AServerTv;

	struct event_base *     	 pt_AServerBase;
	struct evconnlistener *    pt_AServerListener;
	struct sockaddr_in          t_AServerSin;
	struct bufferevent *         pt_AServerBev;
	
}T_TcpServer,*PT_TcpServer;


void *tcp_mcu_thread(void*);
int search_mcu_client(char *mac);


