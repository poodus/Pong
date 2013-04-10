/*
 
 ePing
 
 Using the Internet Control Message Protocol (ICMP) ECHO_REQUEST
 and ECHO_REPLY messages to measure round-trip-delays and packet
 loss across network paths.
 
 Created as a 2013 NDSU Capstone Project with specifications
 and requiremetns provided by Ericsson.
 
 An extension/reimplementation of the original ping.c.
 Inspiration and design inspired by:
 Author -
 Mike Muuss
 U. S. Army Ballistic Research Laboratory
 December, 1983
 Modified at Uc Berkeley
 
 */



/*
 
 Imports
 
 */
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/select.h>
#include <stdlib.h>
#include <signal.h>
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
 
 Initialize Structs
 
 */

struct sockaddr_in *socketAddress;
struct sockaddr whereto;
struct in_addr destIP;
struct in_addr srcIP;
struct icmp * icmpHeader;
struct icmp * receivedICMPHeader;
struct icmp * receivedIPHeader;
struct ip * ipHeader;
u_char * packet[100];
u_char * packetIP[100];

/*
 
 Global Variables
 
 */
int sent;
int processID;
int icmpPayloadLength = 30;
int pingsToSend = 5;
int pingsSent = 0;

/*
 
 Checksum()
 
 Simple checksum function for ICMP Header. This implentation was
 adapted from Mike Musss' version of ping.c
 
 */
static u_short checksum(u_short *ICMPHeader, int len)
{
    register int nleft = len;
    register u_short *ICMPPointer = ICMPHeader;
    register int sum = 0;
    u_short answer = 0;
    
    /*
     Our algorithm is simple, using a 32 bit accumulator (sum), we add
     sequential 16 bit words to it, and at the end, fold back all the
     carry bits from the top 16 bits into the lower 16 bits.
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
        *(u_char *)(&answer) = *(u_char *) ICMPPointer;
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
void ping(int socketDescriptor, int icmpPayloadLength)
{
	// Fill in some data to send
	// Save tick count when sent (milliseconds)
	// Compute checksum
	icmpHeader->icmp_cksum = 0;
	icmpHeader->icmp_cksum = checksum((u_short *)icmpHeader, sizeof(*icmpHeader));
	// Send the packet
	sent = sendto(socketDescriptor, packet, icmpPayloadLength+8, 0, (struct sockaddr *)&whereto, sizeof(&whereto));
	// Print out if the packet sent or not
	if(sent > 0)
	{
		printf("SENT\n");
//        printf("Type: %d\t", icmpHeader->icmp_type);
//        printf("Code: %d\t", icmpHeader->icmp_code);
//        printf("Seq : %d\t", icmpHeader->icmp_seq);
//        printf("Checksum: %d \t\n", icmpHeader->icmp_cksum);
        // Increment packet sequence number
        icmpHeader->icmp_seq++;
        pingsSent++;
	}
	else
	{
		printf("Ping not sent.\n\n");
	}
}

/*
 
 report()
 
 This function reports the final statistics, either to the command line
 or to a CSV (comma separated value) file.
 
*/
void report()
{
    printf("Statistics: ");
}

/*
 
 listen()
 
 This function receives ECHO_REPLY packets sent to the host computer,
 and passes the information off to report()
 
*/
void listen(int socketDescriptor, sockaddr_in * fromWhom)
{
	// Setting some variables needed for select()
	char receivedPacketBuffer[512];
	fd_set readfds;
    //FD_ZERO(&readfds);
	FD_SET(socketDescriptor, &readfds);
	struct timeval timeout;
	timeout.tv_sec = 2; // timeout period, seconds (added second, if that matters)
	timeout.tv_usec = 0; // timeuot period, microseconds 1,000,000 micro = second
    // TODO Make this timeout dependent on how many pings have been sent...
	int selectStatus = select(socketDescriptor+1, &readfds, NULL, NULL, &timeout);
    
    socklen_t fromWhomLength = sizeof(fromWhom);
	if(selectStatus == -1)
	{
		printf("LISTEN: Error in select()\n");
	}
	else if(selectStatus == 0)
	{
		printf("I'm tired of waiting. Timeout.\n");
	}
	else
	{
		if(FD_ISSET(socketDescriptor, &readfds))
		{
            /* Receive the data */
			ssize_t bytesReceived = recvfrom(socketDescriptor, (char *)receivedPacketBuffer, sizeof(receivedPacketBuffer), 0, (struct sockaddr *)&fromWhom, &fromWhomLength);
            //printf("OH DEAR! AN ERROR! : %s\n", strerror(errno));
			printf("%zd bytes received\n", bytesReceived);
            
            /* Format the received data into the IP struct, then shift bits */
            struct ip * receivedIPHeader = (struct ip *) receivedPacketBuffer;
            int headerLength = receivedIPHeader->ip_hl << 2;
            
            /* Format the received data into the ICMP struct */
            receivedICMPHeader = (struct icmp *)(receivedPacketBuffer + headerLength);
            
            /* Check if the packet was an ECHO_REPLY, and if it was meant for our computer using the ICMP id,
             which we set to the process ID */
            if (receivedICMPHeader->icmp_type == 0)
            {
                if(receivedICMPHeader->icmp_id != processID)
                {
                    printf("Not our packet\n");
                    printf("processID = %d \t ID = %d\n", processID, receivedICMPHeader->icmp_id);
                }
            }
            else
            {
                printf("Not a reply\n");
            }

		}
	}
}


/*
 
 buildPing()
 
 buildPing() initializes the ICMP Header and IP Header structs, which
 contain essential packet information. The IP address information is
 set in main().
 
 */
void buildPing()
{
	icmpHeader = (struct icmp *) packet;
    ipHeader= (struct ip *) packetIP;
	icmpHeader->icmp_type = 8; // This shouldn't change
	icmpHeader->icmp_code = 0;
	icmpHeader->icmp_cksum = 0;
	icmpHeader->icmp_seq = 1;
	icmpHeader->icmp_id = processID;
	ipHeader->ip_p = 1;
	ipHeader->ip_ttl = 64; //Recommended value, according to the internet.
	ipHeader->ip_hl = 5;
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
    
    /*
     
     Variables for command line flag processing
     
     */
	const char* destination = argv[1];
	bool timeBetweenReq = 0; // -q
    int msecsBetweenReq = 1000;
	bool timeBetweenRepReq = 0; // -b
    int msecsBetweenRepReq = 0;
	bool datagramSize = 0; // -d
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
    pingsToSend = 5; // DEFAULT VALUE of 5
	
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
                if(pingsToExclude >= pingsToSend)
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
				pingsToSend = atoi(argv[i + 1]);
				printf("Flag -n set! %d pings to be sent.\n", pingsToSend);
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
	whereto.sa_family=AF_INET;
	/*
     
     UNIX block for setting the source and destination IP address
     
     */
    #if __unix__
	socketAddress = (struct sockaddr_in *)&whereto;
	if(inet_pton(AF_INET,destination,&(socketAddress->sin_addr))!=1)
	{
		printf("inet_pton error for Socket Address\n");
        exit(0);
	}
	/*
     
     APPLE block for setting the source and destination IP address
     
     */
    #elif __APPLE__
	socketAddress = (struct sockaddr_in *)&whereto;
	if(inet_pton(AF_INET,destination,&(socketAddress->sin_addr))!=1)
	{
		printf("inet_pton error for Socket Address\n");
        exit(0);
	}
	/*
     
     WINDOWS block for setting the source and destination IP address
     
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
    
    /*
     
     Set up the sockets
     
     */
	sockaddr_in sourceSocket;
	sourceSocket.sin_addr = srcIP;
	sourceSocket.sin_family = AF_INET;
    
    /* We use the process ID from this program as our ICMP id */
	processID = getpid() & 0xFFFF;
    
    /* Create socket descriptor for sending and receiving traffic */
	int socketDescriptor = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    printf("----------------------------------\n");
    
    /* Initialize some ICMP and IP header values */
    buildPing();
    
    /* Counting variable */
    int i = 0;
    
    /*
     
     Execute the ping/listen functions with multi-threading
     
    */
    #pragma omp parallel sections
    {
        #pragma omp section
        for (i = 0; i < pingsToSend; i++)
        {
            ping(socketDescriptor, icmpPayloadLength);
            usleep(msecsBetweenReq*1000);
        }
        
        #pragma omp section
        while(1) // TODO make this timeout...
        {
            if(i >= pingsToSend-1)
            {
                break;
            }
            listen(socketDescriptor, &sourceSocket);
        }
        
    }
    
    /* Print final statistics and quit */
    report();
}
