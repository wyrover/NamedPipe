// NamePipeServerDemo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>
#include <stddef.h>
#include <conio.h>
#include "Core\NamedPipeServer.h"
#include <locale.h>

BOOL g_bExit = FALSE;
class CNamedPipeEvent : public IIPCEvent
{
public:
    CNamedPipeEvent()
    {

    }

    virtual ~CNamedPipeEvent()
    {

    }

    virtual void OnConnect(IIPCObject* pServer, IIPCConnector* pClient)
    {
        _tsetlocale(LC_ALL, _T("chs"));
        _tprintf_s(_T("客户端连接到来\r\n"));
    }

    virtual void OnDisConnect(IIPCObject* pServer, IIPCConnector* pClient)
    {
        _tsetlocale(LC_ALL, _T("chs"));
        _tprintf_s(_T("客户端断开连接\r\n"));
    }

    virtual void OnCreate(IIPCObject* pServer)
    {

    }

    virtual void OnClose(IIPCObject* pServer)
    {

    }

    virtual void OnRecv(IIPCObject* pServer, IIPCConnector* pClient, LPCVOID lpBuf, DWORD dwBufSize)
    {
        _tsetlocale(LC_ALL, _T("chs"));

        BYTE sBuf[MAX_PATH] = {0};
        memcpy_s(sBuf, MAX_PATH, lpBuf, MAX_PATH);
        _tprintf_s(_T("%s"), sBuf);

//		TCHAR* sReply = _T("Hello,Client\r\n");
        IIPCConnectorIterator* pClientIterator = pServer->GetClients();

        for(pClientIterator->Begin(); !pClientIterator->End(); pClientIterator->Next())
        {
            IIPCConnector* aClient = pClientIterator->GetCurrent();

            if(NULL == aClient)
                continue;

            if (!aClient->SendMessage(sBuf, _tcslen((TCHAR*)sBuf)*sizeof(TCHAR)))
				break;
        }
    }

    virtual void OnSend(IIPCObject* pServer, IIPCConnector* pClient, LPVOID lpBuf, DWORD dwBufSize)
    {

    }
};

DWORD __stdcall PostThread(LPVOID lpParam)
{
    IIPCObject* pServer = (IIPCObject*)lpParam;

    IIPCConnectorIterator* pClientIterator = pServer->GetClients();

    while(FALSE == g_bExit)
    {
        for(pClientIterator->Begin(); !pClientIterator->End(); pClientIterator->Next())
        {
            IIPCConnector* aClient = pClientIterator->GetCurrent();

            if(NULL == aClient)
                continue;

            TCHAR* sReply = _T("Hello,Client\r\n");
            aClient->SendMessage(sReply, _tcslen(sReply)*sizeof(TCHAR));
        }

        Sleep(10);
    }

    return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
    IIPCEvent* pEvent = new CNamedPipeEvent;
    IIPCObject* pNamedPipeServer = new CNamedPipeServer(pEvent);

    if(NULL == pNamedPipeServer)
        return -1;

    if(!pNamedPipeServer->Create(_T("NamedPipeServer")))
        return -1;

//    HANDLE hThread = CreateThread(NULL, 0, PostThread, pNamedPipeServer, 0, NULL);

    _getch();

//     g_bExit = TRUE;
//     WaitForSingleObject(hThread, INFINITE);
//     CloseHandle(hThread);

    pNamedPipeServer->Close();
    delete pNamedPipeServer;
    pNamedPipeServer = NULL;
    delete pEvent;
    pEvent = NULL;
    return 0;
}

