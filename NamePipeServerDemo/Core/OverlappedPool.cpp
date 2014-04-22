#include "stdafx.h"
#include "OverlappedPool.h"
#include <assert.h>


COverlappedPool::COverlappedPool(void)
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

    LPOVERLAPPED_PACKAGE lpOvItem = new OVERLAPPED_PACKAGE[dwSize];
    //m_pEventHandleArr = new HANDLE[dwSize];

    for(DWORD i = 0; i < dwSize; i++)
    {
        m_vecOvItem.push_back(&lpOvItem[i]);

        //m_pEventHandleArr[i] = lpOvItem[i].ov.hEvent;
    }
}

void COverlappedPool::Close()
{
    EnterCriticalSection(&m_cslock);
	DWORD dwPoolSize = m_vecOvItem.size();

	for(DWORD i = 0; i < dwPoolSize; i++)
	{
		LPOVERLAPPED_PACKAGE lpOvItem = m_vecOvItem[i];

		if(NULL != lpOvItem)
		{
			delete lpOvItem;			lpOvItem = NULL;
		}
	}

    m_vecOvItem.clear();

//     delete[] m_pEventHandleArr;
//     m_pEventHandleArr = NULL;
    LeaveCriticalSection(&m_cslock);
}

LPOVERLAPPED_PACKAGE COverlappedPool::Alloc(IPC_MESSAGE_TYPE messageType)
{
    LPOVERLAPPED_PACKAGE itemNotUsed = new OVERLAPPED_PACKAGE;
	itemNotUsed->dwStatus=messageType;
//     EnterCriticalSection(&m_cslock);
//     DWORD dwPoolSize = m_vecOvItem.size();
// 
//     for(DWORD i = 0; i < dwPoolSize; i++)
//     {
//         LPOVERLAPPED_PACKAGE lpOvItem = m_vecOvItem[i];
// 
//         if((NULL != lpOvItem) && (FALSE == lpOvItem->bUsed))
//         {
//             ResetEvent(lpOvItem->ovHeader.hEvent);
//             lpOvItem->dwStatus = messageType;
//             lpOvItem->bUsed = TRUE;
//             itemNotUsed = lpOvItem;
//             break;
//         }
//     }
// 
//     LeaveCriticalSection(&m_cslock);
     return itemNotUsed;
}

void COverlappedPool::Release(LPOVERLAPPED_PACKAGE lpo)
{
	CloseHandle(lpo->hEvent);
//     EnterCriticalSection(&m_cslock);
//     assert(lpo->bUsed);
// 
//     if((NULL != lpo) && (TRUE == lpo->bUsed))
//     {
//         ResetEvent(lpo->ovHeader.hEvent);
//         lpo->bUsed = FALSE;
//     }
// 
//     LeaveCriticalSection(&m_cslock);
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

LPOVERLAPPED_PACKAGE CreateOverlapped(IPC_MESSAGE_TYPE messageType)
{
	LPOVERLAPPED_PACKAGE package=new OVERLAPPED_PACKAGE;
	ZeroMemory(package,sizeof(OVERLAPPED_PACKAGE));
	package->hEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
	package->dwStatus=messageType;
	return package;
}
