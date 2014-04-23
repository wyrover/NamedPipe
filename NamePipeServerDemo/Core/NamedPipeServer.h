// ²Î¿¼ÎÄÕÂMSDN
// I/O Completion Ports http://msdn.microsoft.com/en-us/library/windows/desktop/aa365198%28v=vs.85%29.aspx
// Synchronous and Asynchronous I/O http://msdn.microsoft.com/en-us/library/windows/desktop/aa365683%28v=vs.85%29.aspx
// Synchronization and Overlapped Input and Output http://msdn.microsoft.com/en-us/library/windows/desktop/ms686358%28v=vs.85%29.aspx
#pragma once
#include "IIPCInterface.h"
#include <map>

class CNamedPipeServer : public IIPCObject , public IIPCEvent , public IIPCConnectorIterator
{
public:
    CNamedPipeServer(IIPCEvent* pEvent);

    virtual ~CNamedPipeServer(void);

    // IIPCObject
    virtual BOOL Create(LPCTSTR lpPipeName);

    virtual void Close();

    virtual IIPCConnectorIterator* GetClients();

    // IIPCEvent
    virtual void OnConnect(IIPCObject* pServer, IIPCConnector* pClient);

    virtual void OnDisConnect(IIPCObject* pServer, IIPCConnector* pClient);

    virtual void OnCreate(IIPCObject* pServer);

    virtual void OnClose(IIPCObject* pServer);

    virtual void OnRecv(IIPCObject* pServer, IIPCConnector* pClient, LPCVOID lpBuf, DWORD dwBufSize);

    virtual void OnSend(IIPCObject* pServer, IIPCConnector* pClient, LPVOID lpBuf, DWORD dwBufSize);

    // IIPCClientIterator
    virtual void Begin();

    virtual BOOL End();

    virtual void Next();

    virtual IIPCConnector* GetCurrent();

protected:

    BOOL _CreateIOCPThreadPool(DWORD dwThreadNum);

    static DWORD __stdcall _IOCPThreadProc(LPVOID lpParam);

    DWORD _IOCPThread();

    BOOL WaitPipeConnection();

    BOOL CloseConnection(IIPCConnector* pConnector);

    void CreateConnection(HANDLE hCom);

    void AddClient(HANDLE hPort, IIPCConnector* pClient);

    void RemoveClient(HANDLE hPort);

    IIPCConnector* FindClient(HANDLE hPort);

    LPIPC_DATA_PACKAGE CreateOverlapped(IPC_MESSAGE_TYPE messageType);

    friend class CNamedPipeConnector;

private:

    IIPCEvent* m_pEventHandler;

    HANDLE m_hCompletionPort;

    TCHAR m_sNamedPipe[MAX_PATH];

    CRITICAL_SECTION m_csConnnectorMap;

    ConnectorMap m_connectorMap;

    ConnectorMap::const_iterator m_citCurrent;
};

class CNamedPipeConnector : public IIPCConnector
{
public:
    CNamedPipeConnector(HANDLE hCom, IIPCObject* pServer, IIPCEvent* pEvent);

    virtual ~CNamedPipeConnector();

    virtual HANDLE GetHandle();

    virtual DWORD GetSID();

    virtual LPCTSTR GetName();

    virtual BOOL SendMessage(LPCVOID lpBuf, DWORD dwBufSize);

    virtual BOOL PostMessage(LPCVOID lpBuf, DWORD dwBufSize);

    virtual BOOL RequestAndReply(LPVOID lpSendBuf, DWORD dwSendBufSize, LPVOID lpReplyBuf, DWORD dwReplyBufSize, LPDWORD dwTransactSize);

    friend class CNamedPipeServer;

protected:
    BOOL PostReadRequestToIOCP();

    BOOL PostWriteRequestToIOCP();

private:

    HANDLE m_hCom;

    IIPCObject* m_pServer;

    IIPCEvent* m_pEventSensor;

    TCHAR m_sName[MAX_PATH];

    IPC_DATA_OVERLAPPEDEX m_recvPackage;

    IPC_DATA_OVERLAPPEDEX m_sendPackage;

};

