#pragma once

//#include "stdafx.h"
#include "ReDefine.h"
#include "BaseType.h"

enum Road_View { Close = 0, RoadDistance = 1, Full = 2 };

enum path_method { LEVEL_FIRST = 0, TIME_FIRST = 1, DIST_FIRST = 2, NO_FREEWAY = 3, FREEWAY1_FRIST = 4, FREEWAY2_FRIST = 5,FREEWAY_FRIST = 6};
enum path_state { PATH_FAILED = -1 , NOT_MATCH = -2, PATH_FULL = -3, PATH_SUCCEED = 1 };
enum path_angel_type{ ANGLE_UPRIGHT , ANGLE_UPLEFT ,ANGLE_UP ,ANGLE_RIGHT ,ANGLE_LEFT,ANGLE_DOWNRIGHT , ANGLE_DOWN,ANGLE_DOWNLEFT};
enum path_direction  { Failed = -1, WestSouth = 0, WestNorth = 1, EastSouth = 2, EastNorth = 3, WestToEast = 4, EastToWest = 5};

typedef struct
{
	ROADLINK Road;
	Pos pos;
	Pos pos2;
} ROAD_SITE;


struct PRIORITY_ROADLINK
{
	bool operator < (const PRIORITY_ROADLINK& lhs) const
	{
		return nPriority < lhs.nPriority;
	}
	ROADLINK Road;
	int nPriority;
};

struct NAVI_PATH
{
	bool operator ==(const NAVI_PATH& lhs) const
	{
		return Road == lhs.Road;
	}

	ROADLINK Road;
	vector<Pos> vPos;
	VInt vLength;
	VInt vAngle;
	UINT32 nLength;
};

struct NAVI_PATH2
{
	vector<ROADLINK> vRoad;
	vector<Pos> vPos;
	VInt vLength;
	VInt vAngle;
	UINT32 nLength;
};


typedef struct
{
	Pos pos;
	int nNodeNum;
} ROAD_INTERSECTION;                   // 非路段上資訊

typedef struct 
{
	ROADLINK Road;
	ROADLINK NextRoad;
	int nOverAngle;
	int nOverDist;
	char chNextRoadName[100];
	bool bNext;
	char chSecNextRoadName[100];
	int  nSecOverAngle;

} CURRENT_ROAD;

typedef struct 
{
	int nCurrRoad;
	int nCurrIndex;
	char chCurrRoadName[100];
	char chNextRoadName[100];
	char chSecNextRoadName[100];
	int nOverAngle;
	int nOverDist;
	int nSecDist;
	int nSecOverAngle;
} CURRENT_ROAD2;



struct NODE_INFO
{
	UINT32 nCost : 28;
	UINT32 nSeq  : 4;
};

struct ROAD_POS
{
	Pos pos;
	int nAngle;
	bool bDirection;
	bool bOut;
	int nNodeIndex;
};

struct VECTOR_POS
{
	Pos pos;
	int nAngle;
};


struct NAVI_POS
{
	ROADLINK MatchRoad;
	ROAD_POS vecPos;
	float fSpeed;
	bool bMatch;
};

struct MATCHLINK
{
	bool operator < (const MATCHLINK& lhs) const
	{
		return fProperty > lhs.fProperty;
	}

	ROADLINK Road;
	Pos pos;
	bool bDirection;
	int nNodeIndex;
	int nAngleOffset;
	int nAngle;
	int nDist;
	float fProperty;
};

struct CONNECTSTATE
{
	bool operator == (const CONNECTSTATE& lhs)
	{
		return Road == lhs.Road;
	}
	ROADLINK Road;
	float fConnect;
};

struct MATCH_LINK
{
	bool operator< (const MATCH_LINK& rhs)
	{
		return nDist < rhs.nDist;
	}

	ROADLINK Road;
	ROAD_POS vecPos;
	int nDist;
	int nPriority;
};

typedef struct
{
	int nRoadIndex;
	int nNodeIndex;
	Pos pos;
	int nAngle;
} NAVI_LINK;

typedef struct
{
	int nPathIndex;
	int nRoadIndex;
	int nNodeIndex;
	Pos pos;
	int nAngle;
} NAVI_LINK2;



typedef struct
{	
	int nAngle;
	char chRoadName[100];
	int nDist;
	ROADLINK Road;
	ROADLINK NextRoad;
} PATH_LIST;

struct PRIORITY_PATH
{
	bool operator < (const PRIORITY_PATH& lhs) const
	{
		return nCost < lhs.nCost;
	}

	lstRoadLink listPath;
	int nPathLength;
	int nCost;

};
