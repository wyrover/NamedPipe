#include "stdafx.h"
#include "NamedPipeClient.h"


CNamedPipeClient::CNamedPipeClient(IIPCEvent* pEvent): m_pEventSensor(pEvent)
    , m_hPipe(INVALID_HANDLE_VALUE)
{
    ZeroMemory(&m_sendPackage, sizeof(IPC_DATA_OVERLAPPEDEX));
    ZeroMemory(&m_recvPackage, sizeof(IPC_DATA_OVERLAPPEDEX));
    m_recvPackage.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_sendPackage.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}


CNamedPipeClient::~CNamedPipeClient(void)
{
    Close();

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
    return PostMessage(lpBuf, dwBufSize);
}

BOOL CNamedPipeClient::PostMessage(LPCVOID lpBuf, DWORD dwBufSize)
{
    m_sendPackage.emMessageType = IPC_MESSAGE_WRIE;
    m_sendPackage.ipcDataPackage.dwProcessID = GetCurrentProcessId();
    m_sendPackage.ipcDataPackage.dwDataSize = dwBufSize;
    memcpy_s(&m_sendPackage.ipcDataPackage.lpData, SYELOG_MAXIMUM_MESSAGE, lpBuf, dwBufSize);
    m_sendPackage.ipcDataPackage.dwTotalSize = sizeof(IPC_DATA_OVERLAPPEDEX) - SYELOG_MAXIMUM_MESSAGE + dwBufSize;
    DWORD dwWrited = 0;
    BOOL bSucess = WriteFile(m_hPipe, &m_sendPackage.ipcDataPackage, m_sendPackage.ipcDataPackage.dwTotalSize, &dwWrited, &m_sendPackage);
    return ((bSucess == TRUE) || (GetLastError() == ERROR_IO_PENDING));
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
    m_sendPackage.emMessageType = IPC_MESSAGE_READ_WRITE;
    m_sendPackage.ipcDataPackage.dwProcessID = GetCurrentProcessId();
    m_sendPackage.ipcDataPackage.dwDataSize = dwSendBufSize;
//    memcpy_s(m_sendPackage.ipcDataPackage.lpData, SYELOG_MAXIMUM_MESSAGE, lpSendBuf, dwSendBufSize);
    m_sendPackage.ipcDataPackage.dwTotalSize = sizeof(IPC_DATA_PACKAGE);
	m_sendPackage.ipcDataPackage.msgType=IPC_MESSAGE_READ_WRITE;
    m_recvPackage.emMessageType = IPC_MESSAGE_READ_WRITE;
    DWORD dwWrited = 0;
    BOOL bSucess = TransactNamedPipe(m_hPipe, &m_sendPackage.ipcDataPackage, m_sendPackage.ipcDataPackage.dwTotalSize, &m_recvPackage.ipcDataPackage, sizeof(IPC_DATA_PACKAGE), &dwWrited, &m_sendPackage);

    if(GetLastError() == ERROR_IO_PENDING)
    {
        if(GetOverlappedResult(m_hPipe, &m_sendPackage, dwTransactSize, TRUE))
            bSucess = TRUE;
    }

    if(bSucess)
    {
        memcpy_s(lpReplyBuf, dwReplyBufSize, m_recvPackage.ipcDataPackage.lpData, m_recvPackage.ipcDataPackage.dwDataSize);
        *dwTransactSize = m_recvPackage.ipcDataPackage.dwDataSize;
    }

    return TRUE;
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
            if(GetLastError() == ERROR_BROKEN_PIPE)
            {
                if(NULL == lpo)
                {
                    CloseConnection(pConnector);
                    continue;
                }
            }
        }

        IPC_MESSAGE_TYPE messageType = lpo->emMessageType;

        BOOL b = FALSE;

        switch(messageType)
        {
            case IPC_MESSAGE_CLIENTCONNECT:
            {
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
                OnRecv(this, pConnector, pConnector->m_recvPackage.ipcDataPackage.lpData, pConnector->m_recvPackage.ipcDataPackage.dwDataSize);
                break;
            }

            case IPC_MESSAGE_WRIE:
            {
                PostReadRequestToIOCP();
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

BOOL CNamedPipeClient::PostReadRequestToIOCP()
{
    m_recvPackage.emMessageType = IPC_MESSAGE_READ;
    BOOL bSucess = ReadFile(m_hPipe, &m_recvPackage.ipcDataPackage , sizeof(IPC_DATA_PACKAGE), NULL, &m_recvPackage);

    if(!bSucess && GetLastError() == ERROR_BROKEN_PIPE)
        return FALSE;

    return (bSucess || (GetLastError() == ERROR_IO_PENDING));
}
