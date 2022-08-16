
#ifndef _EASYTCP_H_
#define _EASYTCP_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_TCP_SIZE		4096

typedef struct _tcpServerInfo {
	int serverSock;
	int acceptSock;
	int maxSize;
	uint16_t port;
	char pattern[4];		// reverse the order of bytes means to stop, 'PaTn' to 'nTaP'
	char ipAddr[16];		// ASCII IP address like 192.160.0.10
	struct sockaddr_in client;
	void (*callback)(char *dataBuffer, int dataSize, struct _tcpServerInfo *tsi);
} TSI;

typedef struct _tcpClientInfo {
	int sock;
	int maxSize;
	uint16_t port;
	char pattern[4];
	char ipAddr[16];		// ASCII IP address like 192.160.0.10
} TCI;

TSI *easyTcpServer(char *ipAddr, uint16_t port, char *pattern, void (*callback)(char *dataBuffer, int dataSize, TSI *tsi));
int easyTcpSendBack(TSI *tsi, char *dataBuffer, int dataSize);
void easyTcpServerFree(TSI *tsi);

TCI *easyTcpClient(char *ipAddr, uint16_t port, char *pattern);
int easyTcpSendPacket(TCI *tci, char *dataBuffer, int dataSize);
void easyTcpClientShutdown(TCI *tci);
void easyTcpClientFree(TCI *tci);

#endif
