#pragma once

#include <windows.h>
class CIOCompletionPort
{
private:
	HANDLE m_hIOCP;

public:

	CIOCompletionPort() : m_hIOCP(NULL)
	{
	}

	~CIOCompletionPort()
	{
		Close();
	}

	/*!
	* Create a new completion port that is not associated with any I/O 
	* devices, and set the maximum number of threads that the operating 
	* system can allow to concurrently process I/O completion packets for 
	* the I/O completion port.
	* 
	* \param dwNumberOfConcurrentThreads
	* The maximum number of threads that the operating system can allow to 
	* concurrently process I/O completion packets for the I/O completion 
	* port. If this parameter is zero, the system allows as many concurrently 
	* running threads as there are processors in the system. 
	*/	
	BOOL Create(DWORD dwNumberOfConcurrentThreads = 0)
	{
		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 
			dwNumberOfConcurrentThreads);
		return (m_hIOCP != NULL);
	}

	BOOL Close()
	{
		// Ensure that the handle of the I/O completion port is closed
		if (m_hIOCP != NULL && CloseHandle(m_hIOCP))
		{
			m_hIOCP = NULL;
			return TRUE;
		}
		return FALSE;
	}

	/*!
	* Associate a device with the port. The system appends this information 
	* to the completion port's device list. To have the results of 
	* asynchronous I/O requests queued to the completion port you need to 
	* associate the file handles with the completion port. 
	* 
	* When an asynchronous I/O request for a device completes, the system 
	* checks to see whether the device is associated with a completion port 
	* and, if it is, the system appends the completed I/O request entry to 
	* the end of the completion port's I/O completion queue. 
	* 
	* \param hDevice
	* An open device handle to be associated with the I/O completion port. 
	* The handle has to have been opened for overlapped I/O completion.
	* 
	* \param CompKey
	* A value that has meaning to you; the operating system does not care  
	* what you pass here. We may use this parameter to differentiate the 
	* different devices associated to the completion port. One completion key 
	* stands for a device handle. The value is included in the completion 
	* packet for any I/O requests for the given file handle.
	*/
	BOOL AssociateDevice(HANDLE hDevice, ULONG_PTR CompKey)
	{
		HANDLE h = CreateIoCompletionPort(hDevice, m_hIOCP, CompKey, 0);
		return h == m_hIOCP;
	}

	/*!
	* Queue your own completion packets. Although completion packets are 
	* normally queued by the operating system as asynchronous I/O requests 
	* are completed, you can also queue your own completion packets. This is 
	* achieved using the PostQueuedCompletionStatus function. 
	* 
	* \param CompKey
	* The value to be returned through the lpCompletionKey parameter of the 
	* GetQueuedCompletionStatus function.
	* 
	* \param dwNumBytes
	* The value to be returned through the lpNumberOfBytesTransferred 
	* parameter of the GetQueuedCompletionStatus function.
	* 
	* \param po
	* The value to be returned through the lpOverlapped parameter of the 
	* GetQueuedCompletionStatus function.
	*/
	BOOL QueuePacket(ULONG_PTR CompKey, DWORD dwNumBytes = 0, 
		OVERLAPPED* po = NULL)
	{
		return PostQueuedCompletionStatus(m_hIOCP, dwNumBytes, CompKey, po);
	}

	/*!
	* Attempts to dequeue an I/O completion packet from the specified I/O 
	* completion port. If there is no completion packet queued, the function 
	* waits for a pending I/O operation associated with the completion port 
	* to complete. You can dequeue completion packets on any thread in the 
	* process that created the completion port. All that the thread needs is 
	* the port handle.
	* 
	* \param pCompKey
	* A pointer to a variable that receives the completion key value 
	* associated with the file handle whose I/O operation has completed. A 
	* completion key is a per-file key that is specified in a call to 
	* AssociateDevice.
	* 
	* \param pdwNumBytes
	* A pointer to a variable that receives the number of bytes transferred 
	* during an I/O operation that has completed.
	* 
	* \param ppo
	* A pointer to a variable that receives the address of the OVERLAPPED 
	* structure that was specified when the completed I/O operation was 
	* started. 
	* 
	* \param dwMilliseconds
	* The number of milliseconds that the caller is willing to wait for a 
	* completion packet to appear at the completion port. If a completion 
	* packet does not appear within the specified time, the function times 
	* out, returns FALSE, and sets *lpOverlapped to NULL.
	*/
	BOOL DequeuePacket(ULONG_PTR* pCompKey, PDWORD pdwNumBytes, 
		OVERLAPPED** ppo, DWORD dwMilliseconds = INFINITE)
	{
		return GetQueuedCompletionStatus(m_hIOCP, pdwNumBytes, pCompKey, ppo, 
			dwMilliseconds);
	}
};