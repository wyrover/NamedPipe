// NamePipeClientDemo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "..\NamePipeServerDemo\Core\NamedPipeClient.h"
#include "..\NamePipeServerDemo\Core\IIPCInterface.h"
#include <conio.h>
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
         TCHAR* sSendMsg = _T("客户端连接到来啊啊啊\r\n");
//         pClient->PostMessage(sSendMsg, _tcslen(sSendMsg)*sizeof(TCHAR));

// 		static int x = 0;
// 		TCHAR aBuf[MAX_PATH] = {0};
// 		_stprintf_s(aBuf, _T("客户端 %d 发来数据 %d \r\n"), GetCurrentProcessId(), x++);
// 
// 		 if(!pClient->PostMessage(sSendMsg, _tcslen(sSendMsg)*sizeof(TCHAR)))
//  			return ;
    }

    virtual void OnDisConnect(IIPCObject* pServer, IIPCConnector* pClient)
    {

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
        _tprintf_s(_T("%s"), lpBuf);

//        TCHAR* sReply = _T("Hello,Server\r\n");

        static int x = 0;
        TCHAR aBuf[MAX_PATH] = {0};
        _stprintf_s(aBuf, _T("客户端 %d 发来数据 %d \r\n"), GetCurrentProcessId(), x++);
// 
//         if(!pClient->PostMessage(aBuf, _tcslen(aBuf)*sizeof(TCHAR)))
//             return ;
    }

    virtual void OnSend(IIPCObject* pServer, IIPCConnector* pClient, LPVOID lpBuf, DWORD dwBufSize)
    {

    }
};

DWORD __stdcall SendThread(LPVOID lpParam)
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

            TCHAR* sReply = _T("Hello,Server\r\n");
            aClient->SendMessage(sReply, _tcslen(sReply)*sizeof(TCHAR));
        }

        Sleep(10);
//        g_bExit = TRUE;
    }

    return 0;
}

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

            TCHAR* sRequest = _T("Hello,Server\r\n");
            DWORD dwRequestSize = _tcslen(sRequest) * sizeof(TCHAR);
            aClient->PostMessage(sRequest, dwRequestSize);

//             TCHAR sReply[MAX_PATH] = {0};
//             DWORD dwReplySize = 0;
//             if (aClient->RequestAndReply(sRequest, dwRequestSize, sReply, MAX_PATH, &dwReplySize))
//          {
//              int y=10;
//          }
//          int x=0;
        }

        Sleep(10);
//        g_bExit = TRUE;
    }

    return 0;
}

void TestRequestAndReply(IIPCObject* pNamedPipeClient)
{
    IIPCConnectorIterator* pClientIterator = pNamedPipeClient->GetClients();

//  while(FALSE == g_bExit)
    {
        for(pClientIterator->Begin(); !pClientIterator->End(); pClientIterator->Next())
        {
            IIPCConnector* aClient = pClientIterator->GetCurrent();

            if(NULL == aClient)
                continue;

            TCHAR* sRequest = _T("你好,服务端\r\n");
            DWORD dwRequestSize = _tcslen(sRequest) * sizeof(TCHAR);

            TCHAR sReply[MAX_PATH] = {0};
            DWORD dwReplySize = 0;

            if(aClient->RequestAndReply(sRequest, dwRequestSize, sReply, MAX_PATH, &dwReplySize))
            {
				_tsetlocale(LC_ALL, _T("chs"));
				_tprintf_s(_T("%s"), sReply);
            }

            int x = 0;
        }

//      g_bExit = TRUE;
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    IIPCEvent* pEvent = new CNamedPipeEvent;

    IIPCObject* pNamedPipeClient = new CNamedPipeClient(pEvent);

    if(NULL == pNamedPipeClient)
        return -1;

    if(!pNamedPipeClient->Create(_T("NamedPipeServer")))
        return -1;

//    TestRequestAndReply(pNamedPipeClient);

    HANDLE hThread = CreateThread(NULL, 0, SendThread, pNamedPipeClient, 0, NULL);

    _getch();

     g_bExit = TRUE;
     WaitForSingleObject(hThread, INFINITE);
     CloseHandle(hThread);

    pNamedPipeClient->Close();
    delete pNamedPipeClient;
    pNamedPipeClient = NULL;

    delete pEvent;
    pEvent = NULL;
    return 0;
}

