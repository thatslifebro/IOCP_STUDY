#include "Service.h"
#include <WinSock2.h>

Service::Service(IocpCoreRef iocpCore, ListenerRef listener, std::function<SessionRef(void)> sessionFactory, int maxSessionCount)
	: _iocpCore(iocpCore), _listener(listener), _sessionFactory(sessionFactory), _maxSessionCount(maxSessionCount)
{
}

bool Service::Start()
{
	return _listener->StartAccept(shared_from_this());
}

void Service::AddSession(SessionRef session)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_sessions.insert(session);
	_sessionCount++;
}

void Service::ReleaseSession(SessionRef session)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_sessions.erase(session);
	_sessionCount--;
}

