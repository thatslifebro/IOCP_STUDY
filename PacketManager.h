#pragma once

#include "ConcurrentQueue.h"
#include <iostream>
#include <functional>

class PacketManager
{
	// IO������ ��Ŷ�� �޾Ƽ� ť�� �ִ´�.
	ConcurrentQueue _queue;  // char�� �������� clientinfo �޾ƾ��ҵ�.
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

					// ��Ŷ �ڵ鷯 ȣ���ϱ�. ������ �� SendEchoData ȣ��.
					SendEchoData(packet);
					// ���⼭ �ڵ鷯 ȣ�� �ϴµ�, 
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

		// ������ ���� ���
		_packetProccessThread.join();
	}

	void OneSend(stClientInfo* clientInfo, char* data, size_t dataSize)
	{
		// ť�� �ֱ�. POP�� Send�� ���� �Ϸ��ٰ� ������.
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