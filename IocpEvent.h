#pragma once
#include <winsock2.h>
#include <mswsock.h>
#include <WS2tcpip.h>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#include "Types.h"

enum EventType : USHORT
{
	Connect,
	Disconnect,
	Accept,
	Recv,
	Send,
};

class IocpEvent : public OVERLAPPED
{
public:
	IocpEvent(EventType type);
	void Init();

public:
	EventType eventType;
	IocpObjectRef owner;
};

class AcceptEvent : public IocpEvent
{
public:
	AcceptEvent() : IocpEvent(EventType::Accept) {}

public:
	SessionRef session = nullptr;
};

class ConnectEvent : public IocpEvent
{
public:
	ConnectEvent() : IocpEvent(EventType::Connect) {}
};

class DisconnectEvent : public IocpEvent
{
public:
	DisconnectEvent() : IocpEvent(EventType::Disconnect) {}
};

class RecvEvent : public IocpEvent
{
public:
	RecvEvent() : IocpEvent(EventType::Recv) {}
};

class SendEvent : public IocpEvent
{
public:
	SendEvent() : IocpEvent(EventType::Send) {}
	
	std::vector<BYTE> buffer;
};