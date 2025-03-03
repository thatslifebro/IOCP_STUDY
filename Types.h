#pragma once
#include <memory>

using ServiceRef = std::shared_ptr<class Service>;
using IocpCoreRef = std::shared_ptr<class IocpCore>;
using ListenerRef = std::shared_ptr<class Listener>;
using IocpObjectRef = std::shared_ptr<class IocpObject>;
using SessionRef = std::shared_ptr<class Session>;