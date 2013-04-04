/*
 
    ePing
 
    Using the Internet Control Message Protocol (ICMP) "ECHO" facility,
    measure round-trip-delays and packet loss across network paths.
 
*/

//TODO remove this when debugging is done
#include <iostream>


/*

    Imports

*/
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/unistd.h>
#include <sys/select.h>

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
 
	IP HEADER struct
 
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


int sent;
int processID;

/*

	Checksum()

	Simple checksum function for ICMP Header. This implentation was taken from Mike Musss' version of ping.c

*/
static u_short checksum(u_short *ICMPHeader, int len)
{
        register int nleft = len;
        register u_short *ICMPPointer = ICMPHeader;
        register int sum = 0;
        u_short answer = 0;

        /*
         * Our algorithm is simple, using a 32 bit accumulator (sum), we add
         * sequential 16 bit words to it, and at the end, fold back all the
         * carry bits from the top 16 bits into the lower 16 bits.
         */
        while (nleft > 1)
        {
            sum += *ICMPPointer;
			ICMPPointer++;
            nleft -= 2;
        }
    
        /* mop up an odd byte, if necessary */
        if (nleft == 1)
        {
            *(u_char *)(&answer) = *(u_char *)ICMPPointer;
            sum += answer;
        }
    
        /* add back carry outs from top 16 bits to low 16 bits */
        sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
        sum += (sum >> 16);                     /* add carry */
        answer = (u_short)~sum;                          /* truncate to 16 bits */
        return(answer);
}


/*
 
    ping()
 
    This method actually sends our ECHO_REQUEST across the internet.
    It computes the ICMP checksum and sends the packet to the address
    specified in main().
 
*/
void ping(int socketDescriptor,int datagramSize)
{
	// Fill in some data to send
	// Save tick count when sent (milliseconds)
	// Compute checksum
	icmpHeader->icmp_cksum =0;
	icmpHeader->icmp_cksum = checksum((u_short *)icmpHeader, sizeof(*icmpHeader));

	// Send the packet
	sent = sendto(socketDescriptor, (char *)outpack, 64, 0, &whereto, sizeof(struct sockaddr));

	// Print out if the packet sent or not
	if(sent > 0)
	{
		printf("SENT ");
        // Increment packet sequence number
        icmpHeader->icmp_seq++;
        printf("Seq : %d\t", icmpHeader->icmp_seq);
	}
	else
	{
		printf("Ping not sent.\n");
	}
}


/*
 
    report()
 
    This function grabs relevent data from the ICMP ECHO_REPLY and
    prints out the statistics of the pings ran with the program.
    It is called by listen().
 
 */
void report(char* receivedPacketBuffer, int length)
{
    /* Create ICMP struct to hold the received data */
    struct icmp *icmp;
    /* Format the received data into the ICMP struct */
    icmp = (struct icmp *) (receivedPacketBuffer + length);
    /* Check if the packet was meant for our computer using the ICMP id,
     which we set to the process ID */
    if(icmpHeader->icmp_id != processID)
    {
        printf("Not our packet\n");
        printf("processID = %d \t ID = %d\n", processID, icmpHeader->icmp_id);
    }
    printf("Seq : %d \n", icmpHeader->icmp_seq);
	/* Any missing packets? icmp_seq */
	/* Delays for each packet */
	/* Print it! */
}

/*
 
    listen()
 
    This function receives ECHO_REPLY packets sent to the host computer,
    and passes the information off to report()
 
 */
void listen(int socketDescriptor, sockaddr *fromWhom)
{
	// Setting some variables needed for select()
	char receivedPacketBuffer[512];
	fd_set *readfds;
	FD_SET(socketDescriptor, readfds);
	struct timeval timeout;
	timeout.tv_sec = 2; // timeout period, seconds (added second, if that matters)
	timeout.tv_usec = 0; // timeuot period, microseconds 1,000,000 micro = second
	socklen_t fromWhomLength;
	fromWhomLength = sizeof fromWhom;
	int selectStatus;
	selectStatus = select(socketDescriptor+1, readfds, NULL, NULL, &timeout);
	if(selectStatus == -1)
	{
		printf("Something terrible has happened! Error in select()\n");
	}
	else if(selectStatus == 0)
	{
		printf("I'm tired of waiting. Timeout.\n");
	}
	else
	{
		if(FD_ISSET(socketDescriptor, readfds))
		{
			recvfrom(socketDescriptor, &receivedPacketBuffer, sizeof receivedPacketBuffer, 0, fromWhom, &fromWhomLength);
			printf("RECEIVED ");
            report(receivedPacketBuffer, sizeof receivedPacketBuffer);
		}
	}
}


/*
 
    buildPing()
 
    buildPing() initializes the ICMP Header and IP Header structs, which
    contain essential packet information. The IP address information is
    set in main().
 
 */
void buildPing(int datagramSize, int seq)
{
	icmpHeader = (struct icmp *)outpack;
	icmpHeader->icmp_type = 8;
	icmpHeader->icmp_code = 0;
	icmpHeader->icmp_cksum = 0;
	icmpHeader->icmp_seq = seq;
	icmpHeader->icmp_id = processID;
	IPHeader.protocol = 1;
	IPHeader.timeToLive = 64; //Recommended value, according to the internet.
	IPHeader.versionHeaderLength = sizeof(struct tagIPHeader) + 64; // TODO 64 shouldn't be hardcoded
}

/*
 
    main()
 
    Where the magic happens. Command line flags are set, program
    control is set.
 
 */
char *argv[2];
int main(int argc, const char** argv)
{
	printf("----------------------------------\n");
	const int datagramSize = 50;
    
    /*
     
        Variables for command line flag processing
        
    */
	const char* destination = argv[1];
	bool timeBetweenReq = 0; // -q
		int msecsBetweenReq = 0;
	bool timeBetweenRepReq = 0; // -b
		int msecsBetweenRepReq = 0;
	//bool datagramSize = 0; // -d
		int bytesDatagram = 0;
	bool payloadSize = 0; // -p
		int bytesPayload = 0;
	bool randSizeMinMax = 0; // -l
		int bytesSizeMin = 0;
		int bytesSizeMax = 0;
	bool randSizeAvgStd = 0; // -r
		int bytesSizeAvg = 0;
		int bytesSizeStd = 0;
	bool randTimeMinMax = 0; // -s
		int msecsTimeMin = 0;
		int msecsTimeMax = 0;
	bool randTimeAvgStd = 0; // -t
		int msecsTimeAvg = 0;
		int msecsTimeStd = 0;
	bool increasingSize = 0; // -i
		int sizeInitial = 0;
		int sizeGrowth = 0;
	bool excludingPing = 0; // -e
		int pingsToExclude = 0;
	bool multiplePings = 0; // -n
		int numberOfPings = 5; // DEFAULT VALUE of 5
	
	for(int i = 2; i < argc; i++) {
	// argv[0] is the ./a which is input
	// argv[1] is the IPv4 address, MUST be valid
		if(strcmp(argv[i],"-q") == 0)
		{
			if(timeBetweenRepReq || randTimeMinMax || randTimeAvgStd)
			{
				printf("-q flag not set, conflicting with previously set flag.\n");
			}
			else
			{
				if(i + 1 < argc && atoi(argv[i+1]) > 0)
				{
					timeBetweenReq = true;
					msecsBetweenReq = atoi(argv[i+1]);
					printf("Flag -q set! Waiting %d milliseconds between ping requests.\n",msecsBetweenReq);
					i++;
				}
				else
				{
					printf("-q flag not set, requires parameter.\n");
				}
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
				if(i + 1 < argc && atoi(argv[i+1]) > 0)
				{
					timeBetweenRepReq = true;
					msecsBetweenRepReq = atoi(argv[i+1]);
					printf("Flag -b set! Waiting %d milliseconds between receiving a reply and sending a request\n", msecsBetweenRepReq);
					i++;
				}
				else
				{
					printf("-b flag not set, requires parameter.\n");
				}
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
				if(i + 1 < argc && atoi(argv[i+1]) > 0)
				{
					//datagramSize = true;
					bytesDatagram = atoi(argv[i+1]);
					printf("Flag -d set! Datagram will be %d bytes large.\n", bytesDatagram);
					i++;
				}
				else
				{
					printf("-d flag not set, requires parameter.\n");
				}
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
				if(i + 1 < argc && atoi(argv[i+1]) > 0)
				{
					payloadSize = true;
					bytesPayload = atoi(argv[i+1]);
					printf("Flag -p set! Payload size will be %d bytes large.\n", bytesPayload);
					i++;
				}
				else
				{
					printf("-p flag not set, requires parameter.\n");
				}
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
				if(i + 2 < argc && atoi(argv[i+2]) > 0)
				{
					randSizeMinMax = true;
					if(atoi(argv[i+1]) > atoi(argv[i+2]))
					{
						bytesSizeMin = atoi(argv[i+2]);
						bytesSizeMax = atoi(argv[i+1]);
					}
					else
					{
						bytesSizeMin = atoi(argv[i+1]);
						bytesSizeMax = atoi(argv[i+2]);
					}
					printf("Flag -l set! Random size will be between %d ", bytesSizeMin);
					printf("and %d.\n", bytesSizeMax);
					i += 2;
				}
				else
				{
					printf("-l flag not set, requires parameters.\n");
				}
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
				if(i + 2 < argc && atoi(argv[i+2]) > 0)
				{
					randSizeAvgStd = true;
					bytesSizeAvg = atoi(argv[i+1]);
					bytesSizeStd = atoi(argv[i+2]);
					printf("Flag -r set! Random size will average %d",bytesSizeAvg);
					printf(" with a std. dev. of %d.\n", bytesSizeStd);
					i += 2;
				}
				else
				{
					printf("-r flag not set, requires parameters.\n");
				}
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
				if(i + 2 < argc && atoi(argv[i+2]) > 0)
				{
					randTimeMinMax = true;
					if(atoi(argv[i+1]) > atoi(argv[i+2]))
					{
						msecsTimeMin = atoi(argv[i+2]);
						msecsTimeMax = atoi(argv[i+1]);
					}
					else
					{
						msecsTimeMin = atoi(argv[i+1]);
						msecsTimeMax = atoi(argv[i+2]);
					}
					printf("Flag -s set! Random time between requests will be between %d ", msecsTimeMin);
					printf("and %d.\n", msecsTimeMax);
					i += 2;
				}
				else
				{
					printf("-s flag not set, requires parameters.\n");
				}
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
				if(i + 2 < argc && atoi(argv[i+2]) > 0)
				{
					randTimeAvgStd = true;
					msecsTimeAvg = atoi(argv[i+1]);
					msecsTimeStd = atoi(argv[i+2]);
					printf("Flag -t set! Random time between requests will average %d ",msecsTimeAvg);
					printf("with a std. dev. of %d.\n", msecsTimeStd);
					i += 2;
				}
				else
				{
					printf("-t flag not set, requires parameters.\n");
				}
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
				if(i + 2 < argc && atoi(argv[i+2]) > 0)
				{
					increasingSize = true;
					sizeInitial = atoi(argv[i+1]);
					sizeGrowth = atoi(argv[i+2]);
					printf("Flag -i set! Pings will have an initial size of %d ", sizeInitial);
					printf("and grow at a rate of %d per request.\n", sizeGrowth);
					i += 2;
				}
				else
				{
					printf("-i flag not set, requires parameters.\n");
				}
			}
		}
		else if(strcmp(argv[i],"-e") == 0)
		{
			if(i + 1 < argc && atoi(argv[i+1]) > 0)
			{
				excludingPing = true;
				pingsToExclude = atoi(argv[i+1]);
                if(pingsToExclude >= numberOfPings)
                {
                    printf("Trying to exclude more pings than you send huh? Not funny.\n");
                    exit(0);
                }
				printf("Flag -e set! %d earliest pings to be excluded from final statistics.\n", pingsToExclude);
				i++;
			}
			else
			{
				printf("-e flag not set, requires parameter.\n");
			}
			
		}
		else if(strcmp(argv[i],"-n") == 0)
		{
			if(i + 1 < argc && atoi(argv[i+1]) > 0)
			{
				multiplePings = true;
				numberOfPings = atoi(argv[i + 1]);
				printf("Flag -n set! %d pings to be sent.\n", numberOfPings);
				i++;
			}
			else
			{
				printf("-n flag not set, requires parameter.\n");
                
			}
			
		}
		else
		{
			printf("Flag not recognized, \"%s\"\n",argv[i]);
		}
	}
	printf("Destination IP set to: %s\n", argv[1]);
	char hostName[128];
	gethostname(hostName, 128);
	if((hostName) == NULL)
	{
		printf("gethostname error: returned null\n");
	}
	hostent *hostIP;
	hostIP=gethostbyname(hostName);
	IPHeader.sourceIPAddress = srcIP;
	IPHeader.destinationIPAddress = destIP;
	whereto.sa_family=AF_INET;
	/*
	
		UNIX block for setting the address
		
	*/
	#if __unix__
	printf("main() mark 4.5\n");
	inet_pton(AF_INET,hostIP->h_name,&srcIP);
	printf("main() mark 5 (unix)\n");
	socketAddress = (struct sockaddr_in *)&whereto;
	if(inet_pton(AF_INET,destination,&IPHeader.destinationIPAddress)!=1)
	{
		// TODO Add error message
		printf("inet_pton error for IP Header\n");
        exit(0);
	}
	if(inet_pton(AF_INET,destination,&(socketAddress->sin_addr))!=1)
	{
		printf("inet_pton error for Socket Address\n");
        exit(0);
	}
	/*
	
		APPLE block for setting the address
		
	*/
	#elif __APPLE__
	inet_pton(AF_INET,hostIP->h_name,&srcIP);
	socketAddress = (struct sockaddr_in *)&whereto;
	if(inet_pton(AF_INET,destination,&IPHeader.destinationIPAddress)!=1)
	{
		// Add error message, etc.
		printf("inet_pton error for IP Header\n");
        exit(0);
	}
	if(inet_pton(AF_INET,destination,&(socketAddress->sin_addr))!=1)
	{
		printf("inet_pton error for Socket Address\n");
        exit(0);
	}
	/*
	
		WINDOWS block for setting the address
		
	*/
	#elif __WINDOWS__
	if(InetPton(AF_INET,destination,&IPHeader.destinationIPAddress)!=1)
	{
		int error=WSAGetLastError();
		printf((char*)error);
        exit(0);
	}
	if(InetPton(AF_INET,destination,&(socketAddress->sin_addr))!=1)
	{
		printf("inet_pton error for Socket Address\n");
        exit(0);
	}
	InetPton(AF_INET,hostIP,&IPHeader.sourceIPAddress);
	#endif
	socketAddress->sin_port=htons(3490);
	sockaddr_in sourceSocket;//The sockAddr to listen on
	sourceSocket.sin_port = htons(3490);
	sourceSocket.sin_addr = srcIP;
	sourceSocket.sin_family = AF_INET;
	processID = getpid() & 0xFFFF;
    /* Socket descriptors for sending and receiving traffic */
	int inSocketDescriptor = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	int outSocketDescriptor = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	bind(outSocketDescriptor,&whereto, sizeof(sourceSocket));
    printf("----------------------------------\n");
    buildPing(datagramSize, 0);
    if(excludingPing)
    {
        for(int i = 0; i < pingsToExclude; i++)
        {
            ping(outSocketDescriptor, datagramSize);
        }
    }
    for(int i = 0; i < numberOfPings-pingsToExclude; i++)
    {
        ping(outSocketDescriptor, datagramSize);
        listen(inSocketDescriptor,(sockaddr *) &sourceSocket);
    }
	printf("----------------------------------\n");
}
