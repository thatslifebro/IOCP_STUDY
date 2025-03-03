#pragma once
#include <iostream>

#include "Types.h"
#include "IocpCore.h"
#include "Service.h"
#include "IocpObject.h"

class Session : public IocpObject
{
public:
	Session();

	SOCKET GetSocket() { return _socket; }

	void SetService(ServiceRef service) { _service = service; }

	virtual HANDLE GetHandle() override;
	virtual void Dispatch(class IocpEvent* iocpEvent, int numOfBytes = 0) override;

	void SetSockAddr(SOCKADDR_IN sockAddr) { _sockAddr = sockAddr; }

	SessionRef GetSessionRef() { return std::static_pointer_cast<Session>(shared_from_this()); }

public:

	void ProcessConnect();
	void RegisterRecv();
	void RegisterDisconnect();
	void Disconnect(const WCHAR* cause);
	void ProcessRecv(int numbOfBytes);

	void Send(BYTE* buffer, int len);
	void RegisterSend(SendEvent* sendEvent);
	void ProcessSend(SendEvent* sendEvent, int len);
	

public:
	void virtual OnConnected() abstract;
	void virtual OnRecv(BYTE* buffer, int numOfBytes) abstract;
	void virtual OnDisconnected() abstract;
	void virtual OnSend(int numOfBytes) abstract;

public:
	BYTE _recvBuffer[1000];

private:
	void HandleError(int errCode);

private:
	ConnectEvent _connectEvent;
	DisconnectEvent _disconnectEvent;
	RecvEvent _recvEvent;

private:
	SOCKET _socket = INVALID_SOCKET;
	ServiceRef _service = nullptr;

	SOCKADDR_IN _sockAddr;

	std::atomic_bool _connected = false;
};