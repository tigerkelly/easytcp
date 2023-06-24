
ARC=libeasytcp.a

all: $(ARC) easytcpserv easytcpcli

$(ARC): easytcp.o
	$(AR) -r $(ARC) easytcp.o

easytcp.o: easytcp.c easytcp.h
	cc -c easytcp.c -o easytcp.o

easytcpserv.o: easytcpserv.c easytcp.h
	cc -c easytcpserv.c -o easytcpserv.o

easytcpcli.o: easytcpcli.c easytcp.h
	cc -c easytcpcli.c -o easytcpcli.o

easytcpserv: easytcpserv.o easytcp.o
	cc easytcpserv.o -o easytcpserv -L./ -leasytcp -lpthread

easytcpcli: easytcpcli.o easytcp.o
	cc easytcpcli.o -o easytcpcli -L./ -leasytcp -lpthread

clean:
	rm -rf *.o easytcpserv easytcpcli libeasytcp.a
