#pragma once

#include "BaseType.h"
#include "BaseMath.h"
#include "PriorityRoad.h"
#include "FindPath.h"
#include <math.h>
#include <ostream>
#include <hash_map>


using namespace std;
using namespace stdext;

class CMessageBox;
class CLayerPath : public CFindPath
{
public:
	CLayerPath();
	virtual ~CLayerPath();

	virtual void Initial( int nPathCount, CMessageBox *pMsgBox);

	virtual void SetStart(const Pos& posStart);
	virtual void SetStart(const ROADLINK& RoadStart);
	virtual void SetStart(const ROADLINK& RoadStart, const Pos& posStart);
	virtual void SetDest(const Pos& posDest);
	virtual void SetDest(const ROADLINK& RoadDest);
	virtual void SetDest(const ROADLINK& RoadDest, const Pos& posDest);
	virtual void SetNotTrun(bool bTrun) { m_bTrun = bTrun; }

	path_state FindInPrivate(bool bPrivate);
	void CheckPrivate();

	ROADLINK& GetStart() { return m_pathStart.Road; }
	ROADLINK& GetDest() { return m_pathDest.Road; }

	virtual path_state HighLayerFindRoad(vRoadLink & vRoad, VPos & vPos,const int & nSize, const int SearchDist,const bool bTurn, vector<PRIORITY_PATH> & v_RoadPath);
	virtual path_state MidLayerFindRoad(vRoadLink & vRoad, VPos & vPos,const int & nSize, const int SearchDist,const bool bTurn);
	virtual path_state FindInReverse(const bool bTurn);
	void AddReversePath(const ROADLINK& Road);
	virtual path_state Find();
	virtual lstRoadLink& GetPath() { return m_lstPath; }
	virtual DWORD GetTime() const { return m_dwUsedTime; }

	int GetPathLength();
	int GetAllDistance();
	DWORD GetPathCost() { return m_dwMaxCost; }

	virtual void PathAdj();

	ROADLINK GetReverseRoad(const ROADLINK& road);

	virtual void FinishProgress();

	void SetNodeInfo(const ROADLINK& Road, UINT32 nCost);
protected:
	virtual void InitialData();
	virtual bool IsBreakFind(const ROADLINK& /*road*/) { return false; }
	virtual int GetRoadCost(const ROADLINK& Road);
	virtual int GetHCost(const ROADLINK& Road, bool bUseDijkstra = false);
	void ReleaseNodeTable();

private:	
	int GetDestDistance(const ROADLINK& Road);
	
	void SetNodeInfo(const ROADLINK& Road, UINT32 nCost, UINT16 nSeq);
	UINT32 GetCost(const ROADLINK& Road);
	UINT16 GetPathSeq(const ROADLINK& Road);

	void AddPath(const ROADLINK& Road);
	ROADLINK GetAroundRoad(const ROADLINK& Road, UINT16 ConnectIndex);
	void PathAmendment();
	void StartAdj();
	void DestAdj();

	bool IsEqualRoad(const ROADLINK& Road1, const ROADLINK& Road2) const;
	bool IsArriveDest(const ROADLINK& Road, CLayerPath *pLayerPath = 0);
	void SetProgress(const ROADLINK& Road, int nAllDistance, int& nProgress);
	void ConnectToRoad(const ROADLINK& CurrRoad, const ROADLINK& Link, ROADLINK& NextRoad);
	
	ROADLINK GetTrunRoad(const ROADLINK& Road);
	float GetRoadLevelValue(const ROADLINK& Road);

	void CheckRS( ROADLINK & CurrRoad, vRoadLink & vLink, CLayerPath *pRS = 0);
protected:
	ROAD_SITE m_pathStart;
	ROAD_SITE m_pathDest;
	list<ROADLINK> m_lstPath;
	CPriorityRoad m_PriorityRoad;

private:
	vector<NODE_INFO *> m_vNode;	
	DWORD m_dwUsedTime;
	DWORD m_dwMaxCost;
	bool m_bTrun;
	DWORD m_dwLastUpdate;
	static int m_nMaxProgress;
	list<ROADLINK> m_lstPriDestPath;
	bool m_bEast;

	int m_nRoadCount;

	static int m_nRouteTime;
private:
	struct HIGH_PRIORITY_ROADLINK
{
	bool operator < (const HIGH_PRIORITY_ROADLINK& lhs) const
	{
		return nPriority < lhs.nPriority;
	}
	ROADLINK road;
	int nPriority;
	Pos pos;
};
};
