#pragma once

#include "ReDefine.h"
#include "PathDefine.h"
#include "MapData.h"
#include "DebugPath.h"
#include "BaseMath.h"

class CMessageBox;
class CFindPath 
{
public:
	CFindPath() : m_pDebugPath(0), m_bPayRoad(false) {}
	virtual ~CFindPath() {}

	virtual void Initial( int nPathCount, CMessageBox *pMsgBox ) = 0;

	virtual void SetStart(const Pos& posStart) = 0;
	virtual void SetStart(const ROADLINK& RoadStart) {};
	virtual void SetStartDirection(bool bDirection) {} 
	virtual void SetDest(const Pos& posDest) = 0;
	virtual void SetPayRoad(bool bPayRoad) { m_bPayRoad = bPayRoad; }
	virtual void SetMethod(path_method method) { m_Method = method; }
	virtual void SetMaxLayer(int nMaxLayer) { m_nMaxLayer = nMaxLayer; }
	virtual void SetPathAmendment(bool bAdj) { m_bAdj = bAdj; }
	virtual void SetSendProgress(bool bProgress) { m_bProgress = bProgress; }
	virtual void SetNotTrun(bool bTrun) {}
	virtual bool IsFind() { return true; }
	virtual void PathAdj() {}

	virtual void SetCurrDirection( int nDirection ) { m_nCurrDirection = nDirection; }
	virtual void SetLastPath(vector<NAVI_PATH2>& vlstPath) {}

	virtual path_state Find() = 0;
	virtual path_state FindInReverse(const bool bTurn) = 0;

	virtual lstRoadLink& GetPath() = 0;
	virtual DWORD GetTime() const = 0;

	virtual void ToPath(lstRoadLink& lstPathDest, lstRoadLink& lstPathSrc);
	void SetDebugPath(CDebugPath *pDebugPath) { m_pDebugPath = pDebugPath; }

	virtual void FinishProgress(){}
protected:
	CDebugPath *m_pDebugPath;
	path_method m_Method;
	bool m_bPayRoad;
	int  m_nMaxLayer;
	bool m_bAdj;
	bool m_bProgress;
	CBaseMath m_BaseMath;
	CMessageBox *m_pMsgBox;
	int m_nCurrDirection; 
};
