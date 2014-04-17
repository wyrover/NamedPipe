#include "stdafx.h"
#include "NamedPipeClient.h"


CNamedPipeClient::CNamedPipeClient(IIPCEvent* pEvent): m_pEventSensor(pEvent)
    , m_hPipe(INVALID_HANDLE_VALUE)
    , m_hSendThread(NULL)
    , m_hRecvThread(NULL)
{

}


CNamedPipeClient::~CNamedPipeClient(void)
{

}

BOOL CNamedPipeClient::Create(LPCTSTR lpPipeName)
{
    ZeroMemory(&m_ovRead, sizeof(OVERLAPPED));
    HANDLE hReadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if(NULL == hReadEvent)
        return FALSE;

    m_ovRead.hEvent = hReadEvent;

    ZeroMemory(&m_ovWrite, sizeof(OVERLAPPED));
    HANDLE hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if(NULL == hWriteEvent)
        return FALSE;

    m_ovWrite.hEvent = hWriteEvent;

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

    if(NULL != m_ovRead.hEvent)
    {
        CloseHandle(m_ovRead.hEvent);
        m_ovRead.hEvent = NULL;
    }

    if(NULL != m_ovWrite.hEvent)
    {
        CloseHandle(m_ovWrite.hEvent);
        m_ovWrite.hEvent = NULL;
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
    TCHAR sBuf[SYELOG_MAXIMUM_MESSAGE] = {0};

    while(TRUE)
    {
        if(!ReadFile(m_hPipe, sBuf, SYELOG_MAXIMUM_MESSAGE, &dwReaded, &m_ovRead))
        {
            if(GetLastError() == ERROR_IO_PENDING)
            {
                if(!GetOverlappedResult(m_hPipe, &m_ovRead, &dwReaded, TRUE))
                    break;
                else
                {
                    OnRecv(this, this, sBuf, dwReaded);
                    continue;
                }
            }
            else if(GetLastError() == ERROR_BROKEN_PIPE)
                break;
        }
        else
            OnRecv(this, this, sBuf, dwReaded);
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
    BOOL bSucess =::WriteFile(m_hPipe, lpBuf, dwBufSize, &dwWrited, NULL);

    return bSucess;
}

BOOL CNamedPipeClient::PostMessage(LPCVOID lpBuf, DWORD dwBufSize)
{
    if(NULL == m_hPipe)
        return FALSE;

    if(NULL != m_pEventSensor)
        m_pEventSensor->OnSend(this, this, (LPVOID)lpBuf, dwBufSize);

    DWORD dwWrited = 0;
    BOOL bSucess =::WriteFile(m_hPipe, lpBuf, dwBufSize, &dwWrited, &m_ovWrite);

    return ((bSucess) || (GetLastError() == ERROR_IO_PENDING));
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
