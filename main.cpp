#include "IocpCore.h"
#include "Service.h"

class GameSession : public Session
{
	void virtual OnConnected() override
	{
		std::cout << "GameSession OnConnected." << std::endl;
	}

	void virtual OnRecv(BYTE* buffer, int numOfBytes) override
	{
		std::string s(reinterpret_cast<char*>(buffer), numOfBytes);

		std::cout << "GameSession OnRecv : " << s << std::endl;

		Send(buffer, numOfBytes);
	}

	void virtual OnDisconnected() override
	{
		std::cout << "GameSession OnDisconnected" << std::endl;
	}

	void virtual OnSend(int numOfBytes) override
	{
		std::cout << "GameSession OnSend : " << numOfBytes << " bytes" << std::endl;
	}
};

int main()
{
	std::vector<std::thread> threads;

	ServiceRef service = std::make_shared<Service>(
		std::make_shared<IocpCore>(),
		std::make_shared<Listener>(11021),
		std::make_shared<GameSession>,
		100
	);

	if (service->Start() == false)
	{
		std::cout << "서버 시작 안됨." << std::endl;
		return 0;
	}

	for (int i = 0; i < 5; i++)
	{
		threads.emplace_back(std::thread([=] {
			while (true)
			{
				service->GetIocpCore()->Dispatch();
			}
		}));
	}

	for (int i = 0; i < threads.size(); i++)
	{
		threads[i].join();
	}
}
