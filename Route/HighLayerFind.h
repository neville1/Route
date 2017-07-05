#pragma once

#include "FindPath.h"
#include "HighLayerInfo.h"

class CMessgeBox;
class CHighLayerFind : public CFindPath
{
public:
	CHighLayerFind() {}
	~CHighLayerFind() {}

	virtual void Initial( int nPathCount, CMessageBox *pMsgBox);

	virtual void SetStart(const Pos& posStart);
	virtual void SetDest(const Pos& posDest);
	virtual bool IsFind();

	virtual path_state Find();

	virtual lstRoadLink& GetPath();
	virtual DWORD GetTime() const;

	virtual void FinishProgress(); 
	virtual path_state FindInReverse(const bool bTurn) { return PATH_FAILED;}

private:
	void GetStartLinkCost(const int& nStartIndex,PRIORITY_PATH& PriorityPath);
	void GetDestLinkCost(const int& nDestIndex,PRIORITY_PATH& PriorityPath);

private:

	ROAD_SITE m_pathStart;
	ROAD_SITE m_pathDest;
	vRoutePoint m_vLinkStart;
	vRoutePoint m_vLinkDest;
	int m_nInPos;
	int m_nOutPos;
	int m_nSearchDist;
	int m_nPathCount;
	int m_nLowLayerCount;
	lstRoadLink m_lstPath;

	vector<PRIORITY_PATH> m_vStartPath;
	vector<PRIORITY_PATH> m_vDestPath;
};
