#pragma once

#include "Types.h"
#include "Listener.h"
#include <set>
#include <functional>
#include <mutex>

class Service : public std::enable_shared_from_this<Service>
{
public:
	Service(IocpCoreRef iocpCore, ListenerRef listener, std::function<SessionRef(void)> sessionFactory,  int maxSessionCount);

	bool Start();

	IocpCoreRef GetIocpCore() { return _iocpCore; }

	int GetMaxSessionCount() { return _maxSessionCount; }

	void AddSession(SessionRef session);
	void ReleaseSession(SessionRef session);

	SessionRef GetNewSession() { return _sessionFactory(); }

private:
	std::function<SessionRef(void)> _sessionFactory;
	IocpCoreRef _iocpCore = nullptr;
	ListenerRef _listener = nullptr;
	std::set<SessionRef> _sessions;
	int _sessionCount = 0;

	int _maxSessionCount;
	std::mutex _mutex;
};
