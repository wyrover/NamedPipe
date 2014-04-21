#pragma once
#include <windows.h>
#include <map>

enum NAMEDPIPE_STATUS
{
    NAMED_PIPE_CONNECT,
    NAMED_PIPE_READING,
    NAMED_PIPE_WRIEING,
    NAMED_PIPE_DISCONNECT
};

const int SYELOG_MAXIMUM_MESSAGE = 4096;

typedef struct _SYELOG_MESSAGE
{
    _SYELOG_MESSAGE()
    {
        ZeroMemory(szRequest, SYELOG_MAXIMUM_MESSAGE);
        dwTotalSize = 0;
        nProcessId = 0;
        dwRequestLen = 0;
    }

    ~_SYELOG_MESSAGE()
    {

    }

    DWORD       dwTotalSize;
    DWORD       nProcessId;
    FILETIME    ftOccurance;
    DWORD       dwRequestLen;
    TCHAR       szRequest[SYELOG_MAXIMUM_MESSAGE];

} NAMED_PIPE_MESSAGE, *PSYELOG_MESSAGE;

typedef struct _CLIENT
{
    _CLIENT()
    {
        ZeroMemory(&ovlappedRead, sizeof(OVERLAPPED));
        ovlappedRead.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        hPipe = NULL;
        emPipeStatus = NAMED_PIPE_CONNECT;
        ZeroMemory(&Message, sizeof(NAMED_PIPE_MESSAGE));
    }

    ~_CLIENT()
    {
        if(NULL != ovlappedRead.hEvent)
        {
            CloseHandle(ovlappedRead.hEvent);
            ovlappedRead.hEvent = NULL;
        }
    }

    OVERLAPPED          ovlappedRead;
    HANDLE              hPipe;
    NAMEDPIPE_STATUS    emPipeStatus;
    NAMED_PIPE_MESSAGE      Message;

} CLIENT, *PCLIENT;

#define pure_virtual __declspec(novtable)

struct pure_virtual IIPCConnector
{
    virtual ~IIPCConnector() = 0 {};
    virtual DWORD GetSID() = 0;
    virtual LPCTSTR GetName() = 0;
    virtual HANDLE GetHandle() = 0;
    virtual BOOL SendMessage(LPCVOID lpBuf, DWORD dwBufSize) = 0;
    virtual BOOL PostMessage(LPCVOID lpBuf, DWORD dwBufSize) = 0;
    virtual BOOL RequestAndReply(LPVOID lpSendBuf, DWORD dwSendBufSize, LPVOID lpReplyBuf, DWORD dwReplyBufSize, LPDWORD dwTransactSize) = 0;
};

struct pure_virtual IIPCConnectorIterator
{
    virtual ~IIPCConnectorIterator() = 0 {};
    virtual void Begin() = 0;
    virtual BOOL End() = 0;
    virtual void Next() = 0;
    virtual IIPCConnector* GetCurrent() = 0;
};

typedef std::map<HANDLE, IIPCConnector*> ConnectorMap;

struct pure_virtual IIPCObject
{
    virtual ~IIPCObject() = 0 {};
    virtual BOOL Create(LPCTSTR lpPipeName) = 0;
    virtual void Close() = 0;
    virtual IIPCConnectorIterator* GetClients() = 0;
};

struct pure_virtual IIPCEvent
{
    virtual ~IIPCEvent() = 0 {};
    virtual void OnConnect(IIPCObject* pServer, IIPCConnector* pClient) = 0;
    virtual void OnDisConnect(IIPCObject* pServer, IIPCConnector* pClient) = 0;
    virtual void OnCreate(IIPCObject* pServer) = 0;
    virtual void OnClose(IIPCObject* pServer) = 0;
    virtual void OnRecv(IIPCObject* pServer, IIPCConnector* pClient, LPCVOID lpBuf, DWORD dwBufSize) = 0;
    virtual void OnSend(IIPCObject* pServer, IIPCConnector* pClient, LPVOID lpBuf, DWORD dwBufSize) = 0;
};