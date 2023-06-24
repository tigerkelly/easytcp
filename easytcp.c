/* easytcp.c - An example of how to move data across a TCP/IP connection.
 * Written by Richard Kelly Wiles
 * Date 08/10/2022
 * This software is free to use as long as this comment section is left unchanged at the top of the file.
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#include "easytcp.h"

/* For moving data across a TCP/IP connection I like to split the data up
 * into packets with an 8 byte header with the following format.
 *    char pattern[4];
 *    int pktSize;
 * The pattern can be any unlikly sequence of bytes, like "PaTn" but it should not be a palindrome like sequence.
 * If the server does not see the 4 byte pattern it may close the connection and
 * goes back to looking for connections.
 * Why I do this is so that I do not have to byte read the TCP conections which
 * is very CPU intinsive.
 * With this method I read 8 bytes of data and look for the pattern, if the pattern
 * eixists then I use the pktSize field to read that maney bytes from the stream.  If the
 * pattern does not match I either close the connection or I keep reading 8 bytes of data
 * until I find the pattern again.  Conintuing to read 8 bytes to find pattern does not
 * work well IMO casue it could take a long time to find the header packet if the data is very large
 * or because we miss the header cause of odd number of bytes read.
 *
 * If your packets are small, say 16 bytes then reading byte by byte without the header maybe ok, CPU wise but
 * it is always good to know that you have a complete packet in IMO.
 *
 * With TCP/IP you do not have to worry about lost packets cause the TCP/IP protocol will handle resending the
 * lost packets even over wireless networks.  When TCP/IP has to resend a packet it stops delievering data to
 * the application which can cause issues for the application.  If you application is expecting data to arrive
 * in a timely manor then it may break your application.  Lets say you are trying to send a video stream to another
 * system to be played to the user.  Video frames can be very large so lost packets can be very common.  The TCP
 * connection will resend lost packets but this may cause the video to be choppy or studdering video.  This is why
 * most YouTube or Netflix services buffer up N seconds of video before starting to play the video.
 *
 * TCP has a lot of over head so it always takes longer time to send data, this is why most applications that
 * need speed use UDP.
 *    **** See my easyudp library also ****
 */

bool stopAcceptThread = false;
bool stopCaptureThread = false;

void *_easyTcpCapture(void *param);
void *_easyTcpAccept(void *param);
int _easyTcpReadPacket(int sock, char *buffer, int pktSize);

// pattern is used in the 8 data header.

TSI *easyTcpServer(char *ipAddr, uint16_t port, char *pattern, void (*callback)(char *dataBuffer, int dataSize, TSI *tsi)) {
	int sock;
	int connfd;
	struct sockaddr_in servaddr, client;

	// socket create and verification
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		printf("Create socket failed. %s\n", strerror(errno));
		exit(0);
	}

	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	if (ipAddr == NULL)
		servaddr.sin_addr.s_addr = INADDR_ANY;
	else
		servaddr.sin_addr.s_addr = inet_addr(ipAddr);
	servaddr.sin_port = htons(port);

	// Binding newly created socket to given IP and verification
	if ((bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
		printf("Bind failed. %s\n", strerror(errno));
		exit(0);
	}

	if ((listen(sock, 5)) != 0) {
		printf("Listen failed. %s\n", strerror(errno));
		exit(0);
	}

	pthread_t thd;
	TSI *tsi = calloc(1, sizeof(TSI));  // Have to malloc structure when passing to thread.

	tsi->serverSock = sock;
	tsi->port = port;
	tsi->maxSize = MAX_TCP_SIZE;
	tsi->callback = callback;
	strcpy(tsi->ipAddr, ipAddr);
	memcpy(tsi->pattern, pattern, 4);

	pthread_create(&thd, NULL, _easyTcpAccept, (void *) tsi);

	return tsi;
}

void *_easyTcpAccept(void *param) {
	int connfd;
	socklen_t len;
	struct sockaddr_in client;

	TSI *tsi = (TSI *)param;

	len = sizeof(client);

	while (stopAcceptThread == false) {
		// Accept the data packet from client and verification
		connfd = accept(tsi->serverSock, (struct sockaddr *)&client, &len);
		if (connfd < 0) {
			printf("Accept failed. %s\n", strerror(errno));
			exit(0);
		}


		pthread_t thd;
		TSI *pp = calloc(1, sizeof(TSI));  // Have to malloc structure when passing to thread.

		pp->maxSize = tsi->maxSize;
		pp->acceptSock = connfd;
		pp->serverSock = tsi->serverSock;
		pp->client = client;
		pp->port = tsi->port;
		pp->callback = tsi->callback;
		memcpy(pp->pattern, tsi->pattern, 4);

		// The thread is passed needed information in the TSI structure.
		// Each new connection gets its own capture thread.
		pthread_create(&thd, NULL, _easyTcpCapture, (void *) pp);
	}
}

// Only to be called by the _easyTcpAccept function.
void *_easyTcpCapture(void *param) {
	int n, len;
	char revStr[5];
	char header[8];

	TSI *tsi = (TSI *)param;

	memset(header, 0, sizeof(header));
	memset(revStr, 0, sizeof(revStr));

	revStr[0] = tsi->pattern[3];
	revStr[1] = tsi->pattern[2];
	revStr[2] = tsi->pattern[1];
	revStr[3] = tsi->pattern[0];

	char buffer[tsi->maxSize + 1];

	while (stopCaptureThread == false) {

		// Read the header part of the packet to validate the packet and get the length.
		int size = _easyTcpReadPacket(tsi->acceptSock, header, 8);

		if (size != 8) {
			printf("Failed to read header.\n");
			continue;
		}

		if (memcmp(header, revStr, 4) == 0) {
			// The client said to stop.
			break;
		}

		if (memcmp(header, tsi->pattern, 4) != 0) {
			printf("Header pattern does not match.\n");
			break;		// pattern does not match, leave capture thread.
			// NOTE: You could try and discard header and try again.
		}

		int pktSize = ntohl(*(unsigned int *)&header[4]);

		if (pktSize > tsi->maxSize) {
			printf("Packet size is greater than max packet size.\n");
			break;
			// NOTE: You could try discarding the over size packet and then continuing.
		}

		size = _easyTcpReadPacket(tsi->acceptSock, buffer, pktSize);

		if (size != pktSize) {
			printf("Did not read complete packet.\n");
			continue;
		}

		buffer[size] = '\0';

		if (size == pktSize) {
			// Does not come back until the callback returns, so callback needs to be quick.
			(*tsi->callback)(buffer, size, tsi);
		}

		sleep(1);			// TAKE ACTION: Remove this sleep when in production.
	}
	printf("Client closed connection.\n");
	close(tsi->acceptSock);

	return NULL;
}

// Reads a packet or header from the TCP stream.
// Returns the number of bytes read.
int _easyTcpReadPacket(int sock, char *buffer, int pktSize) {
	int size, n;

	// You could add a retry counter here also if you like, so that you can count the number of
	// times it took to read pktSize bytes of data.  Could log the number of retries if it is over N times,
	// just for informational data.  Most of the time it should only take one try if it is small.

	do {
		n = recv(sock, &buffer[size], (pktSize - size), 0);
		if (n < 0)
			break;		// Got a read error.
		size += n;
	} while (size < pktSize);

	return size;
}

int easyTcpSendPacket(TCI *tci, char *buffer, int pktSize) {

	int n;
	char header[8];

	// Build and send header first.
	memcpy(header, tci->pattern, 4);
	int num = htonl(pktSize);
	memcpy(&header[4], (char *)&num, 4);

	n = send(tci->sock, header, 8, 0);
	if (n < 0)
		return n;

	// Send data packet
	n = send(tci->sock, buffer, pktSize, 0);

	return n;
}

void stopTcpServer(int sock) {
	stopAcceptThread = true;
	stopCaptureThread = true;

	if (sock > 0)
		close(sock);
}

TCI *easyTcpClient(char *ipAddr, uint16_t port, char *pattern) {
	int sock;
	struct sockaddr_in servaddr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		printf("Create socket failed.\n");
		exit(0);
	}
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = inet_addr(ipAddr);

	if (connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
		printf("Connection to server failed.\n");
		exit(0);
	}

	TCI *tci = (TCI *) calloc(1, sizeof(TCI));

	tci->sock = sock;
	tci->maxSize = MAX_TCP_SIZE;
	tci->port = port;
	memcpy(tci->pattern, pattern, 4);
	strcpy(tci->ipAddr, ipAddr);

	return tci;
}

void easyTcpClientShutdown(TCI *tci) {
	if (tci != NULL) {
		char header[8];

		memset(header, 0, sizeof(header));

		// reversing the pattern cause server to close connection.
		header[0] = tci->pattern[3];
		header[1] = tci->pattern[2];
		header[2] = tci->pattern[1];
		header[3] = tci->pattern[0];

		send(tci->sock, header, 8, 0);
	}
}

void easyTcpServerShutdown(TSI *tsi) {
	if (tsi != NULL) {
		if (tsi->serverSock > 0)
			close(tsi->serverSock);
		if (tsi->acceptSock > 0)
			close(tsi->acceptSock);
		free(tsi);
	}
}

void easyTcpClientFree(TCI *tci) {
	if (tci != NULL) {
		if (tci->sock > 0)
			close(tci->sock);
		free(tci);
	}
}
