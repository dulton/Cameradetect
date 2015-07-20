#pragma once

#include "Common.h"

class CSingleCam
{	
public:
		CSingleCam(string videostream,uint8 index);
		~CSingleCam(void);
		
		VideoCapture  	m_vcap;
		uint32                       frameNum;
		string			m_videoStream;
		Mat  				readFrame;
		Mat                          displayFrame;
		uint16         		errReadNum;
		uint16                       errOPEN;
		
		uint8 	m_index;
		int 	 	m_rows; 	 /* high */
		int 	 	m_cols; 	  /* width */
		int		m_fps;
					
		int GetCamParam();
		int GetCamFrame();
		
protected:
		
private:

};


