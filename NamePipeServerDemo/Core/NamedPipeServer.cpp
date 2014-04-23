#include "stdafx.h"
#include "NamedPipeServer.h"
#include <tchar.h>
#include <locale.h>


CNamedPipeServer::CNamedPipeServer(IIPCEvent* pEvent): m_pEvent(pEvent)
{

}

CNamedPipeServer::~CNamedPipeServer()
{

}

BOOL CNamedPipeServer::Create(LPCTSTR lpPipeName)
{
    if(NULL == lpPipeName)
        return FALSE;

    _tcscpy_s(m_sPipeName, MAX_PATH, _T("\\\\.\\pipe\\"));
    _tcscat_s(m_sPipeName, lpPipeName);

    const DWORD dwThreadCount = GetCpuNum();

    if(!m_iocp.Create(dwThreadCount))
        return FALSE;

    m_hThreadIOCP = new HANDLE[dwThreadCount];

    for(DWORD i = 0; i < dwThreadCount; ++i)
    {
        // The threads run CompletionThread
        m_hThreadIOCP[i] = CreateThread(0, 0, IOCompletionThread, this, 0, NULL);
    }

    WaitClientConnect();
    return TRUE;
}

void CNamedPipeServer::Close()
{
    m_iocp.Close();
    const DWORD dwThreadCount = GetCpuNum();
    WaitForMultipleObjects(dwThreadCount, m_hThreadIOCP, TRUE, INFINITE);

    for(DWORD i = 0; i < dwThreadCount; i++)
    {
        CloseHandle(m_hThreadIOCP[i]);
        m_hThreadIOCP[i] = NULL;
    }

    delete m_hThreadIOCP;
    m_hThreadIOCP = NULL;

    for(ConnectorMap::const_iterator cit = m_connectorMap.begin(); cit != m_connectorMap.end(); cit++)
    {
        IIPCConnector* pClient = cit->second;

        if(NULL != pClient)
        {
            delete pClient;
            pClient = NULL;
        }
    }

    m_connectorMap.clear();
}

IIPCConnectorIterator* CNamedPipeServer::GetClients()
{
    throw std::logic_error("The method or operation is not implemented.");
}

void CNamedPipeServer::Begin()
{
    throw std::logic_error("The method or operation is not implemented.");
}

BOOL CNamedPipeServer::End()
{
    throw std::logic_error("The method or operation is not implemented.");
}

void CNamedPipeServer::Next()
{
    throw std::logic_error("The method or operation is not implemented.");
}

IIPCConnector* CNamedPipeServer::GetCurrent()
{
    throw std::logic_error("The method or operation is not implemented.");
}

DWORD WINAPI CNamedPipeServer::IOCompletionThread(LPVOID lpParam)
{
    CNamedPipeServer* pThis = (CNamedPipeServer*)lpParam;

    if(NULL == lpParam)
        return -1;

    CIOCompletionPort* iocp = &pThis->m_iocp;

    CNamedPipeConnector* pClient = 0;
    DWORD dwBytesTransferred = 0;
    IPC_DATA_OVERLAPPEDEX* po = NULL;
    BOOL bSucess = FALSE;

    while(TRUE)
    {
        bSucess = iocp->DequeuePacket((ULONG_PTR*)&pClient, &dwBytesTransferred, (OVERLAPPED **)&po, INFINITE);

        if(pClient == 0 || NULL == po)
        {
            break;
        }
        else if(!bSucess && GetLastError() == ERROR_BROKEN_PIPE)
        {
            pClient->m_bExit = TRUE;
            continue;
        }

        IPC_MESSAGE_TYPE type = po->emMessageType;

        switch(type)
        {
            case IPC_MESSAGE_CLIENTCONNECT:
                pClient->DoRead();
                pThis->WaitClientConnect();
                break;

            case IPC_MESSAGE_READ:
                _tprintf(_T("%d bytes were read: %s"), dwBytesTransferred, po->ipcDataPackage.lpData);
                pThis->m_pEvent->OnRecv(pThis, pClient, &po->ipcDataPackage, dwBytesTransferred);
                pClient->DoRead();
                break;

            case IPC_MESSAGE_WRIE:
                break;

            case IPC_MESSAGE_READ_WRITE:
                break;
        }
    }

    _tprintf(_T("DequeuePacket failed w/err 0x%08lx\n"), GetLastError());

    return 0;
}

BOOL CNamedPipeServer::WaitClientConnect()
{
    CheckExit();

    CNamedPipeConnector* pClient = new CNamedPipeConnector();

    if(!pClient->Create(m_sPipeName))
    {
        delete pClient;
        pClient = NULL;
        return FALSE;
    }

    m_connectorMap.insert(std::make_pair(pClient->GetHandle(), pClient));

    if(!m_iocp.AssociateDevice(pClient->GetHandle(), (ULONG_PTR)pClient))
        return FALSE;

    if(!pClient->WaitConnect())
        return FALSE;

    return TRUE;
}

DWORD CNamedPipeServer::GetCpuNum()
{
    SYSTEM_INFO sysInfo = { 0 };
    GetNativeSystemInfo(&sysInfo);
    return sysInfo.dwNumberOfProcessors;
}

void CNamedPipeServer::CheckExit()
{
    for(ConnectorMap::const_iterator cit = m_connectorMap.begin(); cit != m_connectorMap.end();)
    {
        CNamedPipeConnector* pClient = (CNamedPipeConnector*)cit->second;

        if(NULL != pClient)
        {
            if(pClient->m_bExit)
            {
                _tsetlocale(LC_ALL, _T("chs"));
                _tprintf(_T("¿Í»§¶Ë %d ¹Ø±Õ\n"), pClient->GetSID());
                pClient->Close();
                delete pClient;
                pClient = NULL;
                m_connectorMap.erase(cit++);
            }
            else
                cit++;
        }
        else
            cit++;
    }
}

CNamedPipeConnector::CNamedPipeConnector(): m_bExit(FALSE)
{
    ZeroMemory(&m_sendPackage, sizeof(IPC_DATA_OVERLAPPEDEX));
    ZeroMemory(&m_recvPackage, sizeof(IPC_DATA_OVERLAPPEDEX));
    ZeroMemory(&m_connPackage, sizeof(IPC_DATA_OVERLAPPEDEX));

    m_sendPackage.emMessageType = IPC_MESSAGE_WRIE;
    m_recvPackage.emMessageType = IPC_MESSAGE_READ;
    m_connPackage.emMessageType = IPC_MESSAGE_CLIENTCONNECT;
}

CNamedPipeConnector::~CNamedPipeConnector()
{

}

DWORD CNamedPipeConnector::GetSID()
{
    return m_recvPackage.ipcDataPackage.dwProcessID;
}

LPCTSTR CNamedPipeConnector::GetName()
{
    return NULL;
}

BOOL CNamedPipeConnector::SendMessage(LPCVOID lpBuf, DWORD dwBufSize)
{
    return PostMessage(lpBuf, dwBufSize);
}

BOOL CNamedPipeConnector::PostMessage(LPCVOID lpBuf, DWORD dwBufSize)
{
    m_sendPackage.emMessageType = IPC_MESSAGE_WRIE;
    m_sendPackage.ipcDataPackage.dwDataSize = dwBufSize;
    m_sendPackage.ipcDataPackage.dwProcessID = GetCurrentProcessId();
    GetSystemTimeAsFileTime(&m_sendPackage.ipcDataPackage.ftOccurance);
    m_sendPackage.ipcDataPackage.dwTotalSize = sizeof(IPC_DATA_PACKAGE) - SYELOG_MAXIMUM_MESSAGE * sizeof(TCHAR) + dwBufSize;
    m_sendPackage.ipcDataPackage.msgType = IPC_MESSAGE_WRIE;
    memcpy_s(m_sendPackage.ipcDataPackage.lpData, SYELOG_MAXIMUM_MESSAGE, lpBuf, dwBufSize);

    DWORD dwWrited = 0;
    BOOL bSucess = m_pipe.WriteFile(&m_sendPackage.ipcDataPackage, m_sendPackage.ipcDataPackage.dwTotalSize, &dwWrited, &m_sendPackage);
    return (bSucess || GetLastError() == ERROR_IO_PENDING);
}

BOOL CNamedPipeConnector::RequestAndReply(LPVOID lpSendBuf, DWORD dwSendBufSize, LPVOID lpReplyBuf, DWORD dwReplyBufSize, LPDWORD dwTransactSize)
{
    return FALSE;
}

BOOL CNamedPipeConnector::Create(LPCTSTR lpPipeName)
{
    BOOL bSucess = m_pipe.CreateNamedPipe(lpPipeName,         // pipe name
                                          PIPE_ACCESS_DUPLEX |       // read-only access
                                          FILE_FLAG_OVERLAPPED,       // overlapped mode
                                          PIPE_TYPE_MESSAGE |         // message-type pipe
                                          PIPE_READMODE_MESSAGE |     // message read mode
                                          PIPE_WAIT,                   // blocking mode
                                          PIPE_UNLIMITED_INSTANCES,   // unlimited instances
                                          0,                          // output buffer size
                                          0,                          // input buffer size
                                          NMPWAIT_USE_DEFAULT_WAIT,                      // client time-out
                                          NULL);                      // no security attributes

    return bSucess;
}

void CNamedPipeConnector::Close()
{
    m_pipe.FlushFileBuffers();
    m_pipe.DisconnectNamedPipe();
    m_pipe.Close();
}

HANDLE CNamedPipeConnector::GetHandle()
{
    return m_pipe.GetHandle();
}

BOOL CNamedPipeConnector::DoRead()
{
    BOOL bSucess = m_pipe.ReadFile(&m_recvPackage.ipcDataPackage, sizeof(IPC_DATA_PACKAGE), NULL, &m_recvPackage);
    return (bSucess || ERROR_IO_PENDING == GetLastError());
}

BOOL CNamedPipeConnector::WaitConnect()
{
    return m_pipe.ConnectNamedPipe(&m_connPackage);
}

BOOL CNamedPipeConnector::DoReadWrite()
{
    return FALSE;
}
