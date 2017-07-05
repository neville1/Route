#pragma once
#include "findpath.h"

class CMessageBox;
class CMidLayerFind : public CFindPath
{
public:
	CMidLayerFind(void);
	~CMidLayerFind(void);

	virtual void Initial( int nPathCount, CMessageBox *pMsgBox );

	virtual void SetStart(const Pos& posStart);
	virtual void SetDest(const Pos& posDest);
	virtual bool IsFind();

	virtual path_state Find();

	virtual lstRoadLink& GetPath();
	virtual DWORD GetTime() const;

	virtual void FinishProgress(); 
	virtual path_state FindInReverse(const bool bTurn) { return PATH_FAILED;}
private:

	ROAD_SITE m_pathStart;
	ROAD_SITE m_pathDest;
	vRoadLink m_vLinkStart;
	vRoadLink m_vLinkDest;
	int m_nInPos;
	int m_nOutPos;
	int m_nSearchDist;
	int m_nPathCount;
	int m_nLowLayerCount;
	lstRoadLink m_lstPath;
};
