#pragma once
#include <windows.h>
#include <vector>

class COverlappedPool
{
public:
    typedef struct _OVERLAPPED_ITEM
    {
        OVERLAPPED ov;
        BOOL bUsed;
    } OVERLAPPED_ITEM, *LPOVERLAPPED_ITEM;

	typedef std::vector<LPOVERLAPPED_ITEM> ovItemVector;

    COverlappedPool(void);
    virtual ~COverlappedPool(void);

    void Create(DWORD dwSize = 20);
    void Close();
    LPOVERLAPPED Alloc();
    void Release(LPOVERLAPPED lpo);
	DWORD WaitAll(BOOL bWait,DWORD dwTimeout);

protected:
	LPOVERLAPPED_ITEM FindItemByOv(LPOVERLAPPED lpo);

private:
	ovItemVector m_vecOvItem;
	CRITICAL_SECTION m_cslock;
	HANDLE* m_pEventHandleArr;
};

