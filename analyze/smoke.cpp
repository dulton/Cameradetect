#include "smoke.h"
#include "Analyze.h"
#include "Wavelet.h"

extern T_Camera              t_Camera;

CSmoke::CSmoke(uint8 index)
{
	m_index = index;

	waveletThres = 35;
	initialized = false;
	element = getStructuringElement( cv::MORPH_RECT, Size( 3, 3 ));
	blockSize = 2;

	alarm  = 0;
}

void CSmoke::initThreshold(int cols, int rows, float initValue) 
{
	thres = Mat(rows, cols, CV_32FC1);

	for(int i = 0;i < rows;i++) {
		for(int j = 0;j < cols;j++) {
			thres.at<float>(i, j) = initValue;
		}
	}
}
void CSmoke::updateBackground(Mat& frame1, Mat& frame2, Mat& frame3) //更新背景、前景以及阈值
{
	float alpha = 0.95;

	for(int i = 0;i < frame1.rows;i++) 
	{
		for(int j = 0;j < frame1.cols;j++) 
		{
			float f1 = frame1.at<float>(i, j);
			float f2 = frame2.at<float>(i, j);
			float f3 = frame3.at<float>(i, j);
			float t = thres.at<float>(i, j);

			
			if(abs(f1 - f2) > t && abs(f1 - f3) > t) //超出阈值时，背景和阈值更新
			{
				background.at<float>(i, j) = alpha * background.at<float>(i, j) + (1 - alpha) * f2;
				
				thres.at<float>(i, j) = thres.at<float>(i, j) * alpha + (1 - alpha) * (5 * abs(f2 - background.at<float>(i, j)));//z++
			}
			foreground.at<float>(i, j) = (f1 - (0.62 * background.at<float>(i, j))) / 0.38;//不断更新前景色
			if(foreground.at<float>(i, j) > 0.9607) //显示出了图像变化的轮廓
				curRefinedFg.at<float>(i, j) = 1;
			else
				curRefinedFg.at<float>(i, j) = 0;
		}
	}
}

void CSmoke::SmokeDetectorMorphology_Operations( Mat& src, Mat& dst)//进行形态学运算，先开运算再闭运算。获得图像形态并去除干扰   图像变化的轮廓
{
	Mat tmp = Mat::zeros(src.rows, src.cols, src.type());
	morphologyEx(src, tmp, MORPH_OPEN, element);
	morphologyEx(tmp, dst, MORPH_CLOSE, element);
	tmp.release();
}

void CSmoke::SmokeDetectorlabelBlobs(const cv::Mat &binary)//标记连通区域
{
    blobs.clear();
 
    // Using labels from 2+ for each blob
    cv::Mat label_image;
    binary.convertTo(label_image, CV_32FC1);
 
    int label_count = 2; // starts at 2 because 0,1 are used already     Scalar（0）=[0,0,0,0]    Scalar（1）=[1,0,0,0]   Scalar（2）=[2,0,0,0]

    for(int y=0; y < binary.rows; y++) 
	{
        for(int x=0; x < binary.cols; x++) 
		{
            if((int)label_image.at<float>(y,x) != 1) 
			{
                continue;
            }
 
            cv::Rect rect;
            cv::floodFill(label_image, cv::Point(x,y), cv::Scalar(label_count), &rect, cv::Scalar(0), cv::Scalar(0), 4);//将像素值为1附近的像素值为[0,0,0,0]的点赋值[2,0,0,0]  只考虑当前像素水平方向和垂直方向的相邻点
 
            vector<Point>  blob;
 
            for(int i=rect.y; i < (rect.y+rect.height); i++) 
			{
                for(int j=rect.x; j < (rect.x+rect.width); j++) 
				{
                    if((int)label_image.at<float>(i,j) != label_count)
					{
                        continue;
                    }
 	                
                    blob.push_back(cv::Point(j,i));
                }
            }
 
			blobs.push_back(blob);
            label_count++;
        }
    }
}

void CSmoke::getEdgeModel(Mat& frame) {
	if (frame.empty()) return;
	const int NIter = 3;

    Mat Src = Mat(frame.rows, frame.cols, CV_32FC1);
    Mat Dst = Mat(frame.rows, frame.cols, CV_32FC1);

    //Dst = 0;
    frame.convertTo(Src,CV_32FC1);//在缩放或不缩放的情况下转换为另一种数据类型
    cvHaarWavelet(Src,Dst,NIter);//哈尔小波转换应用于图像压缩
	
	for(int n = 0;n < 3;n ++) 
	{
		int scale = pow(2, n + 1);
		Mat tmp1 = Dst(Rect(0, frame.rows / scale, frame.cols / scale, frame.rows / scale));
		Mat tmp2 = Dst(Rect(frame.cols / scale , frame.rows / scale, frame.cols / scale, frame.rows / scale));
		Mat tmp3 = Dst(Rect(frame.cols / scale , 0, frame.cols / scale, frame.rows / scale));

		for(int i = 0;i < tmp1.rows;i++) 
		{
			for(int j = 0;j < tmp1.cols;j++) 
			{
				tmp1.at<float>(i, j) = sqrt(pow(tmp1.at<float>(i, j), 2) + pow(tmp2.at<float>(i, j), 2) + pow(tmp3.at<float>(i, j), 2));
			}
		}
		scale = pow(2, 2 - n); //4 

		double M=0,m=0;
		minMaxLoc(tmp1,&m,&M);//寻找矩阵(一维数组当作向量,用Mat定义) 中最小值和最大值的位置
		for(int i = 0;i < tmp1.rows;i += scale) 
		{
			for(int j = 0;j < tmp1.cols;j += scale) 
			{
				float max = m;

				for(int x = 0;x < scale;x++) 
				{
					for(int y = 0;y < scale;y++) 
					{
						if(max < tmp1.at<float>(i + x, j + y)) 
						{
							max = tmp1.at<float>(i + x, j + y);			
						}
					}
				}
				EMap[n].at<float>(i / scale, j / scale) = max;
			}
		}
		tmp1.release();
		tmp2.release();
		tmp3.release();
	}
	Src.release();
	Dst.release();
}

Rect CSmoke::getBoundaryofBlob(vector<Point> blob) {
	int minX = 0;
	int maxX = 0;
	int minY = 0;
	int maxY = 0;
	
	for(int i = 0;i < blob.size();i++) {
		if(i == 0) {
			minX = blob[i].x;
			maxX = blob[i].x;
			minY = blob[i].y;
			maxY = blob[i].y;
		}else{
			if(blob[i].x > maxX) maxX = blob[i].x;
			if(blob[i].x < minX) minX = blob[i].x;
			if(blob[i].y > maxY) maxY = blob[i].y;
			if(blob[i].y < minY) minY = blob[i].y;
		}
	}
	return Rect(minX, minY, maxX - minX, maxY - minY);
}

float CSmoke::getBinaryMotionMask(Mat& prevFrame,Mat& curFrame, Mat& motionMask, vector<Point> blob) 
{
	int mCols = prevFrame.cols / blockSize;//Frame.smoke.blockSize = 2
	int mRows = prevFrame.rows / blockSize;
	int m = 0;
	int n = 0;
	float cs = 0;
	float c = 0;
	Mat tmp = Mat::zeros(mRows, mCols, CV_8UC1);

	for(int a = 0;a < blob.size();a++)
	{
		int i = floor(blob[a].y / blockSize);
		int j = floor(blob[a].x / blockSize);
		if(tmp.at<uchar>(i, j) == 1) continue;
		tmp.at<uchar>(i, j) = 1;
		c++;
		m = i - 1;
		if(m < 0) continue;
		for(int p = -1;p < 2;p++) 
		{
			n = j + p;
			if(n < 0 || n == mCols) continue;
			float max = 0;
			float min = 255;

			for(int x = 0;x < blockSize;x++) 
			{
				for(int y = 0;y < blockSize;y++) 
				{
					if(m * blockSize + x < 0 || m * blockSize + x >= prevFrame.rows || 
						n * blockSize + y < 0 ||  n * blockSize + y >= prevFrame.cols) continue;
					if(i * blockSize + x < 0 || i * blockSize + x >= prevFrame.rows || 
						j * blockSize + y < 0 ||  j * blockSize + y >= prevFrame.cols) continue;
					max += abs(curFrame.at<float>((m *blockSize) + x, (n * blockSize) + y) - prevFrame.at<float>((i * blockSize) + x, (j * blockSize) + y));
				}
			}

			max = max / pow(blockSize, 2);
			if(max < 0.005)
			{
				motionMask.at<float>(i, j) = 1;
				motionMask.at<float>(m, n) = 1;
				cs++;
			}
		}
	}

	tmp.release();
	if(c == 0) 
	{
		return 0;
	}else
	{
		return cs * 100 / c;
	}
}

float CSmoke::getWeberContrast(Mat& inputFrame, Mat& background, vector<Point> blob) 
{//当前帧与背景对比
	float s = 0;

	for(int i = 0;i < blob.size();i++)
	{
		float iframe = inputFrame.at<float>(blob[i].y, blob[i].x);
		float bg = background.at<float>(blob[i].y, blob[i].x);

		iframe = abs(iframe - bg) / bg;//像素差值/背景像素
		s += iframe;//累计
	}
	return s / blob.size();//除以点数，得到平均
}

void CSmoke::smokeDetect(Mat& frame1, Mat& frame2, Mat& frame3) 
{
	updateBackground(frame1, frame2, frame3);//不断刷新背景、前景等
	//                                           src                    dst
	SmokeDetectorMorphology_Operations(curRefinedFg, curMorph_Foreground);//进行形态学运算，先开运算再闭运算。获得图像形态并去除干扰   图像变化的轮廓
	SmokeDetectorlabelBlobs(curMorph_Foreground);//检测到的目标在Frame.smoke.blob中

	Mat frameGray;
	cvtColor(prevFrame, frameGray, CV_BGR2GRAY);//RGB转灰度图像
	
	equalizeHist(frameGray, frameGray);//灰度图像直方图均衡化   图像明亮  白色
	getEdgeModel(frameGray);//运用小波分析，得到边缘模型  EMap[0]~Emap[2]
	//imshow("Frame.smoke.EMap[0]",Frame.smoke.EMap[0]);
	//imshow("Frame.smoke.EMap[1]",Frame.smoke.EMap[1]);
	//imshow("Frame.smoke.EMap[2]",Frame.smoke.EMap[2]);
	Mat curEMap;
	getEnergyMap(frameGray, curEMap);//能量分布,分析结果在curEMap中
	//imshow("test1", curEMap);
	Mat diffEMap = Mat::zeros(curEMap.rows, curEMap.cols, CV_8UC1);
	frameGray.release();
	Mat blurMap = Mat::zeros(EMap[0].rows, EMap[0].cols, CV_8UC1);//全局数组EMap[3]
	/*对EMap[0]逐点分析，在EMap[0~3]中有一点超出阈值的情况下，
	如果EMap[1]该点的像素值不是最小，则如果EMap[0]小于阈值=>blurMap.at<char>(i, j) = 1*/
	for(int i = 0;i < EMap[0].rows;i++) 
	{
		for(int j = 0;j < EMap[0].cols;j++) 
		{
			if(EMap[0].at<float>(i, j) > waveletThres ||EMap[1].at<float>(i, j) >waveletThres ||EMap[2].at<float>(i, j) > waveletThres) //超出阈值
			{
				if((EMap[0].at<float>(i, j) < EMap[1].at<float>(i, j) && EMap[1].at<float>(i, j) < EMap[2].at<float>(i, j))
				|| (EMap[1].at<float>(i, j) > EMap[0].at<float>(i, j) && EMap[1].at<float>(i, j) > EMap[2].at<float>(i, j))) 
				{
					if(EMap[0].at<float>(i, j) < waveletThres) 
					{
						blurMap.at<char>(i, j) = 1;//白色
					}
				}
			}
		}
	}
      //如果bgEMap.at<float>(i, j)的像素值与能量分布图像素差大于90=>diffEMap.at<char>(i, j) = 1
	for(int i = 0;i < curEMap.rows;i++) 
	{
		for(int j = 0;j < curEMap.cols;j++) 
		{
			if(bgEMap.at<float>(i, j) - curEMap.at<float>(i, j) > 90) 
			{
				diffEMap.at<char>(i, j) = 1;
			}		
		}
	}

	Mat motion = Mat::zeros(frame1.rows / blockSize, frame1.cols / blockSize, CV_32FC1); //1/4图像
	//对检测到的目标检测
	for(int i = 0;i < lastblobs.size();i++) 
	{
		//if(Frame.smoke.lastblobs[i].size() < 500) continue;    z--
		Rect rect = getBoundaryofBlob(lastblobs[i]);//得到图像的边界及分辨率
		//if(rect.height < 24 || rect.height < 48) continue;
		int width = frame1.cols / 8;//40
		int height = frame1.rows / 8;//30

		Mat blob = Mat::zeros(height, width, CV_8UC1);//1/64
		float blobCount = 0;
		float blurCount = 0;

		for(int n = 0;n < lastblobs[i].size();n++) //点集
		{
			//double floor( double arg );   功能： 函数返回参数不大于arg的最大整数
			int y = (int)floor((float)lastblobs[i][n].y / 8);
			int x = (int)floor((float)lastblobs[i][n].x / 8);
			if(blob.at<char>(y, x) == 0) blobCount++;
			blob.at<char>(y, x) = 1;//wihte
		
		}

		for(int x = 0;x < curEMap.rows;x++) 
		{
			for(int y = 0;y < curEMap.cols;y++) 
			{
				if(blob.at<char>(x, y) == 1 && diffEMap.at<char>(x, y) == 1) 
				{//white，且像素差>90
					blurCount++;	
					
				}
			}
		}

		blurCount = blurCount / blobCount;//前后像素差大于90的比例
	//	std::cout << blurCount << std::endl;

		Mat contourBlur = Mat::zeros(height, width, CV_8UC1);//1/64   轮廓模糊
		float contourBlurCount = 0;

		for(int x = 0;x< blob.cols;x++) 
		{
			bool flag = false;

			for(int y = 0;y < blob.rows;y++) 
			{
				if(blob.at<char>(y, x) > 0)//检测到点
				{
					if(blurMap.at<char>(y, x) > 0) 
					{
						flag = true;
						if(contourBlur.at<char>(y, x) == 0) contourBlurCount++;
						contourBlur.at<char>(y, x) = 1;
					}
					else
					{
						break;
					}		
				}
				else
				{
					if(flag) break;
				}
			}
		}

		for(int x = 0;x< blob.cols;x++) 
		{
			bool flag = false;

			for(int y = blob.rows - 1;y >= 0;y--) 
			{
				if(blob.at<char>(y, x) > 0) 
				{
					if(blurMap.at<char>(y, x) > 0) 
					{
						flag = true;
						if(contourBlur.at<char>(y, x) == 0) contourBlurCount++;
						contourBlur.at<char>(y, x) = 1;
					}else
					{
						break;
					}		
				}
				else
				{
					if(flag) break;
				}
			}
		}

		for(int y = 0;y< blob.rows;y++) 
		{
			bool flag = false;

			for(int x = 0;x < blob.cols;x++) 
			{
				if(blob.at<char>(y, x) > 0) 
				{
					if(blurMap.at<char>(y, x) > 0) 
					{
						flag = true;
						if(contourBlur.at<char>(y, x) == 0) contourBlurCount++;
						contourBlur.at<char>(y, x) = 1;
					}
					else
					{
						break;
					}		
				}
				else
				{
					if(flag) break;
				}
			}
		}
		for(int y = 0;y< blob.rows;y++) 
		{
			bool flag = false;

			for(int x = blob.cols - 1;x >= 0;x--) 
			{
				if(blob.at<char>(y, x) > 0) 
				{
					if(blurMap.at<char>(y, x) > 0)
					{
						flag = true;
						if(contourBlur.at<char>(y, x) == 0) contourBlurCount++;
						contourBlur.at<char>(y, x) = 1;
					}
					else
					{
						break;
					}		
				}
				else
				{
					if(flag) break;
				}
			}
		}

		contourBlurCount = contourBlurCount / blobCount;

		Mat motionMask = Mat(frame1.rows / blockSize, frame1.cols / blockSize, CV_32FC1);//1/4
 
		float p = getBinaryMotionMask(frame2, frame1, motionMask, lastblobs[i]);

		add(motion, motionMask, motion);

		float cw = getWeberContrast(foreground, background, lastblobs[i]);
		/*
		std::cout << p << std::endl;
		std::cout << cw << std::endl;
		std::cout << blurCount << std::endl;
		std::cout << contourBlurCount << std::endl;
		*/
		float total = 0;	
		//根据对比度区分
		if(cw > 0.5 && p > 20 )//if(cw > 0.5 && p > 20 && blurCount > 0.3 && contourBlurCount > 0.15) 
		{
			for(int n = 0;n < lastblobs[i].size();n++) 
			{
				float b = prevFrame.at<Vec3b>(lastblobs[i][n].y, lastblobs[i][n].x).val[0];
				float g = prevFrame.at<Vec3b>(lastblobs[i][n].y, lastblobs[i][n].x).val[1];
				float r = prevFrame.at<Vec3b>(lastblobs[i][n].y, lastblobs[i][n].x).val[2];
				//根据颜色区分
				float I = (r + g + b) / 3;
				float Cmax = std::max(r, std::max(g, b));
				float Cmin = std::min(r, std::min(g, b));
				if(Cmax - Cmin < 35) 
				{
					if(I > 80 && I < 235) 
					{
						total++;
					}
				}
				else
				{
					if(I > 80 && I < 235) 
					{
						if(Cmax == b && (Cmax - Cmin) < 45) 
						{
							total++;
						}
					}
				}
			}
			total = total / (float)lastblobs[i].size();
			if(total > 0) //if(total > 0.9)   原来的值
			{
				SmokeRegion.push_back(rect);//框住烟雾区域
			}
		}
		motionMask.release();
		contourBlur.release();
		blob.release();		
	}
	lastblobs.clear();
	for(int i = 0;i < blobs.size();i++)
	{
		vector<Point> tmp;
		for(int j = 0;j < blobs[i].size();j++) 
		{
			tmp.push_back(Point(blobs[i][j].x, blobs[i][j].y));
		}
		lastblobs.push_back(tmp);
	}
	curMorph_Foreground.copyTo(prevMorph_Foreground);//
	//imshow("test",Frame.smoke.curMorph_Foreground);
	curEMap.release();
	diffEMap.release();
	blurMap.release();
	motion.release();
}

vector<Rect>  CSmoke::detectSmoke(Mat& originFrame) 
{
	Mat frameGray;
	
	SmokeRegion.clear();
	
	if(!initialized)
	{	
		resize(originFrame, originFrame, Size(320, 240));//将图像 src 改变尺寸得到与 dst 同样大小
		EMap[0] = Mat::zeros(originFrame.rows / 8, originFrame.cols / 8, CV_32FC1);
		EMap[1] = Mat::zeros(originFrame.rows / 8, originFrame.cols / 8, CV_32FC1);
		EMap[2] = Mat::zeros(originFrame.rows / 8, originFrame.cols / 8, CV_32FC1);
		initThreshold(originFrame.cols,originFrame.rows, 0.01);//all pixel value=0.01

		cvtColor(originFrame, frameGray, CV_BGR2GRAY);//convert to gray
		//imshow("Gray",frameGray);
		equalizeHist(frameGray, frameGray);//灰度图象直方图均衡化  曝光均匀（更宽、理想统一的亮度值分布）
		//imshow("equalizeHist",frameGray);
		getFilteredFrame(frameGray, frame1);// 获取过滤框架  

		getEnergyMap(frameGray, bgEMap);//能量图  明亮
		//imshow("test5", Frame.smoke.frame1);
		frame1.copyTo(background);
		foreground = Mat(background.rows, background.cols,background.type());
		curRefinedFg = Mat(background.rows, background.cols, background.type());
		prevMorph_Foreground = Mat::zeros(background.rows, background.cols, background.type());
		frameGray.release();
		initialized = true;
		return SmokeRegion;
	}

	if(!originFrame.empty())
	{
		
		float w_rate = (float)originFrame.cols / 320;
		float h_rate = (float)originFrame.rows / 240;
		resize(originFrame, originFrame, Size(320, 240));
		cvtColor(originFrame, frameGray, CV_BGR2GRAY);
		equalizeHist(frameGray, frameGray);
		getFilteredFrame(frameGray,frame1);
		if(!frame1.empty() && !frame2.empty() && !frame3.empty()) 
		{
			smokeDetect(frame1, frame2, frame3);//frame1为当前帧，frame2为上一帧，frame3为上上帧
			for(int i = 0;i < SmokeRegion.size();i++) 
			{

				SmokeRegion[i].x = SmokeRegion[i].x * w_rate;
				SmokeRegion[i].width = SmokeRegion[i].width * w_rate;
				SmokeRegion[i].y = SmokeRegion[i].y * h_rate;
				SmokeRegion[i].height = SmokeRegion[i].height * h_rate;
			}
		}
		if(!frame2.empty()) 
		{
			if(!frame3.empty()) 
			{
				frame3.release();
			}
			frame2.copyTo(frame3);//frame3为上上帧
			if(background.empty()) 
			{
						
			}
			frame2.release();
		}
		frame1.copyTo(frame2);//frame2为上一帧
		frame1.release();
		frameGray.release();
		if(!prevFrame.empty()) prevFrame.release();
		originFrame.copyTo(prevFrame);
	}
	return SmokeRegion;
}


int CSmoke::SmokeAlarmDetectRun(Mat & displayframe)
{
	Mat tmp;
	displayframe.copyTo(tmp);
	alarm = 0;

	resize(tmp, tmp, Size(960, 540));

	if(t_Camera.t_SinCam[m_index].t_Camvarparam.t_CamSmkAlarm.DetectEn){
		for(int j = 0; j<t_Camera.t_SinCam[m_index].t_Camvarparam.t_CamSmkAlarm.Rects.size();j++){
			rectangle(displayframe, t_Camera.t_SinCam[m_index].t_Camvarparam.t_CamSmkAlarm.Rects[j], Scalar(0, 255, 255), 2, 8, 0);
		}
	}
	
	vector<Rect> smokeRegion = detectSmoke(tmp);
	
	if(smokeRegion.size() > 0) 
	{
		alarmIs = true;
		alarm = 1;

		for(int i = 0;i < smokeRegion.size();i++) 
		{
		//	rectangle(smokeAlarmCap, smokeRegion[i], Scalar(0, 0, 255),2, 8, 0);//red
			if(smokeRegion[i].height*smokeRegion[i].width>28000)
			rectangle(displayframe, smokeRegion[i], Scalar(0, 0, 255), 2, 8, 0);
		}
	}

}

