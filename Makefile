
all: easytcpserv easytcpcli

easytcp.o: easytcp.c easytcp.h
	cc -c easytcp.c -o easytcp.o

easytcpserv.o: easytcpserv.c easytcp.h
	cc -c easytcpserv.c -o easytcpserv.o

easytcpcli.o: easytcpcli.c easytcp.h
	cc -c easytcpcli.c -o easytcpcli.o

easytcpserv: easytcpserv.o easytcp.o
	cc easytcpserv.o easytcp.o -o easytcpserv -lpthread

easytcpcli: easytcpcli.o easytcp.o
	cc easytcpcli.o easytcp.o -o easytcpcli -lpthread

clean:
	rm -rf *.o easytcpserv easytcpcli
