
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "easytcp.h"

void callback(char *dataBuffer, int dataSize, TSI *tsi);

int main(int argc, char *argv[]) {

	TSI *tsi = easyTcpServer("192.168.0.121", 8383, "PaTn", &callback);
	if (tsi == NULL) {
		printf("Failed to create socket.\n");
		exit(1);
	}

	while(true) {
		sleep(2);
	}

	return 0;
}

void callback(char *dataBuffer, int dataSize, TSI *tsi) {

	printf("Got buffer size %d '%s' from %s\n", dataSize, dataBuffer, inet_ntoa(tsi->client.sin_addr));

}
