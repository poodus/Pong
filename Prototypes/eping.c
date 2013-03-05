/*
 * ePing
 *
 *
 * Using the Internet Control Message Protocol (ICMP) "ECHO" facility,
 * measure round-trip-delays and packet loss across network paths.
 *
*/

//TODO remove this when debugging is done
#include <iostream>


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
#if __unix__
#include <arpa/inet.h>
#include <time.h>

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
	ICMP HEADER 
*/
struct tagICMPHeader{
	u_char type;
	u_char code;
	u_short checksum;
	u_short identifier;
	u_short sequenceNumber;
};


/* Socket Address ipv4 */
struct sockaddr_in *socketAddress;
struct sockaddr whereto;

struct in_addr destIP;
struct in_addr srcIP;


/*
	Initialize Structs
*/

tagIPHeader IPHeader;
tagICMPHeader * icmpHeader;
u_char outpack[100];

// Variable to see if the packet was sent
int sent;


/*

	Checksum()

	Taken from Mike Musss' version of ping.c
	from the public domain

*/

static int checksum(u_short *ICMPHeader, int len)
{
	printf("checksum() begin\n");
        register int nleft = len;
        register u_short *ICMPPointer = ICMPHeader;
        register int sum = 0;
        u_short answer = 0;

        /*
         * Our algorithm is simple, using a 32 bit accumulator (sum), we add
         * sequential 16 bit words to it, and at the end, fold back all the 
         * carry bits from the top 16 bits into the lower 16 bits.
         */
        while (nleft > 1)  {

            sum = sum+ *ICMPPointer;
			*ICMPPointer++;
            nleft -= 2;
        }

        /* mop up an odd byte, if necessary */
        if (nleft == 1) {
                *(u_char *)(&answer) = *(u_char *)ICMPPointer ;
                sum += answer;
        }

        /* add back carry outs from top 16 bits to low 16 bits */
        sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
        sum += (sum >> 16);                     /* add carry */
        answer = ~sum;                          /* truncate to 16 bits */
        return(answer);
	printf("checksum() end");
}


/*

	ping()

*/
void ping(int socketDescriptor,int REQ_DATASIZE)
{
	printf("ping() begin\n");
	// Fill in some data to send
	//memset(ICMPEchoRequest.charfillData, 'A', REQ_DATASIZE);
	
	// Save tick count when sent (milliseconds)

	// Compute checksum
	icmpHeader->checksum = checksum((u_short *)&icmpHeader, REQ_DATASIZE);

	// Send the packet
	sent = sendto(socketDescriptor, (char *)outpack, REQ_DATASIZE, 0, &whereto, sizeof(struct sockaddr));
	
	// Increment packet sequence number
	icmpHeader->sequenceNumber++;

	// Print out if the packet sent or not
	if(sent > 0)
	{
		printf("Ping sent!\n");
	}
	else
	{
		printf("Ping not sent.\n");
	}
	
	printf("ping() end\n");
	
}



/*

	listen()

*/
void listen(int socketDescriptor, sockaddr *fromWhom)
{
	// Setting some flags needed for select()
	
	fd_set readfds; //If this line doesn't work, try 'struct fd_set readfds', may additionally need preprocessor stuff
	//readfds.fd_count = 1; // Set # of sockets (I **think**)
	//readfds.fd_array[0] = raw; // Should be the sets of socket 
	struct timeval timeout;
	timeout.tv_sec = 2; // timeout period, seconds (added second, if that matters)
	timeout.tv_usec = 0; // timeuot period, microseconds 1,000,000 micro = second
	
	socklen_t fromWhomLength;
	fromWhomLength = sizeof fromWhom;
	
	char buf[512];
	
	// The following are functions we will probably need to use later
	// On second thought, we recieve (ping) packets one at a time, not as a set. We may not need these after all.
	// FD_SET(int fd, fd_set *set);		Add fd to the set
	// FD_CLR(int fd, fd_set *set);		Remove fd to the set
	// FD_ISSET(int fd, fd_set *set);	Returns trye if fd is in the set(probably won't use this one)
	// FD_ZERO(fd_set *set);			Clears all entries from the set
	int selectStatus;
	printf("Listening...");
	selectStatus = select(socketDescriptor + 1, &readfds, NULL, NULL, &timeout);
	if(selectStatus == -1) 
	{
		printf("Something terrible has happened! Error in select()\n");
	}
	else if(selectStatus == 0)
	{
		printf("I'm tired of waiting. Timeout occurred. Packet took too long to reply.\n");
	}
	else
	{
		printf("(I think) this means we have a reply!\n");
		if(FD_ISSET(socketDescriptor, &readfds))
		{
			recvfrom(socketDescriptor, buf, sizeof buf, 0, fromWhom, &fromWhomLength);
			printf("Packet receieved! (probably)\n");
		}
	}
	
	// Get the info out of it
	
	// Was it an error packet? Uh oh!
	
	/* Lost packets: was this packet in order with the sequence? */
	
}


/*

	report()

*/
void report()
{
	// Any missing packets?
	// Delays for each packet
	// Print it!
	

}




/*

	buildPing()

*/

void buildPing(int REQ_DATASIZE, int seq)
{
	printf("buildPing() begin\n");
	icmpHeader = (struct tagICMPHeader *)outpack;
	//register struct tagICMPHeader *icmpHeader;
	icmpHeader->type = 8;
	icmpHeader->code = 0;
	icmpHeader->sequenceNumber = seq;
	// Fill packet
	#if __unix__
	//time(&ICMPEchoRequest.time);
	#elif __WINDOWS__
	//ICMPEchoRequest.time = time(NULL);
	#endif
	IPHeader.protocol = 1;
	// IPHeader.versionHeaderLength=4;
	IPHeader.timeToLive = 64;//Recommended value, according to the internet.
	IPHeader.versionHeaderLength = 0b01000101;
	printf("buildPing() end\n");
}

/*

	main()

*/
//char *argv[2];
int main(int argc, const char* argv[])
{
	printf("argc = %d\n", argc);\
	//const char* flag[2];
	
	const char* destination = argv[1];
	
	for(int i = 2; i < argc; i++) {
	// argv[0] is the ./a which is input
	// argv[1] is the IPv4 address, MUST be valid
		
		if(strcmp(argv[i],"-a") == 0)
		{
			printf("Flag a set\n");
		}
		else if(strcmp(argv[i],"-b") == 0)
		{
			printf("Flag b set\n");
		}
		else if(strcmp(argv[i],"-c") == 0)
		{
			printf("Flag c set\n");
		}
		else if(strcmp(argv[i],"-d") == 0)
		{
			printf("Flag d set\n");
		}
		else
		{
			printf("Flag not recognized, \"%s\"\n",argv[i]);
		}
	}
	printf("Flags are set and heading out to %s\n",argv[1]);
		
	
	
	// REMOVE THIS LATER
	int REQ_DATASIZE =  50;
	// STOP REMOVING
	printf("main() begin\n");
	//const char* destination="127.0.0.1";
	char hostName[128];
	printf("main() mark 1\n");
	gethostname(hostName, 128);
	if((hostName)==NULL)
	{
		printf("gethostname error: returned null\n");
	}
	printf("main() mark 2\n");
	hostent *hostIP;
	printf("main() mark 3\n");
	hostIP=gethostbyname(hostName);
	printf("main() mark 4\n");
	IPHeader.sourceIPAddress=srcIP;
	IPHeader.destinationIPAddress=destIP;

	#if __unix__
	printf("main() mark 4.5\n");
	inet_pton(AF_INET,hostIP->h_name,&srcIP);
	printf("main() mark 5 (unix)\n");
	socketAddress = (struct sockaddr_in *)&whereto;
	if(inet_pton(AF_INET,destination,&IPHeader.destinationIPAddress)!=1)
	{
		// int error=WSAGetLastError();
		// printf((char*)error);
		// Add error message, etc.
		printf("inet_pton error for IP Header\n");
	}
	if(inet_pton(AF_INET,destination,&(socketAddress->sin_addr))!=1)
	{
		printf("inet_pton error for Socket Address\n");
	}

	#elif __WINDOWS__
	printf("main() mark 5 (windows)\n");
	if(InetPton(AF_INET,destination,&IPHeader.destinationIPAddress)!=1)
	{
		int error=WSAGetLastError();
		printf((char*)error);
	}
	if(InetPton(AF_INET,destination,&(socketAddress->sin_addr))!=1)
	{
		printf("inet_pton error for Socket Address\n");
	}
	InetPton(AF_INET,hostIP,*IPHeader.sourceIPAddress);
	#endif

	printf("main() mark 6\n");
	int seq=0;
	int inSocketDescriptor;
	int outSocketDescriptor;
	printf("main() mark 7\n");
	inSocketDescriptor=socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	printf("main() mark 8\n");
	outSocketDescriptor=socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	buildPing(REQ_DATASIZE,seq);
	ping(outSocketDescriptor,REQ_DATASIZE);
	printf("I'm about to call listen()!\n");
	listen(inSocketDescriptor, &whereto);
	report();
	printf("main() end\n");

}

