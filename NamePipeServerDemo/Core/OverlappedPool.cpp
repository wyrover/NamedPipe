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
        ZeroMemory(&lpOvItem[i], sizeof(OVERLAPPED_PACKAGE));
        lpOvItem[i].ovHeader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        lpOvItem[i].dwProcessID = GetCurrentProcessId();
        GetSystemTimeAsFileTime(&lpOvItem[i].ftOccurance);
        lpOvItem[i].dwStatus = NAMED_PIPE_CONNECT;
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
            SetEvent(lpOvItem->ovHeader.hEvent);
            WaitForSingleObject(lpOvItem->ovHeader.hEvent, INFINITE);
            CloseHandle(lpOvItem->ovHeader.hEvent);
            lpOvItem->ovHeader.hEvent = NULL;
            delete lpOvItem;
            lpOvItem = NULL;
        }
    }

    m_vecOvItem.clear();

//     delete[] m_pEventHandleArr;
//     m_pEventHandleArr = NULL;
    LeaveCriticalSection(&m_cslock);
}

LPOVERLAPPED_PACKAGE COverlappedPool::Alloc()
{
    LPOVERLAPPED_PACKAGE itemNotUsed = NULL;
    EnterCriticalSection(&m_cslock);
    DWORD dwPoolSize = m_vecOvItem.size();

    for(DWORD i = 0; i < dwPoolSize; i++)
    {
        LPOVERLAPPED_PACKAGE lpOvItem = m_vecOvItem[i];

        if((NULL != lpOvItem) && (FALSE == lpOvItem->bUsed))
        {
            if(NULL != lpOvItem->ovHeader.hEvent)
                ResetEvent(lpOvItem->ovHeader.hEvent);

            lpOvItem->bUsed = TRUE;
            itemNotUsed = lpOvItem;
            break;
        }
    }

    LeaveCriticalSection(&m_cslock);
    return itemNotUsed;
}

void COverlappedPool::Release(LPOVERLAPPED_PACKAGE lpo)
{
    EnterCriticalSection(&m_cslock);
    assert(lpo->bUsed);

    if((NULL != lpo) && (TRUE == lpo->bUsed))
    {
        if(NULL != lpo->ovHeader.hEvent)
            SetEvent(lpo->ovHeader.hEvent);

        lpo->bUsed = FALSE;
    }

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
