#include "stdafx.h"
#include "NamedPipeClient.h"


CNamedPipeClient::CNamedPipeClient(IIPCEvent* pEvent): m_pEventSensor(pEvent)
    , m_hPipe(INVALID_HANDLE_VALUE)
{

}


CNamedPipeClient::~CNamedPipeClient(void)
{

}

BOOL CNamedPipeClient::Create(LPCTSTR lpPipeName)
{
    TCHAR sPipeName[MAX_PATH] = {0};
    _tcscpy_s(sPipeName, MAX_PATH, _T("\\\\.\\pipe\\"));
    _tcscat_s(sPipeName, lpPipeName);

    while(TRUE)
    {
        m_hPipe = CreateFile(sPipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

        if(m_hPipe != INVALID_HANDLE_VALUE)
        {
            OnCreate(this);
            break;
        }

        DWORD dwError = GetLastError();

        if(ERROR_PIPE_BUSY != dwError)
            return FALSE;

        if(!WaitNamedPipe(sPipeName, 5000))
            return FALSE;
    }

    m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);

    if(NULL == m_hCompletionPort)
        return FALSE;

    if(NULL == CreateIoCompletionPort(m_hPipe, m_hCompletionPort, (ULONG_PTR)this, 0))
        return FALSE;

    if(!_CreateIOCPThreadPool(0))
        return FALSE;


    DWORD dwMode = PIPE_READMODE_MESSAGE ;
    BOOL fSuccess = SetNamedPipeHandleState(m_hPipe, &dwMode, NULL, NULL);

    if(!fSuccess)
        return FALSE;

    return TRUE;
}

void CNamedPipeClient::Close()
{
    if(NULL != m_hPipe)
    {
        CloseHandle(m_hPipe);
        m_hPipe = NULL;
    }

    if(NULL != m_hCompletionPort)
    {
        CloseHandle(m_hCompletionPort);
        m_hCompletionPort = NULL;
    }
}


void CNamedPipeClient::OnConnect(IIPCObject* pServer, IIPCConnector* pClient)
{
    if(NULL != m_pEventSensor)
        m_pEventSensor->OnConnect(pServer, pClient);
}

void CNamedPipeClient::OnDisConnect(IIPCObject* pServer, IIPCConnector* pClient)
{
    if(NULL != m_pEventSensor)
        m_pEventSensor->OnDisConnect(pServer, pClient);
}

void CNamedPipeClient::OnCreate(IIPCObject* pServer)
{
    if(NULL != m_pEventSensor)
        m_pEventSensor->OnCreate(pServer);
}

void CNamedPipeClient::OnClose(IIPCObject* pServer)
{
    if(NULL != m_pEventSensor)
        m_pEventSensor->OnClose(pServer);
}

void CNamedPipeClient::OnRecv(IIPCObject* pServer, IIPCConnector* pClient, LPCVOID lpBuf, DWORD dwBufSize)
{
    if(NULL == lpBuf || 0 == dwBufSize)
        return;

    if(NULL != m_pEventSensor)
        m_pEventSensor->OnRecv(pServer, pClient, lpBuf, dwBufSize);
}

void CNamedPipeClient::OnSend(IIPCObject* pServer, IIPCConnector* pClient, LPVOID lpBuf, DWORD dwBufSize)
{
    if(NULL != m_pEventSensor)
        m_pEventSensor->OnSend(pServer, pClient, lpBuf, dwBufSize);
}

HANDLE CNamedPipeClient::GetHandle()
{
    return m_hPipe;
}

BOOL CNamedPipeClient::SendMessage(LPCVOID lpBuf, DWORD dwBufSize)
{
    LPIPC_DATA_PACKAGE sendPackage = GenericMessage((LPVOID)lpBuf,dwBufSize,IPC_MESSAGE_WRIE);

    DWORD dwWrited = 0;
    BOOL bSucess = WriteFile(m_hPipe, sendPackage, sendPackage->dwTotalSize, &dwWrited, sendPackage);
    return ((bSucess == TRUE) || (GetLastError() == ERROR_IO_PENDING));
}

BOOL CNamedPipeClient::PostMessage(LPCVOID lpBuf, DWORD dwBufSize)
{
    return SendMessage(lpBuf, dwBufSize);
}

IIPCConnectorIterator* CNamedPipeClient::GetClients()
{
    return this;
}

void CNamedPipeClient::Begin()
{
    m_iIterator = 0;
}

BOOL CNamedPipeClient::End()
{
    return (1 == m_iIterator);
}

void CNamedPipeClient::Next()
{
    m_iIterator++;
}

IIPCConnector* CNamedPipeClient::GetCurrent()
{
    return this;
}

BOOL CNamedPipeClient::RequestAndReply(LPVOID lpSendBuf, DWORD dwSendBufSize, LPVOID lpReplyBuf, DWORD dwReplyBufSize, LPDWORD dwTransactSize)
{
//     NAMED_PIPE_MESSAGE requestMessage;
//     GenericMessage(&requestMessage, lpSendBuf, dwSendBufSize);
//     NAMED_PIPE_MESSAGE replyMessage;
//     replyMessage.dwTotalSize = sizeof(NAMED_PIPE_MESSAGE);
//
//     OVERLAPPED ov = {0};
//     ov.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
//     BOOL bSucess = TransactNamedPipe(m_hPipe, &requestMessage, requestMessage.dwTotalSize, &replyMessage, replyMessage.dwTotalSize, dwTransactSize, &ov);
//
//     if(!bSucess)
//     {
//         if(GetLastError() == ERROR_IO_PENDING)
//         {
//             if(GetOverlappedResult(m_hPipe, &ov, dwTransactSize, TRUE))
//                 bSucess = TRUE;
//         }
//     }
//
//     if(bSucess)
//     {
//         memcpy_s(lpReplyBuf, dwReplyBufSize, replyMessage.szRequest, replyMessage.dwRequestLen);
//         *dwTransactSize = requestMessage.dwRequestLen;
//     }
//
//     CloseHandle(ov.hEvent);
//     return bSucess;

    return FALSE;
}

DWORD CNamedPipeClient::GetSID()
{
    return GetCurrentProcessId();
}

LPCTSTR CNamedPipeClient::GetName()
{
    return m_sName;
}

BOOL CNamedPipeClient::_CreateIOCPThreadPool(DWORD dwThreadNum)
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

DWORD __stdcall CNamedPipeClient::_IOCPThreadProc(LPVOID lpParam)
{
    CNamedPipeClient* pThis = (CNamedPipeClient*)lpParam;

    if(NULL == pThis)
        return -1;

    return pThis->_IOCPThread();
}

DWORD CNamedPipeClient::_IOCPThread()
{
    CNamedPipeClient* pConnector = NULL;
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

        IPC_MESSAGE_TYPE messageType=((LPIPC_DATA_PACKAGE)lpo)->emMessageType;

        CloseHandle(lpo->hEvent);
		delete lpo;
		lpo=NULL;

        BOOL b = FALSE;

        switch(messageType)
        {
            case IPC_MESSAGE_CLIENTCONNECT:
            {

                LPIPC_DATA_PACKAGE package = GenericMessage(NULL,0,IPC_MESSAGE_READ);

                if(NULL == package)
                    break;

                b = ReadFile(pConnector->GetHandle(), &pConnector->m_recvPackage, sizeof(IPC_DATA_PACKAGE), NULL, package);

                if(!b)
                {
                    if(GetLastError() != ERROR_IO_PENDING)
                        break;
                }

                OnConnect(this, pConnector);

                break;
            }

            case IPC_MESSAGE_CLIENTDISCONNECT:
            {
                CloseConnection(pConnector);
                break;
            }

            case IPC_MESSAGE_READ:
            {
                OnRecv(this, pConnector, pConnector->m_recvPackage.lpData, pConnector->m_recvPackage.dwDataSize);

                LPIPC_DATA_PACKAGE package = GenericMessage(NULL,0,IPC_MESSAGE_READ);

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

            case IPC_MESSAGE_WRIE:
            {
                LPIPC_DATA_PACKAGE package = GenericMessage(NULL,0,IPC_MESSAGE_READ);

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

BOOL CNamedPipeClient::CloseConnection(CNamedPipeClient* pConnector)
{
    FlushFileBuffers(m_hPipe);
    DisconnectNamedPipe(m_hPipe);

    if(NULL != pConnector)
        OnDisConnect(this, pConnector);

    CloseHandle(m_hPipe);
    m_hPipe = INVALID_HANDLE_VALUE;

    return TRUE;
}

LPIPC_DATA_PACKAGE CNamedPipeClient::GenericMessage(LPVOID lpBuf, DWORD dwBufSize, IPC_MESSAGE_TYPE messageType)
{
    LPIPC_DATA_PACKAGE package = new IPC_DATA_PACKAGE;
    ZeroMemory(package, sizeof(OVERLAPPED));
    package->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    package->emMessageType = messageType;

    package->dwProcessID = GetCurrentProcessId();
    GetSystemTimeAsFileTime(&package->ftOccurance);
    package->dwDataSize = dwBufSize;
	if (NULL!=lpBuf)
		memcpy_s(package->lpData, SYELOG_MAXIMUM_MESSAGE, lpBuf, dwBufSize);
    package->dwTotalSize = sizeof(IPC_DATA_PACKAGE) - SYELOG_MAXIMUM_MESSAGE  + dwBufSize;
    return package;
}
