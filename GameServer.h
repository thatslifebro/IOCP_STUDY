#pragma once

#include "IocpCore.h"
#include "PacketManager.h"

#define MAX_CLIENT_COUNT 100

class GameServer : public IocpCore3
{
private:
	PacketManager _packetManager;

public:

	GameServer() : _packetManager([this](stClientInfo* client, char* data, size_t length) {
		DoWSASend(client, data, length);
	})
	{
	}

	void StartServer(UINT16 port)
	{
		InitServer();
		BindAndListen(port);
		StartNetworkServer(MAX_CLIENT_COUNT);
		_packetManager.StartPacketProccessThread();
	}

	void RecvCompletionHandler(stClientInfo* clientInfo, DWORD byteTransfered)
	{
		clientInfo->_recvCursor += byteTransfered;

		if (clientInfo->_recvCursor < sizeof(unsigned short))
		{
			RecvData(clientInfo);
			return;
		}

		if (clientInfo->_packetSize == 0)
		{
			unsigned short value = *reinterpret_cast<unsigned short*>(clientInfo->_recvBuffer);

			clientInfo->_packetSize = value;
		}

		// ´Ù µé¾î¿È
		if (clientInfo->_recvCursor >= clientInfo->_packetSize)
		{
			_packetManager.Push(clientInfo, byteTransfered);
			clientInfo->_packetSize = 0;
			clientInfo->_recvCursor = 0;
		}
		
		RecvData(clientInfo);
	}

	void StopServer()
	{
		_packetManager.StopPacketProccessThread();
		StopNetworkThreads();
	}

	void DoWSASend(stClientInfo* clientInfo, char* data, size_t dataSize)
	{
		clientInfo->_sendCursor = dataSize;

		memcpy(clientInfo->_sendBuffer, data, dataSize);

		WSABUF wsaBuf;
		wsaBuf.buf = clientInfo->_sendBuffer;
		wsaBuf.len = (ULONG)dataSize;

		clientInfo->_sendOverlapped._wsaBuf = wsaBuf;
		clientInfo->_sendOverlapped._ioType = IO_SEND;

		DWORD sendFlag = 0;

		WSASend(clientInfo->_clientSocket, &wsaBuf, 1, NULL, sendFlag, &clientInfo->_sendOverlapped._overlapped, NULL);
	}
};