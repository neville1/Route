#include "StdAfx.h"
#include "MidLayerFind.h"
#include "IniFile.h"
#include "FindMap.h"
#include "LayerPath.h"

CMidLayerFind::CMidLayerFind(void)
{
}

CMidLayerFind::~CMidLayerFind(void)
{
}

void CMidLayerFind::Initial( int nPathCount, CMessageBox *pMsgBox )
{
	CIniFile PathSetting;
	PathSetting.OpenFile(_T("RoadPath.ini"));

	m_nInPos  = PathSetting.GetSingleData("MidLayerInPos");
	m_nOutPos = PathSetting.GetSingleData("MidLayerOutPos");
	m_nSearchDist = PathSetting.GetSingleData("MidLayerConnectDist");
	m_nPathCount  = PathSetting.GetSingleData("MidLayerPathCount");
	m_nLowLayerCount = PathSetting.GetSingleData("LowLayerPathCount");
	PathSetting.ReleaseFile();

	m_pMsgBox = pMsgBox;
}

void CMidLayerFind::SetStart(const Pos& posStart)
{
	CFindMap RoadMatch;

	ROADLINK Road = RoadMatch.GetNearRoad(posStart);
	Road.nConnect = StartPos;
	m_pathStart.Road = Road;

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	m_pathStart.pos = pRoadInfo->GetConnectPos(m_pathStart.Road);
	if (m_pathStart.Road.nLayer <= MidLayer)
	{
		m_vLinkStart.resize(1);
		m_vLinkStart[0] = m_pathStart.Road;
		return;
	}

	CFindMap FindMap;

	FindMap.GetInLayerConnectRoad(m_vLinkStart, m_nInPos, m_pathStart.pos, m_nSearchDist,
		                          MidLayer);
}

void CMidLayerFind::SetDest(const Pos& posDest)
{
	CFindMap FindMap;

	ROADLINK Road = FindMap.GetNearRoad(posDest);
	Road.nConnect = EndPos;
	m_pathDest.Road = Road;

	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	m_pathDest.pos = pRoadInfo->GetConnectPos(m_pathDest.Road);

	if (m_pathDest.Road.nLayer <= MidLayer)
	{
		m_vLinkDest.resize(1);
		m_vLinkDest[0] = m_pathDest.Road;
		return;
	}

	FindMap.GetOutLayerConnectRoad(m_vLinkDest, m_nOutPos, m_pathDest.pos, m_nSearchDist, MidLayer);
}

bool CMidLayerFind::IsFind()
{
	return !m_vLinkStart.empty() && !m_vLinkDest.empty();
}

path_state CMidLayerFind::Find()
{
	int nDistance = m_BaseMath.ToLength(m_pathStart.pos, m_pathDest.pos);
	CLayerPath MidPath;
	lstRoadLink lstMidPath;
	int nMinLength = INT_MAX;
	int nMidStart = 0;
	int nMidDest = 0;
	for (int nStartIndex = 0; nStartIndex < (int) m_vLinkStart.size(); ++nStartIndex)
	{
		for (int nDestIndex = 0; nDestIndex < (int) m_vLinkDest.size(); ++nDestIndex)
		{
			MidPath.Initial( m_nPathCount, m_pMsgBox);

			MidPath.SetStart(m_vLinkStart[nStartIndex]);
			MidPath.SetDest(m_vLinkDest[nDestIndex]);
			MidPath.SetMaxLayer(MidLayer);
			MidPath.SetMethod(m_Method);
			MidPath.SetPayRoad(m_bPayRoad);
			MidPath.SetSendProgress(m_bProgress);
			MidPath.SetPathAmendment(true);
			MidPath.SetDebugPath(m_pDebugPath);

			path_state state  = MidPath.Find();
			MidPath.PathAdj();

			int nPathLength = MidPath.GetPathLength();
			if (state == PATH_FAILED || nPathLength < nDistance)
			{
				continue;
			}
			if (nPathLength < nMinLength)
			{
				nMinLength = nPathLength;
				nMidStart = nStartIndex;
				nMidDest = nDestIndex;
				lstMidPath.clear();
				ToPath(lstMidPath, MidPath.GetPath());
			}
		}
	}
	if (lstMidPath.empty())
	{
		return PATH_FAILED;
	}

	m_lstPath.clear();
	if (m_pathStart.Road != m_vLinkStart[nMidStart])
	{
		MidPath.Initial( m_nLowLayerCount, m_pMsgBox );

		MidPath.SetStart(m_pathStart.Road);
		MidPath.SetDest(m_vLinkStart[nMidStart]);
		MidPath.SetDebugPath(m_pDebugPath);
		MidPath.SetMaxLayer(LowLayer);
		MidPath.SetMethod(m_Method);
		MidPath.SetPayRoad(m_bPayRoad);
		MidPath.SetSendProgress(false);
		MidPath.SetPathAmendment(true);
		path_state state = MidPath.Find();
		if (state != PATH_SUCCEED)
		{
			return PATH_FAILED;
		}
		ToPath(m_lstPath, MidPath.GetPath());
	}
	ToPath(m_lstPath, lstMidPath);

	if (m_pathDest.Road != m_vLinkDest[nMidDest])
	{
		MidPath.Initial( m_nLowLayerCount, m_pMsgBox);

		MidPath.SetStart(m_vLinkDest[nMidDest]);
		MidPath.SetDest(m_pathDest.Road);
		MidPath.SetDebugPath(m_pDebugPath);
		MidPath.SetMaxLayer(LowLayer);
		MidPath.SetMethod(m_Method);
		MidPath.SetPayRoad(m_bPayRoad);
		MidPath.SetSendProgress(false);
		MidPath.SetPathAmendment(true);
		path_state state = MidPath.Find();
		if (state != PATH_SUCCEED)
		{
			return PATH_FAILED;
		}
		ToPath(m_lstPath, MidPath.GetPath());
	}

	return PATH_SUCCEED;
}

lstRoadLink& CMidLayerFind::GetPath()
{
	return m_lstPath;
}

DWORD CMidLayerFind::GetTime() const
{
	return 0;
}

void CMidLayerFind::FinishProgress()
{
	CLayerPath path;

	path.Initial( 0, m_pMsgBox);
	path.FinishProgress();
}
