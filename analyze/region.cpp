#include "region.h"
#include "Analyze.h"

extern T_Camera           t_Camera;

CRegion::CRegion(uint8 index)
{
	m_index = index;
	
	m_colsZoomRate	= 2;
	m_rowsZoomRate	= 2;
	resolution  = 0.001;

	alarm = 0;
}
void CRegion::motiondetective(Mat &dispalyFrame,Mat &morph)
{

	bool motionAlarmDone = false;
	Mat motionAlarmCap;	
	
	for(unsigned int i = 0;i<t_Camera.t_SinCam[m_index].t_Camvarparam.t_CamAlarmRegionAlarm.Rects.size();i++)
	{
		Rect rt = t_Camera.t_SinCam[m_index].t_Camvarparam.t_CamAlarmRegionAlarm.Rects[i];
		if(rt.x + rt.width < morph.cols && rt.y + rt.height < morph.rows)
		{
			Mat tmp = morph(rt);//�൱��tmp��rt�ı���
			int ret = countNonZero(tmp);//�����������Ԫ�ظ���
			tmp.release();			
			motionAlarmDone = true;
			float rate = (float)ret / (float)(rt.width * rt.height);
			//printf(" rate = %f\n",rate);
			if(rate > resolution)
			{
				for(int i = rt.y;i < rt.y + rt.height;i++) 
				{
					for(int j = rt.x;j < rt.x + rt.width;j++) 
					{
						int R = dispalyFrame.at<Vec3b>(i, j).val[2];
						int G = dispalyFrame.at<Vec3b>(i, j).val[1];
						int B = dispalyFrame.at<Vec3b>(i, j).val[0];
							
						R = std::min(255, R + ret);//MFC�Ƚ����ߣ�������Сֵ
						G = std::max(0, G - ret);//MFC�Ƚ����ߣ��������ֵ
						B = std::min(0, B - ret);
						dispalyFrame.at<Vec3b>(i, j) = Vec3b(B, G, R);
					}
				}
				alarm = 1;
				rectangle(dispalyFrame, rt, Scalar( 0, 0, 255 ), 2, 8, 0);
			}
			else
			{
				rectangle(dispalyFrame, rt, Scalar( 0, 255, 255 ), 2, 8, 0);
			}
		}
	}
	
	morph.release();
	
}

void CRegion::algorithmMorphology_Operations( Mat& src, Mat& dst )
{
		Mat element = getStructuringElement( cv::MORPH_RECT, Size( 3,3));//���3*3�ĺ�
		Mat element1 = getStructuringElement( cv::MORPH_RECT, Size( 3,3 ));//���3*3�ĺ�
		Mat tmp = Mat::zeros(src.rows, src.cols, src.type());
		morphologyEx(src, tmp, MORPH_OPEN, element);//ͳ�ƶ�ֵͼ���е�����������ŵ�tmp�У�tmpΪ�м����
		morphologyEx(tmp, dst, MORPH_CLOSE, element1);//ȥ��tmp�е�������������򣬴�ŵ�dst��Ŀ�����

		tmp.release();
		element.release();
		element1.release();
}

int CRegion::alarmRegionDetectRun(Mat &dispalyFrame)
{
	int time_use=0;
	struct timeval start;
	struct timeval end;

    	gettimeofday(&start,NULL);
    	alarm   =  0;
	dispalyFrame.copyTo(TmpFrame);

	m_zoomRows	=	TmpFrame.rows  /m_rowsZoomRate;
	m_zoomCols	  	=   	TmpFrame.cols   /m_colsZoomRate;

	w_Rate = (float)TmpFrame.cols / m_zoomCols;
	h_Rate = (float)TmpFrame.rows / m_zoomRows;
	
	mog(TmpFrame,foregrondframe,0.001);   // 0.001	
	
	frameindex++;
	if(frameindex >= 250) frameindex= 250;
	if(frameindex <250) return 2;
	
	foregrondframe.copyTo(mask);
	// ��ʴ  
	cv::erode(mask, mask, cv::Mat());          
	// ����  
	cv::dilate(mask, mask, cv::Mat());  
	
	algorithmMorphology_Operations(mask, mask);

	motiondetective(dispalyFrame,mask);
	
	gettimeofday(&end,NULL);
	time_use=(end.tv_sec-start.tv_sec)*1000+(end.tv_usec-start.tv_usec)/1000;//΢��
	//printf("time_use is %d\n",time_use);


	return 0;
}

