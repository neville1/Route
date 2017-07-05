#include "StdAfx.h"
#include "PriorityRoad.h"

void CPriorityRoad::Initial(int nMaxSize)
{
	m_vPriorityRoad.resize(nMaxSize);
}

bool CPriorityRoad::push(const PRIORITY_ROADLINK& P_Road)
{
	if(m_nCount == (int)m_vPriorityRoad.size() - 1)
	{
		return false;
	}

	m_nCount++;
	PRIORITY_ROADLINK tmpRoad;
	m_vPriorityRoad[m_nCount] = tmpRoad = P_Road;
	m_vPriorityRoad[0].nPriority = -1;
	int k = m_nCount;

	while( m_vPriorityRoad[ k >> 1].nPriority >= tmpRoad.nPriority )
	{
		m_vPriorityRoad[k] = m_vPriorityRoad[ k >> 1];
		k = k >> 1;
	}

	m_vPriorityRoad[k] = tmpRoad;
	return true;
}

void CPriorityRoad::pop()
{
	m_vPriorityRoad[1] = m_vPriorityRoad[m_nCount];
	m_nCount--;

	PRIORITY_ROADLINK tmpRoad;
	tmpRoad = m_vPriorityRoad[1];
	int k = 1;
	int j = 0;
	while( k <= ( m_nCount >> 1) )
	{
		j = k * 2; 
		if(m_vPriorityRoad[j].nPriority > m_vPriorityRoad[j + 1].nPriority) 
		{
			if(j < m_nCount)
			{
				j++;
			}
		}
		if( tmpRoad.nPriority <= m_vPriorityRoad[j].nPriority )
		{
			break;
		}
		m_vPriorityRoad[k] = m_vPriorityRoad[j];
		k = j;
	}
	m_vPriorityRoad[k] = tmpRoad;
}

PRIORITY_ROADLINK CPriorityRoad::front()
{
	return m_vPriorityRoad[1];
}

void CPriorityRoad::clear()
{
	m_nCount = 0;
}