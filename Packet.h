#pragma once

#include "ClientInfo.h"
#include "PacketID.h"

class Packet
{
public:
	char* _data;

	unsigned short _packetSize;
	unsigned short _packetID;

	stClientInfo* _clientInfo;

	Packet(stClientInfo* clientInfo, char* data, unsigned short packetSize) : _clientInfo(clientInfo)
	{
		_packetSize = packetSize;
		_packetID = *reinterpret_cast<unsigned short*>(clientInfo->_recvBuffer + sizeof(unsigned short));

		_data = new char[packetSize];

		ZeroMemory(_data, packetSize);
		// 복사하기
		memcpy(_data, data, packetSize);
	}

	~Packet()
	{
		delete[] _data;
	}

};