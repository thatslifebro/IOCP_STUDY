#pragma once

#include <WinSock2.h>

#define BUF_SIZE 1024

enum IOOperationType
{
	IO_ACCEPT = 0,
	IO_RECV = 1,
	IO_SEND = 2
};

struct stOverlappedEx
{
	WSAOVERLAPPED _overlapped; // Overlapped IO를 처리하기 위해 Device Driver에서 필요한 구조체.
	WSABUF _wsaBuf; // IO에 사용될 버퍼의 정보 담는 구조체. 길이와 버퍼의 주소를 가지고 있음.
	int _sessionIndex;
	IOOperationType _ioType;
};

struct stClientInfo
{
	SOCKET _clientSocket;
	stOverlappedEx _sendOverlapped;
	stOverlappedEx _recvOverlapped;
	stOverlappedEx _acceptOverlapped;

	int _sessionIndex;

	char _recvBuffer[BUF_SIZE];
	char _sendBuffer[BUF_SIZE];
	char _acceptBuffer[BUF_SIZE];

	std::queue<std::shared_ptr<char[]>>* _sendQueue;
	std::mutex* _mutex;

	stClientInfo() : _clientSocket(INVALID_SOCKET)
	{
		_sendQueue = new std::queue<std::shared_ptr<char[]>>();
		_mutex = new std::mutex();
		ZeroMemory(_recvBuffer, BUF_SIZE);
		ZeroMemory(_sendBuffer, BUF_SIZE);
		ZeroMemory(&_sendOverlapped, sizeof(stOverlappedEx)); // msdn : 이 구조체의 사용되지 않는 멤버는 함수 호출에서 구조체를 사용하려면 항상 0으로 초기화해야 합니다. 그렇지 않으면 함수가 실패하고 ERROR_INVALID_PARAMETER 반환할 수 있습니다.
		ZeroMemory(&_recvOverlapped, sizeof(stOverlappedEx));
		ZeroMemory(&_acceptOverlapped, sizeof(stOverlappedEx));
	}

	// 복사 생성자와 복사 대입 연산자는 삭제
	stClientInfo(const stClientInfo&) = delete;
	stClientInfo& operator=(const stClientInfo&) = delete;

	// 이동 생성자와 이동 대입 연산자는 허용
	stClientInfo(stClientInfo&&) = default;
	stClientInfo& operator=(stClientInfo&&) = default;

	void PopSendQueue()
	{
		std::lock_guard<std::mutex> block(*_mutex);
		if (!_sendQueue->empty())
		{
			auto sentData = _sendQueue->front();
			_sendQueue->pop();
		}
	}
};