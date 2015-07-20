#pragma once

#include "Common.h"

#include "tinyxml.h"  
#include "tinystr.h"  

typedef struct   cfg_param{
	char		server_ip[20];
	char		server_port[8];
	
	char 	udpadr[20];
	char 	udpport[8];
	
}T_DeviceParam;

class CXmlparser
{

public:
	CXmlparser(char* infile);
	~CXmlparser();

	TiXmlDocument *ConfigDocument;
	TiXmlElement *RootElement;
	
	TiXmlElement * ServerElement;
	TiXmlElement * UdpElement;
	
	TiXmlElement *ServerIpElement;
	TiXmlElement *ServerPortElement ;

	TiXmlElement *UdpIpElement;
	TiXmlElement *UdpPortElement ;

	void GetConfigparam(T_DeviceParam &t_DevParam);
protected:
private:
};

