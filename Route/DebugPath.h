#pragma once

#include "MapData.h"
#include "pathDefine.h"

class CMapData;
class CDrawAdminister;
class CCoorTransform;
class CDebugPath
{
public:
	CDebugPath() {}
	~CDebugPath() {}

	void SetStepTimer(int nTimer) { m_nStepTimer = nTimer; }
	
	//201001 Modify by Joyce for Singleton CMapData
	//void Initial(CMapData *pMapData, CDrawAdminister *pDrawAdmin);
	void Initial( CDrawAdminister *pDrawAdmin );

	void DrawSetpPath(const ROADLINK& road);
	
private:
	CWnd *m_pViewWnd;

	//201001 Modify by Joyce for Singleton CMapData
	//CMapData *m_pMapData;

	CCoorTransform * m_pTrans;
	int m_nStepTimer;
	int m_nCount;
};
