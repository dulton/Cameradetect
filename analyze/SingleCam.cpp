#include "SingleCam.h"

CSingleCam::CSingleCam(string videostream,uint8 index)
	:m_videoStream(videostream),m_index(index)
{
	errReadNum  = 0;
	frameNum = 0;
	errOPEN = 0;
}

CSingleCam::~CSingleCam(void)
{
}

int CSingleCam::GetCamParam()
{	
	Mat tmpframe;
	uint32  num  = 0;
	m_vcap.open(CV_CAP_FIREWARE);
	if(!m_vcap.open(m_videoStream)) {
		cout << "Error opening video stream or file!" << endl;
		return -1;
   	 }
   	 
   	 m_cols    =	m_vcap.get(CV_CAP_PROP_FRAME_WIDTH);
	 m_rows   = 	m_vcap.get(CV_CAP_PROP_FRAME_HEIGHT);
	 m_fps      =     m_vcap.get(CV_CAP_PROP_FPS);
	 
	 while(!(m_vcap.read(tmpframe))) {   
	 	num++;
	 	if(num > 500)
	 		return -1;
	 }
	 readFrame .create(m_rows,m_cols,tmpframe.type());
	 return 0;
}

int CSingleCam::GetCamFrame()
{
	if(!(m_vcap.read(readFrame))) {   
		//cout<<" Error Read Frame!"<<endl;
		errReadNum++;
		if(errReadNum >2000){
			errReadNum = 0;
		}
		usleep(40*1000);
		return -1;
	} 

	errReadNum = 0;

	return 0;
}
