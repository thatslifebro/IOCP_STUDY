#pragma once

#include "IOCPServer.h"
#include "PacketManager.h"

#define MAX_CLIENT_COUNT 100

class GameServer : public IOCPServer
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
		_packetManager.Push(clientInfo, byteTransfered);

		RecvData(clientInfo);
	}

	void StopServer()
	{
		_packetManager.StopPacketProccessThread();
		StopNetworkThreads();
	}

	void DoWSASend(stClientInfo* clientInfo, char* data, size_t dataSize)
	{
		WSABUF wsaBuf;
		wsaBuf.buf = data;
		wsaBuf.len = (ULONG)dataSize;

		clientInfo->_sendOverlapped._wsaBuf = wsaBuf;

		WSASend(clientInfo->_clientSocket, &wsaBuf, 1, NULL, 0, &clientInfo->_sendOverlapped._overlapped, NULL);
	}
};