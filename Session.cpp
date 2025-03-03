#include "Session.h"

LPFN_DISCONNECTEX DisconnectEx;

Session::Session()
{
    _socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}

HANDLE Session::GetHandle()
{
    return reinterpret_cast<HANDLE>(_socket);
}

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

void Session::ProcessConnect()
{
    //_connectEvent.owner = nullptr;
    _connected.store(true);
    _service->AddSession(GetSessionRef());

    OnConnected();

    RegisterRecv();
}

void Session::RegisterRecv()
{
    if (_connected.load() != true)
    {
        return;
    }

    _recvEvent.Init();
    _recvEvent.owner = shared_from_this();

    WSABUF wsaBuf;
    wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer);
    wsaBuf.len = sizeof(_recvBuffer);

    DWORD numOfBytes = 0;
    DWORD flags = 0;
    if (WSARecv(_socket, &wsaBuf, 1, OUT & numOfBytes, OUT & flags, &_recvEvent, nullptr) == SOCKET_ERROR)
    {
        int errCode = WSAGetLastError();
        if (errCode != WSA_IO_PENDING)
        {
            HandleError(errCode);
        }
    }
}

void Session::RegisterDisconnect()
{
    _disconnectEvent.Init();
    _disconnectEvent.owner = shared_from_this(); // ADD_REF

    if (false == DisconnectEx(_socket, &_disconnectEvent, TF_REUSE_SOCKET, 0))
    {
        int errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING)
        {
            _disconnectEvent.owner = nullptr; // RELEASE_REF
            return;
        }
    }

    return;
}

void Session::ProcessRecv(int numOfBytes)
{
    _recvEvent.owner = nullptr;

    if (numOfBytes == 0)
    {
        // ¿¡·¯Ã³¸®.
        return;
    }

    OnRecv(_recvBuffer, numOfBytes); // ÄÁÅÙÃ÷ÄÚµå

    RegisterRecv();
}

void Session::Send(BYTE* buffer, int len)
{
    SendEvent* sendEvent = new SendEvent();

    sendEvent->Init();
    sendEvent->owner = shared_from_this();
    sendEvent->buffer.resize(len);
    memcpy(sendEvent->buffer.data(), buffer, len);

    RegisterSend(sendEvent);
}

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

void Session::ProcessSend(SendEvent* sendEvent, int len)
{
    sendEvent->owner = nullptr;
    delete sendEvent;

    if (len == 0)
    {
        Disconnect(L"Send 0");
        return;
    }

    // ÄÁÅÙÃ÷ ÄÚµå¿¡¼­ ÀçÁ¤ÀÇ
    OnSend(len);
}

void Session::Disconnect(const WCHAR* cause)
{
    if (_connected.exchange(false) == false)
        return;

    // TEMP
    std::wcout << "Disconnect : " << cause << std::endl;

    OnDisconnected(); // ÄÁÅÙÃ÷ ÄÚµå¿¡¼­ ÀçÁ¤ÀÇ
    _service->ReleaseSession(GetSessionRef());

    RegisterDisconnect();
}

void Session::HandleError(int errorCode)
{
    switch (errorCode)
    {
    case WSAECONNRESET:
    case WSAECONNABORTED:
        Disconnect(L"HandleError");
        break;
    default:
        // TODO : Log
        std::cout << "Handle Error : " << errorCode << std::endl;
        break;
    }
}