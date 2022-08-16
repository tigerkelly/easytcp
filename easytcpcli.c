
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "easytcp.h"

int main(int argc, char *argv[]) {

	char recvBuf[MAX_TCP_SIZE];

	TCI *tci = easyTcpClient("192.168.0.121", 8383, "PaTn");
	if (tci == NULL) {
		printf("Failed to create TCP socket.\n");
		exit(1);
	}

	easyTcpSendPacket(tci, "From easytcpcli", 15);

	easyTcpClientShutdown(tci);

	easyTcpClientFree(tci);

	return 0;
}
