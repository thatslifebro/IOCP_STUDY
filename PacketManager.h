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
		// 패킷 핸들러 등록
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
		//TODO : 최적화 필요
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

		// 스레드 종료 대기
		_packetProccessThread.join();
	}

	void OneSend(stClientInfo* clientInfo, char* data, size_t dataSize)
	{
		// 큐에 넣기. POP은 Send가 정말 완료됬다고 했을때.
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

		// RES 패킷 만들어서 OneSend로 보낼건데 이게 보내진게 확인되야 삭제해야함. 어디에 보관할까. 고민. clientInfo에 패킷을 넣는게 좋아보임.

		if (id == "" or pw == "")
		{
			// error 처리
			packet->_clientInfo->_responseData.Result = (unsigned short)ErrorCode::LoginPacketWrongSize;
			return;
		}

		if (LoginInfoDatabase[id] == pw)
		{
			// 로그인 처리
			
			// TODO : 유저 매니저 만들어서 저장하기? 음.. 나중엔 소켓 접속한사람 저장을 해놓고, 거기에 id 정보 추가하는 식으로 처리해보자.ㅣㅣ

			// 로그인 response 패킷 보내기.
			packet->_clientInfo->_responseData.Result = (unsigned short)ErrorCode::None;
		}
		else
		{
			// error 처리
			packet->_clientInfo->_responseData.Result = (unsigned short)ErrorCode::LoginPacketNotRegister;
			return;
		}

		OneSend(packet->_clientInfo, packet->_clientInfo->_responseData.Data, 2);
	}

	void Chat(std::shared_ptr<Packet> packet)
	{
		// 채팅 처리
	}
};