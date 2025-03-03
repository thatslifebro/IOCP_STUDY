#pragma once
#include <WinSock2.h>
#include <memory>

class IocpObject : public std::enable_shared_from_this<IocpObject>
{
public:
	virtual HANDLE GetHandle() abstract;
	virtual void Dispatch(class IocpEvent* iocpEvent, int numOfBytes = 0) abstract;
};