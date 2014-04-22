#pragma once
#include <windows.h>
#include <map>

enum OVERLAPPED_STATUS
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

// typedef struct _CLIENT: OVERLAPPED
// {
//     _CLIENT()
//     {
//         hPipe = NULL;
//         emPipeStatus = NAMED_PIPE_CONNECT;
//         ZeroMemory(&replyMessage, sizeof(NAMED_PIPE_MESSAGE));
//     }
//
//     HANDLE              hPipe;
//     OVERLAPPED_STATUS    emPipeStatus;
//     NAMED_PIPE_MESSAGE  requestMessage;
//  NAMED_PIPE_MESSAGE  replyMessage;
//
// } CLIENT, *PCLIENT;

typedef struct _OVERLAPPED_PACKAGE
{
    OVERLAPPED ovHeader;                            // OVERLAPPED同步头
    HANDLE hCom;                                    // 通信句柄
    DWORD dwProcessID;                              // 当前进程ID
    FILETIME ftOccurance;                           // 异步投递发生时间
    DWORD dwRequestSize;                            // 用户携带请求数据量
    BYTE lpRequestBuf[SYELOG_MAXIMUM_MESSAGE];      // 用户请求缓冲区
    DWORD dwReplySize;                              // 用户回应数据量
    BYTE lpReply[SYELOG_MAXIMUM_MESSAGE];           // 用户回应缓冲区
    OVERLAPPED_STATUS dwStatus;                     // 当前异步投递状态
    BOOL bUsed;                                     // 当前是否正在被使用
    BOOL bAsync;                                    // 是同步请求还是异步请求
    DWORD dwTotalSize;                              // 通信包整体大小

} OVERLAPPED_PACKAGE, *LPOVERLAPPED_PACKAGE;

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