#pragma once
#include "IIPCInterface.h"
#include "..\NamedPipeWrapper.h"
#include "IoCompletePort.h"

class CNamedPipeServer: public IIPCObject,public IIPCConnectorIterator
{
public:
	CNamedPipeServer(IIPCEvent* pEvent);

	virtual ~CNamedPipeServer();

	virtual BOOL Create( LPCTSTR lpPipeName );

	virtual void Close();

	virtual IIPCConnectorIterator* GetClients();

	virtual void Begin();

	virtual BOOL End();

	virtual void Next();

	virtual IIPCConnector* GetCurrent();

protected:

	void CheckExit();

	static DWORD WINAPI IOCompletionThread(LPVOID lpParam);

	BOOL WaitClientConnect();

	DWORD GetCpuNum();

private:

	CIOCompletionPort m_iocp;

	ConnectorMap m_connectorMap;

	ConnectorMap::const_iterator m_citCurrent;
	
	TCHAR m_sPipeName[MAX_PATH];

	HANDLE* m_hThreadIOCP;

	IIPCEvent* m_pEvent;
};

class CNamedPipeConnector : public IIPCConnector
{
public:
	CNamedPipeConnector();

	virtual ~CNamedPipeConnector();

	BOOL Create(LPCTSTR lpPipeName);

	BOOL WaitConnect();

	void Close();

	BOOL DoRead();

	BOOL DoReadWrite();

	HANDLE GetHandle();

	virtual DWORD GetSID();

	virtual LPCTSTR GetName();

	virtual BOOL SendMessage( LPCVOID lpBuf, DWORD dwBufSize );

	virtual BOOL PostMessage( LPCVOID lpBuf, DWORD dwBufSize );

	virtual BOOL RequestAndReply( LPVOID lpSendBuf, DWORD dwSendBufSize, LPVOID lpReplyBuf, DWORD dwReplyBufSize, LPDWORD dwTransactSize );

	friend class CNamedPipeServer;

private:
	CNamedPipeWrapper m_pipe;

	IPC_DATA_OVERLAPPEDEX m_recvPackage;

	IPC_DATA_OVERLAPPEDEX m_sendPackage;

	IPC_DATA_OVERLAPPEDEX m_connPackage;

	BOOL m_bExit;
};