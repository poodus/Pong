/*
 * ePing
 *
 *
 * Using the Internet Control Message Protocol (ICMP) "ECHO" facility,
 * measure round-trip-delays and packet loss across network paths.
 *
*/


/*
*
* Imports
*
*/
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <errno.h>
#if __unix__
#include <arpa/inet.h>
#elif __WINDOWS__
#include <windows.h>
#include <winsock2.h>
#endif

/*
	IP HEADER
*/
struct tagIPHeader
{
	u_char versionHeaderLength;
	u_char typeOfService;
	short totalLength;
	short ID;
	short flagsFragOff;
	u_char timeToLive;
	u_char protocol;
	u_short checksum;
	struct in_addr sourceIPAddress;
	struct in_addr destinationIPAddress;
};
/*
	Address Info Struct
*/

// struct addrinfo {
	// int ai_flags; // AI_Passive
	// int ai_family; // Inet? Inet6? Unspecified?
	// int ai_socktype; // Stream or Datagram: SOCK_STREAM or SOCK_DIAGRAM
	// int ai_protocol; //
	// size_t ai_addrlen;  // Size of ai_addr in bytes
	// struct sockaddr *ai_addr; // other struct
	// char *ai_canonname; //full canonical hostname
	// struct addrinfo *ai_next; // linked list, next node
// };




/* 
	ICMP HEADER 
*/
typedef struct tagICMPHeader{
	u_char type;
	u_char code;
	u_short checksum;
	u_short identifier;
	u_short sequenceNumber;
}icmpHeader;

struct tagICMPEchoRequest{
	tagICMPHeader icmpHeader;
	time_t time;
	//char charfillData[REQ_DATASIZE];
	char charfillData[3];
};
/*
	Initialize Structs
*/
// tagICMPHeader ICMPHeader;
tagICMPEchoRequest ICMPEchoRequest;
addrinfo addressInfo;
tagIPHeader IPHeader;



/*

	Ping()

*/
void ping(int socketDescriptor,int REQ_DATASIZE)
{
	// Fill in some data to send
	memset(ICMPEchoRequest.charfillData, ' ', REQ_DATASIZE);

	// Save tick count when sent (milliseconds)
	// echoRequest.time = gettime ...;

	// Put data in packet and compute checksum
	ICMPEchoRequest.icmpHeader.checksum= in_cksum( &ICMPEchoRequest, sizeof(ICMPEchoRequest));
	
	// readfds.fd_count = 1; // set size
	// readfds.fd_array[0] = raw; // socket set
	// timeout.tv_sec = 10; // timeout (s)
	// timeout.tv_usec = 0; // timeout (us)

	//if((rc = select(1, &socketDescriptor, NULL, NULL, &timeout)) == SOCKET_ERROR){
	//	errexit(perror("select() failed %d\n"));
	//}
	
	sent = sendto(s, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", len, flags, to, tolen);
	
	// Increment sequence number
	ICMPEchoRequest.icmpHeader.sequenceNumber++;
	
}



/*

	Listen()

*/
void listen(int socketDescriptor)
{
	/* Wait for reply... or timeout */
	
	/* Receive reply */
	
	// Get the info out of it
	
	// Was it an error packet? Uh oh!
	
	/* Lost packets: was this packet in order with the sequence? */
	
 	
}


/*

	Report()

*/
void report()
{
	// Any missing packets?
	// Delays for each packet
	// Print it!
	

}



/*

in_cksum not from 
*/
static int in_cksum(u_short *addr, int len)
{
        register int nleft = len;
        register u_short *w = addr;
        register int sum = 0;
        u_short answer = 0;

        /*
         * Our algorithm is simple, using a 32 bit accumulator (sum), we add
         * sequential 16 bit words to it, and at the end, fold back all the
         * carry bits from the top 16 bits into the lower 16 bits.
         */
        while (nleft > 1)  {
                sum += *w++;
                nleft -= 2;
        }

        /* mop up an odd byte, if necessary */
        if (nleft == 1) {
                *(u_char *)(&answer) = *(u_char *)w ;
                sum += answer;
        }

        /* add back carry outs from top 16 bits to low 16 bits */
        sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
        sum += (sum >> 16);                     /* add carry */
        answer = ~sum;                          /* truncate to 16 bits */
        return(answer);
}

/*
	buildPing()
*/

void buildPing(int REQ_DATASIZE, int seq){
	ICMPEchoRequest.icmpHeader.type='8';
	ICMPEchoRequest.icmpHeader.code='0';
	ICMPEchoRequest.icmpHeader.sequenceNumber=seq;
	ICMPEchoRequest.time=time(NULL);
	IPHeader.protocol=1;
	IPHeader.versionHeaderLength=4;
}

/*

	Main()

*/
char *argv[2];
int main(int argc, const char** argv){
printf("Hello World\n");
	const char* destination="127.0.0.1";
	char* hostName[128];
	gethostname(*hostName,128);
	struct hostent* hostIP;
	hostIP=gethostbyname(*hostName);
	#if __unix__
	inet_pton(2,*hostName,&IPHeader.sourceIPAddress);
	if(inet_pton(2,destination,&IPHeader.destinationIPAddress)!=1){
		// int error=WSAGetLastError();
		// printf((char*)error);
		//Add error message, etc.
	};
	#elif __WINDOWS__

	if(InetPton(2,destination,&IPHeader.destinationIPAddress)!=1){
		int error=WSAGetLastError();
		printf((char*)error);
	}
	InetPton(2,hostIP,*IPHeader.sourceIPAddress);
	#endif
	int seq=0;
	int REQ_DATASIZE=10;
	int inSocketDescriptor;
	int outSocketDescriptor;
	inSocketDescriptor=socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	outSocketDescriptor=socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	// Grab arguments from command line and set flags
	// Number of Pings
	// Packet Size
	buildPing(REQ_DATASIZE,seq);
	ping(outSocketDescriptor,REQ_DATASIZE);
	listen(inSocketDescriptor);
	report();

}


