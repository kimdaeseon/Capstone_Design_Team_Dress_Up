#include "SocketServer.h"

int __cdecl main(void)
{
	SocketServer server = SocketServer();

	server.connection();

	for (int i = 0; i < 10; i++) {
		server.sendData("yea it works!");
	}

	server.sendData("fin");

	server.close();

}