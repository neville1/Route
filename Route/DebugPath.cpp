#include "stdafx.h"
#include "DebugPath.h"
#include "DrawAdminister.h"

//201001 Modify by Joyce for Singleton CMapData
//void CDebugPath::Initial(CMapData *pMapData, CDrawAdminister *pDrawAdmin)
void CDebugPath::Initial( CDrawAdminister *pDrawAdmin)
{
	//201001 Modify by Joyce for Singleton CMapData
	//m_pMapData = pMapData;
	m_pTrans = pDrawAdmin->GetMapCoorTrans();
	m_nCount = 1;
}

void CDebugPath::DrawSetpPath(const ROADLINK& road)
{
	//201001 Modify by Joyce for Singleton CMapData
	//CRoadInfo *pRoadInfo = m_pMapData->GetRoadInfo();
	CRoadInfo *pRoadInfo = CMapData::GetInstance()->GetRoadInfo();

	vector<Pos> vPos;
	pRoadInfo->GetPos(vPos, road.nGrid, road.nObj, road.nLayer);

	m_pTrans->ToView2DPos(vPos);
	m_pTrans->ToViewRotate(vPos);
	if (m_pTrans->Is3DMode())
	{
		m_pTrans->ToView3DPos(vPos);
	}

	CReDraw *pDraw = CReDraw::GetInstance();
	pDraw->DrawPolyLine((POINT *) &vPos[0], (int) vPos.size(), RGB(255, 0, 0));
	pDraw->Flip();
}
