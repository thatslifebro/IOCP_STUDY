#include "GameServer.h"


int main()
{
	GameServer server;
	server.StartServer(11021);

	std::cout << "Server Start" << std::endl;

	std::cout << "Press any key to exit..." << std::endl;
	
	getchar();

	server.StopServer();

	std::cout << "Server End" << std::endl;

	return 0;
}
