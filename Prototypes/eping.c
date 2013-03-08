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
	Initialize Structs
*/

struct sockaddr_in *socketAddress;
struct sockaddr whereto;
struct in_addr destIP;
struct in_addr srcIP;
struct icmp * icmpHeader;
tagIPHeader IPHeader;
u_char outpack[100];

// Variable to see if the packet was sent
int sent;

int ident;

/*

	Checksum()

	Taken from Mike Musss' version of ping.c
	from the public domain

*/

static u_short checksum(u_short *ICMPHeader, int len)
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

            sum += *ICMPPointer;
			ICMPPointer++;
            nleft -= 2;
        }

        /* mop up an odd byte, if necessary */
        if (nleft == 1) {
                *(u_char *)(&answer) = *(u_char *)ICMPPointer;
                sum += answer;
        }

        /* add back carry outs from top 16 bits to low 16 bits */
		std::cout<<(sum>>16)<<std::endl;
        sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
		
		std::cout<<sum<<std::endl;
        sum += (sum >> 16);                     /* add carry */
		std::cout<<sum<<std::endl;
        answer = (u_short)~sum;                          /* truncate to 16 bits */
		std::cout<<sum<<std::endl;
		std::cout<<sizeof(answer)<<std::endl;
        printf("checksum() end\n");
        printf("------------------\n");
        return(answer);

}


/*

	ping()

*/
void ping(int socketDescriptor,int REQ_DATASIZE)
{
	printf("ping() begin\n");
	register int cc = 56;

	// Fill in some data to send

	// Save tick count when sent (milliseconds)

	// Compute checksum
	
	icmpHeader->icmp_cksum =0;
	icmpHeader->icmp_cksum = checksum((u_short *)icmpHeader, sizeof(*icmpHeader));
	// icmpHeader->icmp_cksum = htons(63231);
	

	// Send the packet
	sent = sendto(socketDescriptor, (char *)outpack, 64, 0, &whereto, sizeof(struct sockaddr));

	// Increment packet sequence number
	icmpHeader->icmp_seq++;

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
	printf("------------------\n");

}



/*

	listen()

*/
void listen(int socketDescriptor, sockaddr *fromWhom)
{
	printf("listen() begin\n");
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
	printf("listen() end\n");
	printf("------------------\n");

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
	icmpHeader = (struct icmp *)outpack;
	icmpHeader->icmp_type = 8;
	icmpHeader->icmp_code = 0;
	icmpHeader->icmp_cksum = 0;
	icmpHeader->icmp_seq = seq;
	icmpHeader->icmp_id = ident;
	// Fill packet
	#if __unix__
	//time(&ICMPEchoRequest.time);
	#elif __WINDOWS__
	//ICMPEchoRequest.time = time(NULL);
	#endif
	IPHeader.protocol = 1;
	IPHeader.timeToLive = 64; //Recommended value, according to the internet.
	IPHeader.versionHeaderLength = sizeof(struct tagIPHeader) + 64;
	printf("buildPing() end\n");
	printf("------------------\n");
}

/*

	main()

*/
char *argv[2];
int main(int argc, const char** argv)
{
	printf("------------------\n");
	// REMOVE THIS LATER
	const int REQ_DATASIZE = 50;
	// STOP REMOVING
	printf("main() begin\n");
	
	printf("argc = %d\n", argc);\
	//const char* flag[2];
	
	const char* destination = argv[1];
	bool timeBetweenReq = 0; // -q
	bool timeBetweenRepReq = 0; // -b
	bool datagramSize = 0; // -d
	bool payloadSize = 0; // -p
	bool randSizeMinMax = 0; // -l
	bool randSizeAvgStd = 0; // -r
	bool randTimeMinMax = 0; // -s
	bool randTimeAvgStd = 0; // -t
	bool increasingSize = 0; // -i
	bool excludingPing = 0; // -e
		int pingsToExclude = 0;
	bool multiplePings = 0; // -n
		int numberOfPings = 0;
	
	for(int i = 2; i < argc; i++) {
	// argv[0] is the ./a which is input
	// argv[1] is the IPv4 address, MUST be valid
		
		if(strcmp(argv[i],"-q") == 0)
		{
			printf("timeBetweenRepReq = %d\n", timeBetweenRepReq);
			printf("randTimeMinMax = %d\n", randTimeMinMax);
			printf("randTimeAvgStd = %d\n", randTimeAvgStd);
			if(timeBetweenRepReq || randTimeMinMax || randTimeAvgStd)
			{
				printf("-q flag not set, conflicting with previously set flag.\n");
			}
			else
			{
				timeBetweenReq = true;
				printf("Flag -q set!\n");
			}
		}
		else if(strcmp(argv[i],"-b") == 0)
		{
			if(timeBetweenReq || randTimeMinMax || randTimeAvgStd)
			{
				printf("-b flag not set, conflicting with previously set flag.\n");
			}
			else
			{
				timeBetweenRepReq = true;
				printf("Flag -b set!\n");
			}
		}
		else if(strcmp(argv[i],"-d") == 0)
		{
			if(payloadSize || randSizeMinMax || randSizeAvgStd)
			{
				printf("-d flag not set, conflicting with previously set flag.\n");
			}
			else
			{
				datagramSize = true;
				printf("Flag -d set!\n");
			}
		}
		else if(strcmp(argv[i],"-p") == 0)
		{
			if(datagramSize || randSizeMinMax || randSizeAvgStd)
			{
				printf("-p flag not set, conflicting with previously set flag.\n");
			}
			else
			{
				payloadSize = true;
				printf("Flag -p set!\n");
			}
		}
		else if(strcmp(argv[i],"-l") == 0)
		{
			if(datagramSize || payloadSize || increasingSize || randSizeAvgStd)
			{
				printf("-l flag not set, conflicting with previously set flag.\n");
			}
			else
			{
				randSizeMinMax = true;
				printf("Flag -l set!\n");
			}
		}
		else if(strcmp(argv[i],"-r") == 0)
		{
			if(datagramSize || payloadSize || increasingSize || randSizeMinMax)
			{
				printf("-r flag not set, conflicting with previously set flag.\n");
			}
			else
			{
				randSizeAvgStd = true;
				printf("Flag -r set!\n");
			}
		}
		else if(strcmp(argv[i],"-s") == 0)
		{
			if(timeBetweenReq || timeBetweenRepReq || randTimeAvgStd)
			{
				printf("-s flag not set, conflicting with previously set flag.\n");
			}
			else
			{
				randTimeMinMax = true;
				printf("Flag -s set!\n");
			}
		}
		else if(strcmp(argv[i],"-t") == 0)
		{
			if(timeBetweenReq || timeBetweenRepReq || randTimeMinMax)
			{
				printf("-t flag not set, conflicting with previously set flag.\n");
			}
			else
			{
				randTimeAvgStd = true;
				printf("Flag -t set!\n");
			}
		}
		else if(strcmp(argv[i],"-i") == 0)
		{
			if(datagramSize || payloadSize || randSizeMinMax || randSizeAvgStd)
			{
				printf("-i flag not set, conflicting with previously set flag.\n");
			}
			else
			{
				increasingSize = true;
				printf("Flag -i set!\n");
			}
		}
		else if(strcmp(argv[i],"-e") == 0)
		{
			excludingPing = true;
			pingsToExclude = atoi(argv[i + 1]);
			i++;
			printf("Flag -e set! %d earliest pings to be excluded.\n", pingsToExclude);
			
		}
		else if(strcmp(argv[i],"-n") == 0)
		{
			multiplePings = true;
			numberOfPings = atoi(argv[i + 1]);
			i++;
			printf("Flag -n set! %d pings to be sent.\n", numberOfPings);
			
		}
		else
		{
			printf("Flag not recognized, \"%s\"\n",argv[i]);
		}
	}
	printf("Flags are set and heading out to %s\n",argv[1]);

	char hostName[128];
	printf("main() mark 1\n");
	gethostname(hostName, 128);
	if((hostName) == NULL)
	{
		printf("gethostname error: returned null\n");
	}
	printf("main() mark 2\n");
	hostent *hostIP;
	printf("main() mark 3\n");
	hostIP=gethostbyname(hostName);
	printf("main() mark 4\n");
	IPHeader.sourceIPAddress = srcIP;
	IPHeader.destinationIPAddress = destIP;
	whereto.sa_family=AF_INET;
	/*
	
		UNIX block for setting the address
		
	*/
	#if __unix__
	printf("main() mark 4.5\n");
	//inet_pton(AF_INET,hostIP->h_name,&srcIP);
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
	/*
	
		APPLE block for setting the address
		
	*/
	#elif __APPLE__
	printf("main() mark 4.5\n");
	//inet_pton(AF_INET,hostIP->h_name,&srcIP);
	printf("main() mark 5 (APPLE)\n");
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
	/*
	
		WINDOWS block for setting the address
		
	*/
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
	InetPton(AF_INET,hostIP,&IPHeader.sourceIPAddress);
	#endif

	printf("main() mark 6\n");
	int seq = 1;
	ident = getpid() & 0xFFFF;
	int inSocketDescriptor;
	int outSocketDescriptor;
	printf("main() mark 7\n");
	inSocketDescriptor=socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	printf("main() mark 8\n");
	outSocketDescriptor=socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	buildPing(REQ_DATASIZE,seq);
	ping(outSocketDescriptor,REQ_DATASIZE);
	listen(inSocketDescriptor, &whereto);
	report();
	printf("main() end\n");
	printf("------------------\n");
}
