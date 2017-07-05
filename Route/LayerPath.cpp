#include "stdafx.h"
#include "LayerPath.h"
#include "MsgDefine.h"
#include "MapData.h"
#include "FindMap.h"
#include "DebugTool.h"
#include "LocalInfo.h"
#include "MessageBox.h"

int CLayerPath::m_nMaxProgress = 0;
int CLayerPath::m_nRouteTime = 0;

#define ROUTE_IN_TAIWAN

CLayerPath::CLayerPath() 
: m_dwUsedTime(0)
{

	m_dwMaxCost = 0;
	m_bAdj = true;
	m_nMaxLayer = LowLayer;
	m_Method = DIST_FIRST;
	m_bTrun = false;
	m_dwLastUpdate = 0;
	m_nRoadCount = 0;
}

CLayerPath::~CLayerPath()
{
	ReleaseNodeTable();
}

void CLayerPath::Initial( int nPathCount,  CMessageBox *pMsgBox )
{	

	m_PriorityRoad.Initial(nPathCount);
	int nGridCount = CMapData::GetInstance()->GetGridCount();
	m_vNode.resize(nGridCount * RoadLayerCount);
    
	m_pMsgBox = pMsgBox;
}

void CLayerPath::SetStart(const Pos& posStart)
{
	CFindMap findMap;

	ROADLINK Road = findMap.GetNearRoad(posStart);

    m_pathStart.Road = Road;
	m_pathStart.Road.nConnect = StartPos;

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	m_pathStart.pos = pRoadInfo->GetConnectPos(m_pathStart.Road);
	ROADLINK trunRoad = GetTrunRoad(m_pathStart.Road);
	m_pathStart.pos2 = pRoadInfo->GetConnectPos(trunRoad);

	if(m_pathStart.pos2 == posStart)
	{
		m_pathStart.Road.nConnect = EndPos;
		m_pathStart.pos = pRoadInfo->GetConnectPos(m_pathStart.Road);
	}
}

void CLayerPath::SetStart(const ROADLINK& RoadStart)
{
	m_pathStart.Road = RoadStart;
	m_pathStart.Road.nConnect = StartPos;

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();
	m_pathStart.pos = pRoadInfo->GetConnectPos(m_pathStart.Road);
}

void CLayerPath::SetStart(const ROADLINK& RoadStart, const Pos& posStart)
{
	m_pathStart.Road = RoadStart;
	m_pathStart.Road.nConnect = StartPos;
	
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	m_pathStart.pos = pRoadInfo->GetConnectPos(m_pathStart.Road);
	ROADLINK trunRoad = GetTrunRoad(m_pathStart.Road);
	m_pathStart.pos2 = pRoadInfo->GetConnectPos(trunRoad);

    m_pathStart.Road.nConnect = EndPos;
    m_pathStart.pos2 = m_pathStart.pos;
    m_pathStart.pos = pRoadInfo->GetConnectPos(m_pathStart.Road);
}

void CLayerPath::SetDest(const Pos& posDest)
{
	CFindMap RoadMatch;

	ROADLINK Road = RoadMatch.GetNearRoad(posDest);
	m_pathDest.Road = Road;
	m_pathDest.Road.nConnect = EndPos;
	
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	m_pathDest.pos = pRoadInfo->GetConnectPos(m_pathDest.Road);
	ROADLINK trunRoad = GetTrunRoad(m_pathDest.Road);
	m_pathDest.pos2 = pRoadInfo->GetConnectPos(trunRoad);

	if(m_pathDest.pos2 == posDest)
	{
		m_pathDest.Road.nConnect = StartPos;
		m_pathDest.pos2 = m_pathDest.pos;
		m_pathDest.pos = pRoadInfo->GetConnectPos(m_pathDest.Road);
	}
}

void CLayerPath::SetDest(const ROADLINK& RoadDest)
{
	m_pathDest.Road = RoadDest;
	m_pathDest.Road.nConnect = EndPos;
	
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	m_pathDest.pos = pRoadInfo->GetConnectPos(m_pathDest.Road);
	ROADLINK trunRoad = GetTrunRoad(m_pathDest.Road);
	m_pathDest.pos2 = pRoadInfo->GetConnectPos(trunRoad);
}

void CLayerPath::SetDest(const ROADLINK& RoadDest, const Pos& posDest)
{
	m_pathDest.Road = RoadDest;
	m_pathDest.Road.nConnect = EndPos;
	
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	m_pathDest.pos = pRoadInfo->GetConnectPos(m_pathDest.Road);
	ROADLINK trunRoad = GetTrunRoad(m_pathDest.Road);
	m_pathDest.pos2 = pRoadInfo->GetConnectPos(trunRoad);

	if(m_pathDest.pos2 == posDest)
	{
		m_pathDest.Road.nConnect = StartPos;
		m_pathDest.pos2 = m_pathDest.pos;
		m_pathDest.pos = pRoadInfo->GetConnectPos(m_pathDest.Road);
	}
}

path_state CLayerPath::Find()
{	
	try 
	{
	CheckPrivate();
	InitialData();	
	if (m_pathStart.Road == m_pathDest.Road)
	{
		if(!m_lstPriDestPath.empty())
		{
			m_lstPath.clear();
			ToPath(m_lstPath, m_lstPriDestPath);
			m_lstPath.push_front(m_pathStart.Road);
			return PATH_SUCCEED;
		}
		m_lstPath.push_back(m_pathStart.Road);
		return PATH_SUCCEED;
	}

	DWORD dwStartTime = ::GetTickCount();
	ROADLINK &RoadStart = m_pathStart.Road;

	if(m_nRouteTime == 0)
	{
		m_nRouteTime = ::GetTickCount();
	}

	int nProgress = 0;
	ROADLINK CurrRoad;
	
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	int nAllDistance = GetAllDistance();
	CLayerPath *pRS = this; //nAllDistance < 500 ? this : 0;

	bool bPrivate = true;
	if(pRoadInfo->IsPrivateRoad( m_pathStart.Road.nGrid, m_pathStart.Road.nObj, m_pathStart.Road.nLayer))
	{
		bPrivate = false;
	}

	while(true)
	{

		CurrRoad = m_PriorityRoad.front().Road;
		m_PriorityRoad.pop();
		m_nRoadCount++;

		#ifndef NAVI_COM
			if (m_bProgress)
			{
				SetProgress(CurrRoad,nAllDistance,nProgress);
			}
		#endif

		vRoadLink vLink;
		pRoadInfo->GetCrossRoad(vLink, CurrRoad, pRS);

		for (int i = 0; i < (int) vLink.size(); ++i)
		{

			if ((int) vLink[i].nLayer > m_nMaxLayer)
			{
				continue;
			}

			if (CurrRoad == vLink[i])
			{
				continue;
			}

			if (m_bPayRoad)
			{
				if (pRoadInfo->GetRoadPay(vLink[i].nGrid, vLink[i].nObj, vLink[i].nLayer))
				{
					continue;
				}
			}

			ROADLINK NextRoad;
			ConnectToRoad(CurrRoad, vLink[i], NextRoad);
			UINT32 nLastCost = GetCost(CurrRoad);
			UINT32 nRoadCost = GetRoadCost(NextRoad);
			UINT32 nCurrCost = nLastCost + nRoadCost;
			UINT32 nOldCost = GetCost(NextRoad);

			if (m_pDebugPath)
			{
				m_pDebugPath->DrawSetpPath(NextRoad);
			}

			if (nOldCost == -1 || nCurrCost < nOldCost)
			{
				if ((CurrRoad.nGrid != NextRoad.nGrid))
				{
					ROADLINK TrunRoad = GetTrunRoad(NextRoad);
					UINT16 nSeq = pRoadInfo->GetOwnerIndex(TrunRoad);
					UINT16 nSeq2 = (nSeq == 0 ? 1 : 0);
					vRoadLink vLink;
					pRoadInfo->GetConnectRoad(vLink, TrunRoad);
					for(int i = 0; i < (int)vLink.size(); i++)
					{
						if( vLink[i].nObj == CurrRoad.nObj && vLink[i].nLayer == CurrRoad.nLayer && vLink[i].nGrid == CurrRoad.nGrid)
						{
							nSeq2 = i;
						}
					}
					SetNodeInfo(TrunRoad, nLastCost, nSeq2);
				}
				if (CurrRoad.nLayer != NextRoad.nLayer)
				{
					int nSeq = pRoadInfo->GetOwnerIndex(CurrRoad);
					ROADLINK TrunRoad = GetTrunRoad(NextRoad);
					int nSeq2 = pRoadInfo->GetOwnerIndex(TrunRoad);
					UINT32 nOldCost = GetCost(TrunRoad);
					if (nOldCost == -1 || nCurrCost < nOldCost)
					{
						SetNodeInfo(TrunRoad, nLastCost, nSeq);
					}
				}

				SetNodeInfo(NextRoad, nCurrCost);
				if(IsArriveDest(NextRoad, pRS))
				{	
					CurrRoad = NextRoad;
					goto CreatePath;
				}

				if(IsBreakFind(NextRoad))                         			
				{	
					m_lstPriDestPath.clear();
					CurrRoad = NextRoad;
					goto CreatePath;
				}

				if(!bPrivate)
				{
					if(!pRoadInfo->IsPrivateRoad(NextRoad.nGrid, NextRoad.nObj, NextRoad.nLayer))
					{
						bPrivate = true;
					}
				}

				PRIORITY_ROADLINK P_Road;
				int nHCost = GetHCost(NextRoad);
				if (nHCost < 0)
				{
					RE_ASSERT(false, CStringA("CLayerPath::Find"));
				}

				P_Road.nPriority = nCurrCost + nHCost;
				P_Road.Road = NextRoad;
				if (!m_PriorityRoad.push(P_Road))
				{
					CLayerPath::m_nMaxProgress = 0;
					CLayerPath::m_nRouteTime = 0;
					return PATH_FAILED;
				}
			}

			if(m_nRouteTime != 0 && (::GetTickCount() - m_nRouteTime > 90000))
			{
				CLayerPath::m_nMaxProgress = 0;
				CLayerPath::m_nRouteTime = 0;
				return PATH_FAILED;
			}
		}
		if(m_PriorityRoad.empty())
		{
			CLayerPath::m_nMaxProgress = 0;
			CLayerPath::m_nRouteTime = 0;
			return PATH_FAILED;
		}		
	}

CreatePath:
	AddPath(CurrRoad);
	if((int)m_lstPath.size() == 0)
	{
		CLayerPath::m_nMaxProgress = 0;
		CLayerPath::m_nRouteTime = 0;
		return PATH_FAILED;
	}

	if(!m_lstPriDestPath.empty())
	{
		if(pRoadInfo->GetConnectPos(CurrRoad) == m_pathDest.pos)
		{
			m_bAdj = false;
		}
	}

	PathAmendment();

	if( !m_lstPriDestPath.empty())
	{
		ToPath(m_lstPath, m_lstPriDestPath);
	}

	m_dwUsedTime = ::GetTickCount() - dwStartTime;
	return PATH_SUCCEED;
	}
	catch (...)
	{
		CLayerPath::m_nMaxProgress = 0;
		CLayerPath::m_nRouteTime = 0;
		return PATH_FAILED;
	}
}

void CLayerPath::InitialData()
{
	m_lstPath.clear();

	int nGridCount = CMapData::GetInstance()->GetGridCount();

	ReleaseNodeTable();

	m_PriorityRoad.clear();
	PRIORITY_ROADLINK P_Road;
	P_Road.Road = m_pathStart.Road;
	P_Road.Road.nConnect = StartPos;
	P_Road.nPriority = 1;

	ROADLINK roadReturn = GetTrunRoad(P_Road.Road);
	PRIORITY_ROADLINK P_RoadReturn;
	P_RoadReturn.Road = roadReturn;
	P_RoadReturn.nPriority = 1;

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	int nOneWay = pRoadInfo->GetOneWay(m_pathStart.Road.nGrid, m_pathStart.Road.nObj, m_pathStart.Road.nLayer);

	if(nOneWay == NegativeDirection)
	{
		m_PriorityRoad.push(P_Road);
		SetNodeInfo(P_Road.Road, 0);
		return;
	}

	if(nOneWay == PositiveDirection)
	{
		m_PriorityRoad.push(P_RoadReturn);
		SetNodeInfo(roadReturn, 0);
		return;
	}

	m_PriorityRoad.push(P_Road);
	SetNodeInfo(P_Road.Road, 0);

	m_PriorityRoad.push(P_RoadReturn);
	SetNodeInfo(roadReturn, 0);
	return;
}

void CLayerPath::SetNodeInfo(const ROADLINK& Road, UINT32 nCost)
{	
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	UINT16 nSeq = pRoadInfo->GetOwnerIndex(Road);

	SetNodeInfo(Road, nCost, nSeq);

	vRoadLink vRoad;
	pRoadInfo->GetConnectRoad(vRoad, Road);
	Pos posConnect = pRoadInfo->GetConnectPos(Road);
	for(int i = 0; i < (int)vRoad.size(); ++i)
	{
		if(vRoad[i].nLayer != Road.nLayer)
		{
			vRoad[i].nConnect = pRoadInfo->GetConnectPos(vRoad[i]) == posConnect ? vRoad[i].nConnect : !vRoad[i].nConnect;
			SetNodeInfo(vRoad[i], nCost, nSeq);
		}
	}
}

void CLayerPath::SetNodeInfo(const ROADLINK& Road, UINT32 nCost, UINT16 nSeq)
{
	int nGridCount = CMapData::GetInstance()->GetGridCount();

	int nRoadIndex = nGridCount * (Road.nLayer - 1) + Road.nGrid;
	
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	if (m_vNode[nRoadIndex] == 0)
	{
		UINT32 nConnCount = pRoadInfo->GetConnectPosCount(Road.nGrid, Road.nLayer);
		RE_ASSERT(nConnCount > 0, CStringA("CLayerPath::SetNodeInfo"));

		m_vNode[nRoadIndex] = new NODE_INFO[nConnCount];

		RE_ASSERT(m_vNode[nRoadIndex], CStringA("CLayerPath::SetNodeInfo"));
		memset(m_vNode[nRoadIndex], 0xFF, sizeof(NODE_INFO) * nConnCount);
	}
	UINT32 nPointId = pRoadInfo->GetConnectID(Road);

	RE_ASSERT(nSeq != 15, CStringA("CLayerPath::SetNodeInfo"));

	m_vNode[nRoadIndex][nPointId].nCost = nCost;
	m_vNode[nRoadIndex][nPointId].nSeq = nSeq;
}

UINT32 CLayerPath::GetCost(const ROADLINK& Road)
{
	int nGridCount = CMapData::GetInstance()->GetGridCount();

	int nRoadIndex = nGridCount * (Road.nLayer - 1) + Road.nGrid;
	if (m_vNode[nRoadIndex] == 0)
	{
		return -1;
	}

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	UINT32 nPointId = pRoadInfo->GetConnectID(Road);
	NODE_INFO *pNode = m_vNode[nRoadIndex] + nPointId;

	return m_vNode[nRoadIndex][nPointId].nCost;
}

UINT16 CLayerPath::GetPathSeq(const ROADLINK& Road)
{
	int nGridCount = CMapData::GetInstance()->GetGridCount();

	int nRoadIndex = nGridCount * (Road.nLayer - 1) + Road.nGrid;

	if (m_vNode[nRoadIndex] == 0)
	{
		return USHRT_MAX;
	}

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	UINT32 nPointId = pRoadInfo->GetConnectID(Road);

	return m_vNode[nRoadIndex][nPointId].nSeq;
}

int CLayerPath::GetAllDistance()
{
	return GetDestDistance(m_pathStart.Road);
}

int CLayerPath::GetDestDistance(const ROADLINK& Road)
{	
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	Pos pos = pRoadInfo->GetConnectPos(Road);
	int nDist = m_BaseMath.ReduceDistacne(m_pathDest.pos, pos);
	return nDist;
}

void CLayerPath::AddPath(const ROADLINK& Road)
{
	ROADLINK CurrRoad;
	CurrRoad = Road;
	m_lstPath.push_back(CurrRoad);
    int nLastCost = INT_MAX;
	bool bFrist = true;
	DWORD dwAddPathTime = ::GetTickCount();
	while (true) 
	{
		if( ::GetTickCount() - dwAddPathTime > 8000)
		{
			m_lstPath.clear();
			break;
		}
		UINT16 nSeq = GetPathSeq(CurrRoad);
		if (nSeq == 15)
		{
			ROADLINK TrunRoad = GetTrunRoad(CurrRoad);
			nSeq = GetPathSeq(TrunRoad);
			break;
		}

		CurrRoad = GetAroundRoad(CurrRoad, nSeq);
		m_lstPath.push_front(CurrRoad);

		ROADLINK TrunRoad = GetTrunRoad(CurrRoad);
		int nCost = GetCost(TrunRoad);
		if (bFrist)
		{
			m_dwMaxCost = nCost;
			bFrist = false;
		}
		if (nCost == 0 || nCost == -1)
		{ 
			break;
		}
	}
}

ROADLINK CLayerPath::GetAroundRoad(const ROADLINK& Road, UINT16 ConnectIndex)
{
	vRoadLink vLink;
    CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	pRoadInfo->GetConnectRoad(vLink, Road);

	RE_ASSERT((int) vLink.size() > ConnectIndex, CStringA("CLayerPath::GetAroundRoad"));

	ROADLINK RoadDest;
	ConnectToRoad(Road, vLink[ConnectIndex], RoadDest);

	return RoadDest;
}

void CLayerPath::PathAmendment()
{
	PathAdj();
	if (m_bAdj)
	{
		DestAdj();
	}
	StartAdj();
}

void CLayerPath::PathAdj()
{
	if (m_lstPath.empty())
	{
		return;
	}
	
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	bool bLoop = true;
	while (bLoop)
	{
		bLoop = false;
		list<ROADLINK>::iterator iterStart = m_lstPath.begin();
		list<ROADLINK>::iterator iterEnd = --m_lstPath.end();
		list<ROADLINK>::iterator iterNext;
		for (iterStart; iterStart != iterEnd; ++iterStart)
		{
			iterNext = iterStart;
			++iterNext;

			if (IsEqualRoad(*iterStart, *iterNext))
			{
				m_lstPath.erase(iterNext);
				bLoop = true;
				break;
			}
		}
	}
}

void CLayerPath::StartAdj()
{	
	if((int)m_lstPath.size() == 1)
	{
		return;
	}

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	ROADLINK& RoadStart = m_pathStart.Road;
	list<ROADLINK>::iterator iter = m_lstPath.begin();
	list<ROADLINK>::iterator iterNext = ++m_lstPath.begin();

	ROADLINK NextRoad = *iterNext;
	Pos posStart  = pRoadInfo->GetConnectPos(NextRoad);
	RoadStart.nConnect = StartPos;
	Pos posStartS = pRoadInfo->GetConnectPos(RoadStart);
	RoadStart.nConnect = EndPos;
	Pos posStartE = pRoadInfo->GetConnectPos(RoadStart);

	if (posStart != posStartS && posStart != posStartE)
	{
	//	ASSERT(0);
	}

	iter->nConnect = (posStart == posStartS) ? StartPos : EndPos;
}

void CLayerPath::DestAdj()
{	
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	list<ROADLINK>::iterator iterEnd = --m_lstPath.end();
	if (m_pathDest.Road == *iterEnd)
	{
		return;
	}
	ROADLINK roadDestStart = m_pathDest.Road;
	roadDestStart.nConnect = StartPos;
	ROADLINK roadDestEnd   = m_pathDest.Road;
	roadDestEnd.nConnect   = EndPos;

	ROADLINK roadFin = *iterEnd;
	roadFin.nConnect = !roadFin.nConnect;
	Pos posDestStart  = pRoadInfo->GetConnectPos(roadDestStart);
	Pos posDestEnd    = pRoadInfo->GetConnectPos(roadDestEnd);
	Pos posFin        = pRoadInfo->GetConnectPos(roadFin);

	if (posFin != posDestStart && posFin != posDestEnd)
	{
		//ASSERT(0);
	}

	m_pathDest.Road.nConnect = (posFin == posDestStart) ? StartPos : EndPos;
	m_lstPath.push_back(m_pathDest.Road);
}

bool CLayerPath::IsEqualRoad(const ROADLINK& Road1, const ROADLINK& Road2) const
{
	return Road1.nGrid == Road2.nGrid && 
		   Road1.nObj  == Road2.nObj &&
		   Road1.nLayer == Road2.nLayer;
}

bool CLayerPath::IsArriveDest(const ROADLINK& Road, CLayerPath *pLayerPath)
{	
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();
	
	if( Road == m_pathDest.Road )
	{
		return true;
	}

	vRoadLink vRoad;
	vRoad.clear();
	pRoadInfo->GetCrossRoad( vRoad, Road, pLayerPath);
	for(int i = 0; i < (int) vRoad.size(); ++i)
	{
		if( vRoad[i] == m_pathDest.Road )
		{
			return true;
		}
	}
	return false;
}

void CLayerPath::SetProgress(const ROADLINK& Road, int nAllDistance, int& nProgress)
{
	if( nProgress < 100)
	{
		DWORD dwSleepTime = nAllDistance < 35000 ? 450 : 650;
		if( nProgress > 0)
		{
			dwSleepTime = (DWORD)((float)dwSleepTime - ( (float) dwSleepTime / 2.0f) / (float)nProgress);
		}
#ifdef REQUEST_FROM_SKSD
		if( CLayerPath::m_nMaxProgress > 90)
		{
			dwSleepTime *= 6;
		}
		if( CLayerPath::m_nMaxProgress > 75)
		{
			dwSleepTime *= 4;
		}
		if( CLayerPath::m_nMaxProgress > 50)
		{
			dwSleepTime *= 2;
		}
#endif
		if(::GetTickCount() - m_dwLastUpdate > dwSleepTime)
		{
			if (nProgress < CLayerPath::m_nMaxProgress)
			{
				nProgress++;
				return;
			}
			CLayerPath::m_nMaxProgress = nProgress;
			CWinApp *pApp = AfxGetApp();
			if (!pApp)
			{
				return;
			}
			CWnd *pWnd = pApp->m_pMainWnd;
			pWnd->SendMessage(REMSG_PATH_PROGRESS, (WPARAM) &CLayerPath::m_nMaxProgress);
			m_dwLastUpdate = ::GetTickCount();
			nProgress++;
		}
	}
}

void CLayerPath::ConnectToRoad(const ROADLINK& CurrRoad, const ROADLINK& Link, ROADLINK& NextRoad)
{	
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	Pos posConnect = pRoadInfo->GetConnectPos(CurrRoad);

	NextRoad.nGrid = Link.nGrid;
	NextRoad.nObj = Link.nObj;
	NextRoad.nLayer = Link.nLayer;
	NextRoad.nConnect = StartPos;
	Pos posNextStart = pRoadInfo->GetConnectPos(NextRoad);
	NextRoad.nConnect = posConnect == posNextStart ? EndPos : StartPos;
}

int CLayerPath::GetPathLength()
{
	int nLength = 0;
	
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	for(list<ROADLINK>::iterator iter = m_lstPath.begin();
    iter != m_lstPath.end(); ++iter)
	{
		nLength += pRoadInfo->GetLength(iter->nGrid, iter->nObj, iter->nLayer);
	}
	return nLength;
}

void CLayerPath::ReleaseNodeTable()
{
	int nCount = (int) m_vNode.size();
	for (int i = 0; i < nCount; ++i)
	{
		if (m_vNode[i] != 0)
		{
			delete [] m_vNode[i];
			m_vNode[i] = 0;
		}
	}
}

ROADLINK CLayerPath::GetTrunRoad(const ROADLINK& Road)
{
	ROADLINK TrunRoad = Road;
	TrunRoad.nConnect = !TrunRoad.nConnect;
	return TrunRoad;
}


int CLayerPath::GetRoadCost(const ROADLINK& Road)
{	
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	switch (m_Method)
	{
	case TIME_FIRST:
		{
			int nDist = pRoadInfo->GetLength(Road.nGrid, Road.nObj, Road.nLayer);
			int nSpeedLimit = pRoadInfo->GetSpeedLimit(Road.nGrid, Road.nObj, Road.nLayer);
			int nLevel = pRoadInfo->GetLevel(Road.nGrid, Road.nObj, Road.nLayer);
			nSpeedLimit = min(nSpeedLimit, 150);

			float fTimeCost = 1.0f - ((float) nSpeedLimit / 150.0f);
			if (Road.nLayer == LowLayer)
			{
				return (int)((float)nDist * fTimeCost * 2.5f);
			}
			if (Road.nLayer == MidLayer)
			{
				return (int)((float)nDist * fTimeCost * 1.8f);
			}
			return (int)((float)nDist * fTimeCost);
		}
	case DIST_FIRST:
		{
			return pRoadInfo->GetLength(Road.nGrid, Road.nObj, Road.nLayer);
		}
	case LEVEL_FIRST:
		{
			int nDist = pRoadInfo->GetLength(Road.nGrid, Road.nObj, Road.nLayer);
			Map_Road_Type nLevel = pRoadInfo->GetLevel(Road.nGrid, Road.nObj, Road.nLayer);

			if (nLevel == FREEWAY)
			{
				return nDist;
			}

			if (nLevel > NORMAL_ROAD)
			{
				return (int)((float)nDist * 8.0f);
			}

			return (int)((float)nDist * 2.0f);
		}
	case NO_FREEWAY:
		{
			int nDist = pRoadInfo->GetLength(Road.nGrid, Road.nObj, Road.nLayer);
			int nSpeedLimit = pRoadInfo->GetSpeedLimit(Road.nGrid, Road.nObj, Road.nLayer);
			int nLevel = pRoadInfo->GetLevel(Road.nGrid, Road.nObj, Road.nLayer);

			nSpeedLimit = min(nSpeedLimit, 150);

			float fTimeCost = 1.0f - ((float) nSpeedLimit / 150.0f);

			if (Road.nLayer == LowLayer)
			{
				return (int)((float)nDist * fTimeCost * 2.5f);
			}
			if (Road.nLayer == MidLayer)
			{
				return (int)((float)nDist * fTimeCost * 1.2f);
			}

			if( nLevel == FREEWAY)
			{
				return (int)((float)nDist * fTimeCost * 100.0f);
			}
			
			return (int)((float)nDist * fTimeCost);
		}
	case FREEWAY1_FRIST:
		{
			int nDist = pRoadInfo->GetLength(Road.nGrid, Road.nObj, Road.nLayer);
			int nSpeedLimit = pRoadInfo->GetSpeedLimit(Road.nGrid, Road.nObj, Road.nLayer);
			int nLevel = pRoadInfo->GetLevel(Road.nGrid, Road.nObj, Road.nLayer);

			nSpeedLimit = min(nSpeedLimit, 150);
			float fTimeCost = 1.0f - ((float) nSpeedLimit / 150.0f);

			if (Road.nLayer == LowLayer)
			{
				return (int)((float)nDist * fTimeCost * 2.5f);
			}
			if (Road.nLayer == MidLayer)
			{
				return (int)((float)nDist * fTimeCost * 1.2f);
			}

			int nNum = pRoadInfo->GetNum(Road.nGrid, Road.nObj, Road.nLayer);
			if(nLevel == FREEWAY && nNum == 1)
			{
				return (int)((float)nDist * fTimeCost * 0.1f);
			}
			if( nLevel == FREEWAY && nNum == 3)
			{
				return (int)((float)nDist * fTimeCost * 10.0f);
			}
			if (nLevel == FREEWAY)
			{
				return (int)((float)nDist * fTimeCost * 0.6f);
			}

			return (int)((float)nDist * fTimeCost);
		}
	case FREEWAY2_FRIST:
		{
			int nDist = pRoadInfo->GetLength(Road.nGrid, Road.nObj, Road.nLayer);
			int nSpeedLimit = pRoadInfo->GetSpeedLimit(Road.nGrid, Road.nObj, Road.nLayer);
			int nLevel = pRoadInfo->GetLevel(Road.nGrid, Road.nObj, Road.nLayer);

			nSpeedLimit = min(nSpeedLimit, 150);
			float fTimeCost = 1.0f - ((float) nSpeedLimit / 150.0f);

			if (Road.nLayer == LowLayer)
			{
				return (int)((float)nDist * fTimeCost * 2.5f);
			}
			if (Road.nLayer == MidLayer)
			{
				return (int)((float)nDist * fTimeCost * 1.2f);
			}

			int nNum = pRoadInfo->GetNum(Road.nGrid, Road.nObj, Road.nLayer);
			if(nLevel == FREEWAY && nNum == 3)
			{
				return (int)((float)nDist * fTimeCost * 0.1f);
			}
			if( nLevel == FREEWAY && nNum == 1)
			{
				return (int)((float)nDist * fTimeCost * 10.0f);
			}
			if (nLevel == FREEWAY)
			{
				return (int)((float)nDist * fTimeCost * 0.6f);
			}
			
			return (int)((float)nDist * fTimeCost);
		}
	case FREEWAY_FRIST:
		{
			int nDist = pRoadInfo->GetLength(Road.nGrid, Road.nObj, Road.nLayer);
			int nLevel = pRoadInfo->GetLevel(Road.nGrid, Road.nObj, Road.nLayer);
			int nSpeedLimit = pRoadInfo->GetSpeedLimit(Road.nGrid, Road.nObj, Road.nLayer);

			float fTimeCost = 1.0f - ((float) nSpeedLimit / 150.0f);

			if (Road.nLayer == LowLayer)
			{
				return (int)((float)nDist * fTimeCost * 2.5f);
			}
			if (Road.nLayer == MidLayer)
			{
				return (int)((float)nDist * fTimeCost * 1.2f);
			}

			if(nLevel == FREEWAY)
			{
				return (int)((float)nDist * fTimeCost * 0.3f);
			}
			return (int)((float)nDist * fTimeCost);
		}
	}

	RE_ASSERT(false, CStringA("CLayerPath::GetRoadCost"));
	return 0;
}

int CLayerPath::GetHCost(const ROADLINK& Road, bool bUseDijkstra)
{
	if(bUseDijkstra)
	{
		return 0;
	}

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	switch (m_Method)
	{
	case TIME_FIRST:
		{
			Pos pos = pRoadInfo->GetConnectPos(Road);
			return (int) ((float) m_BaseMath.ToLength(m_pathDest.pos, pos) * 1.0f);   // Use TravelTime
		}
	case DIST_FIRST:
		{
			Pos pos = pRoadInfo->GetConnectPos(Road);
			return m_BaseMath.ReduceDistacne(m_pathDest.pos, pos);
		}
	case LEVEL_FIRST:
		{
			Pos pos = pRoadInfo->GetConnectPos(Road);
			return m_BaseMath.ReduceDistacne(m_pathDest.pos, pos);
		}
	case NO_FREEWAY:
		{
			Pos pos = pRoadInfo->GetConnectPos(Road);
			return (int) ((float) m_BaseMath.ToLength(m_pathDest.pos, pos) * 0.6f);
		}
	case FREEWAY1_FRIST:
		{
			Pos pos = pRoadInfo->GetConnectPos(Road);
			return (int) ((float) m_BaseMath.ToLength(m_pathDest.pos, pos) * 0.6f);
		}
	case FREEWAY2_FRIST:
		{
			Pos pos = pRoadInfo->GetConnectPos(Road);
			return (int) ((float) m_BaseMath.ToLength(m_pathDest.pos, pos) * 0.6f);
		}
	case FREEWAY_FRIST:
		{
			Pos pos = pRoadInfo->GetConnectPos(Road);
			return (int) ((float) m_BaseMath.ToLength(m_pathDest.pos, pos) * 0.6f);
		}
	}
	RE_ASSERT(false, CStringA("CLayerPath::GetHCost"));
	return 0;
}

float CLayerPath::GetRoadLevelValue(const ROADLINK& Road)
{	
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	int nNextLevel = pRoadInfo->GetLevel(Road.nGrid, Road.nObj, Road.nLayer);
	int nRoadLevelCost[] = { 150, 120, 100, 100, 100, 50, 50, 30 };

	return (float)nRoadLevelCost[nNextLevel - 1] / 100.0f;
}

ROADLINK CLayerPath::GetReverseRoad(const ROADLINK& road)
{
	UINT16 nSeq = GetPathSeq(road);
	if (nSeq == 65535)
	{
		ROADLINK road;
		road.nGrid = -1;
		return road;
		int a = 0;
	}
	if (nSeq == 15)
	{
		ROADLINK TrunRoad = GetTrunRoad(road);
		nSeq = GetPathSeq(TrunRoad);
		ROADLINK road;
		road.nGrid = -1;
		return road;
	}

	ROADLINK NextRoad = GetAroundRoad(road, nSeq);
	ROADLINK TrunRoad = GetTrunRoad(NextRoad);

    return NextRoad;
}

void CLayerPath::FinishProgress()
{
	CWinApp *pApp = AfxGetApp();
	if (!pApp)
	{
		return;
	}

	if(CLayerPath::m_nMaxProgress > 100)
	{
		return;
	}

	while( CLayerPath::m_nMaxProgress < 100)
	{

		m_pMsgBox->SetMsg(strMsg);

		CLayerPath::m_nMaxProgress += (100 - m_nMaxProgress) > 5 ? 5 : (100 - m_nMaxProgress);
	}
	CLayerPath::m_nMaxProgress = 0;
	CLayerPath::m_nRouteTime = 0;
}

path_state CLayerPath::HighLayerFindRoad(vRoadLink & vRoad, VPos & vPos,const int & nSize, const int SearchDist,const bool bTurn, vector<PRIORITY_PATH> & v_RoadPath)
{	

	m_lstPath.clear();

	int nPointSize = nSize;

	int nGridCount = CMapData::GetInstance()->GetGridCount();
	ReleaseNodeTable();

	m_PriorityRoad.clear();
	PRIORITY_ROADLINK P_Road;
	P_Road.Road = m_pathStart.Road;
	P_Road.Road.nConnect = StartPos;
	P_Road.nPriority = 1;

	ROADLINK roadReturn = GetTrunRoad(P_Road.Road);
	PRIORITY_ROADLINK P_RoadReturn;
	P_RoadReturn.Road = roadReturn;
	P_RoadReturn.nPriority = 1;

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	int nOneWay = pRoadInfo->GetOneWay(m_pathStart.Road.nGrid, m_pathStart.Road.nObj, m_pathStart.Road.nLayer);

	if(nOneWay == NegativeDirection)
	{
		if(!bTurn)
		{
			m_PriorityRoad.push(P_Road);
			SetNodeInfo(P_Road.Road, 0);
		}
		else
		{
			m_PriorityRoad.push(P_RoadReturn);
			SetNodeInfo(P_RoadReturn.Road, 0);
		}
	}
	else if(nOneWay == PositiveDirection)
	{
		if(!bTurn)
		{
			m_PriorityRoad.push(P_RoadReturn);
			SetNodeInfo(P_RoadReturn.Road, 0);
		}
		else
		{
			m_PriorityRoad.push(P_Road);
			SetNodeInfo(P_Road.Road, 0);
		}
	}
	else
	{
		m_PriorityRoad.push(P_Road);
		SetNodeInfo(P_Road.Road, 0);

		m_PriorityRoad.push(P_RoadReturn);
		SetNodeInfo(roadReturn, 0);
	}

	if (m_pathStart.Road == m_pathDest.Road)
	{
		m_lstPath.push_back(m_pathStart.Road);
		return PATH_SUCCEED;
	}

	DWORD dwStartTime = ::GetTickCount();
	ROADLINK &RoadStart = m_pathStart.Road;

	ROADLINK CurrRoad;

	if(m_nRouteTime == 0)
	{
		m_nRouteTime = ::GetTickCount();
	}

	int nProgress = 0;
	int nAllDistance = GetAllDistance();
	int nSearchDistance = SearchDist;
	int nNextRoadDistance = 0;

	vector<HIGH_PRIORITY_ROADLINK> vPriorityRoad;
	vector<PRIORITY_PATH> vPriorityPath;
	vPriorityRoad.clear();
	vPriorityPath.clear();

	CLayerPath *pRS = this;
	while(true)
	{
		CurrRoad = m_PriorityRoad.front().Road;
		m_PriorityRoad.pop();
		m_nRoadCount++;

		#ifndef NAVI_COM
			if (m_bProgress)
			{
				SetProgress(CurrRoad,nAllDistance,nProgress);
			}
		#endif

		vRoadLink vLink;
		if(!bTurn)
		{
			pRoadInfo->GetCrossRoad(vLink, CurrRoad, pRS);
		}
		else
		{
			pRoadInfo->GetConnectRoad(vLink, CurrRoad);
		}

		for (int i = 0; i < (int) vLink.size(); ++i)
		{

			if ((int) vLink[i].nLayer > m_nMaxLayer)
			{
				continue;
			}

			if (CurrRoad == vLink[i])
			{
				continue;
			}

			if (m_bPayRoad)
			{
				if (pRoadInfo->GetRoadPay(vLink[i].nGrid, vLink[i].nObj, vLink[i].nLayer))
				{
					continue;
				}
			}

			if( bTurn)
			{
				int nOneWay = pRoadInfo->GetOneWay(vLink[i].nGrid,vLink[i].nObj,vLink[i].nLayer);

				Pos posConnect = pRoadInfo->GetConnectPos(CurrRoad);
				vLink[i].nConnect = StartPos;
				Pos posNextStart = pRoadInfo->GetConnectPos(vLink[i]);
				vLink[i].nConnect = posConnect == posNextStart ? StartPos : EndPos;

				if(nOneWay == PositiveDirection && vLink[i].nConnect == StartPos)
				{
					continue;
				}
				if(nOneWay == NegativeDirection && vLink[i].nConnect == EndPos)
				{
					continue;
				}
			}
			
			ROADLINK NextRoad;
			ConnectToRoad(CurrRoad, vLink[i], NextRoad);
			UINT32 nLastCost = GetCost(CurrRoad);
			UINT32 nRoadCost = GetRoadCost(NextRoad);
			UINT32 nCurrCost = nLastCost + nRoadCost;
			UINT32 nOldCost = GetCost(NextRoad);

			if (m_pDebugPath)
			{
				m_pDebugPath->DrawSetpPath(NextRoad);
			}

			if (nOldCost == -1 || nCurrCost < nOldCost)
			{
				if ((CurrRoad.nGrid != NextRoad.nGrid))
				{
					ROADLINK TrunRoad = GetTrunRoad(NextRoad);
					UINT16 nSeq = pRoadInfo->GetOwnerIndex(TrunRoad);
					UINT16 nSeq2 = (nSeq == 0 ? 1 : 0);
					vRoadLink vLink;
					pRoadInfo->GetConnectRoad(vLink, TrunRoad);
					for(int i = 0; i < (int)vLink.size(); i++)
					{
						if( vLink[i].nObj == CurrRoad.nObj && vLink[i].nLayer == CurrRoad.nLayer)
						{
							nSeq2 = i;
						}
					}
					SetNodeInfo(TrunRoad, nCurrCost, nSeq2);
				}
				if (CurrRoad.nLayer != NextRoad.nLayer)
				{
					int nSeq = pRoadInfo->GetOwnerIndex(CurrRoad);
					ROADLINK TrunRoad = GetTrunRoad(NextRoad);
					int nSeq2 = pRoadInfo->GetOwnerIndex(TrunRoad);
					UINT32 nOldCost = GetCost(TrunRoad);
					if (nOldCost == -1 || nCurrCost < nOldCost)
					{
						SetNodeInfo(TrunRoad, nLastCost, nSeq);
					}
				}

				SetNodeInfo(NextRoad, nCurrCost);

				PRIORITY_ROADLINK P_Road;
				int nHCost = GetHCost(NextRoad);
				nHCost /= 2;
				if (nHCost < 0)
				{
					ASSERT(0);
					return PATH_FAILED;
				}
				P_Road.nPriority = nCurrCost + nHCost;
				P_Road.Road = NextRoad;

				if (!m_PriorityRoad.push(P_Road))
				{
					return PATH_FAILED;
				}

				Map_Road_Type nCurrRoadLevel = pRoadInfo->GetLevel(CurrRoad.nGrid, CurrRoad.nObj, CurrRoad.nLayer);
				Map_Road_Type nNextRoadLevel = pRoadInfo->GetLevel(NextRoad.nGrid, NextRoad.nObj, NextRoad.nLayer);
				if (nCurrRoadLevel != nNextRoadLevel)
				{
					if(nNextRoadLevel == FREEWAY)
					{
						HIGH_PRIORITY_ROADLINK RoadData;
						RoadData.road = NextRoad;
						RoadData.pos = pRoadInfo->GetConnectPos(RoadData.road);
						RoadData.nPriority = nCurrCost;
						vPriorityRoad.push_back(RoadData);

						if(bTurn)
						{
							AddReversePath(NextRoad);
							PathAmendment();
						}
						else
						{
							AddPath(NextRoad);
							if(!m_lstPriDestPath.empty())
							{
								if(pRoadInfo->GetConnectPos(CurrRoad) == m_pathDest.pos)
								{
									m_bAdj = false;
								}
							}

							PathAmendment();

							if( !m_lstPriDestPath.empty())
							{
								ToPath(m_lstPath, m_lstPriDestPath);
							}
						}

						PRIORITY_PATH RoadPath;
						ToPath(RoadPath.listPath, m_lstPath);
						RoadPath.nPathLength = GetPathLength();
						RoadPath.nCost = GetPathCost();
						vPriorityPath.push_back(RoadPath);
						m_lstPath.clear();
					}
				}

				if(m_nRouteTime != 0 && (::GetTickCount() - m_nRouteTime > 90000))
				{
					CLayerPath::m_nMaxProgress = 0;
					CLayerPath::m_nRouteTime = 0;
					return PATH_FAILED;
				}

				Pos NextRoadPos = pRoadInfo->GetConnectPos(NextRoad);
				nNextRoadDistance = m_BaseMath.ToLength(NextRoadPos,m_pathStart.pos);
				if( nNextRoadDistance > nSearchDistance)
				{
					if(vPriorityRoad.empty())
					{
						nSearchDistance += SearchDist;
						if( nSearchDistance > ( nAllDistance))
						{
							return PATH_FAILED;
						}
					}
					else if((int)(vPriorityRoad.size()) < nPointSize && nSearchDistance < nAllDistance / 2)
					{
						nSearchDistance += SearchDist;
					}
					else
					{
						CFindMap findArea;

						for( int i = 0; i < (int) vPriorityRoad.size(); ++i)
						{
							ROADLINK CurrRoad = vPriorityRoad[i].road;
							Pos NextRoadPos = vPriorityRoad[i].pos;

							int nCheckCount = 10;
							for(int j = 0; j < nCheckCount; j++)
							{
								ROADLINK NextRoad;
								Pos pos;
								pos = findArea.GetHighLayerConnectRoad(CurrRoad, NextRoad, bTurn);
								if( NextRoad.nGrid == 0 && NextRoad.nLayer == 0 && NextRoad.nObj == 0)
								{
									j = nCheckCount;
									continue;
								}
								CurrRoad = NextRoad;
								NextRoadPos = pos;
							}

							int nChangeSum = m_BaseMath.ToLength(vPriorityRoad[i].pos,m_pathDest.pos) - m_BaseMath.ToLength(NextRoadPos,m_pathDest.pos);

							if( nChangeSum < 0)
							{
								vPriorityRoad[i].nPriority = (int)(2.0f * (float)vPriorityRoad[i].nPriority);
								vPriorityPath[i].nCost = (int)(2.0f * (float)vPriorityPath[i].nCost);
							}
						}

						if(!bTurn)
						{
							sort(vPriorityRoad.begin(),vPriorityRoad.end());
							sort(vPriorityPath.begin(),vPriorityPath.end());
						}
						vRoad.clear();
						vPos.clear();
						v_RoadPath.clear();
						for(int i = 0; i < (int)(vPriorityRoad.size()); i++)
						{
							if( i >= nPointSize)
							{
								i = (int)vPriorityRoad.size();
								continue;
							}
							vRoad.push_back(vPriorityRoad[i].road);
							vPos.push_back(vPriorityRoad[i].pos);
							v_RoadPath.push_back(vPriorityPath[i]);
						}
						m_dwUsedTime = ::GetTickCount() - dwStartTime;
						return PATH_SUCCEED;
					}
				}

			}
		}

		if(m_PriorityRoad.empty())
		{
			return PATH_FAILED;
		}		
	}
}

path_state CLayerPath::MidLayerFindRoad(vRoadLink & vRoad, VPos & vPos,const int & nSize, const int SearchDist,const bool bTurn)
{	
	m_lstPath.clear();

	int nGridCount = CMapData::GetInstance()->GetGridCount();


	ReleaseNodeTable();

	m_PriorityRoad.clear();
	PRIORITY_ROADLINK P_Road;
	P_Road.Road = m_pathStart.Road;
	P_Road.Road.nConnect = StartPos;
	P_Road.nPriority = 1;

	ROADLINK roadReturn = GetTrunRoad(P_Road.Road);
	PRIORITY_ROADLINK P_RoadReturn;
	P_RoadReturn.Road = roadReturn;
	P_RoadReturn.nPriority = 1;

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	int nOneWay = pRoadInfo->GetOneWay(m_pathStart.Road.nGrid, m_pathStart.Road.nObj, m_pathStart.Road.nLayer);

	if(nOneWay == NegativeDirection)
	{
		m_PriorityRoad.push(P_Road);
		SetNodeInfo(P_Road.Road, 0);
	}
	else if(nOneWay == PositiveDirection)
	{
		m_PriorityRoad.push(P_RoadReturn);
		SetNodeInfo(roadReturn, 0);
	}
	else
	{
		m_PriorityRoad.push(P_Road);
		SetNodeInfo(P_Road.Road, 0);

		m_PriorityRoad.push(P_RoadReturn);
		SetNodeInfo(roadReturn, 0);
	}

	if (m_pathStart.Road == m_pathDest.Road)
	{
		m_lstPath.push_back(m_pathStart.Road);
		return PATH_SUCCEED;
	}

	DWORD dwStartTime = ::GetTickCount();
	ROADLINK &RoadStart = m_pathStart.Road;


	ROADLINK CurrRoad;

	if(m_nRouteTime == 0)
	{
		m_nRouteTime = ::GetTickCount();
	}

	int nProgress = 0;
	int nAllDistance = GetAllDistance();
	int nSearchDistance = SearchDist;
	int nNextRoadDistance = 0;

	vector<HIGH_PRIORITY_ROADLINK> vPriorityRoad;
	vPriorityRoad.clear();
	while(true)
	{
		CurrRoad = m_PriorityRoad.front().Road;
		m_PriorityRoad.pop();
		m_nRoadCount++;

		#ifndef NAVI_COM
			if (m_bProgress)
			{
				SetProgress(CurrRoad,nAllDistance,nProgress);
			}
		#endif

		vRoadLink vLink;
		if(!bTurn)
		{
			pRoadInfo->GetCrossRoad(vLink, CurrRoad);
		}
		else
		{
			pRoadInfo->GetConnectRoad(vLink, CurrRoad);
		}

		for (int i = 0; i < (int) vLink.size(); ++i)
		{
#ifdef TEST_MODE_ROUTE
			m_dwUsedTime = ::GetTickCount() - dwStartTime;
			if(m_dwUsedTime > 90000)
			{
				return PATH_FAILED;
			}
#endif
			if ((int) vLink[i].nLayer > m_nMaxLayer)
			{
				continue;
			}

			if (CurrRoad == vLink[i])
			{
				continue;
			}

			if (m_bPayRoad)
			{
				if (pRoadInfo->GetRoadPay(vLink[i].nGrid, vLink[i].nObj, vLink[i].nLayer))
				{
					continue;
				}
			}

#ifdef USE_MAP_TA_2009Q2
			if (pRoadInfo->IsPrivateRoad(vLink[i].nGrid, vLink[i].nObj, vLink[i].nLayer))
			{
				continue;
			}
#endif
			if( bTurn)
			{
				int nOneWay = pRoadInfo->GetOneWay(vLink[i].nGrid,vLink[i].nObj,vLink[i].nLayer);

				Pos posConnect = pRoadInfo->GetConnectPos(CurrRoad);
				vLink[i].nConnect = StartPos;
				Pos posNextStart = pRoadInfo->GetConnectPos(vLink[i]);
				vLink[i].nConnect = posConnect == posNextStart ? StartPos : EndPos;

				if(nOneWay == PositiveDirection && vLink[i].nConnect == StartPos)
				{
					continue;
				}
				if(nOneWay == NegativeDirection && vLink[i].nConnect == EndPos)
				{
					continue;
				}
			}
			
			ROADLINK NextRoad;
			ConnectToRoad(CurrRoad, vLink[i], NextRoad);
			UINT32 nLastCost = GetCost(CurrRoad);
			UINT32 nRoadCost = GetRoadCost(NextRoad);
			UINT32 nCurrCost = nLastCost + nRoadCost;
			UINT32 nOldCost = GetCost(NextRoad);

			if (m_pDebugPath)
			{
#ifndef TEST_MODE_ROUTE
				m_pDebugPath->DrawSetpPath(NextRoad);
#endif
			}

			if (nOldCost == -1 || nCurrCost < nOldCost)
			{
				if ((CurrRoad.nGrid != NextRoad.nGrid))
				{
					ROADLINK TrunRoad = GetTrunRoad(NextRoad);
					UINT16 nSeq = pRoadInfo->GetOwnerIndex(TrunRoad);
					UINT16 nSeq2 = (nSeq == 0 ? 1 : 0);
					vRoadLink vLink;
					pRoadInfo->GetConnectRoad(vLink, TrunRoad);
					for(int i = 0; i < (int)vLink.size(); i++)
					{
						if( vLink[i].nObj == CurrRoad.nObj && vLink[i].nLayer == CurrRoad.nLayer)
						{
							nSeq2 = i;
						}
					}
					SetNodeInfo(TrunRoad, nCurrCost, nSeq2);
				}
				if (CurrRoad.nLayer != NextRoad.nLayer)
				{
					int nSeq = pRoadInfo->GetOwnerIndex(CurrRoad);
					ROADLINK TrunRoad = GetTrunRoad(NextRoad);
					int nSeq2 = pRoadInfo->GetOwnerIndex(TrunRoad);
					UINT32 nOldCost = GetCost(TrunRoad);
					if (nOldCost == -1 || nCurrCost < nOldCost)
					{

						SetNodeInfo(TrunRoad, nLastCost, nSeq);
					}
				}

                Map_Road_Type nCurrRoadLevel = pRoadInfo->GetLevel(CurrRoad.nGrid, CurrRoad.nObj, CurrRoad.nLayer);
				Map_Road_Type nNextRoadLevel = pRoadInfo->GetLevel(NextRoad.nGrid, NextRoad.nObj, NextRoad.nLayer);
				if (nCurrRoadLevel != nNextRoadLevel)
				{
					if(nNextRoadLevel == FREEWAY)
					{
						HIGH_PRIORITY_ROADLINK RoadData;
						RoadData.road = NextRoad;
						RoadData.pos = pRoadInfo->GetConnectPos(RoadData.road);
						RoadData.nPriority = nCurrCost;
						vPriorityRoad.push_back(RoadData);
					}
				}
				SetNodeInfo(NextRoad, nCurrCost);

				Pos NextRoadPos = pRoadInfo->GetConnectPos(NextRoad);
				nNextRoadDistance = m_BaseMath.ToLength(NextRoadPos,m_pathStart.pos);
				if( nNextRoadDistance > nSearchDistance)
				{
					if((int)(vPriorityRoad.size()) < nSize)
					{
						nSearchDistance += SearchDist;
						if( nSearchDistance > ( nAllDistance / 2) )
						{
							if( vPriorityRoad.empty())
							{
								return PATH_FAILED;
							}
							else
							{
								sort(vPriorityRoad.begin(),vPriorityRoad.end());
								vRoad.clear();
								vPos.clear();
								for(int i = 0; i < (int)(vPriorityRoad.size()); i++)
								{
									vRoad.push_back(vPriorityRoad[i].road);
									vPos.push_back(vPriorityRoad[i].pos);
								}
								m_dwUsedTime = ::GetTickCount() - dwStartTime;
								return PATH_SUCCEED;
							}
						}
					}
					else
					{
						sort(vPriorityRoad.begin(),vPriorityRoad.end());
						vRoad.clear();
						vPos.clear();
						for(int i = 0; i < nSize; i++)
						{
							vRoad.push_back(vPriorityRoad[i].road);
							vPos.push_back(vPriorityRoad[i].pos);
						}
						m_dwUsedTime = ::GetTickCount() - dwStartTime;

						return PATH_SUCCEED;
					}
				}

				PRIORITY_ROADLINK P_Road;
				int nHCost = GetHCost(NextRoad,true);
				if (nHCost < 0)
				{
					ASSERT(0);
				}
				P_Road.nPriority = nCurrCost + nHCost;
				P_Road.Road = NextRoad;

                if (!m_PriorityRoad.push(P_Road))
				{
					return PATH_FAILED;
				}
			}

			if(m_nRouteTime != 0 && (::GetTickCount() - m_nRouteTime > 90000))
			{
				CLayerPath::m_nMaxProgress = 0;
				CLayerPath::m_nRouteTime = 0;
				return PATH_FAILED;
			}
		}
		if(m_PriorityRoad.empty())
		{
			return PATH_FAILED;
		}		
	}
}


path_state CLayerPath::FindInReverse(const bool bTurn)
{	
	m_lstPath.clear();

	int nGridCount = CMapData::GetInstance()->GetGridCount();

	ReleaseNodeTable();

	m_PriorityRoad.clear();
	PRIORITY_ROADLINK P_Road;
	P_Road.Road = m_pathStart.Road;
	P_Road.Road.nConnect = StartPos;
	P_Road.nPriority = 1;

	ROADLINK roadReturn = GetTrunRoad(P_Road.Road);
	PRIORITY_ROADLINK P_RoadReturn;
	P_RoadReturn.Road = roadReturn;
	P_RoadReturn.nPriority = 1;

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	int nOneWay = pRoadInfo->GetOneWay(m_pathStart.Road.nGrid, m_pathStart.Road.nObj, m_pathStart.Road.nLayer);

	if(nOneWay == NegativeDirection)
	{
		if(bTurn)
		{
			m_PriorityRoad.push(P_RoadReturn);
			SetNodeInfo(roadReturn, 0);
		}
		else
		{
			m_PriorityRoad.push(P_Road);
			SetNodeInfo(P_Road.Road, 0);
		}
	}
	else if(nOneWay == PositiveDirection)
	{
		if(bTurn)
		{
			m_PriorityRoad.push(P_Road);
			SetNodeInfo(P_Road.Road, 0);
		}
		else
		{
			m_PriorityRoad.push(P_RoadReturn);
			SetNodeInfo(roadReturn, 0);
		}
	}
	else
	{
		m_PriorityRoad.push(P_Road);
		SetNodeInfo(P_Road.Road, 0);

		m_PriorityRoad.push(P_RoadReturn);
		SetNodeInfo(roadReturn, 0);
	}

	if (m_pathStart.Road == m_pathDest.Road)
	{
		m_lstPath.push_back(m_pathStart.Road);
		return PATH_SUCCEED;
	}

	DWORD dwStartTime = ::GetTickCount();
	ROADLINK &RoadStart = m_pathStart.Road;

	ROADLINK CurrRoad;

	CLayerPath *pRS = bTurn ? 0 : this;


	if(m_nRouteTime == 0)
	{
		m_nRouteTime = ::GetTickCount();
	}

	int nProgress = 0;
	int nAllDistance = GetAllDistance();
	int nNextRoadDistance = 0;

	vector<HIGH_PRIORITY_ROADLINK> vPriorityRoad;
	vPriorityRoad.clear();
	while(true)
	{
		CurrRoad = m_PriorityRoad.front().Road;
		m_PriorityRoad.pop();
		m_nRoadCount++;

		#ifndef NAVI_COM
			if (m_bProgress)
			{
				SetProgress(CurrRoad,nAllDistance,nProgress);
			}
		#endif

		vRoadLink vLink;
		if(!bTurn)
		{
			pRoadInfo->GetCrossRoad(vLink, CurrRoad, pRS);
		}
		else
		{
			pRoadInfo->GetConnectRoad(vLink, CurrRoad);
		}

		for (int i = 0; i < (int) vLink.size(); ++i)
		{

			if ((int) vLink[i].nLayer > m_nMaxLayer)
			{
				continue;
			}

			if (CurrRoad == vLink[i])
			{
				continue;
			}

			if (m_bPayRoad)
			{
				if (pRoadInfo->GetRoadPay(vLink[i].nGrid, vLink[i].nObj, vLink[i].nLayer))
				{
					continue;
				}
			}

			if( bTurn)
			{
				int nOneWay = pRoadInfo->GetOneWay(vLink[i].nGrid,vLink[i].nObj,vLink[i].nLayer);

				Pos posConnect = pRoadInfo->GetConnectPos(CurrRoad);
				vLink[i].nConnect = StartPos;
				Pos posNextStart = pRoadInfo->GetConnectPos(vLink[i]);
				vLink[i].nConnect = posConnect == posNextStart ? StartPos : EndPos;

				if(nOneWay == PositiveDirection && vLink[i].nConnect == StartPos)
				{
					continue;
				}
				if(nOneWay == NegativeDirection && vLink[i].nConnect == EndPos)
				{
					continue;
				}
			}
			
			ROADLINK NextRoad;
			ConnectToRoad(CurrRoad, vLink[i], NextRoad);
			UINT32 nLastCost = GetCost(CurrRoad);
			UINT32 nRoadCost = GetRoadCost(NextRoad);
			UINT32 nCurrCost = nLastCost + nRoadCost;
			UINT32 nOldCost = GetCost(NextRoad);

			if (m_pDebugPath)
			{
			}

			if (nOldCost == -1 || nCurrCost < nOldCost)
			{
				if ((CurrRoad.nGrid != NextRoad.nGrid))
				{
					ROADLINK TrunRoad = GetTrunRoad(NextRoad);
					UINT16 nSeq = pRoadInfo->GetOwnerIndex(TrunRoad);
					UINT16 nSeq2 = (nSeq == 0 ? 1 : 0);
					vRoadLink vLink;
					pRoadInfo->GetConnectRoad(vLink, TrunRoad);
					for(int i = 0; i < (int)vLink.size(); i++)
					{
						if( vLink[i].nObj == CurrRoad.nObj && vLink[i].nLayer == CurrRoad.nLayer)
						{
							nSeq2 = i;
						}
					}
					SetNodeInfo(TrunRoad, nCurrCost, nSeq2);
				}
				if (CurrRoad.nLayer != NextRoad.nLayer)
				{
					int nSeq = pRoadInfo->GetOwnerIndex(CurrRoad);
					ROADLINK TrunRoad = GetTrunRoad(NextRoad);
					int nSeq2 = pRoadInfo->GetOwnerIndex(TrunRoad);
					UINT32 nOldCost = GetCost(TrunRoad);
					if (nOldCost == -1 || nCurrCost < nOldCost)
					{

						SetNodeInfo(TrunRoad, nLastCost, nSeq);
					}
				}

				SetNodeInfo(NextRoad, nCurrCost);

				if(IsArriveDest(NextRoad, pRS) || IsBreakFind(NextRoad))                         			
				{	
					CurrRoad = NextRoad;
					goto CreatePath;
				}
				PRIORITY_ROADLINK P_Road;
				int nHCost = GetHCost(NextRoad);
				if (nHCost < 0)
				{
					RE_ASSERT(false, CStringA("CLayerPath::Find"));
				}

				P_Road.nPriority = nCurrCost + nHCost;
				P_Road.Road = NextRoad;
				if (!m_PriorityRoad.push(P_Road))
				{
					CLayerPath::m_nMaxProgress = 0;
					CLayerPath::m_nRouteTime = 0;
					return PATH_FAILED;
				}
			}

			if(m_nRouteTime != 0 && (::GetTickCount() - m_nRouteTime > 90000))
			{
				CLayerPath::m_nMaxProgress = 0;
				CLayerPath::m_nRouteTime = 0;
				return PATH_FAILED;
			}

		}
		if(m_PriorityRoad.empty())
		{
			CLayerPath::m_nMaxProgress = 0;
			CLayerPath::m_nRouteTime = 0;
			return PATH_FAILED;
		}		
	}

CreatePath:
	if( m_pathDest.Road != CurrRoad)
	{
		m_lstPath.push_back(m_pathDest.Road);
	}

	AddReversePath(CurrRoad);
	if((int)m_lstPath.size() == 0)
	{
		CLayerPath::m_nMaxProgress = 0;
		CLayerPath::m_nRouteTime = 0;
		return PATH_FAILED;
	}

	PathAmendment();


	m_dwUsedTime = ::GetTickCount() - dwStartTime;

	return PATH_SUCCEED;
}

void CLayerPath::AddReversePath(const ROADLINK& Road)
{
	ROADLINK CurrRoad;
	CurrRoad = Road;

	m_lstPath.push_back(CurrRoad);
    int nLastCost = INT_MAX;
	bool bFrist = true;
	DWORD dwAddPathTime = ::GetTickCount();
	while (true) 
	{
		if( ::GetTickCount() - dwAddPathTime > 8000)
		{
			m_lstPath.clear();
			break;
		}
		UINT16 nSeq = GetPathSeq(CurrRoad);
		if (nSeq == 15)
		{
			ROADLINK TrunRoad = GetTrunRoad(CurrRoad);
			nSeq = GetPathSeq(TrunRoad);
			break;
		}

		CurrRoad = GetAroundRoad(CurrRoad, nSeq);
		m_lstPath.push_back(CurrRoad);

		ROADLINK TrunRoad = GetTrunRoad(CurrRoad);
		int nCost = GetCost(TrunRoad);
		if (bFrist)
		{
			m_dwMaxCost = nCost;
			bFrist = false;
		}
		if (nCost == 0 || nCost == -1)
		{ 
			break;
		}
	}
}

void CLayerPath::CheckPrivate()
{
	bool bAdj = m_bAdj;
	if(bAdj)
	{
		m_bAdj = false;
	}
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();
	if(pRoadInfo->IsPrivateRoad(m_pathDest.Road.nGrid, m_pathDest.Road.nObj, m_pathDest.Road.nLayer))
	{
		ROAD_SITE pathStart;
		pathStart.pos.x = m_pathStart.pos.x;
		pathStart.pos.y = m_pathStart.pos.y;
		pathStart.pos2.x = m_pathStart.pos2.x;
		pathStart.pos2.y = m_pathStart.pos2.y;
		pathStart.Road.nGrid = m_pathStart.Road.nGrid;
		pathStart.Road.nLayer = m_pathStart.Road.nLayer;
		pathStart.Road.nObj = m_pathStart.Road.nObj;
		pathStart.Road.nConnect = m_pathStart.Road.nConnect;

		ROAD_SITE pathDest;
		pathDest.pos.x = m_pathDest.pos.x;
		pathDest.pos.y = m_pathDest.pos.y;
		pathDest.pos2.x = m_pathDest.pos2.x;
		pathDest.pos2.y = m_pathDest.pos2.y;
		pathDest.Road.nGrid = m_pathDest.Road.nGrid;
		pathDest.Road.nLayer = m_pathDest.Road.nLayer;
		pathDest.Road.nObj = m_pathDest.Road.nObj;
		pathDest.Road.nConnect = m_pathDest.Road.nConnect;

		SetStart(pathDest.Road);
		SetDest(pathStart.Road);
		if( FindInPrivate(true) == PATH_SUCCEED)
		{
			m_lstPriDestPath.clear();
			ToPath(m_lstPriDestPath, m_lstPath);
			m_lstPath.clear();
			ROADLINK &Road = *m_lstPriDestPath.begin();
			SetDest(pRoadInfo->GetConnectPos(Road));
			SetStart(pathStart.Road, pathStart.pos);
			return;
		}
		else if( FindInPrivate(false) == PATH_SUCCEED)
		{
			m_lstPriDestPath.clear();
			ToPath(m_lstPriDestPath, m_lstPath);
			m_lstPath.clear();

			if((int)m_lstPriDestPath.size() == 1)
			{
				SetStart(pathStart.Road, pathStart.pos);
				SetDest(pathDest.Road, pathDest.pos);
				return;
			}

			list<ROADLINK>::iterator iter = m_lstPriDestPath.begin();
			list<ROADLINK>::iterator iterNext = iter;
			iterNext++;
			ROADLINK &Road = *iter;
			ROADLINK &RoadNext = *iterNext;

			Pos posConnect = pRoadInfo->GetConnectPos(Road);

			RoadNext.nConnect = StartPos;
			Pos posNextStart = pRoadInfo->GetConnectPos(RoadNext);
			RoadNext.nConnect = EndPos;
			Pos posNextEnd = pRoadInfo->GetConnectPos(RoadNext);

			Road.nConnect = posConnect == posNextStart ? !Road.nConnect : Road.nConnect;
			Road.nConnect = posConnect == posNextEnd ? !Road.nConnect : Road.nConnect;

			SetDest(pRoadInfo->GetConnectPos(Road));
			SetStart(pathStart.Road, pathStart.pos);
		}
		else
		{
			SetStart(pathStart.Road, pathStart.pos);
			SetDest(pathDest.Road, pathDest.pos);
		}
	}
	if(bAdj)
	{
		m_bAdj = true;
	}
}

path_state CLayerPath::FindInPrivate(bool bPrivate)
{	
	m_lstPath.clear();

	int nGridCount = CMapData::GetInstance()->GetGridCount();

	ReleaseNodeTable();

	m_PriorityRoad.clear();
	PRIORITY_ROADLINK P_Road;
	P_Road.Road = m_pathStart.Road;
	P_Road.Road.nConnect = StartPos;
	P_Road.nPriority = 1;

	ROADLINK roadReturn = GetTrunRoad(P_Road.Road);
	PRIORITY_ROADLINK P_RoadReturn;
	P_RoadReturn.Road = roadReturn;
	P_RoadReturn.nPriority = 1;

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	int nOneWay = pRoadInfo->GetOneWay(m_pathStart.Road.nGrid, m_pathStart.Road.nObj, m_pathStart.Road.nLayer);

	if(nOneWay == NegativeDirection)
	{
		m_PriorityRoad.push(P_Road);
		SetNodeInfo(P_Road.Road, 0);
	}
	else if(nOneWay == PositiveDirection)
	{
		m_PriorityRoad.push(P_RoadReturn);
		SetNodeInfo(roadReturn, 0);
	}
	else
	{
		m_PriorityRoad.push(P_Road);
		SetNodeInfo(P_Road.Road, 0);

		m_PriorityRoad.push(P_RoadReturn);
		SetNodeInfo(roadReturn, 0);
	}

	if (m_pathStart.Road == m_pathDest.Road)
	{
		m_lstPath.push_back(m_pathStart.Road);
		return PATH_SUCCEED;
	}

	DWORD dwStartTime = ::GetTickCount();
	ROADLINK &RoadStart = m_pathStart.Road;

	//int nRoadCount = 0;
	ROADLINK CurrRoad;

	CLayerPath *pRS = this;
#ifdef REQUEST_FROM_SKSD
	pRS = 0;
#endif
	if(m_nRouteTime == 0)
	{
		m_nRouteTime = ::GetTickCount();
	}

	int nProgress = 0;
	int nAllDistance = GetAllDistance();
	int nNextRoadDistance = 0;

	vector<HIGH_PRIORITY_ROADLINK> vPriorityRoad;
	vPriorityRoad.clear();
	while(true)
	{
		CurrRoad = m_PriorityRoad.front().Road;
		m_PriorityRoad.pop();
		m_nRoadCount++;

		#ifndef NAVI_COM
			if (m_bProgress)
			{
				SetProgress(CurrRoad,nAllDistance,nProgress);
			}
		#endif

		vRoadLink vLink;
		pRoadInfo->GetConnectRoad(vLink, CurrRoad);

		for (int i = 0; i < (int) vLink.size(); ++i)
		{
#ifdef TEST_MODE_ROUTE
			m_dwUsedTime = ::GetTickCount() - dwStartTime;
			if(m_dwUsedTime > 90000)
			{
				return PATH_FAILED;
			}
#endif
			if ((int) vLink[i].nLayer > m_nMaxLayer)
			{
				continue;
			}

			if (CurrRoad == vLink[i])
			{
				continue;
			}

			if (m_bPayRoad)
			{
				if (pRoadInfo->GetRoadPay(vLink[i].nGrid, vLink[i].nObj, vLink[i].nLayer))
				{
					continue;
				}
			}

			if(bPrivate)
			{
				if (!pRoadInfo->IsPrivateRoad(vLink[i].nGrid, vLink[i].nObj, vLink[i].nLayer))
				{
					continue;
				}
			}

			int nOneWay = pRoadInfo->GetOneWay(vLink[i].nGrid,vLink[i].nObj,vLink[i].nLayer);

			Pos posConnect = pRoadInfo->GetConnectPos(CurrRoad);
			vLink[i].nConnect = StartPos;
			Pos posNextStart = pRoadInfo->GetConnectPos(vLink[i]);
			vLink[i].nConnect = posConnect == posNextStart ? StartPos : EndPos;

			if(nOneWay == PositiveDirection && vLink[i].nConnect == StartPos)
			{
				continue;
			}
			if(nOneWay == NegativeDirection && vLink[i].nConnect == EndPos)
			{
				continue;
			}
			
			ROADLINK NextRoad;
			ConnectToRoad(CurrRoad, vLink[i], NextRoad);
			UINT32 nLastCost = GetCost(CurrRoad);
			UINT32 nRoadCost = GetRoadCost(NextRoad);
			UINT32 nCurrCost = nLastCost + nRoadCost;
			UINT32 nOldCost = GetCost(NextRoad);

			if (m_pDebugPath)
			{
#ifndef TEST_MODE_ROUTE
				m_pDebugPath->DrawSetpPath(NextRoad);
#endif
			}

			if (nOldCost == -1 || nCurrCost < nOldCost)
			{
				if ((CurrRoad.nGrid != NextRoad.nGrid))
				{
					ROADLINK TrunRoad = GetTrunRoad(NextRoad);
					UINT16 nSeq = pRoadInfo->GetOwnerIndex(TrunRoad);
					UINT16 nSeq2 = (nSeq == 0 ? 1 : 0);
					vRoadLink vLink;
					pRoadInfo->GetConnectRoad(vLink, TrunRoad);
					for(int i = 0; i < (int)vLink.size(); i++)
					{
						if( vLink[i].nObj == CurrRoad.nObj && vLink[i].nLayer == CurrRoad.nLayer)
						{
							nSeq2 = i;
						}
					}
					SetNodeInfo(TrunRoad, nCurrCost, nSeq2);
				}
				if (CurrRoad.nLayer != NextRoad.nLayer)
				{
					int nSeq = pRoadInfo->GetOwnerIndex(CurrRoad);
					ROADLINK TrunRoad = GetTrunRoad(NextRoad);
					int nSeq2 = pRoadInfo->GetOwnerIndex(TrunRoad);
					UINT32 nOldCost = GetCost(TrunRoad);
					if (nOldCost == -1 || nCurrCost < nOldCost)
					{
						//SetNodeInfo(TrunRoad, nCurrCost, nSeq);
						SetNodeInfo(TrunRoad, nLastCost, nSeq);
					}
				}

				SetNodeInfo(NextRoad, nCurrCost);

				if(IsArriveDest(NextRoad, pRS) || IsBreakFind(NextRoad))                         			
				{	
					CurrRoad = NextRoad;
					goto CreatePath;
				}

				if (!pRoadInfo->IsPrivateRoad(vLink[i].nGrid, vLink[i].nObj, vLink[i].nLayer))
				{
					goto CreatePath;
				}

				PRIORITY_ROADLINK P_Road;
				int nHCost = GetHCost(NextRoad);
				if (nHCost < 0)
				{
					RE_ASSERT(false, CStringA("CLayerPath::Find"));
				}

				P_Road.nPriority = nCurrCost + nHCost;
				P_Road.Road = NextRoad;
				if (!m_PriorityRoad.push(P_Road))
				{
					CLayerPath::m_nMaxProgress = 0;
					CLayerPath::m_nRouteTime = 0;
					return PATH_FAILED;
				}
			}

			if(m_nRouteTime != 0 && (::GetTickCount() - m_nRouteTime > 90000))
			{
				CLayerPath::m_nMaxProgress = 0;
				CLayerPath::m_nRouteTime = 0;
				return PATH_FAILED;
			}
		}
		if(m_PriorityRoad.empty())
		{
			CLayerPath::m_nMaxProgress = 0;
			CLayerPath::m_nRouteTime = 0;
			return PATH_FAILED;
		}		
	}

CreatePath:

	AddReversePath(CurrRoad);

	if((int)m_lstPath.size() == 0)
	{
		CLayerPath::m_nMaxProgress = 0;
		CLayerPath::m_nRouteTime = 0;
		return PATH_FAILED;
	}

	PathAmendment();
	m_dwUsedTime = ::GetTickCount() - dwStartTime;
	return PATH_SUCCEED;
}

void CLayerPath::CheckRS( ROADLINK & CurrRoad, vRoadLink & vLink, CLayerPath *pRS)
{
	if (!pRS)
	{
		return;
	}

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	if((int) vLink.size() == 1)
	{
		if( vLink[0] == CurrRoad)
		{
			vLink.clear();
		}
	}

	if(vLink.empty())
	{
		vRoadLink vNoRSLink;
		pRoadInfo->GetCrossRoad(vNoRSLink, CurrRoad);

		if((int) vNoRSLink.size() == 1)
		{
			if( vNoRSLink[0] == CurrRoad)
			{
				vNoRSLink.clear();
			}
		}

		ROADLINK roadCurr;
		ROADLINK roadNext;

		roadCurr = CurrRoad;

		while(vLink.empty() && !vNoRSLink.empty())
		{
			if( roadCurr == CurrRoad && roadCurr.nConnect == CurrRoad.nConnect)
			{
				roadNext = GetReverseRoad(roadCurr);
				SetNodeInfo(roadCurr, INT_MAX);
			}
			else
			{
				int nGridCount = CMapData::GetInstance()->GetGridCount();
				int nRoadIndex = nGridCount * (roadCurr.nLayer - 1) + roadCurr.nGrid;
				UINT32 nPointId = pRoadInfo->GetConnectID(roadCurr);
				UINT16 nSeq = pRoadInfo->GetOwnerIndex(roadCurr);

				if(m_vNode[nRoadIndex][nPointId].nSeq == nSeq)
				{
					SetNodeInfo(roadCurr, INT_MAX);
				}

				roadNext = GetReverseRoad(roadCurr);
				if (roadNext == roadCurr)
				{
					roadCurr.nConnect = !roadCurr.nConnect;
				}
				roadNext = GetReverseRoad(roadCurr);
			}

			pRoadInfo->GetCrossRoad(vLink, roadNext, pRS);

			for(int i = 0; i < (int) vLink.size(); ++i)
			{
				if( vLink[i] == roadCurr)
				{
					vLink.erase(vLink.begin() + i );
					i--;
					continue;
				}
			}
			roadCurr = roadNext;
		}
		CurrRoad = roadNext;
	}
}
