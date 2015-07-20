#pragma once

#include "Common.h"
#include  "CmdDefine.h"
#include "SingleCam.h"

typedef struct _T_CAMERA_FIXED_PARAM{

	char   	URL[SINGLE_URL_LEN_128];
	uint8        CameStatus;	
	uint8        index;
	
}T_CAM_FIXED_PARAM;

typedef struct _ALARM_TIME_{
	
	uint8 hour;
	uint8 min;
	
}T_AlarmTime;

typedef struct _SM_VDCS_VIDEO_ALARM_TIME_INT{

	uint8   Enable;    
	uint8   Day;
	T_AlarmTime Start;
	T_AlarmTime End;
	
}ST_SM_VDCS_VIDEO_ALARM_TIME_INT;

typedef struct _T_CAMERA_FUNC_HUMAN_ALARM{
	
	uint16    	AlarmType;
	uint8  	AutoWarn;
	uint16       MaxNum;
	uint8         HumanDetectEn;
	uint8         Flag;           /*  0 none  1 monitor  2 door 3 ALL  */
	
	vector< Rect >        MonitorZoneRects;
	vector< Line >         DirectionLines;
	
	ST_SM_VDCS_VIDEO_ALARM_TIME_INT  AlarmTime[7];
	
}T_CAM_HUM_ALARM;

typedef struct _T_CAMERA_FUNC_OTH_ALARM{
	
	uint16    	AlarmType;
	uint8  	AutoWarn;
	uint8         DetectEn;
	vector< Rect >        Rects;
	
	ST_SM_VDCS_VIDEO_ALARM_TIME_INT  AlarmTime[7];
	
}T_CAM_OTH_ALARM;


typedef struct _T_CAMERA_VAR_PARAM{

	T_CAM_HUM_ALARM    t_CamHumAlarm;
	T_CAM_OTH_ALARM    t_CamSmkAlarm;
	T_CAM_OTH_ALARM    t_CamFireAlarm;
	T_CAM_OTH_ALARM    t_CamFixedObjAlarm;
	T_CAM_OTH_ALARM    t_CamAlarmRegionAlarm;

}T_CAM_VAR_PARAM;

class Canalyze
{

public:
	Canalyze(T_CAM_FIXED_PARAM t_Camfiexedparam);
	~Canalyze();

	int 	 	m_rows; 	 /* high */
	int 	 	m_cols; 	  /* width */
	
	Mat 		displayFrame;
	char  	window[20];

	/* 各线程使能标志位 */
	uint8 AlarmThreadEnFlag;         
	
	uint8 HumanTimeFlag;
	uint8 SmokeTimeFlag;
	uint8 RegionTimeFlag;
	uint8 FireTimeFlag;

///////////////////////////////////////////////////	
	uint8 humanAlarm;
	uint8 regionAlarm;
	uint8 smokeAlarm;
	uint8 fireAlarm;

	uint8         humanAlarmFlag;
	uint8  	 humanAlarmFlagFlag;
	uint8 	regionAlarmFlag;
	uint8		regionAlarmFlagFlag;
	uint8        smokeAlarmFlag;
	uint8      	 smokeAlarmFlagFlag;
	uint8        fireAlarmFlag;
	uint8      	fireAlarmFlagFlag;


	uint16  humanAlarmFrameNum;
	uint16  regionAlarmFrameNum;
	uint16  smokeAlarmFrameNum;
	uint16  fireAlarmFrameNum;

	uint16     humanAlarmFlagFrame; 
	uint16     regionAlarmFlagFrame; 
	uint16     smokeAlarmFlagFrame; 
	uint16     fireAlarmFlagFrame;

	uint8 	MCUAlarmFlag;
	uint8 	TmpFlag;
	uint16  	readFrameNum;

	int humanALL;
	int humanIN;
	int humanOUT;

	int doorIN[LINENUM];
	int doorOUT[LINENUM];

	vector <Rect>  fireAlarmRect;
	
///////////////////////////////////////////////////////
	int turnon_mcu_alarm();
	int turnoff_mcu_alarm();
	int send_HumAlarm_to_server(T_CAM_HUM_ALARM*pt_Alarm);
	int send_OthAlarm_to_server(T_CAM_OTH_ALARM* pt_Alarm);
	int AlarmRegionalarm(T_CAM_OTH_ALARM* pt_CamAlarmRegionAlarm);	
	
//////////////////////////////////////////////////////////////////////////////////	
	void HumanAlarmThread();
	int   CreateHumanAlarmThread();
	static void* RunHumanAlarmThread(void*  param){
		Canalyze* p = (Canalyze*)param;
		p-> HumanAlarmThread();
		return NULL;
	}

	void AlarmRegionAlarmTHread();
	int   CreateAlarmRegionAlarmThread();
	static void* RunAlarmRegionAlarmTHread(void*  param){
		Canalyze* p = (Canalyze*)param;
		p-> AlarmRegionAlarmTHread();
		return NULL;
	}
	
	void SmokeAlarmThread();
	int CreateSmokeAlarmThread();
	static void* RunSmokeAlarmThread(void*	param){
			Canalyze* p = (Canalyze*)param;
			p-> SmokeAlarmThread();
			return NULL;
		}

	void FireAlarmThread();
	int CreateFireAlarmThread();
	static void* RunFireAlarmThread(void*	param){
			Canalyze* p = (Canalyze*)param;
			p-> FireAlarmThread();
			return NULL;
		}	
	void AlarmClockTimeThread();
	int  CreateAlarmClockTimeThread();
	static void* RunAlarmClockTimeThread(void*  param){
		Canalyze* p = (Canalyze*)param;
		p-> AlarmClockTimeThread();
		return 0;
	}

	int AnalyzeInit();
	void reStartAlarmOn();
	int AlarmStrategy();
	void Rect_in_display(Mat & display);
	int Readframe();

protected:

private:
	
	CSingleCam 		m_singlecam;

};


typedef struct _T_SINGLE_CAMERA{
	
	T_CAM_FIXED_PARAM t_Camfixedparam;

	T_CAM_VAR_PARAM   	t_Camvarparam;

	Canalyze*  CamAnalyze;
	
}T_SINGLE_CAMERA;

typedef struct _Camera_{
	
	uint8		 CameraFlag;
	uint8		 CamNum;
	T_SINGLE_CAMERA  t_SinCam[MAX_CAM_NUM];	
	
	_Camera_()
	{
		memset(this, 0, sizeof( _Camera_));
	}
	
}T_Camera,*PT_Camera;



