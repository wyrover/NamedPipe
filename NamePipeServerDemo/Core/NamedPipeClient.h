#pragma once
#include "IIPCInterface.h"
#include <queue>

class CNamedPipeClient : public IIPCObject , public IIPCEvent , public IIPCConnector, public IIPCConnectorIterator
{
public:

    CNamedPipeClient(IIPCEvent* pEvent);

    virtual ~CNamedPipeClient(void);

    virtual BOOL Create(LPCTSTR lpPipeName);

    virtual void Close();

    virtual IIPCConnectorIterator* GetClients();

    virtual void OnConnect(IIPCObject* pServer, IIPCConnector* pClient);

    virtual void OnDisConnect(IIPCObject* pServer, IIPCConnector* pClient);

    virtual void OnCreate(IIPCObject* pServer);

    virtual void OnClose(IIPCObject* pServer);

    virtual void OnRecv(IIPCObject* pServer, IIPCConnector* pClient, LPCVOID lpBuf, DWORD dwBufSize);

    virtual void OnSend(IIPCObject* pServer, IIPCConnector* pClient, LPVOID lpBuf, DWORD dwBufSize);

    virtual HANDLE GetHandle();

    virtual DWORD GetSID();

    virtual LPCTSTR GetName();

    virtual BOOL SendMessage(LPCVOID lpBuf, DWORD dwBufSize);

    virtual BOOL PostMessage(LPCVOID lpBuf, DWORD dwBufSize);

    virtual BOOL RequestAndReply(LPVOID lpSendBuf, DWORD dwSendBufSize, LPVOID lpReplyBuf, DWORD dwReplyBufSize, LPDWORD dwTransactSize);

    virtual void Begin();

    virtual BOOL End();

    virtual void Next();

    virtual IIPCConnector* GetCurrent();

protected:

    BOOL GenericMessage(NAMED_PIPE_MESSAGE* pMessage, LPCVOID lpRequest, DWORD dwRequestLen);

    static DWORD __stdcall _RecvThreadProc(LPVOID lpParam);

    DWORD _RecvThread();

private:
    HANDLE m_hRecvThread ;

    IIPCEvent* m_pEventSensor;

    HANDLE m_hPipe;

    int m_iIterator;

//     OVERLAPPED m_ovRead;
//
//     OVERLAPPED m_ovWrite;

    DWORD m_dwPID;

    TCHAR m_sName[MAX_PATH];

    OVERLAPPED_PACKAGE m_DataPackage;
};

