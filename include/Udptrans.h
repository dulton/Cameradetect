#pragma once

#include "Common.h"

class CUdptrans
{
public:
	CUdptrans(int width ,int height,char*ip,char* port);
	~CUdptrans();

	AVFormatContext* 	ptFormatCtx;
	AVOutputFormat*	ptOutFmt;
	AVStream* 		ptVideoSt;
	AVCodecContext* 	ptCodecCtx;
	AVCodec* 		ptCodec;
	AVDictionary *        	ptParam;
	AVPacket 		pkt;
	AVFrame* 		m_pYUVFrame;
	
	Mat displayFrame;
	Mat YUVFrame;
	Mat rgb_frame;
	
	uint8_t* 	m_PicBuf;
	int  		m_width;
	int 		m_height;
	int    		m_PicSize;
	int    		m_Ysize;

	char * 	m_RDIP;
	char *      m_RDPort;
	
	int  	UdptransPre();
	int 	UdptransRun(Mat& img);
	int 	UdptransRelease();
protected:
		
private:	
};
