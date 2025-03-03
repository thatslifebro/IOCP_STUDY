#include "Listener.h"

Listener::Listener(int port) : _port(port)
{
	// TODO 이부분은 진짜 시작부터 해야할듯?
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (_socket == INVALID_SOCKET)
	{
		std::cout << "WSASocket Error : " << WSAGetLastError() << std::endl;
		return;
	}
}

bool Listener::StartAccept(ServiceRef service)
{
	_service = service;

	service->GetIocpCore()->Register(shared_from_this());

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(_port);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(_socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		std::cout << "bind Error : " << WSAGetLastError() << std::endl;
		return false;
	}

	if (listen(_socket, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cout << "listen Error : " << WSAGetLastError() << std::endl;
		return false;
	}

	for (int i = 0; i < _service->GetMaxSessionCount(); i++)
	{
		AcceptEvent* acceptEvent = new AcceptEvent();
		acceptEvent->owner = shared_from_this();
		_acceptEvents.push_back(acceptEvent);
		RegisterAccept(acceptEvent);
	}

	return true;
}

void Listener::RegisterAccept(AcceptEvent* acceptEvent)
{
	SessionRef session = _service->GetNewSession();
	session->SetService(_service);
	_service->GetIocpCore()->Register(session);

	acceptEvent->Init();
	acceptEvent->session = session;

	DWORD bytesReceived = 0;
	if (false == AcceptEx(_socket, session->GetSocket(), session->_recvBuffer, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, OUT & bytesReceived, static_cast<LPOVERLAPPED>(acceptEvent)))
	{
		const int errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			// 일단 다시 Accept 걸어준다
			RegisterAccept(acceptEvent);
		}
	}
}

HANDLE Listener::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Listener::Dispatch(IocpEvent* iocpEvent, int numOfBytes)
{
	if (iocpEvent->eventType != EventType::Accept)
	{
		std::cout << "Listenr에 Accept가 아닌 event가 발생함!" << std::endl;
	}
	else
	{
		AcceptEvent* acceptEvent = static_cast<AcceptEvent*>(iocpEvent);
		ProcessAccept(acceptEvent);
	}
}

void Listener::ProcessAccept(AcceptEvent* acceptEvent)
{
	SessionRef session = acceptEvent->session;

	SOCKADDR_IN sockAddr;
	int sizeOfSockAddr = sizeof(sockAddr);

	getpeername(session->GetSocket(), reinterpret_cast<SOCKADDR*>(&sockAddr), &sizeOfSockAddr);

	session->SetSockAddr(sockAddr);

	session->ProcessConnect();

	RegisterAccept(acceptEvent);
}