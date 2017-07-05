#pragma once
#include "layerpath.h"

class COffsetPath :	public CLayerPath
{
public:
	COffsetPath(void);
	virtual ~COffsetPath(void);

	virtual void SetStartDirection(bool bDirection);
	virtual void SetStart(const ROADLINK& RoadStart);
	virtual void SetLastPath(vector<NAVI_PATH2>& vlstPath);

	virtual bool IsBreakFind(const ROADLINK& road);
	virtual path_state Find();
protected:
	virtual void InitialData();

private:
	int GetMinAngle(const ROADLINK& road, int nAngle);

private:
	bool m_bDirection;
	vector<NAVI_PATH2> * m_pLastPath;
	int m_nFindIndex;
	int m_nRoadIndex;
	bool m_bFind;
};
