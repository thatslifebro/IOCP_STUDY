#pragma once

#include "ConcurrentQueue.h"
#include <iostream>
#include <functional>
#include <unordered_map>
#include "LoginInfo.h"
#include "LoginReq.h"
#include "ErrorCode.h"

class PacketManager
{
	ConcurrentQueue _queue; 
	std::thread _packetProccessThread;

	bool _isRunning = false;

	std::function<void(stClientInfo*, char*, size_t)> DoWSASend;

	std::unordered_map<PACKET_ID, std::function<void(std::shared_ptr<Packet>)>> PacketHandlerMap;

public:

	PacketManager(std::function<void(stClientInfo*, char*, size_t)> doWSASend) : DoWSASend(doWSASend)
	{
		// ��Ŷ �ڵ鷯 ���
		PacketHandlerMap[PACKET_ID::DEV_ECHO] = [this](std::shared_ptr<Packet> packet) { SendEchoData(packet); };
		PacketHandlerMap[PACKET_ID::LOGIN_REQ] = [this](std::shared_ptr<Packet> packet) { Login(packet); };
		PacketHandlerMap[PACKET_ID::ROOM_CHAT_REQ] = [this](std::shared_ptr<Packet> packet) { Chat(packet); };
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
					auto str = std::string(packet->_data, packet->_packetSize);
					
					if (PacketHandlerMap.find(static_cast<PACKET_ID>(packet->_packetID)) == PacketHandlerMap.end())
					{
						std::cout << "Unknown Packet ID : " << packet->_packetID << std::endl;
						continue;
					}

					PacketHandlerMap[static_cast<PACKET_ID>(packet->_packetID)](packet);
				}
			}
		});
	}

	void Push(stClientInfo* clientInfo, int byteTransfered)
	{
		//TODO : ����ȭ �ʿ�
		auto packet = std::make_shared<Packet>(clientInfo, clientInfo->_recvBuffer, clientInfo->_packetSize);

		auto temp = new char[BUF_SIZE];

		memcpy(temp, clientInfo->_recvBuffer+clientInfo->_packetSize, BUF_SIZE - clientInfo->_packetSize);

		ZeroMemory(clientInfo->_recvBuffer, BUF_SIZE);

		memcpy(clientInfo->_recvBuffer, temp, BUF_SIZE - clientInfo->_packetSize);

		delete[] temp;

		_queue.Push(packet);
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
			auto sendData = std::make_shared<char[]>(dataSize);

			memcpy(sendData.get(), data, dataSize);

			clientInfo->PushSendQueue(sendData);
			
			DoWSASend(clientInfo, data, dataSize);
		}
		else
		{
			auto sendData = std::make_shared<char[]>(dataSize);
			memcpy(sendData.get(), data, dataSize);

			clientInfo->PushSendQueue(sendData);
		}
	}

	void SendEchoData(std::shared_ptr<Packet> packet)
	{
		OneSend(packet->_clientInfo, packet->_data, packet->_packetSize);
	}

	void Login(std::shared_ptr<Packet> packet)
	{
		LoginReq loginPacket(packet->_clientInfo, packet->_data, packet->_packetSize);

		auto id = loginPacket.GetID();
		auto pw = loginPacket.GetPW();

		// RES ��Ŷ ���� OneSend�� �����ǵ� �̰� �������� Ȯ�εǾ� �����ؾ���. ��� �����ұ�. ���. clientInfo�� ��Ŷ�� �ִ°� ���ƺ���.

		if (id == "" or pw == "")
		{
			// error ó��
			packet->_clientInfo->_responseData.Result = (unsigned short)ErrorCode::LoginPacketWrongSize;
			return;
		}

		if (LoginInfoDatabase[id] == pw)
		{
			// �α��� ó��
			
			// TODO : ���� �Ŵ��� ���� �����ϱ�? ��.. ���߿� ���� �����ѻ�� ������ �س���, �ű⿡ id ���� �߰��ϴ� ������ ó���غ���.�Ӥ�

			// �α��� response ��Ŷ ������.
			packet->_clientInfo->_responseData.Result = (unsigned short)ErrorCode::None;
		}
		else
		{
			// error ó��
			packet->_clientInfo->_responseData.Result = (unsigned short)ErrorCode::LoginPacketNotRegister;
			return;
		}

		OneSend(packet->_clientInfo, packet->_clientInfo->_responseData.Data, 2);
	}

	void Chat(std::shared_ptr<Packet> packet)
	{
		// ä�� ó��
	}
};