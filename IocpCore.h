#pragma once
#pragma comment(lib, "ws2_32")
#pragma comment(lib,"mswsock.lib")

#include <iostream>
#include <vector>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <cstring>

#include "IocpEvent.h"
#include "IocpObject.h"


#define MAX_WORKER_THREAD 4

class IocpCore : public std::enable_shared_from_this<IocpCore>
{
public:
	IocpCore();

	~IocpCore();

	void Register(IocpObjectRef iocpObject);

	bool Dispatch();

private:
	HANDLE _iocpHandle;

};