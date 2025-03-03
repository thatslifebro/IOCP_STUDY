#include "IocpCore.h"

IocpCore::IocpCore()
{
	_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, MAX_WORKER_THREAD);
}

IocpCore::~IocpCore()
{
	CloseHandle(_iocpHandle);
}

void IocpCore::Register(IocpObjectRef iocpObject)
{
	CreateIoCompletionPort(iocpObject->GetHandle(), _iocpHandle, 0, 0);
}

bool IocpCore::Dispatch()
{
	DWORD numOfBytes = 0;
	ULONG_PTR key = NULL;
	IocpEvent* iocpEvent = nullptr;

	if (GetQueuedCompletionStatus(_iocpHandle, OUT & numOfBytes, OUT & key, reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), INFINITE))
	{
		IocpObjectRef iocpObject = iocpEvent->owner;
		iocpObject->Dispatch(iocpEvent, numOfBytes);
	}
	else
	{
		auto errCode = WSAGetLastError();
		switch (errCode)
		{
		case WAIT_TIMEOUT:
			return false;
		default:
			// TODO : 로그 찍기
			IocpObjectRef iocpObject = iocpEvent->owner;
			iocpObject->Dispatch(iocpEvent, numOfBytes);
			break;
		}
		std::cout << "GetQueuedCompletionStatus Error : " << errCode << std::endl; // TODO : 이값이 64 면 비정상적 종료 인것 같다. 이 클라에대한 종료 처리를 해줘야할듯.
	}

	return true;
}
