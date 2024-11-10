#pragma once

#include "ClientInfo.h"

class Packet
{
public:
	char* _data;
	size_t _dataSize;
	stClientInfo* _clientInfo;

	Packet(stClientInfo* clientInfo, char* data, size_t byteTransfered) : _clientInfo(clientInfo)
	{
		_dataSize = byteTransfered;
		_data = new char[strlen(data) + 1];
		// 복사하기
		memcpy(_data, data, strlen(data) + 1);
	}

	~Packet()
	{
		delete[] _data;
	}

	
};