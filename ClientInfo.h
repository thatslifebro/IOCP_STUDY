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
	WSAOVERLAPPED _overlapped; // Overlapped IO�� ó���ϱ� ���� Device Driver���� �ʿ��� ����ü.
	WSABUF _wsaBuf; // IO�� ���� ������ ���� ��� ����ü. ���̿� ������ �ּҸ� ������ ����.
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
		ZeroMemory(&_sendOverlapped, sizeof(stOverlappedEx)); // msdn : �� ����ü�� ������ �ʴ� ����� �Լ� ȣ�⿡�� ����ü�� ����Ϸ��� �׻� 0���� �ʱ�ȭ�ؾ� �մϴ�. �׷��� ������ �Լ��� �����ϰ� ERROR_INVALID_PARAMETER ��ȯ�� �� �ֽ��ϴ�.
		ZeroMemory(&_recvOverlapped, sizeof(stOverlappedEx));
		ZeroMemory(&_acceptOverlapped, sizeof(stOverlappedEx));
	}

	// ���� �����ڿ� ���� ���� �����ڴ� ����
	stClientInfo(const stClientInfo&) = delete;
	stClientInfo& operator=(const stClientInfo&) = delete;

	// �̵� �����ڿ� �̵� ���� �����ڴ� ���
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