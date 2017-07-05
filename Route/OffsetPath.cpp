#include "StdAfx.h"
#include "OffsetPath.h"

COffsetPath::COffsetPath(void) : m_pLastPath(0)
{
}

COffsetPath::~COffsetPath(void)
{
}

void COffsetPath::SetStartDirection(bool bDirection)
{
	m_bDirection = bDirection;
}

void COffsetPath::SetStart(const ROADLINK& RoadStart)
{
	m_pathStart.Road = RoadStart;
	m_pathStart.Road.nConnect = m_bDirection ? StartPos : EndPos;

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	m_pathStart.pos = pRoadInfo->GetConnectPos(m_pathStart.Road);
}


void COffsetPath::SetLastPath(vector<NAVI_PATH2>& vlstPath)
{
	m_pLastPath = &vlstPath;
}

void COffsetPath::InitialData()
{
	m_lstPath.clear();

	int nGridCount = CMapData::GetInstance()->GetGridCount();

	ReleaseNodeTable();
	m_PriorityRoad.clear();

	ROADLINK road = m_pathStart.Road;

	PRIORITY_ROADLINK P_Road;
	road.nConnect = m_bDirection ? StartPos : EndPos;
	P_Road.Road = road;
	P_Road.nPriority = 0;
	m_PriorityRoad.push(P_Road);
	SetNodeInfo(road, 0);
}

bool COffsetPath::IsBreakFind(const ROADLINK& road) 
{ 
	if (!m_pLastPath)
	{
		return false;
	}

	int nPathCount = (int) m_pLastPath->size();
	for (int i = 0; i < nPathCount; ++i)
	{
		for (int j = 0; j < (int) m_pLastPath->at(i).vRoad.size(); ++j)
		{			
			if (m_pLastPath->at(i).vRoad[j] == road)
			{
				SetDest(road);
				m_nFindIndex = i;
				m_nRoadIndex = j;
				m_bFind = true;
				return true;
			}
		}
	}

	m_bFind = false;
	return false; 
}

path_state COffsetPath::Find()
{
	path_state state = CLayerPath::Find();
	if (state != PATH_SUCCEED)
	{
		return state;
	}
	if (!m_bFind)
	{	
		return state;
	}

	int nPathCount = (int) m_pLastPath->size();
	if (m_nFindIndex < 0 || m_nFindIndex > nPathCount - 1)
	{
		return PATH_FAILED;
	}

	list<ROADLINK>::iterator iterPath = m_lstPath.end();
	iterPath--;
	
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	Pos posLast = pRoadInfo->GetConnectPos(m_pLastPath->at(m_nFindIndex).vRoad[m_nRoadIndex]);
	Pos posPath = pRoadInfo->GetConnectPos(*iterPath);

	list<ROADLINK> lastPath;
	for (int i = m_nFindIndex; i < (int) m_pLastPath->size(); ++i)
	{
		int j = 0;
		if (i == m_nFindIndex)
		{
			j = m_nRoadIndex;
		}
		for (j; j < (int) m_pLastPath->at(i).vRoad.size(); ++j)
		{
			lastPath.push_back(m_pLastPath->at(i).vRoad[j]);
		}
	}

	ToPath(m_lstPath, lastPath);

	return state;
}

