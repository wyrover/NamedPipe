#pragma once
#include <windows.h>
#include <vector>
#include "IIPCInterface.h"

class COverlappedPool
{
public:

    typedef std::vector<LPOVERLAPPED_PACKAGE> ovItemVector;

    COverlappedPool(void);
    virtual ~COverlappedPool(void);

    void Create(DWORD dwSize = 20);
    void Close();
    LPOVERLAPPED_PACKAGE Alloc(IPC_MESSAGE_TYPE messageType);
    void Release(LPOVERLAPPED_PACKAGE lpo);
    DWORD WaitAll(BOOL bWait, DWORD dwTimeout);

// protected:
//  LPOVERLAPPED_PACKAGE FindItemByOv(LPOVERLAPPED lpo);

private:
    ovItemVector m_vecOvItem;
    CRITICAL_SECTION m_cslock;
//  HANDLE* m_pEventHandleArr;
};

LPOVERLAPPED_PACKAGE CreateOverlapped(IPC_MESSAGE_TYPE messageType);

