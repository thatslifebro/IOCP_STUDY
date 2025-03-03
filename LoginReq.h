#pragma once
#include "Packet.h"
#include <string>

class LoginReq : Packet
{
public:
	using Packet::Packet;

	std::string GetID()
	{
		if (_packetSize < 4 + MAX_USER_ID_BYTE_LENGTH + MAX_USER_PW_BYTE_LENGTH)
		{
			// error 贸府 鞘夸
			return "";
		}

		std::string userID(_data + 4);

		if (userID.size() > MAX_USER_ID_BYTE_LENGTH)
		{
			return "";
		}

		return userID;
	}

	std::string GetPW()
	{
		if (_packetSize < 4 + MAX_USER_ID_BYTE_LENGTH + MAX_USER_PW_BYTE_LENGTH)
		{
			// error 贸府 鞘夸
			return "";
		}

		std::string userPW(_data + 4 + MAX_USER_ID_BYTE_LENGTH);
		if (userPW.size() > MAX_USER_PW_BYTE_LENGTH)
		{
			return "";
		}

		return userPW;
	}
};