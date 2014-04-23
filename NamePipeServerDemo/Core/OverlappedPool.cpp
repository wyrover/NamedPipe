#include "stdafx.h"
#include "OverlappedPool.h"
#include <assert.h>


COverlappedPool::COverlappedPool(void):m_packageArr(NULL)
{
    InitializeCriticalSection(&m_cslock);
}


COverlappedPool::~COverlappedPool(void)
{
    DeleteCriticalSection(&m_cslock);
}

void COverlappedPool::Create(DWORD dwSize /*= 20*/)
{
    if(dwSize <= 0)
        return ;
	if (NULL==m_packageArr)
		m_packageArr= new IPC_DATA_PACKAGE[dwSize];
}

void COverlappedPool::Close()
{
    EnterCriticalSection(&m_cslock);
	delete[] m_packageArr;
	m_packageArr=NULL;
    LeaveCriticalSection(&m_cslock);
}

LPIPC_DATA_PACKAGE COverlappedPool::Alloc(IPC_MESSAGE_TYPE messageType)
{
    LPIPC_DATA_PACKAGE itemNotUsed = NULL;
     EnterCriticalSection(&m_cslock);
	 DWORD dwPoolSize=sizeof(m_packageArr)/sizeof(m_packageArr[0]);
	 for ( DWORD i=0;i<dwPoolSize;i++)
	 {
		 LPIPC_DATA_PACKAGE aPackage=&m_packageArr[i];
		 if (NULL!=aPackage && aPackage->bUsed)
		 {
			 itemNotUsed=aPackage;
			 break;
		 }
	 }
     LeaveCriticalSection(&m_cslock);
     return itemNotUsed;
}

void COverlappedPool::Release(LPIPC_DATA_PACKAGE lpo)
{
//	CloseHandle(lpo->hEvent);
     EnterCriticalSection(&m_cslock);
     assert(lpo->bUsed);
	 lpo->bUsed=FALSE;
     LeaveCriticalSection(&m_cslock);
}

// LPOVERLAPPED_PACKAGE COverlappedPool::FindItemByOv(LPOVERLAPPED lpo)
// {
//     LPOVERLAPPED_PACKAGE pFindItem = NULL;
//     EnterCriticalSection(&m_cslock);
//     DWORD dwPoolSize = m_vecOvItem.size();
//
//     for(DWORD i = 0; i < dwPoolSize; i++)
//     {
//         LPOVERLAPPED_PACKAGE lpOvItem = m_vecOvItem[i];
//
//         if(NULL == lpOvItem)
//             continue;
//
//         if(lpOvItem->ovHeader.hEvent == lpo->hEvent)
//         {
//             pFindItem = lpOvItem;
//             break;
//         }
//     }
//
//     LeaveCriticalSection(&m_cslock);
//     return pFindItem;
// }

DWORD COverlappedPool::WaitAll(BOOL bWait, DWORD dwTimeout)
{
    //DWORD dwVecSize = m_vecOvItem.size();
    //return WaitForMultipleObjects(dwVecSize, m_pEventHandleArr, TRUE, dwTimeout);
    return 0;
}