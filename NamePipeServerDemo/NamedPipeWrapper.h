#pragma once
#include <windows.h>

class CNamedPipeWrapper
{
public:
    CNamedPipeWrapper(void): m_hPipe(INVALID_HANDLE_VALUE)
    {

    }
    virtual ~CNamedPipeWrapper(void)
    {

    }

    void Close()
    {
        if(INVALID_HANDLE_VALUE != m_hPipe)
            CloseHandle(m_hPipe);

        m_hPipe = INVALID_HANDLE_VALUE;
    }

    HANDLE GetHandle()
    {
        return m_hPipe;
    }

    BOOL CreateNamedPipe(
        __in      LPCTSTR lpName,
        __in      DWORD dwOpenMode,
        __in      DWORD dwPipeMode,
        __in      DWORD nMaxInstances,
        __in      DWORD nOutBufferSize,
        __in      DWORD nInBufferSize,
        __in      DWORD nDefaultTimeOut,
        __in_opt  LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
    {
        if(INVALID_HANDLE_VALUE != m_hPipe)
            return TRUE;

        m_hPipe =::CreateNamedPipe(lpName, dwOpenMode, dwPipeMode, nMaxInstances, nOutBufferSize, nInBufferSize, nDefaultTimeOut, lpSecurityAttributes);
        return (INVALID_HANDLE_VALUE != m_hPipe);
    }

    BOOL ConnectNamedPipe(
        LPOVERLAPPED lpOverlapped
    )
    {
        return ::ConnectNamedPipe(m_hPipe, lpOverlapped);
    }

    BOOL ReadFile(
        LPVOID lpBuffer,
        DWORD nNumberOfBytesToRead,
        LPDWORD lpNumberOfBytesRead,
        LPOVERLAPPED lpOverlapped
    )
    {
        return ::ReadFile(m_hPipe, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
    }

    BOOL WriteFile(
        LPCVOID lpBuffer,
        DWORD nNumberOfBytesToWrite,
        LPDWORD lpNumberOfBytesWritten,
        LPOVERLAPPED lpOverlapped
    )
    {
        return ::WriteFile(m_hPipe, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
    }

    BOOL FlushFileBuffers(
    )
    {
        return ::FlushFileBuffers(m_hPipe);
    }

    BOOL DisconnectNamedPipe(
    )
    {
        return ::DisconnectNamedPipe(m_hPipe);
    }

    BOOL TransactNamedPipe(
        __in         LPVOID lpInBuffer,
        __in         DWORD nInBufferSize,
        __out        LPVOID lpOutBuffer,
        __in         DWORD nOutBufferSize,
        __out        LPDWORD lpBytesRead,
        __inout_opt  LPOVERLAPPED lpOverlapped
    )
    {
        return ::TransactNamedPipe(m_hPipe, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesRead, lpOverlapped);
    }

    BOOL WaitNamedPipe(
        __in  LPCTSTR lpNamedPipeName,
        __in  DWORD nTimeOut
    )
    {
        return ::WaitNamedPipe(lpNamedPipeName, nTimeOut);
    }

private:
    HANDLE m_hPipe;
};

