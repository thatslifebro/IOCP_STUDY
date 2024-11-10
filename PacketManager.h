#pragma once

#include "ConcurrentQueue.h"
#include <iostream>

class PacketManager
{
	// IO������ ��Ŷ�� �޾Ƽ� ť�� �ִ´�.
	ConcurrentQueue _queue;  // char�� �������� clientinfo �޾ƾ��ҵ�.
	std::thread _packetProccessThread;


	bool _isRunning = false;

	// ��Ŷ �ڵ鷯 ��� �س���. ��� ���߿� �ϱ�.

public:
	// �����带 ��������.
	// �� ������� ť�� �ִ� �����͸� ������ ó���ؾ���.

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

			clientInfo->_sendQueue->push(sendData);

			// ���� �����Ͱ� ���ٸ� �ٷ� ������
			WSABUF wsaBuf;
			wsaBuf.buf = data;
			wsaBuf.len = (ULONG)dataSize;

			clientInfo->_sendOverlapped._wsaBuf = wsaBuf;
			clientInfo->_sendOverlapped._ioType = IO_SEND;

			// ������ ����
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