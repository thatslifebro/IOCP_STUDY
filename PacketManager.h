#pragma once

#include "ConcurrentQueue.h"
#include <iostream>
#include <functional>

class PacketManager
{
	// IO에서는 패킷을 받아서 큐에 넣는다.
	ConcurrentQueue _queue;  // char만 받지말고 clientinfo 받아야할듯.
	std::thread _packetProccessThread;

	bool _isRunning = false;

	std::function<void(stClientInfo*, char*, size_t)> DoWSASend;

public:

	PacketManager(std::function<void(stClientInfo*, char*, size_t)> doWSASend) : DoWSASend(doWSASend)
	{
	}

	void StartPacketProccessThread()
	{
		_isRunning = true;

	    _packetProccessThread = std::thread([this]() {
			while (_isRunning)
			{
				if (!_queue.Empty())
				{
					auto packet = _queue.Pop();
					std::cout << packet->_data << std::endl;

					// 패킷 핸들러 호출하기. 지금은 걍 SendEchoData 호출.
					SendEchoData(packet);
					// 여기서 핸들러 호출 하는데, 
				}
			}
		});
	}

	void SendEchoData(std::shared_ptr<Packet> packet)
	{
		OneSend(packet->_clientInfo, packet->_data, packet->_dataSize);
	}

	void Push(stClientInfo* clientInfo, int byteTransfered)
	{
		_queue.Push(clientInfo, byteTransfered);
	}

	void StopPacketProccessThread()
	{
		_isRunning = false;

		// 스레드 종료 대기
		_packetProccessThread.join();
	}

	void OneSend(stClientInfo* clientInfo, char* data, size_t dataSize)
	{
		// 큐에 넣기. POP은 Send가 정말 완료됬다고 했을때.
		if (clientInfo->_sendQueue->empty())
		{
			auto sendData = std::make_shared<char[]>(dataSize + 1);
			memcpy(sendData.get(), data, dataSize + 1);

			clientInfo->PushSendQueue(sendData);

			DoWSASend(clientInfo, data, dataSize);
		}
		else
		{
			auto sendData = std::make_shared<char[]>(dataSize + 1);
			memcpy(sendData.get(), data, dataSize + 1);

			clientInfo->PushSendQueue(sendData);
		}
	}
};