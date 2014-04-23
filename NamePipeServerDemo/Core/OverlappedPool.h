#pragma once
#include <windows.h>
#include <vector>
#include "IIPCInterface.h"

class COverlappedPool
{
public:

    typedef std::vector<LPIPC_DATA_PACKAGE> ovItemVector;

    COverlappedPool(void);
    virtual ~COverlappedPool(void);

    void Create(DWORD dwSize = 20);
    void Close();
    LPIPC_DATA_PACKAGE Alloc(IPC_MESSAGE_TYPE messageType);
    void Release(LPIPC_DATA_PACKAGE lpo);
    DWORD WaitAll(BOOL bWait, DWORD dwTimeout);

// protected:
//  LPOVERLAPPED_PACKAGE FindItemByOv(LPOVERLAPPED lpo);

private:
    CRITICAL_SECTION m_cslock;

	LPIPC_DATA_PACKAGE m_packageArr;
};

