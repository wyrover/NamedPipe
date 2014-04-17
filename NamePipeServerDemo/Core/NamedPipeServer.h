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

    BOOL CloseConnection(PCLIENT pClient);

    void CreateConnection(PCLIENT pClient);

    void AddClient(HANDLE hPort, IIPCConnector* pClient);

    void RemoveClient(HANDLE hPort);

    IIPCConnector* FindClient(HANDLE hPort);

private:

    IIPCEvent* m_pEventHandler;

    HANDLE m_hCompletionPort;

    TCHAR m_sNamedPipe[MAX_PATH];

    CRITICAL_SECTION m_csReadIng;

    ConnectorMap m_connectorMap;

    ConnectorMap::const_iterator m_citCurrent;
};

class CNamedPipeConnector : public IIPCConnector
{
public:
    CNamedPipeConnector(HANDLE hPipe, IIPCObject* pServer, IIPCEvent* pEvent);

    virtual ~CNamedPipeConnector();

    virtual HANDLE GetHandle();

    virtual BOOL SendMessage(LPCVOID lpBuf, DWORD dwBufSize);

    virtual BOOL PostMessage(LPCVOID lpBuf, DWORD dwBufSize);

private:
    HANDLE m_hPipe;

    IIPCObject* m_pServer;

    IIPCEvent* m_pEventSensor;

    OVERLAPPED m_ovPostMessage;
};

