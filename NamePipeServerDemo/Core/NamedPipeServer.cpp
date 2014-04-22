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

LPIPC_DATA_PACKAGE CNamedPipeServer::CreateOverlapped(IPC_MESSAGE_TYPE messageType)
{
	LPIPC_DATA_PACKAGE package = new IPC_DATA_PACKAGE;
	ZeroMemory(package,sizeof(OVERLAPPED));
	package->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	package->emMessageType = messageType;

	package->dwProcessID = GetCurrentProcessId();
	GetSystemTimeAsFileTime(&package->ftOccurance);
	package->dwDataSize = 0;
	ZeroMemory(package->lpData, SYELOG_MAXIMUM_MESSAGE);
	package->dwTotalSize = 0;

	return package;
}

DWORD CNamedPipeServer::_IOCPThread()
{
    CNamedPipeConnector* pConnector = NULL;
    BOOL b;
    LPOVERLAPPED lpo;
    DWORD nBytes;

    for(BOOL fKeepLooping = TRUE; fKeepLooping;)
    {
        lpo = NULL;
        nBytes = 0;
        b = GetQueuedCompletionStatus(m_hCompletionPort, &nBytes, (PULONG_PTR)&pConnector, &lpo, INFINITE);

        if(!b && lpo == NULL)
        {
            fKeepLooping = FALSE;
            break;
        }
        else if(!b)
        {
            if(GetLastError() == ERROR_BROKEN_PIPE)
            {
                if(NULL == lpo)
                {
                    CloseConnection(pConnector);
                    continue;
                }
            }
        }

       CloseHandle(lpo->hEvent);

        CNamedPipeConnector* pNamedConnector = dynamic_cast<CNamedPipeConnector*>(pConnector);

        if(NULL == pNamedConnector)
            return FALSE;

		IPC_MESSAGE_TYPE messageType=((LPIPC_DATA_PACKAGE)lpo)->emMessageType;

		CloseHandle(lpo->hEvent);
		delete lpo;
		lpo=NULL;

        BOOL b = FALSE;

        switch(messageType)
        {
            case IPC_MESSAGE_CLIENTCONNECT:
            {

                LPIPC_DATA_PACKAGE package = CreateOverlapped(IPC_MESSAGE_READ);

                if(NULL == package)
                    break;

                b = ReadFile(pConnector->GetHandle(), &pConnector->m_recvPackage, sizeof(IPC_DATA_PACKAGE), NULL, package);

                if(!b)
                {
                    if(GetLastError() != ERROR_IO_PENDING)
                        break;
                }

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
                OnRecv(this, pConnector, pConnector->m_recvPackage.lpData, pConnector->m_recvPackage.dwDataSize);

                LPIPC_DATA_PACKAGE package = CreateOverlapped(IPC_MESSAGE_READ);

                if(NULL == package)
                    break;

                b = ReadFile(pConnector->GetHandle(), &pConnector->m_recvPackage, sizeof(IPC_DATA_PACKAGE), NULL, package);

                if(!b && GetLastError() == ERROR_BROKEN_PIPE)
                    CloseConnection(pNamedConnector);

                if(GetLastError() == ERROR_IO_PENDING)
                {
                    OutputDebugString(_T("IO PENDING\r\n"));
                }

                break;
            }

            case IPC_MESSAGE_WRIE:
            {
				LPIPC_DATA_PACKAGE package = CreateOverlapped(IPC_MESSAGE_READ);

				if(NULL == package)
					break;

				b = ReadFile(pConnector->GetHandle(), &pConnector->m_recvPackage, sizeof(IPC_DATA_PACKAGE), NULL, package);

				if(!b && GetLastError() == ERROR_BROKEN_PIPE)
					CloseConnection(pConnector);

				if(GetLastError() == ERROR_IO_PENDING)
				{
					OutputDebugString(_T("IO PENDING\r\n"));
				}
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

        LPIPC_DATA_PACKAGE package = CreateOverlapped(IPC_MESSAGE_CLIENTCONNECT);

        if(!ConnectNamedPipe(hPipe, package))
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
    CNamedPipeConnector* pNamedConnector = dynamic_cast<CNamedPipeConnector*>(pConnector);
    HANDLE hCom = pNamedConnector->m_hCom;
    FlushFileBuffers(hCom);
    DisconnectNamedPipe(hCom);

    if(NULL != pConnector)
        OnDisConnect(this, pConnector);

    RemoveClient(hCom);
    CloseHandle(hCom);
    hCom = INVALID_HANDLE_VALUE;

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
        delete pConnectorFind;
        pConnectorFind = NULL;
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
    IPC_DATA_PACKAGE sendPackage;
    sendPackage.dwDataSize = dwBufSize;
    memcpy_s(sendPackage.lpData, SYELOG_MAXIMUM_MESSAGE, lpBuf, dwBufSize);
    sendPackage.dwTotalSize = sizeof(IPC_DATA_PACKAGE) - SYELOG_MAXIMUM_MESSAGE  + dwBufSize;

    DWORD dwWrited = 0;
    BOOL bSucess = WriteFile(m_hCom, &sendPackage, sendPackage.dwTotalSize, &dwWrited, &sendPackage);
    return ((bSucess == TRUE) || (GetLastError() == ERROR_IO_PENDING));
}

BOOL CNamedPipeConnector::PostMessage(LPCVOID lpBuf, DWORD dwBufSize)
{
    return SendMessage(lpBuf, dwBufSize);
}

CNamedPipeConnector::CNamedPipeConnector(HANDLE hCom, IIPCObject* pServer, IIPCEvent* pEvent): m_hCom(hCom), m_pServer(pServer), m_pEventSensor(pEvent)
{
    CNamedPipeServer* aServer = dynamic_cast<CNamedPipeServer*>(pServer);
}

CNamedPipeConnector::~CNamedPipeConnector()
{
}

HANDLE CNamedPipeConnector::GetHandle()
{
    return m_hCom;
}

BOOL CNamedPipeConnector::RequestAndReply(LPVOID lpSendBuf, DWORD dwSendBufSize, LPVOID lpReplyBuf, DWORD dwReplyBufSize, LPDWORD dwTransactSize)
{
    if(NULL == m_hCom)
        return FALSE;

//     NAMED_PIPE_MESSAGE requestMessage;
//     GenericMessage(&requestMessage, lpSendBuf, dwSendBufSize);
//     NAMED_PIPE_MESSAGE replyMessage;
//     replyMessage.dwTotalSize = sizeof(NAMED_PIPE_MESSAGE);
//
//     BOOL bSucess = TransactNamedPipe(m_hCom, &requestMessage, requestMessage.dwTotalSize, &replyMessage, replyMessage.dwTotalSize, dwTransactSize, &m_recvPackage.ovHeader);
//
//     if(!bSucess)
//     {
//         if(GetLastError() == ERROR_IO_PENDING)
//         {
//             if(GetOverlappedResult(m_hCom, &m_recvPackage.ovHeader, dwTransactSize, TRUE))
//                 bSucess = TRUE;
//         }
//     }
//
//     if(bSucess)
//     {
//         memcpy_s(lpReplyBuf, dwReplyBufSize, replyMessage.szRequest, replyMessage.dwRequestLen);
//         *dwTransactSize = requestMessage.dwRequestLen;
//     }

//    return bSucess;

    return FALSE;
}

DWORD CNamedPipeConnector::GetSID()
{
    return m_recvPackage.dwProcessID;
}

LPCTSTR CNamedPipeConnector::GetName()
{
    return m_sName;
}
