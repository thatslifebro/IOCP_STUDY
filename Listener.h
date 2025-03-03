#pragma once

#include <iostream>

#include "Types.h"
#include "IocpCore.h"
#include "Service.h"
#include "IocpObject.h"
#include "IocpEvent.h"
#include "Session.h"

class Listener : public IocpObject
{
public: 
	Listener(int port);

	bool StartAccept(ServiceRef service);

	void RegisterAccept(AcceptEvent* accpetEvent);

	virtual HANDLE GetHandle() override;
	virtual void Dispatch(class IocpEvent* iocpEvent, int numOfBytes = 0) override;

	void ProcessAccept(AcceptEvent* acceptEvent);

private:
	SOCKET _socket = INVALID_SOCKET;
	std::vector<AcceptEvent*> _acceptEvents;
	ServiceRef _service = nullptr;
	int _port = 0;
};