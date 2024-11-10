#pragma once

#include "IOCPServer.h"
#include "PacketManager.h"

#define MAX_CLIENT_COUNT 100

class GameServer : public IOCPServer
{
	PacketManager _packetManager;

public:
	void StartServer(UINT16 port)
	{
		InitServer();
		BindAndListen(port);
		StartNetworkServer(MAX_CLIENT_COUNT);
		_packetManager.StartPacketProccessThread();
	}

	void RecvCompletionHandler(stClientInfo* clientInfo, DWORD byteTransfered)
	{
		//clientInfo->_recvBuffer[byteTransfered] = NULL;
		//std::cout << clientInfo->_recvBuffer << std::endl;
		// Echo
		//SendEchoData(clientInfo, byteTransfered);
		_packetManager.Push(clientInfo, byteTransfered);

		RecvData(clientInfo);
	}

	void StopServer()
	{
		_packetManager.StopPacketProccessThread();
		StopNetworkThreads();
	}
};