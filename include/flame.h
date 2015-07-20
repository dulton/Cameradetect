#pragma once

#include "Common.h"
#include "VideoHandler.h"
#include "FlameDetector.h"

class CFlame
{
public:
	CFlame(uint8 index,VideoHandler** videoHandler);
	~CFlame();

	uint8 	m_index;
	VideoHandler *handler ;

	uint8 alarm;

	vector<Rect > FlameRect;
	int FlameAlarmRun(Mat& displayFrame,void* videoHandler);
};
