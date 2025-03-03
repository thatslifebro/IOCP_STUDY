#pragma once
#pragma comment(lib, "ws2_32")
#pragma comment(lib,"mswsock.lib")

#include <iostream>
#include <vector>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <cstring>


#include "ConcurrentQueue.h"
#include "IocpEvent.h"
#include "IocpObject.h"


#define MAX_WORKER_THREAD 4

class IocpCore : public std::enable_shared_from_this<IocpCore>
{
public:
	IocpCore();

	~IocpCore();

	void Register(IocpObjectRef iocpObject);

	bool Dispatch();

private:
	HANDLE _iocpHandle;
	std::vector<stClientInfo> _clientInfos;

	std::thread _acceptThread;
	std::vector<std::thread> _workerThreads;

	bool _isRunning = false;

//public:
	//void AcceptCompletionHandler(stOverlappedEx overlappedEx)
	//{
	//	auto clientInfo = &_clientInfos[overlappedEx._sessionIndex];

	//	CreateIoCompletionPort((HANDLE)clientInfo->_clientSocket, _iocpHandle, (ULONG_PTR)clientInfo, 0);

	//	std::cout << "Accept Success" << std::endl;

	//	// Recv 걸어놓기
	//	RecvData(clientInfo);
	//}

	//void SendCompletionHandler(stClientInfo* clientInfo, DWORD byteTransfered)
	//{
	//	// buffer 초기화
	//	// 다 안보내졌으면 기다리기
	//	if (clientInfo->_sendCursor == byteTransfered)
	//	{
	//		ZeroMemory(clientInfo->_sendBuffer, BUF_SIZE);

	//		if (!clientInfo->_sendQueue->empty())
	//		{
	//			clientInfo->PopSendQueue();
	//		}
	//	}
	//	else if (clientInfo->_sendCursor < byteTransfered)
	//	{
	//		// 다 안보내졌으면 기다리기
	//		clientInfo->_sendCursor -= byteTransfered;
	//	}
	//	else
	//	{
	//		auto error = WSAGetLastError();
	//		std::cout << "GetQueuedCompletionStatus Error : " << error << std::endl;
	//		return;
	//	}

	//	if (!clientInfo->_sendQueue->empty())
	//	{
	//		auto data = clientInfo->_sendQueue->front();
	//		DoWSASend(clientInfo, data.get(), strlen(data.get()));
	//	}
	//}

	//void RecvData(stClientInfo* clientInfo)
	//{
	//	//OverlappedEx에 버퍼정보 전달.
	//	WSABUF wsaBuf;
	//	wsaBuf.buf = clientInfo->RecvBuffPos();
	//	wsaBuf.len = BUF_SIZE - clientInfo->_recvCursor;

	//	clientInfo->_recvOverlapped._wsaBuf = wsaBuf;
	//	clientInfo->_recvOverlapped._ioType = IO_RECV;

	//	// 받기 예약
	//	DWORD recvFlag = 0;
	//	WSARecv(clientInfo->_clientSocket, &wsaBuf, 1, NULL, &recvFlag, &clientInfo->_recvOverlapped._overlapped, NULL);
	//}

	//void StopNetworkThreads()
	//{
	//	_isRunning = false;

	//	_acceptThread.join();

	//	for (auto& workerThread : _workerThreads)
	//	{
	//		workerThread.join();
	//	}
	//}
};