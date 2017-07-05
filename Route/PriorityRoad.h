#pragma once

#include "PathDefine.h"

class CPriorityRoad
{
public:
	CPriorityRoad() : m_nCount(0) {}
	~CPriorityRoad() {}

	void Initial(int nMaxSize);

	PRIORITY_ROADLINK front();
	bool push(const PRIORITY_ROADLINK& P_Road);
	void pop();
	bool empty() const { return m_nCount ? false : true; };
	void clear();

	int GetCount() const { return m_nCount; }

private:
	vector<PRIORITY_ROADLINK> m_vPriorityRoad;
	int m_nCount;


};
