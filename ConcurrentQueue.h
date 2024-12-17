#pragma once
#include <queue>
#include <memory>
#include <mutex>

#include "Packet.h"

//TODO : 구현된 concurrentqueue가져다쓰기
class ConcurrentQueue
{
	std::queue<std::shared_ptr<Packet>> _queue;
	std::mutex _mutex;

public:
	void Push(std::shared_ptr<Packet> packet)
	{
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