# 구조 정리

(25.03.22) Echo서버까지 완료되어있음.

#### 서버 Todo
- 네트워크라이브러리
    - Recv Buffer, Send Buffer 만들기.
    - Packet 받기 준비.

- 게임 서버 로직
    - todo

## 주요 로직

### main.cpp

#### Service 생성
``` cpp
ServiceRef service = std::make_shared<Service>(
		std::make_shared<IocpCore>(),
		std::make_shared<Listener>(11021),
		std::make_shared<GameSession>,
		100
	);
```

##### IocpCore / Listener 생성, Session 생성 함수 등록, 최대 Session 수 지정.

- IocpCore 생성

	_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, MAX_WORKER_THREAD);

- Listener 생성

	listen socket 생성 : WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

- Session 생성 함수 등록

	이후 세션 미리 생성하여 AccceptEx 걸어놓을 때 사용할 함수 등록

- 최대 Session 수 지정

	사실 최대 세션 수가 아니라. 세션 연결 병렬 처리 수이다. 100명이 동시에 와도 처리되도록 하는 것.

#### Service.Start()
- ```_listener->StartAccept(shared_from_this())```
>  1. bind, listen 하기.
>  1. Service로 부터 Session 생성(소켓을 Iocp에 등록) 받아 AcceptEx 걸어두기.
>  1. ``` AcceptEx(_socket, session->GetSocket(), session->_recvBuffer, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, OUT & bytesReceived, static_cast<LPOVERLAPPED>(acceptEvent))```
>  1. 이 때, ```AcceptEvent : IocpEvent : OVERLAPPED``` 에 Session 등록해둠.
>  1. AcceptEvent와 세션 최대 세션 수 만큼 등록해두고 AcceptEvent들 vector에 모아놓음.

#### Iocp worker thread 생성
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
> 1. ```GetQueuedCompletionStatus``` 걸어놓기.
> 1. OVERLAPPED 객체가 IocpEvent이고, IocpEvent의 owner 는 IocpObject(Listener or Session)로 등록해 두었음.
> 1. ```IocpEvent -> owner -> Dispatch(iocpEvent, numOfBytes)``` 시켜서 해당 Object가 알맞은 처리 각각 함.
> 1. 에러 생기면 잘 처리.

#### 흐름
1. Service 생성
	- Listner, IocpCore, SessionFactory 있음.
1. Service Start
	- Listner는 Iocp에 소켓 등록
	- (IocpEvent에 SessionFactory로 만든 Session 등록 및 Session socket도 Iocp등록) * 최대세션수
	- IocpEvent 100개 Listner가 들고 있기
	- 각각 AcceptEx 다 걸어두기
1. Iocp Worker Thread 생성 및 Iocp Dispatch()
    - Iocp GetQueuedCompletionStatus로 대기.
	- 요청오면 받은 IocpEvent의 owner의 Dispatch()로 연결


### Listner

#### Dispatch
Iocp에서 받은 IocpEvent의 owner가 Listner인 경우 실행됨.

``` cpp
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
```

- Listner는 AcceptEx로 Accept만 Iocp에 걸어두기 때문에 EventType이 Accept인 AcceptEvent만 와야한다.
- 이후 ```ProcessAccept```로 처리

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

- IocpEvent에 등록되어있던 Session을 찾는다.
- 기타 설정을 하고, Session의 ```ProcessConnect()``` 실행
- 사용한 AcceptEvent를 다시 AcceptEx 걸어두어 다음 Session을 받도록 한다.

#### 세션의 ProcessConnect()
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
- 세션이 연결된 상태임을 저장
- Service에 연결되어있는 세션들을 저장하고있음. 추가하기.
- ```GameSession : Session```에서 정의한 OnConnected 실행.
- 클라이언트의 Request를 받기위해 Recv를 Iocp에 등록함.(WSARecv)


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
- Iocp에서 IocpEvent의 owner가 Session이면 여기로 옴.
- Recv나 Send를 걸어두었고 완료 통지가 온것이기 때문에 각각을 처리함.

#### ProcessRecv
``` cpp
void Session::ProcessRecv(int numOfBytes)
{
    _recvEvent.owner = nullptr;

    if (numOfBytes == 0)
    {
        // 에러처리.
        return;
    }

    OnRecv(_recvBuffer, numOfBytes); // 컨텐츠코드

    RegisterRecv();
}
```
- _recvEvent를 재사용하기 위해 초기화
- ```GameSession : Session```의 OnRecv 호출
- 다시 Recv 걸어둠.

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

    // 컨텐츠 코드에서 재정의
    OnSend(len);
}
```

- sendEvent는 재사용할 수없음. 한번에 여러개 보낼 수도 있기 때문.
- ```GameSession```의 OnSend 호출

### GameSession

#### OnRecv
Echo서버 이기 때문에 Session의 Send로 받은내용 그대로 전달

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
- IocpEvent 생성후 RegisterSend 호출

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
- WSASend로 보냄.

# README TODO
- Disconnect
- Connect