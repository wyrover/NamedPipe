#include "stdafx.h"
#include "NamedPipeServer.h"
#include <assert.h>
#include <stddef.h>
#include <tchar.h>


CNamedPipeServer::CNamedPipeServer(IIPCEvent* pEvent): m_pEventHandler(pEvent)
    , m_hCompletionPort(NULL)
{
    m_sNamedPipe[MAX_PATH - 1] = _T('\0');
    InitializeCriticalSection(&m_csConnnectorMap);
}


CNamedPipeServer::~CNamedPipeServer(void)
{
    if(NULL != m_hCompletionPort)
    {
        CloseHandle(m_hCompletionPort);
        m_hCompletionPort = NULL;
    }

    DeleteCriticalSection(&m_csConnnectorMap);
}

BOOL CNamedPipeServer::Create(LPCTSTR lpPipeName)
{
    assert(NULL == m_hCompletionPort);
    assert(NULL != lpPipeName);

    _tcscpy_s(m_sNamedPipe, MAX_PATH, _T("\\\\.\\pipe\\"));
    _tcscat_s(m_sNamedPipe, lpPipeName);

    m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);

    if(NULL == m_hCompletionPort)
        return FALSE;

    if(!_CreateIOCPThreadPool(0))
        return FALSE;

    if(NULL == WaitPipeConnection())
        return FALSE;

    return TRUE;
}

void CNamedPipeServer::Close()
{
    if(NULL != m_hCompletionPort)
    {
        CloseHandle(m_hCompletionPort);
        m_hCompletionPort = NULL;
    }
}

BOOL CNamedPipeServer::_CreateIOCPThreadPool(DWORD dwThreadNum)
{
    DWORD dwThread;
    HANDLE hThread;
    DWORD i;
    SYSTEM_INFO SystemInfo;

    GetSystemInfo(&SystemInfo);

    DWORD aThreadNum = (0 == dwThreadNum ? SystemInfo.dwNumberOfProcessors : dwThreadNum);

    for(i = 0; i < 2 * aThreadNum; i++)
    {
        hThread = CreateThread(NULL, 0, _IOCPThreadProc, this, 0, &dwThread);

        if(NULL == hThread)
            return FALSE;

        CloseHandle(hThread);
    }

    return TRUE;
}

DWORD __stdcall CNamedPipeServer::_IOCPThreadProc(LPVOID lpParam)
{
    CNamedPipeServer* pThis = (CNamedPipeServer*)lpParam;

    if(NULL == pThis)
        return -1;

    return pThis->_IOCPThread();
}

DWORD CNamedPipeServer::_IOCPThread()
{
    CNamedPipeConnector* pConnector = NULL;
    BOOL b;
    LPIPC_DATA_OVERLAPPEDEX lpo;
    DWORD nBytes;

    for(BOOL fKeepLooping = TRUE; fKeepLooping;)
    {
        lpo = NULL;
        nBytes = 0;
        b = GetQueuedCompletionStatus(m_hCompletionPort, &nBytes, (PULONG_PTR)&pConnector, (OVERLAPPED**)&lpo, INFINITE);

        if(!b && lpo == NULL)
        {
            fKeepLooping = FALSE;
            break;
        }
        else if(!b)
        {
//            if(GetLastError() == ERROR_BROKEN_PIPE)
            {
                CloseConnection(pConnector);
                continue;
            }
        }

        CNamedPipeConnector* pNamedConnector = dynamic_cast<CNamedPipeConnector*>(pConnector);

        if(NULL == pNamedConnector)
            return FALSE;

        IPC_MESSAGE_TYPE messageType = lpo->emMessageType;

        BOOL b = FALSE;

        switch(messageType)
        {
            case IPC_MESSAGE_CLIENTCONNECT:
            {
                pConnector->PostReadRequestToIOCP();
                OnConnect(this, pConnector);
                WaitPipeConnection();
                break;
            }

            case IPC_MESSAGE_CLIENTDISCONNECT:
            {
                CloseConnection(pNamedConnector);
                break;
            }

            case IPC_MESSAGE_READ:
            {
                OnRecv(this, pConnector, pConnector->m_recvPackage.ipcDataPackage.lpData, pConnector->m_recvPackage.ipcDataPackage.dwDataSize);

                pConnector->PostReadRequestToIOCP();
//                    CloseConnection(pConnector);

                break;
            }

            case IPC_MESSAGE_WRIE:
            {
                break;
            }

            default:
                break;
        }
    }

    return 0;
}

BOOL CNamedPipeServer::WaitPipeConnection()
{
    BOOL bSuccess = FALSE;

    do
    {
        HANDLE hPipe = CreateNamedPipe(m_sNamedPipe, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                       PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                       PIPE_UNLIMITED_INSTANCES, 0, 0, NMPWAIT_USE_DEFAULT_WAIT, NULL);

        if(hPipe == INVALID_HANDLE_VALUE)
            break;

        CNamedPipeConnector* pConnector = new CNamedPipeConnector(hPipe, this, m_pEventHandler);
        AddClient(hPipe, pConnector);

        if(NULL == CreateIoCompletionPort(hPipe, m_hCompletionPort, (ULONG_PTR)pConnector, 0))
            break;

        pConnector->m_sendPackage.emMessageType = IPC_MESSAGE_CLIENTCONNECT;

        if(!ConnectNamedPipe(hPipe, &pConnector->m_sendPackage))
        {
            if(GetLastError() != ERROR_IO_PENDING && GetLastError() != ERROR_PIPE_LISTENING)
                break;
        }

        bSuccess = TRUE;
    }
    while(FALSE);

    return bSuccess;
}

BOOL CNamedPipeServer::CloseConnection(IIPCConnector* pConnector)
{
    EnterCriticalSection(&m_csConnnectorMap);

    if(NULL != pConnector)
    {
        CNamedPipeConnector* pNamedConnector = (CNamedPipeConnector*)pConnector;

        if(NULL != pNamedConnector)
        {
            HANDLE hCom = pNamedConnector->m_hCom;

            if(NULL != pConnector)
                OnDisConnect(this, pNamedConnector);

            RemoveClient(hCom);
            delete pNamedConnector;
            pNamedConnector = NULL;
            hCom = INVALID_HANDLE_VALUE;
        }
    }

    LeaveCriticalSection(&m_csConnnectorMap);
    return TRUE;
}

void CNamedPipeServer::AddClient(HANDLE hPort, IIPCConnector* pClient)
{
    EnterCriticalSection(&m_csConnnectorMap);
    m_connectorMap.insert(std::make_pair(hPort, pClient));
    LeaveCriticalSection(&m_csConnnectorMap);
}

void CNamedPipeServer::RemoveClient(HANDLE hPort)
{
    IIPCConnector* pConnectorFind = FindClient(hPort);

    if(NULL != pConnectorFind)
    {
        EnterCriticalSection(&m_csConnnectorMap);
        m_connectorMap.erase(hPort);
        LeaveCriticalSection(&m_csConnnectorMap);
    }
}

IIPCConnector* CNamedPipeServer::FindClient(HANDLE hPort)
{
    EnterCriticalSection(&m_csConnnectorMap);

    IIPCConnector* pConnectorFind = NULL;
    ConnectorMap::const_iterator cit = m_connectorMap.begin();

    for(cit; cit != m_connectorMap.end(); cit++)
    {
        pConnectorFind = cit->second;

        if(NULL == pConnectorFind)
            continue;

        if(pConnectorFind->GetHandle() == hPort)
            break;
    }

    LeaveCriticalSection(&m_csConnnectorMap);
    return pConnectorFind;
}

void CNamedPipeServer::OnConnect(IIPCObject* pServer, IIPCConnector* pClient)
{
    if(NULL != m_pEventHandler)
        m_pEventHandler->OnConnect(pServer, pClient);
}

void CNamedPipeServer::OnDisConnect(IIPCObject* pServer, IIPCConnector* pClient)
{
    if(NULL != m_pEventHandler)
        m_pEventHandler->OnDisConnect(pServer, pClient);
}

void CNamedPipeServer::OnCreate(IIPCObject* pServer)
{
    if(NULL != m_pEventHandler)
        m_pEventHandler->OnCreate(pServer);
}

void CNamedPipeServer::OnClose(IIPCObject* pServer)
{
    if(NULL != m_pEventHandler)
        m_pEventHandler->OnClose(pServer);
}

void CNamedPipeServer::OnRecv(IIPCObject* pServer, IIPCConnector* pClient, LPCVOID lpBuf, DWORD dwBufSize)
{
    if(0 == dwBufSize || NULL == lpBuf)
        return ;

    if(NULL != m_pEventHandler)
        m_pEventHandler->OnRecv(pServer, pClient, lpBuf, dwBufSize);
}

void CNamedPipeServer::OnSend(IIPCObject* pServer, IIPCConnector* pClient, LPVOID lpBuf, DWORD dwBufSize)
{
    if(NULL != m_pEventHandler)
        m_pEventHandler->OnSend(pServer, pClient, lpBuf, dwBufSize);
}

void CNamedPipeServer::Begin()
{
    EnterCriticalSection(&m_csConnnectorMap);
    m_citCurrent = m_connectorMap.begin();
    LeaveCriticalSection(&m_csConnnectorMap);
}

BOOL CNamedPipeServer::End()
{
    BOOL bEnd = TRUE;
    EnterCriticalSection(&m_csConnnectorMap);
    bEnd = (m_citCurrent == m_connectorMap.end());
    LeaveCriticalSection(&m_csConnnectorMap);
    return bEnd;
}

void CNamedPipeServer::Next()
{
    EnterCriticalSection(&m_csConnnectorMap);

    if(!End())
        m_citCurrent++;

    LeaveCriticalSection(&m_csConnnectorMap);
}

IIPCConnector* CNamedPipeServer::GetCurrent()
{
    IIPCConnector* pConnector = NULL;

    EnterCriticalSection(&m_csConnnectorMap);

    if(!End())
        pConnector = m_citCurrent->second;

    LeaveCriticalSection(&m_csConnnectorMap);
    return pConnector;
}

IIPCConnectorIterator* CNamedPipeServer::GetClients()
{
    return this;
}

void CNamedPipeServer::CreateConnection(HANDLE hCom)
{

}

BOOL CNamedPipeConnector::SendMessage(LPCVOID lpBuf, DWORD dwBufSize)
{
    return PostMessage(lpBuf, dwBufSize);
}

BOOL CNamedPipeConnector::PostMessage(LPCVOID lpBuf, DWORD dwBufSize)
{
    m_sendPackage.emMessageType = IPC_MESSAGE_WRIE;
    m_sendPackage.ipcDataPackage.dwProcessID = GetCurrentProcessId();
    m_sendPackage.ipcDataPackage.dwDataSize = dwBufSize;
    memcpy_s(&m_sendPackage.ipcDataPackage.lpData, SYELOG_MAXIMUM_MESSAGE, lpBuf, dwBufSize);
    m_sendPackage.ipcDataPackage.dwTotalSize = sizeof(IPC_DATA_OVERLAPPEDEX) - SYELOG_MAXIMUM_MESSAGE + dwBufSize;
    DWORD dwWrited = 0;
    BOOL bSucess = WriteFile(m_hCom, &m_sendPackage.ipcDataPackage, m_sendPackage.ipcDataPackage.dwTotalSize, &dwWrited, &m_sendPackage);
    return ((bSucess == TRUE) || (GetLastError() == ERROR_IO_PENDING));
}

CNamedPipeConnector::CNamedPipeConnector(HANDLE hCom, IIPCObject* pServer, IIPCEvent* pEvent): m_hCom(hCom), m_pServer(pServer), m_pEventSensor(pEvent)
{
    CNamedPipeServer* aServer = dynamic_cast<CNamedPipeServer*>(pServer);
    ZeroMemory(&m_recvPackage, sizeof(IPC_DATA_OVERLAPPEDEX));
    ZeroMemory(&m_sendPackage, sizeof(IPC_DATA_OVERLAPPEDEX));
    m_recvPackage.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_sendPackage.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CNamedPipeConnector::~CNamedPipeConnector()
{
    if(NULL != m_hCom)
    {
        FlushFileBuffers(m_hCom);
        DisconnectNamedPipe(m_hCom);
        CloseHandle(m_hCom);
        m_hCom = NULL;
    }

    if(NULL != m_recvPackage.hEvent)
    {
        WaitForSingleObject(m_recvPackage.hEvent, INFINITE);
        CloseHandle(m_recvPackage.hEvent);
        m_recvPackage.hEvent = NULL;
    }

    if(NULL != m_sendPackage.hEvent)
    {
        WaitForSingleObject(m_sendPackage.hEvent, INFINITE);
        CloseHandle(m_sendPackage.hEvent);
        m_sendPackage.hEvent = NULL;
    }
}

HANDLE CNamedPipeConnector::GetHandle()
{
    return m_hCom;
}

BOOL CNamedPipeConnector::RequestAndReply(LPVOID lpSendBuf, DWORD dwSendBufSize, LPVOID lpReplyBuf, DWORD dwReplyBufSize, LPDWORD dwTransactSize)
{
    m_sendPackage.emMessageType = IPC_MESSAGE_READ_WRITE;
    m_sendPackage.ipcDataPackage.dwProcessID = GetCurrentProcessId();
    m_sendPackage.ipcDataPackage.dwDataSize = dwSendBufSize;
    memcpy_s(&m_sendPackage.ipcDataPackage.lpData, SYELOG_MAXIMUM_MESSAGE, lpSendBuf, dwSendBufSize);
    m_sendPackage.ipcDataPackage.dwTotalSize = sizeof(IPC_DATA_OVERLAPPEDEX) - SYELOG_MAXIMUM_MESSAGE + dwSendBufSize;

    m_recvPackage.emMessageType = IPC_MESSAGE_READ_WRITE;
    DWORD dwWrited = 0;
    BOOL bSucess = TransactNamedPipe(m_hCom, &m_sendPackage.ipcDataPackage, m_sendPackage.ipcDataPackage.dwTotalSize, &m_recvPackage.ipcDataPackage, sizeof(IPC_DATA_PACKAGE), &dwWrited, &m_sendPackage);

    if(GetLastError() == ERROR_IO_PENDING)
    {
        if(GetOverlappedResult(m_hCom, &m_sendPackage, dwTransactSize, TRUE))
            bSucess = TRUE;
    }

    if(bSucess)
    {
        memcpy_s(lpReplyBuf, dwReplyBufSize, m_recvPackage.ipcDataPackage.lpData, m_recvPackage.ipcDataPackage.dwDataSize);
        *dwTransactSize = m_recvPackage.ipcDataPackage.dwDataSize;
    }

    return TRUE;
}

DWORD CNamedPipeConnector::GetSID()
{
    return m_recvPackage.ipcDataPackage.dwProcessID;
}

LPCTSTR CNamedPipeConnector::GetName()
{
    return m_sName;
}

BOOL CNamedPipeConnector::PostReadRequestToIOCP()
{
    m_recvPackage.emMessageType = IPC_MESSAGE_READ;
    BOOL bSucess = ReadFile(m_hCom, &m_recvPackage.ipcDataPackage , sizeof(IPC_DATA_PACKAGE), NULL, &m_recvPackage);

    if(!bSucess && GetLastError() == ERROR_BROKEN_PIPE)
        return FALSE;

    return (bSucess || (GetLastError() == ERROR_IO_PENDING));
}

BOOL CNamedPipeConnector::PostWriteRequestToIOCP()
{
    return FALSE;
}
