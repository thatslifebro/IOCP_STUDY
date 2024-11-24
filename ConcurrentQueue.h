#pragma once
#include <queue>
#include <memory>
#include <mutex>

#include "Packet.h"

// Push에 대해서만 lock 검. Pop은 하나의 쓰레드에서 한다는 가정.
class ConcurrentQueue
{
	std::queue<std::shared_ptr<Packet>> _queue;
	std::mutex _mutex;

public:
	void Push(stClientInfo* clientInfo, int byteTransfered)
	{
		// 복사하기
		auto packet = std::make_shared<Packet>(clientInfo, clientInfo->_recvBuffer, byteTransfered);

		// 복사후 버퍼 초기화
		ZeroMemory(clientInfo->_recvBuffer, BUF_SIZE);

		// 큐에 넣기
		std::lock_guard<std::mutex> lock(_mutex);
		_queue.push(packet);
	}

	std::shared_ptr<Packet> Pop()
	{
		if (_queue.empty())
		{
			return nullptr;
		}

		std::lock_guard<std::mutex> lock(_mutex);
		auto packet = _queue.front();
		_queue.pop();
		return packet;
	}

	bool Empty()
	{
		return _queue.empty();
	}

};