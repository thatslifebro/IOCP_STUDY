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

#define MAX_WORKER_THREAD 4

class IOCPServer
{
	HANDLE _IOCPHandle = INVALID_HANDLE_VALUE;
	SOCKET _listenSocket = INVALID_SOCKET;
	std::vector<stClientInfo> _clientInfos;

	std::thread _acceptThread; 
	std::vector<std::thread> _workerThreads;

	bool _isRunning = false;

	virtual void RecvCompletionHandler(stClientInfo* clientInfo, DWORD byteTransfered) = 0;
	virtual void DoWSASend(stClientInfo* clientInfo, char* data, size_t dataSize) = 0;

public:
	IOCPServer()
	{
	}

	virtual ~IOCPServer() // 여길 가상 소멸자로 만들지 않으면 파생 클래스를 포인터로 생성하고 delete할 때 호출이 안됨.주의.
	{
		WSACleanup();
	}

	void InitServer()
	{
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);

		_IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

		if (_IOCPHandle == NULL)
		{
			std::cout << "CreateIoCompletionPort Error : " << WSAGetLastError() << std::endl;
			return;
		}

		_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

		if (_listenSocket == INVALID_SOCKET)
		{
			std::cout << "WSASocket Error : " << WSAGetLastError() << std::endl;
			return;
		}

		_isRunning = true;
	}

	void BindAndListen(UINT16 serverPort)
	{
		SOCKADDR_IN serverAddr;
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(serverPort);
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(_listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		{
			std::cout << "bind Error : " << WSAGetLastError() << std::endl;
			return;
		}

		if (listen(_listenSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			std::cout << "listen Error : " << WSAGetLastError() << std::endl;
			return;
		}
	}

	void StartNetworkServer(UINT16 maxClientCount)
	{
		// 클라이언트 정보 만들어놓기
		for (int i = 0; i < maxClientCount; ++i)
		{
			_clientInfos.emplace_back(stClientInfo());
		}

		// Worker Thread 만들기
		StartWorkThread();

		// Accept
		StartAccept();
	}

	void StartWorkThread()
	{
		for (int i = 0; i < MAX_WORKER_THREAD; ++i)
		{
			_workerThreads.emplace_back(std::thread([this] {
				WorkerThread();
			}));
		}
	}

	void StartAccept()
	{
		//_listenSocket IOCP 등록
		CreateIoCompletionPort((HANDLE)_listenSocket, _IOCPHandle, 0, 0);

		// 전체다 accept 걸어놓기
		for (int i = 0; i < _clientInfos.size(); i++)
		{
			_clientInfos[i];

			if (_clientInfos[i]._clientSocket == INVALID_SOCKET)
			{
				_clientInfos[i]._acceptOverlapped._ioType = IO_ACCEPT;
				_clientInfos[i]._acceptOverlapped._sessionIndex = i;
				_clientInfos[i]._sessionIndex = i;

				_clientInfos[i]._clientSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
				AcceptEx(_listenSocket, _clientInfos[i]._clientSocket, _clientInfos[i]._acceptBuffer, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, &_clientInfos[i]._acceptOverlapped._overlapped);
			}
		}

	}

	void WorkerThread()
	{
		DWORD byteTransfered = 0;
		stClientInfo* clientInfo = NULL;
		stOverlappedEx* overlappedEx;

		while (_isRunning)
		{//wsasend, wsarecv 
			bool result = GetQueuedCompletionStatus(_IOCPHandle, &byteTransfered, (PULONG_PTR)&clientInfo, (LPOVERLAPPED*)&overlappedEx, INFINITE);

			if (result == false)
			{
				auto error = WSAGetLastError();

				std::cout << "GetQueuedCompletionStatus Error : " << error << std::endl; // TODO : 이값이 64 면 비정상적 종료 인것 같다. 이 클라에대한 종료 처리를 해줘야할듯.
				continue;
			}

			if (overlappedEx->_ioType == IO_RECV)
			{
				RecvCompletionHandler(clientInfo, byteTransfered);
			}
			else if (overlappedEx->_ioType == IO_SEND)
			{
				SendCompletionHandler(clientInfo, byteTransfered);
			}
			else if (overlappedEx->_ioType == IO_ACCEPT)
			{
				AcceptCompletionHandler(*overlappedEx);
			}
		}
	}

	void AcceptCompletionHandler(stOverlappedEx overlappedEx)
	{
		auto clientInfo = &_clientInfos[overlappedEx._sessionIndex];

		CreateIoCompletionPort((HANDLE)clientInfo->_clientSocket, _IOCPHandle, (ULONG_PTR)clientInfo, 0);

		// Recv 걸어놓기
		RecvData(clientInfo);
	}

	void SendCompletionHandler(stClientInfo* clientInfo, DWORD byteTransfered)
	{
		// buffer 초기화
		if (strlen(clientInfo->_sendBuffer) == byteTransfered)
		{
			ZeroMemory(clientInfo->_sendBuffer, BUF_SIZE);

			if (!clientInfo->_sendQueue->empty())
			{
				clientInfo->PopSendQueue();
			}
		}
		else
		{
			auto error = WSAGetLastError();
			std::cout << "GetQueuedCompletionStatus Error : " << error << std::endl;
			return;
		}

		if (clientInfo->_sendQueue->empty())
		{
			// error
			std::cout << "Error _sendQueue is empty."<< std::endl;
			return;
		}

		if (!clientInfo->_sendQueue->empty())
		{
			auto data = clientInfo->_sendQueue->front();
			DoWSASend(clientInfo, data.get(), strlen(data.get()));
		}
	}

	void RecvData(stClientInfo* clientInfo)
	{
		//OverlappedEx에 버퍼정보 전달.
		WSABUF wsaBuf;
		wsaBuf.buf = clientInfo->_recvBuffer;
		wsaBuf.len = BUF_SIZE;

		clientInfo->_recvOverlapped._wsaBuf = wsaBuf;
		clientInfo->_recvOverlapped._ioType = IO_RECV;

		// 받기 예약
		DWORD recvFlag = 0;
		WSARecv(clientInfo->_clientSocket, &wsaBuf, 1, NULL, &recvFlag, &clientInfo->_recvOverlapped._overlapped, NULL);
	}

	void StopNetworkThreads()
	{
		_isRunning = false;

		_acceptThread.join();

		for (auto& workerThread : _workerThreads)
		{
			workerThread.join();
		}
	}
};