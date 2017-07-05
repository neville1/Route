#include "StdAfx.h"
#include "HighLayerFind.h"
#include "FindMap.h"
#include "IniFile.h"
#include "LayerPath.h"

void CHighLayerFind::Initial( int nPathCount, CMessageBox *pMsgBox)
{
	CIniFile PathSetting;
	PathSetting.OpenFile(_T("RoadPath.ini"));

	m_nInPos  = PathSetting.GetSingleData("HighLayerInPos");
	m_nOutPos = PathSetting.GetSingleData("HighLayerOutPos");
	m_nSearchDist = PathSetting.GetSingleData("HighLayerConnectDist");
	m_nPathCount  = PathSetting.GetSingleData("HighLayerPathCount");
	m_nLowLayerCount = PathSetting.GetSingleData("LowLayerPathCount");
	PathSetting.ReleaseFile();

	m_pMsgBox = pMsgBox;

}

void CHighLayerFind::SetStart(const Pos& posStart)
{
	CFindMap RoadMatch;

#ifdef USE_MAP_TA_2009Q2
	ROADLINK Road = RoadMatch.GetNearRoad(posStart,false);
#else
	ROADLINK Road = RoadMatch.GetNearRoad(posStart);
#endif
	Road.nConnect = StartPos;
	m_pathStart.Road = Road;

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	m_pathStart.pos = pRoadInfo->GetConnectPos(m_pathStart.Road);
	CStringA datadata = pRoadInfo->GetName((int) Road.nGrid,(int) Road.nObj,(int) Road.nLayer);

	if(pRoadInfo->GetLevel((int) Road.nGrid,(int) Road.nObj,(int) Road.nLayer) == FREEWAY)
	{
		m_vLinkStart.resize(1);
		m_vLinkStart[0].roadNext = m_pathStart.Road;
		m_vLinkStart[0].pos = m_pathStart.pos;
		return;
	}
}

void CHighLayerFind::SetDest(const Pos& posDest)
{
	CFindMap RoadMatch;

#ifdef USE_MAP_TA_2009Q2
	ROADLINK Road = RoadMatch.GetNearRoad(posDest,false);
#else
	ROADLINK Road = RoadMatch.GetNearRoad(posDest);
#endif
	Road.nConnect = EndPos;
	m_pathDest.Road = Road;

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	m_pathDest.pos = pRoadInfo->GetConnectPos(m_pathDest.Road);
	CStringA datadata = pRoadInfo->GetName((int) Road.nGrid,(int) Road.nObj,(int) Road.nLayer);

	if(pRoadInfo->GetLevel((int) Road.nGrid,(int) Road.nObj,(int) Road.nLayer) == FREEWAY)
	{
		m_vLinkDest.resize(1);
		m_vLinkDest[0].roadCurr = m_pathDest.Road;
		m_vLinkDest[0].pos = m_pathDest.pos;
		return;
	}
}

bool CHighLayerFind::IsFind()
{
	if(m_vLinkStart.empty())
	{
		CLayerPath StartPath;
		StartPath.Initial( m_nPathCount, m_pMsgBox );
		StartPath.SetStart(m_pathStart.Road);
		StartPath.SetDest(m_pathDest.Road);
		StartPath.SetMaxLayer(LowLayer);
		StartPath.SetMethod(TIME_FIRST);
		StartPath.SetSendProgress(true);
		StartPath.SetPathAmendment(false);
		StartPath.SetDebugPath(m_pDebugPath);

		vRoadLink vStartRoad;
		VPos vStartPos;
		int nTotalDist = m_BaseMath.ToLength(m_pathStart.pos, m_pathDest.pos);
		nTotalDist = min(nTotalDist, 7500);
		path_state StartState  = StartPath.HighLayerFindRoad(vStartRoad,vStartPos,m_nInPos,nTotalDist,false, m_vStartPath);

		if(StartState == PATH_FAILED)
		{
			return false;
		}

		m_vLinkStart.clear();
		m_vLinkStart.resize((int)vStartRoad.size());
		for( int i = 0 ; i < (int)vStartRoad.size(); i++)
		{
			m_vLinkStart[i].roadNext = vStartRoad[i];
			m_vLinkStart[i].pos = vStartPos[i];
		}
	}

	if(m_vLinkDest.empty())
	{
		CLayerPath DestPath;
		DestPath.Initial( m_nPathCount, m_pMsgBox);
		DestPath.SetStart(m_pathDest.Road);
		DestPath.SetDest(m_pathStart.pos);
		DestPath.SetMaxLayer(LowLayer);
		DestPath.SetMethod(TIME_FIRST);
		DestPath.SetSendProgress(true);
		DestPath.SetPathAmendment(false);
		DestPath.SetDebugPath(m_pDebugPath);

		vRoadLink vDestRoad;
		VPos vDestPos;
		int nTotalDist = m_BaseMath.ToLength(m_pathStart.pos, m_pathDest.pos);
		nTotalDist = min(nTotalDist, 7500);
		path_state DestState  = DestPath.HighLayerFindRoad(vDestRoad,vDestPos,m_nOutPos,nTotalDist,true, m_vDestPath);
		if(DestState == PATH_FAILED)
		{
			return false;
		}

		m_vLinkDest.clear();
		m_vLinkDest.resize((int)vDestRoad.size());
		for( int i = 0 ; i < (int)vDestRoad.size(); i++)
		{
			m_vLinkDest[i].roadCurr = vDestRoad[i];
			m_vLinkDest[i].pos = vDestPos[i];
		}
	}

	return !m_vLinkStart.empty() && !m_vLinkDest.empty();
}

path_state CHighLayerFind::Find()
{
	int nDistance = m_BaseMath.ReduceDistacne(m_pathStart.pos, m_pathDest.pos);
	CLayerPath MidPath;

	int nMinCost = INT_MAX;

	if(m_vStartPath.empty())
	{
		m_vStartPath.resize((int) m_vLinkStart.size());
		for (int nStartIndex = 0; nStartIndex < (int) m_vLinkStart.size(); ++nStartIndex)
		{
			m_vStartPath[nStartIndex].listPath.clear();
			m_vStartPath[nStartIndex].nCost = 0;
			m_vStartPath[nStartIndex].nPathLength = 0;
		}
	}

	if(m_vDestPath.empty())
	{
		m_vDestPath.resize((int) m_vLinkDest.size());
		for (int nDestIndex = 0; nDestIndex < (int) m_vLinkDest.size(); ++nDestIndex)
		{
			m_vDestPath[nDestIndex].listPath.clear();
			m_vDestPath[nDestIndex].nCost = 0;
			m_vDestPath[nDestIndex].nPathLength = 0;
		}
	}

	for (int nStartIndex = 0; nStartIndex < (int) m_vLinkStart.size(); ++nStartIndex)
	{
		for (int nDestIndex = 0; nDestIndex < (int) m_vLinkDest.size(); ++nDestIndex)
		{
			if (m_Method == NO_FREEWAY)
			{
					continue;
			}

			MidPath.Initial( m_nPathCount, m_pMsgBox);
			MidPath.SetStart(m_vLinkStart[nStartIndex].roadNext);
			MidPath.SetDest(m_vLinkDest[nDestIndex].roadCurr);
			MidPath.SetMaxLayer(HighLayer);
			if(m_Method != TIME_FIRST)
			{
				MidPath.SetMethod(m_Method);
			}
			else
			{
				MidPath.SetMethod(LEVEL_FIRST);
			}
			MidPath.SetSendProgress(m_bProgress && (nStartIndex == 0 && nDestIndex  == 0));
			MidPath.SetPathAmendment(false);
			MidPath.SetDebugPath(m_pDebugPath);

			path_state state  = MidPath.Find();

			if (state == PATH_FAILED)
			{
				continue;
			}

			if (m_vStartPath[nStartIndex].listPath.empty() && m_vStartPath[nStartIndex].nCost != -1)
			{
				GetStartLinkCost(nStartIndex,m_vStartPath[nStartIndex]);
			}

			if(m_vDestPath[nDestIndex].listPath.empty() && m_vDestPath[nDestIndex].nCost != -1)
			{
				GetDestLinkCost(nDestIndex,m_vDestPath[nDestIndex]);
			}

			int nPathLength  = MidPath.GetPathLength();
			int nAllPathLength  = MidPath.GetPathLength() + m_vStartPath[nStartIndex].nPathLength + m_vDestPath[nDestIndex].nPathLength;

			if (m_vStartPath[nStartIndex].nCost == -1 || m_vDestPath[nDestIndex].nCost == -1 || nAllPathLength < nDistance || nAllPathLength > (int)((float)nDistance * 2.0f) )// || nPathLength < nDistance )
			{
				continue;
			}
			
			int nPathCost  = MidPath.GetPathCost();
			int nAllCost   = m_vStartPath[nStartIndex].nCost + nPathCost + m_vDestPath[nDestIndex].nCost;

			nAllCost = (int)((float)nAllCost * 0.8f + (float)nAllPathLength * 0.2f);
			if (nAllCost < nMinCost)
			{
				nMinCost = nAllCost;
				m_lstPath.clear();
				ToPath(m_lstPath, m_vStartPath[nStartIndex].listPath);
				ToPath(m_lstPath, MidPath.GetPath());
				ToPath(m_lstPath, m_vDestPath[nDestIndex].listPath);
			}
		}
	}

	if (m_lstPath.empty())
	{
		return PATH_FAILED;
	}

	return PATH_SUCCEED;
}

void CHighLayerFind::GetStartLinkCost(const int & nStartIndex,PRIORITY_PATH& PriorityPath)
{
	CLayerPath MidPath;
	MidPath.Initial( m_nLowLayerCount, m_pMsgBox);
	MidPath.SetStart(m_pathStart.Road);
	MidPath.SetDest(m_vLinkStart[nStartIndex].roadNext);
	MidPath.SetDebugPath(m_pDebugPath);
	MidPath.SetMaxLayer(LowLayer);

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	int nDistance = m_BaseMath.ReduceDistacne(pRoadInfo->GetConnectPos(m_pathStart.Road), m_vLinkStart[nStartIndex].pos);
	if( nDistance < m_nSearchDist)
		MidPath.SetMethod(m_Method);
	else
		MidPath.SetMethod(LEVEL_FIRST);

	MidPath.SetSendProgress(true);
	MidPath.SetPathAmendment(true);
	path_state state = MidPath.Find();
	if (state != PATH_SUCCEED)
	{
		PriorityPath.nCost = -1;
		return;
	}

	ToPath(PriorityPath.listPath, MidPath.GetPath());
	PriorityPath.nCost = MidPath.GetPathCost();
	PriorityPath.nPathLength = MidPath.GetPathLength();
}

void CHighLayerFind::GetDestLinkCost(const int& nDestIndex,PRIORITY_PATH& PriorityPath)
{
	CLayerPath MidPath;

	MidPath.Initial( m_nLowLayerCount, m_pMsgBox);
	MidPath.SetStart(m_vLinkDest[nDestIndex].roadCurr,m_vLinkDest[nDestIndex].pos);
	MidPath.SetDest(m_pathDest.Road);
	MidPath.SetDebugPath(m_pDebugPath);
	MidPath.SetMaxLayer(LowLayer);

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	int nDistance = m_BaseMath.ReduceDistacne( m_vLinkDest[nDestIndex].pos, pRoadInfo->GetConnectPos(m_pathDest.Road));
	if( nDistance < m_nSearchDist)
		MidPath.SetMethod(m_Method);
	else
		MidPath.SetMethod(LEVEL_FIRST);

	MidPath.SetSendProgress(true);
	MidPath.SetPathAmendment(true);
	path_state state = MidPath.Find();
	if (state != PATH_SUCCEED)
	{
		PriorityPath.nCost = -1;
		return;
	}

	ToPath(PriorityPath.listPath, MidPath.GetPath());
	PriorityPath.nCost = MidPath.GetPathCost();
	PriorityPath.nPathLength = MidPath.GetPathLength();
}

lstRoadLink& CHighLayerFind::GetPath()
{
	return m_lstPath;
}

DWORD CHighLayerFind::GetTime() const
{
	return 0;
}

void CHighLayerFind::FinishProgress()
{
	CLayerPath path;
	path.Initial( 0, m_pMsgBox);
	path.FinishProgress();
}
