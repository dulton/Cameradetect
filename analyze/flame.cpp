
#include "flame.h"
#include "Analyze.h"

extern T_Camera           t_Camera;

CFlame::CFlame(uint8 index,VideoHandler** videoHandler)
{
	m_index =index;
	handler = new VideoHandler();
	*videoHandler  =  handler;
}


CFlame::~CFlame()
{
	delete handler;
	handler =NULL;

}

int CFlame::FlameAlarmRun(Mat& displayFrame,void* videoHandler)
{	
  	int  iRet = -1; 
	alarm = 0;
	FlameRect.clear();
	for(unsigned int i = 0;i<t_Camera.t_SinCam[m_index].t_Camvarparam.t_CamFireAlarm.Rects.size();i++){
		Rect rt = t_Camera.t_SinCam[m_index].t_Camvarparam.t_CamFireAlarm.Rects[i];
		iRet =  (int)handler->handle(displayFrame ,rt, videoHandler); 
		if(iRet  == 0)
		{	
			//printf("fire alarm \n");
			FlameRect.clear();
			vector<Rect>  & tmpRect =handler->getDetector().getDecider().alarmRect;
			for(uint16 k=0 ; k <tmpRect.size();k++){
				Rect  tmp;
				tmp = tmpRect[k];
				FlameRect.push_back(tmp);
			}
			//printf("fire rectangle!\n");
			for(uint16 j = 0; j< FlameRect.size(); j++){
				FlameRect[j].x   =FlameRect[j].x +rt .x;
				FlameRect[j].y   =FlameRect[j].y +rt .y;
			}
			//printf("ha ha  ha ha !\n");
			alarm = 1;
		}
		if(iRet == 2) alarm = 0;
	}

	return 0;
}


