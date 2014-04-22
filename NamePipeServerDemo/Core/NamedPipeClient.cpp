#include "stdafx.h"
#include "NamedPipeClient.h"


CNamedPipeClient::CNamedPipeClient(IIPCEvent* pEvent): m_pEventSensor(pEvent)
    , m_hPipe(INVALID_HANDLE_VALUE)
    , m_hRecvThread(NULL)
{
    ZeroMemory(&m_DataPackage, sizeof(OVERLAPPED_PACKAGE));
    m_DataPackage.dwStatus = NAMED_PIPE_CONNECT;
    m_DataPackage.ovHeader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
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

    DWORD dwMode = PIPE_READMODE_MESSAGE ;
    BOOL fSuccess = SetNamedPipeHandleState(m_hPipe, &dwMode, NULL, NULL);

    if(!fSuccess)
        return FALSE;

    OnConnect(this, this);

    m_hRecvThread = CreateThread(NULL, 0, _RecvThreadProc, this, 0, NULL);

    if(NULL == m_hRecvThread)
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

    if(NULL != m_hRecvThread)
    {
        WaitForSingleObject(m_hRecvThread, INFINITE);
        CloseHandle(m_hRecvThread);
        m_hRecvThread = NULL;
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

DWORD __stdcall CNamedPipeClient::_RecvThreadProc(LPVOID lpParam)
{
    CNamedPipeClient* pThis = (CNamedPipeClient*)lpParam;

    if(NULL == pThis)
        return -1;

    return pThis->_RecvThread();
}

DWORD CNamedPipeClient::_RecvThread()
{
    DWORD dwReaded = 0;
    BOOL bSucess = FALSE;

    while(TRUE)
    {
        DWORD dwRet = WaitForSingleObject(m_DataPackage.ovHeader.hEvent, INFINITE);

        if(dwRet == WAIT_OBJECT_0)
        {
            if(m_DataPackage.dwStatus == NAMED_PIPE_WRIEING)
            {
                bSucess = WriteFile(m_hPipe, &m_DataPackage, sizeof(OVERLAPPED_PACKAGE), &dwReaded, &m_DataPackage.ovHeader);
                ResetEvent(m_DataPackage.ovHeader.hEvent);

                if(!bSucess && GetLastError() == ERROR_IO_PENDING)
                {

                }

                m_DataPackage.dwStatus = NAMED_PIPE_READING;
            }

            else if(m_DataPackage.dwStatus == NAMED_PIPE_READING)
            {
                bSucess = WriteFile(m_hPipe, &m_DataPackage, sizeof(OVERLAPPED_PACKAGE), &dwReaded, &m_DataPackage.ovHeader);
                ResetEvent(m_DataPackage.ovHeader.hEvent);

                if(!bSucess && GetLastError() == ERROR_IO_PENDING)
                {

                }

                m_DataPackage.dwStatus = NAMED_PIPE_WRIEING;
            }
        }
    }

    return 0;
}

HANDLE CNamedPipeClient::GetHandle()
{
    return m_hPipe;
}

BOOL CNamedPipeClient::SendMessage(LPCVOID lpBuf, DWORD dwBufSize)
{
    if(NULL == m_hPipe)
        return FALSE;

    if(NULL != m_pEventSensor)
        m_pEventSensor->OnSend(this, this, (LPVOID)lpBuf, dwBufSize);

    DWORD dwWrited = 0;
    NAMED_PIPE_MESSAGE message;
    GenericMessage(&message, lpBuf, dwBufSize);
    BOOL bSucess =::WriteFile(m_hPipe, &message, message.dwTotalSize , &dwWrited, NULL);

    return bSucess;
}

BOOL CNamedPipeClient::PostMessage(LPCVOID lpBuf, DWORD dwBufSize)
{
    m_DataPackage.bAsync = TRUE;
    m_DataPackage.bUsed = TRUE;
    m_DataPackage.dwProcessID = GetCurrentProcessId();
    m_DataPackage.dwReplySize = dwBufSize;
    memcpy_s(m_DataPackage.lpReply, SYELOG_MAXIMUM_MESSAGE, lpBuf, dwBufSize);
    GetSystemTimeAsFileTime(&m_DataPackage.ftOccurance);
    m_DataPackage.dwStatus = NAMED_PIPE_WRIEING;
    m_DataPackage.dwTotalSize = sizeof(OVERLAPPED_PACKAGE) - SYELOG_MAXIMUM_MESSAGE * 2 + dwBufSize;
    m_DataPackage.hCom = m_hPipe;
    SetEvent(m_DataPackage.ovHeader.hEvent);
    return TRUE;
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
    NAMED_PIPE_MESSAGE requestMessage;
    GenericMessage(&requestMessage, lpSendBuf, dwSendBufSize);
    NAMED_PIPE_MESSAGE replyMessage;
    replyMessage.dwTotalSize = sizeof(NAMED_PIPE_MESSAGE);

    OVERLAPPED ov = {0};
    ov.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    BOOL bSucess = TransactNamedPipe(m_hPipe, &requestMessage, requestMessage.dwTotalSize, &replyMessage, replyMessage.dwTotalSize, dwTransactSize, &ov);

    if(!bSucess)
    {
        if(GetLastError() == ERROR_IO_PENDING)
        {
            if(GetOverlappedResult(m_hPipe, &ov, dwTransactSize, TRUE))
                bSucess = TRUE;
        }
    }

    if(bSucess)
    {
        memcpy_s(lpReplyBuf, dwReplyBufSize, replyMessage.szRequest, replyMessage.dwRequestLen);
        *dwTransactSize = requestMessage.dwRequestLen;
    }

    CloseHandle(ov.hEvent);
    return bSucess;
}

BOOL CNamedPipeClient::GenericMessage(NAMED_PIPE_MESSAGE* pMessage, LPCVOID lpRequest, DWORD dwRequestLen)
{
    if(NULL == pMessage)
        return FALSE;

    ZeroMemory(pMessage, sizeof(NAMED_PIPE_MESSAGE));
    pMessage->nProcessId = GetSID();
    GetSystemTimeAsFileTime(&pMessage->ftOccurance);
    memcpy_s(pMessage->szRequest, SYELOG_MAXIMUM_MESSAGE, lpRequest, dwRequestLen);
    pMessage->dwRequestLen = dwRequestLen;
    pMessage->dwTotalSize = sizeof(NAMED_PIPE_MESSAGE) - SYELOG_MAXIMUM_MESSAGE * sizeof(TCHAR) + dwRequestLen;

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
