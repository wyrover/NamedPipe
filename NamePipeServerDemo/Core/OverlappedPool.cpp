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

    LPOVERLAPPED_ITEM lpOvItem = new OVERLAPPED_ITEM[dwSize];
    m_pEventHandleArr = new HANDLE[dwSize];

    for(DWORD i = 0; i < dwSize; i++)
    {
        ZeroMemory(&lpOvItem[i].ov, sizeof(OVERLAPPED));
        lpOvItem[i].ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        lpOvItem[i].bUsed = FALSE;
        m_vecOvItem.push_back(&lpOvItem[i]);

        m_pEventHandleArr[i] = lpOvItem[i].ov.hEvent;
    }
}

void COverlappedPool::Close()
{
    EnterCriticalSection(&m_cslock);
    DWORD dwPoolSize = m_vecOvItem.size();

    for(DWORD i = 0; i < dwPoolSize; i++)
    {
        LPOVERLAPPED_ITEM lpOvItem = m_vecOvItem[i];

        if(NULL != lpOvItem && FALSE == lpOvItem->bUsed)
        {
//            SetEvent(lpOvItem->ov.hEvent);
            WaitForSingleObject(lpOvItem->ov.hEvent, INFINITE);
            CloseHandle(lpOvItem->ov.hEvent);
        }
    }

    delete[] m_pEventHandleArr;
    m_pEventHandleArr = NULL;
    LeaveCriticalSection(&m_cslock);
}

LPOVERLAPPED COverlappedPool::Alloc()
{
    LPOVERLAPPED_ITEM itemNotUsed = NULL;
    EnterCriticalSection(&m_cslock);
    DWORD dwPoolSize = m_vecOvItem.size();

    for(DWORD i = 0; i < dwPoolSize; i++)
    {
        LPOVERLAPPED_ITEM lpOvItem = m_vecOvItem[i];

        if((NULL != lpOvItem) && (FALSE == lpOvItem->bUsed))
        {
//            ResetEvent(lpOvItem->ov.hEvent);
            lpOvItem->bUsed = TRUE;
            itemNotUsed = lpOvItem;
            break;
        }
    }

    LeaveCriticalSection(&m_cslock);
    return &itemNotUsed->ov;
}

void COverlappedPool::Release(LPOVERLAPPED lpo)
{
    EnterCriticalSection(&m_cslock);
    LPOVERLAPPED_ITEM lpItem = FindItemByOv(lpo);
    assert(lpItem->bUsed);

    if((NULL != lpItem) && (TRUE == lpItem->bUsed))
    {
//        SetEvent(lpItem->ov.hEvent);
        lpItem->bUsed = FALSE;
    }

    LeaveCriticalSection(&m_cslock);
}

COverlappedPool::LPOVERLAPPED_ITEM COverlappedPool::FindItemByOv(LPOVERLAPPED lpo)
{
    LPOVERLAPPED_ITEM pFindItem = NULL;
    EnterCriticalSection(&m_cslock);
    DWORD dwPoolSize = m_vecOvItem.size();

    for(DWORD i = 0; i < dwPoolSize; i++)
    {
        LPOVERLAPPED_ITEM lpOvItem = m_vecOvItem[i];

        if(NULL == lpOvItem)
            continue;

        if(lpOvItem->ov.hEvent == lpo->hEvent)
        {
            pFindItem = lpOvItem;
            break;
        }
    }

    LeaveCriticalSection(&m_cslock);
    return pFindItem;
}

DWORD COverlappedPool::WaitAll(BOOL bWait, DWORD dwTimeout)
{
    DWORD dwVecSize = m_vecOvItem.size();
    return WaitForMultipleObjects(dwVecSize, m_pEventHandleArr, TRUE, dwTimeout);
}
