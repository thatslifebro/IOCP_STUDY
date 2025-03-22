# ���� ����

(25.03.22) Echo�������� �Ϸ�Ǿ�����.

#### ���� Todo
- ��Ʈ��ũ���̺귯��
    - Recv Buffer, Send Buffer �����.
    - Packet �ޱ� �غ�.

- ���� ���� ����
    - todo

## �ֿ� ����

### main.cpp

#### Service ����
``` cpp
ServiceRef service = std::make_shared<Service>(
		std::make_shared<IocpCore>(),
		std::make_shared<Listener>(11021),
		std::make_shared<GameSession>,
		100
	);
```

##### IocpCore / Listener ����, Session ���� �Լ� ���, �ִ� Session �� ����.

- IocpCore ����

	_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, MAX_WORKER_THREAD);

- Listener ����

	listen socket ���� : WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

- Session ���� �Լ� ���

	���� ���� �̸� �����Ͽ� AccceptEx �ɾ���� �� ����� �Լ� ���

- �ִ� Session �� ����

	��� �ִ� ���� ���� �ƴ϶�. ���� ���� ���� ó�� ���̴�. 100���� ���ÿ� �͵� ó���ǵ��� �ϴ� ��.

#### Service.Start()
- ```_listener->StartAccept(shared_from_this())```
>  1. bind, listen �ϱ�.
>  1. Service�� ���� Session ����(������ Iocp�� ���) �޾� AcceptEx �ɾ�α�.
>  1. ``` AcceptEx(_socket, session->GetSocket(), session->_recvBuffer, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, OUT & bytesReceived, static_cast<LPOVERLAPPED>(acceptEvent))```
>  1. �� ��, ```AcceptEvent : IocpEvent : OVERLAPPED``` �� Session ����ص�.
>  1. AcceptEvent�� ���� �ִ� ���� �� ��ŭ ����صΰ� AcceptEvent�� vector�� ��Ƴ���.

#### Iocp worker thread ����
``` cpp
for (int i = 0; i < 5; i++)
	{
		threads.emplace_back(std::thread([=] {
			while (true)
			{
				service->GetIocpCore()->Dispatch();
			}
		}));
	}
```
- IocpCore::Dispatch()
> 1. ```GetQueuedCompletionStatus``` �ɾ����.
> 1. OVERLAPPED ��ü�� IocpEvent�̰�, IocpEvent�� owner �� IocpObject(Listener or Session)�� ����� �ξ���.
> 1. ```IocpEvent -> owner -> Dispatch(iocpEvent, numOfBytes)``` ���Ѽ� �ش� Object�� �˸��� ó�� ���� ��.
> 1. ���� ����� �� ó��.

#### �帧
1. Service ����
	- Listner, IocpCore, SessionFactory ����.
1. Service Start
	- Listner�� Iocp�� ���� ���
	- (IocpEvent�� SessionFactory�� ���� Session ��� �� Session socket�� Iocp���) * �ִ뼼�Ǽ�
	- IocpEvent 100�� Listner�� ��� �ֱ�
	- ���� AcceptEx �� �ɾ�α�
1. Iocp Worker Thread ���� �� Iocp Dispatch()
    - Iocp GetQueuedCompletionStatus�� ���.
	- ��û���� ���� IocpEvent�� owner�� Dispatch()�� ����


### Listner

#### Dispatch
Iocp���� ���� IocpEvent�� owner�� Listner�� ��� �����.

``` cpp
void Listener::Dispatch(IocpEvent* iocpEvent, int numOfBytes)
{
	if (iocpEvent->eventType != EventType::Accept)
	{
		std::cout << "Listenr�� Accept�� �ƴ� event�� �߻���!" << std::endl;
	}
	else
	{
		AcceptEvent* acceptEvent = static_cast<AcceptEvent*>(iocpEvent);
		ProcessAccept(acceptEvent);
	}
}
```

- Listner�� AcceptEx�� Accept�� Iocp�� �ɾ�α� ������ EventType�� Accept�� AcceptEvent�� �;��Ѵ�.
- ���� ```ProcessAccept```�� ó��

#### ProcessAccept
``` cpp
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
```

- IocpEvent�� ��ϵǾ��ִ� Session�� ã�´�.
- ��Ÿ ������ �ϰ�, Session�� ```ProcessConnect()``` ����
- ����� AcceptEvent�� �ٽ� AcceptEx �ɾ�ξ� ���� Session�� �޵��� �Ѵ�.

#### ������ ProcessConnect()
``` cpp
void Session::ProcessConnect()
{
    //_connectEvent.owner = nullptr;
    _connected.store(true);
    _service->AddSession(GetSessionRef());

    OnConnected();

    RegisterRecv();
}
```
- ������ ����� �������� ����
- Service�� ����Ǿ��ִ� ���ǵ��� �����ϰ�����. �߰��ϱ�.
- ```GameSession : Session```���� ������ OnConnected ����.
- Ŭ���̾�Ʈ�� Request�� �ޱ����� Recv�� Iocp�� �����.(WSARecv)


### Session

#### Dispatch
``` cpp
void Session::Dispatch(IocpEvent* iocpEvent, int numOfBytes)
{
    switch (iocpEvent->eventType)
    {
    case EventType::Recv:
        ProcessRecv(numOfBytes);
        return;
    case EventType::Send:
        ProcessSend(reinterpret_cast<SendEvent*>(iocpEvent), numOfBytes);
    default :
        return;
    }
}
```
- Iocp���� IocpEvent�� owner�� Session�̸� ����� ��.
- Recv�� Send�� �ɾ�ξ��� �Ϸ� ������ �°��̱� ������ ������ ó����.

#### ProcessRecv
``` cpp
void Session::ProcessRecv(int numOfBytes)
{
    _recvEvent.owner = nullptr;

    if (numOfBytes == 0)
    {
        // ����ó��.
        return;
    }

    OnRecv(_recvBuffer, numOfBytes); // �������ڵ�

    RegisterRecv();
}
```
- _recvEvent�� �����ϱ� ���� �ʱ�ȭ
- ```GameSession : Session```�� OnRecv ȣ��
- �ٽ� Recv �ɾ��.

#### ProcessSend
``` cpp
void Session::ProcessSend(SendEvent* sendEvent, int len)
{
    sendEvent->owner = nullptr;
    delete sendEvent;

    if (len == 0)
    {
        Disconnect(L"Send 0");
        return;
    }

    // ������ �ڵ忡�� ������
    OnSend(len);
}
```

- sendEvent�� ������ ������. �ѹ��� ������ ���� ���� �ֱ� ����.
- ```GameSession```�� OnSend ȣ��

### GameSession

#### OnRecv
Echo���� �̱� ������ Session�� Send�� �������� �״�� ����

#### Session::Send
``` cpp
void Session::Send(BYTE* buffer, int len)
{
    SendEvent* sendEvent = new SendEvent();

    sendEvent->Init();
    sendEvent->owner = shared_from_this();
    sendEvent->buffer.resize(len);
    memcpy(sendEvent->buffer.data(), buffer, len);

    RegisterSend(sendEvent);
}
```
- IocpEvent ������ RegisterSend ȣ��

#### Session::RegisterSend
``` cpp
void Session::RegisterSend(SendEvent* sendEvent)
{
    if (_connected.load() == false)
        return;

    WSABUF wsaBuf;
    wsaBuf.buf = (char*)sendEvent->buffer.data();
    wsaBuf.len = (ULONG)sendEvent->buffer.size();

    DWORD numOfBytes = 0;
    if (WSASend(_socket, &wsaBuf, 1, OUT & numOfBytes, 0, sendEvent, nullptr) == SOCKET_ERROR)
    {
        int errCode = ::WSAGetLastError();
        if (errCode != WSA_IO_PENDING)
        {
            HandleError(errCode);
            sendEvent->owner = nullptr; // RELEASE_REF
            delete sendEvent;
        }
    }
}
```
- WSASend�� ����.

# README TODO
- Disconnect
- Connect