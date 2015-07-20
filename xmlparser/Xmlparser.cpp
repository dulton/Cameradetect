
#include "Xmlparser.h"

CXmlparser::CXmlparser(char* infile)
{
	ConfigDocument= new TiXmlDocument(infile);  
	ConfigDocument->LoadFile();  
	RootElement  	  = ConfigDocument->RootElement();  
	
	ServerElement =  RootElement->FirstChildElement();	
	UdpElement	 =  ServerElement->NextSiblingElement();
					
	ServerIpElement 	 = ServerElement->FirstChildElement();  
	ServerPortElement  = ServerIpElement->NextSiblingElement();  

	UdpIpElement 	= UdpElement->FirstChildElement();  
	UdpPortElement 	= UdpIpElement->NextSiblingElement();  	
}

CXmlparser::~CXmlparser()
{
	
}

void CXmlparser::GetConfigparam(T_DeviceParam &t_DevParam)
{	

	sprintf(t_DevParam.server_ip,"%s", 	 ServerIpElement->FirstChild()->Value());
	sprintf(t_DevParam.server_port,"%s",  ServerPortElement->FirstChild()->Value());

	sprintf(t_DevParam.udpadr,"%s",  UdpIpElement->FirstChild()->Value());
	sprintf(t_DevParam.udpport,"%s",  UdpPortElement->FirstChild()->Value()); 
}

