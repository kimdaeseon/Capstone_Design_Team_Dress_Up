#include "SocketServer.h"
#include "SocketClient.h"
#include <stdio.h>
#include <stdlib.h>
#pragma warning(disable:4996)


int __cdecl main(void)
{
	SocketClient client;
	FILE* file;
	file = fopen("junyong10.obj", "r");
	client.connection();
	char* line;

	while (1) {
		char line[128];
		// read the first word of the line
		if (NULL != file) {
			while (NULL != fgets(line, sizeof(line), file)) {
				client.sendData(string(line));
				client.recvFlag();
			}
			fclose(file);
		}
		return 0;
	}

	client.close();
	
}