#include "IocpEvent.h"

IocpEvent::IocpEvent(EventType type)
{
	eventType = type;
}

void IocpEvent::Init()
{
    OVERLAPPED::Internal = 0;
    OVERLAPPED::InternalHigh = 0;
    OVERLAPPED::Offset = 0;
    OVERLAPPED::OffsetHigh = 0;
    OVERLAPPED::hEvent = 0;
}
