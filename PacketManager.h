#pragma once

#include "ConcurrentQueue.h"
#include <iostream>

class PacketManager
{
	// IO에서는 패킷을 받아서 큐에 넣는다.
	ConcurrentQueue _queue;  // char만 받지말고 clientinfo 받아야할듯.
	std::thread _packetProccessThread;


	bool _isRunning = false;

	// 패킷 핸들러 등록 해놓고. 등등 나중에 하기.

public:
	// 스레드를 만들어야함.
	// 그 스레드는 큐에 있는 데이터를 꺼내서 처리해야함.

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

			clientInfo->_sendQueue->push(sendData);

			// 보낼 데이터가 없다면 바로 보내기
			WSABUF wsaBuf;
			wsaBuf.buf = data;
			wsaBuf.len = (ULONG)dataSize;

			clientInfo->_sendOverlapped._wsaBuf = wsaBuf;
			clientInfo->_sendOverlapped._ioType = IO_SEND;

			// 보내기 예약
			WSASend(clientInfo->_clientSocket, &wsaBuf, 1, NULL, 0, &clientInfo->_sendOverlapped._overlapped, NULL);
		}
		else
		{
			auto sendData = std::make_shared<char[]>(dataSize + 1);
			memcpy(sendData.get(), data, dataSize + 1);

			clientInfo->_sendQueue->push(sendData);
		}
	}
};