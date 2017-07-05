#include "StdAfx.h"
#include "FindPath.h"

void CFindPath::ToPath(lstRoadLink& lstPathDest, lstRoadLink& lstPathSrc)
{
	bool bFristSkip = false;
	if (!lstPathDest.empty() && !lstPathSrc.empty())
	{
		lstRoadLink::iterator iterSrc = lstPathSrc.begin();
		lstRoadLink::iterator iterDest = --lstPathDest.end();
		while (*iterSrc == *iterDest)
		{
			if (iterSrc->nConnect == iterDest->nConnect)
			{
				lstPathSrc.erase(iterSrc);
			}
			else
			{
				lstPathDest.erase(iterDest);
				bFristSkip = true;

			}
			if (lstPathDest.empty() || lstPathSrc.empty())
			{
				break;
			}
			iterSrc = lstPathSrc.begin();
			iterDest = --lstPathDest.end();
		}
	}

	for (lstRoadLink::iterator iter = lstPathSrc.begin(); iter != lstPathSrc.end(); ++iter)
	{
		lstPathDest.push_back(*iter);
	}
}

