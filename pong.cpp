/*
 
 Pong
 
 Using the Internet Control Message Protocol (ICMP) ECHO_REQUEST
 and ECHO_REPLY messages to measure round-trip-delays and packet
 loss across network paths. Eventually, we'd like to expand
 this to UDP pinging as well.
 
 Created as a 2013 NDSU Capstone Project with specifications
 and requirements provided by Ericsson.
 
 An extension/reimplementation of the original ping.c for the 21st
 century.
 Emphasis is on readability and ease of understanding the code.
 Inspiration and design inspired by:
 
 Mike Muuss
 U. S. Army Ballistic Research Laboratory
 December, 1983
 Modified at UC Berkeley
 
 */



/*
 
 Imports and definitions
 
 */

#define IP_MINLENGTH 34
#define ICMP_MINLENGTH 16
#define DEFAULT_LISTEN_TIMEOUT 2
#include <omp.h>
#include <math.h>
#include <iostream>
#include <random>
#include <fstream>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#if __unix__||__APPLE__
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#elif WIN32
#include <windows.h>
#include <winsock2.h>
struct sockaddr_in *socketAddress;
/* ICMP Header for Windows branch */
struct icmp {
    BYTE icmp_type;          // ICMP packet type
    BYTE icmp_code;          // Type sub code
    USHORT icmp_cksum;
    USHORT icmp_id;
    USHORT icmp_seq;
    ULONG icmp_data;    // not part of ICMP, but we need it
};
#endif

/*
 
 Initialize Structs
 
 */

struct sockaddr_in *whereto;
struct in_addr destIP;
struct in_addr srcIP;
struct icmp * icmpHeader;
struct icmp * receivedICMPHeader;
struct icmp * receivedIPHeader;
struct ip * ipHeader;
u_char * packet[100];
u_char * packetIP[100];

/*
 
 Global Variables for stats and keeping track
 of how many pings were sent, how many need
 to be excluded, and more.
 
 */
int sent;
int packetsTimedOut = 0;
int processID;
int icmpPayloadLength = 30;
int pingsToSend = 5;
int pingsSent = 0;
int pingsToExclude = 0;
double totalResponseTime = 0.0;
double sumOfResponseTimesSquared = 0.0;
double roundTripTime = 0.0;
int pingsReceived = 0;
bool excludingPing;
int timeoutInput = DEFAULT_LISTEN_TIMEOUT;
volatile bool killSwitch = true;

/*
 
 Variables for CSV file
 
 */
using namespace std;
ofstream csvOutput;
string csvFileName = "output.csv";

/*
 
 Time settings
 
 */

#if __MACH__
clock_serv_t cclock;
mach_timespec_t sentTime, receivedTime;
#elif __WINDOWS__
#elif __GNUC__
struct timespec sentTime2, receivedTime2;
#endif



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
    
    /* Mop up an odd byte, if necessary */
    if (nleft == 1)
    {
        *(u_char *)(&answer) = *(u_char *) ICMPPointer;
        sum += answer;
    }
    
    /* Add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
    sum += (sum >> 16);                     /* add carry */
    answer = (u_short)~sum;                 /* truncate to 16 bits */
    return(answer);
}

/*
 
 pingICMP()
 
 This method actually sends our ECHO_REQUEST across the internet.
 It computes the ICMP checksum and sends the packet to the address
 specified in main().
 
 */
void pingICMP(int socketDescriptor, int icmpPayloadLength)
{
    /* Get time, put it in the packet */
    
    /* Set time sent */
#ifdef __MACH__
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
    clock_get_time(cclock, (mach_timespec_t *)icmpHeader->icmp_data);
    mach_port_deallocate(mach_task_self(), cclock);

#elif __WINDOWS__
    //  GetTick64Count()
#elif __GNUC__
    clock_gettime(CLOCK_MONOTONIC, (struct timespec *)icmpHeader->icmp_data);
#else
    //clock_gettime(CLOCK_REALTIME, &ts);
#endif
    
    /* Compute checksum */
	icmpHeader->icmp_cksum = 0;
	icmpHeader->icmp_cksum = checksum((u_short *)icmpHeader, sizeof(*icmpHeader));
    
	/* Try to send the packet */
	sent = sendto(socketDescriptor, packet, icmpPayloadLength + 8, 0, (struct sockaddr *)whereto, sizeof(struct sockaddr));
    
    /* Check if the packet sent or not */
	if(sent > 0)
	{
        pingsSent++;
        /* Increment the packet sequence number */
        icmpHeader->icmp_seq++;
	}
	else
	{
		printf("Ping not sent.\n\n");
	}
}

/*
 
 report()
 
 This function reports the final statistics, either to the command line
 or to a CSV (comma separated value) file. Current statistics and output
 information includes pings sent, pings received, pings dropped, pings excluded, 
 average response time, and standard deviation for response time.
 
 */
void report()
{
    if(pingsSent != 0)
    {
        printf("----------------------------------------------------------------\n");
        printf("%d packets sent, %d dropped", pingsSent, (pingsSent - pingsReceived));
        if(excludingPing)
        {
            printf(", %d excluded from summary\n", pingsToExclude);
        }
        else
        {
            printf("\n");
        }
        double average = totalResponseTime / (pingsSent - pingsToExclude);
        printf("Stats avg/stddev : %f / %f\n", average, sqrt((sumOfResponseTimesSquared / (pingsReceived - pingsToExclude)) - (average * average)));
        printf("----------------------------------------------------------------\n");
    }
}

/*
 
 listenICMP()
 
 This function waits from an incoming packet, checks to see if it is our ECHO_REPLY,
 and then gets data out of the packet and computes statistics or doesn't (depending 
 on if we're excluding that response or not).
 
 */
void listenICMP(int socketDescriptor, sockaddr_in * fromWhom, bool quiet, bool excludingPings, int timeoutLength)
{
	/* Setting some variables needed for select() and our file descriptor */
	char receivedPacketBuffer[512];
	fd_set readfds;
	FD_SET(socketDescriptor, &readfds);
	struct timeval timeout;
	timeout.tv_sec = timeoutLength; // timeout period, seconds (added second, if that matters)
    //printf("Timeout length")
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
		//killSwitch = true;
		printf("I'm tired of waiting. Timeout.\n");
        packetsTimedOut++;
		csvOutput << "Dropped" << endl;
        return;
	}
	else
	{
		if(FD_ISSET(socketDescriptor, &readfds))
		{
            
            /* Receive the data */
			ssize_t bytesReceived = recvfrom(socketDescriptor, receivedPacketBuffer, sizeof(receivedPacketBuffer), 0, (struct sockaddr *)&fromWhom, &fromWhomLength);
            
            /* Format the received data into the IP struct, then shift bits */
            struct ip * receivedIPHeader = (struct ip *) receivedPacketBuffer;
            int headerLength = receivedIPHeader->ip_hl << 2;
            
            /* Format the received data into the ICMP struct */
            receivedICMPHeader = (struct icmp *)(receivedPacketBuffer + headerLength);
            
            /* Get the time */
#if __MACH__
            clock_get_time(cclock, &receivedTime);
            mach_timespec_t * sentTime = (mach_timespec_t *)receivedICMPHeader->icmp_data;
            /* Thanks Richard Stevens' book UNIX Network Programming for helping with
             this next chunk of time processing code */
            if ( (receivedTime.tv_nsec -= sentTime->tv_nsec) < 0)
            {
                --receivedTime.tv_sec;
                receivedTime.tv_nsec += 1000000000;
            }
            
#elif __WINDOWS__
            //  GetTick64Count()
#elif __GNUC__
            clock_gettime(CLOCK_MONOTONIC, &receivedTime2);
            struct timespec * sentTime2 = (struct timespec *)receivedICMPHeader->icmp_data;
            /* Thanks Richard Stevens' book UNIX Network Programming for helping with
             this next chunk of time processing code */
            if ( (receivedTime2.tv_nsec -= sentTime2->tv_nsec) < 0)
            {
                --receivedTime2.tv_sec;
                receivedTime2.tv_nsec += 1000000000;
            }
            
#endif
            
            /* Check if the packet was an ECHO_REPLY, and if it was meant for our computer using the ICMP id,
             which we set to the process ID */
            if (receivedICMPHeader->icmp_type == 0)
            {
                
                if(receivedICMPHeader->icmp_id != processID)
                {
                    printf("Not our packet\n");
                    printf("processID = %d \t ID = %d\n", processID, receivedICMPHeader->icmp_id);
                }
                else
                {
                    /* We got a valid reply. Count it! */
                    pingsReceived++;
                    
                    /* Calculate round trip time */
                    if(!excludingPings)
                    {
#if __MACH__
                        receivedTime.tv_sec -= sentTime->tv_sec;
                        roundTripTime = receivedTime.tv_sec * 1000.0 + receivedTime.tv_nsec / 1000000.0;
#elif __GNUC__
                        receivedTime2.tv_sec -= sentTime2->tv_sec;
                        roundTripTime = receivedTime2.tv_sec * 1000.0 + receivedTime2.tv_nsec / 1000000.0;
#endif
                        sumOfResponseTimesSquared += roundTripTime * roundTripTime;
                        totalResponseTime += roundTripTime;
                    }
                    
                    /* Get presentation format of source IP */
                    char str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(receivedIPHeader->ip_src), str, INET_ADDRSTRLEN);
                    
                    /* If we're counting stats for this reply, add data to running stats */
                    if(!excludingPings)
                    {
                        /* If we're in CSV mode, print info to the file instead of to the terminal */
                        if(quiet)
                        {
                            printf(".\n");
                            csvOutput << (bytesReceived + 14) << "," << str << "," << receivedICMPHeader->icmp_seq << "," << (int)receivedIPHeader->ip_ttl << "," << roundTripTime << endl;
                        }
                        /* Otherwise, print received reply information to the terminal */
                        else
                        {
                            printf("%d bytes from %s packet number:%d  ttl:%d  time:%f ms\n", (bytesReceived + 14), str, receivedICMPHeader->icmp_seq, (int)receivedIPHeader->ip_ttl, roundTripTime);
                        }
                    }
                }
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


void interrupt(int signal);
/*
 
 main()
 
 Where the magic happens. Command line flags are set, program
 control is set.
 
 */
char *argv[2];
int main(int argc, const char** argv)
{
	signal(SIGINT, interrupt);
#ifdef WIN32
	WSADATA wsaData;
	int result=WSAStartup(MAKEWORD(2,0), &wsaData);
	if(result != 0) {
        printf("WSAStartup failed with error: %d\n", wsaData);
        return 1;
#endif
        printf("----------------------------------------------------------------\n");
        
        int socketDescriptor;
        
        /*
         
         Variables for command line flag processing
         
         */
        const char* destination = argv[1];
        bool timeBetweenReq = 0; // -q // --request-time
        float msecsBetweenReq = 1000;
        bool timeBetweenRepReq = 0; // -b // --reply-request-time
        int msecsBetweenRepReq = 0;
        bool datagramSizeSet = 0; // -d // --datagram-size
        bool payloadSize = 0; // -p // --payload-size
        bool randSizeMinMax = 0; // -l // --random-size-minmax
        int bytesSizeMin = 0;
        int bytesSizeMax = 0;
        bool randSizeAvgStd = 0; // -r // --random-size-avgstd
        int bytesSizeAvg = 0;
        int bytesSizeStd = 0;
        bool randTimeMinMax = 0; // -s // --random-time-minmax
        int msecsTimeMin = 0;
        int msecsTimeMax = 0;
        bool randTimeAvgStd = 0; // -t // --random-time-avgstd
        int msecsTimeAvg = 0;
        int msecsTimeStd = 0;
        bool increasingSize = 0; // -i // --increasing-size
        int sizeInitial = 0; // Must be greater than or equal to (IP_MINLENGTH + ICMP_MINLENGTH)
        int sizeGrowth = 0;
        excludingPing = 0; // -e // --exclud
        bool multiplePings = 0; // -n // --ping-count
        bool csvMode = 0; // -c // --csv
        
        if(argc - 1 == 0)
        {
            printf("USAGE:\nePing [host IP or domain name]\n[-n/--pings-to-send num of pings to send]\n[-e/--exclude num of pings to exclude from stats]\n\nTIME\n[-q/--request-time time between sent pings]\n[-t/--random-time-avgstd set random, normally distributed time between pings - enter average and standard deviation]\n[-z/--timeout manually set timeout for listening method]\n\nSIZE\n[-p/--payload-size ICMP payload size] OR [-d/--datagram-size size of whole packet]\n[-r/--random-size-avgstd set random, normally distributed packet sizes - enter average and standard deviation]\n\nOUTPUT\n[-c/--csv output reply information to a csv file]\n");
            printf("----------------------------------------------------------------\n");
            return(1);
        }
        else
        {

            /* Check if we're root. If not, we can't create the raw socket necessary for ICMP */
            if(getuid()!=0 && geteuid()!=0)
            {
                printf("UID: %d EUID: %d", getuid(), geteuid());
                printf("\nCan't run. I need root permissions to create raw socket. Sorry.\n");
                printf("----------------------------------------------------------------\n");
                return(1);
            }
            
            /* Create socket descriptor for sending and receiving traffic */
            socketDescriptor = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
            
            /* Drop root permissions */
            seteuid(501);
            setuid(getuid());

        }
        for(int i = 2; i < argc; i++) {
            // argv[0] is the ./a which is input
            // argv[1] is the address (IPv4, DNS, IPv6)
            if(strcmp(argv[i],"-q") == 0 || strcmp(argv[i],"--request-time") == 0)
            {
                if(timeBetweenRepReq || randTimeMinMax || randTimeAvgStd)
                {
                    printf("-q flag not set, conflicting with previously set flag.\n");
                }
                else
                {
                    if(i + 1 < argc && atoi(argv[i + 1]) >= 0)
                    {
                        timeBetweenReq = true;
                        msecsBetweenReq = atoi(argv[i + 1]);
                        printf("Flag -q set! Waiting %f milliseconds between ping requests.\n", msecsBetweenReq);
                        i++;
                    }
                    else
                    {
                        printf("-q flag not set, requires parameter.\n");
                    }
                }
            }
            else if(strcmp(argv[i],"-b") == 0 || strcmp(argv[i],"--reply-request-time") == 0)
            {
                if(timeBetweenReq || randTimeMinMax || randTimeAvgStd)
                {
                    printf("-b flag not set, conflicting with previously set flag.\n");
                }
                else
                {
                    if(i + 1 < argc && atoi(argv[i + 1]) >= 0)
                    {
                        timeBetweenRepReq = true;
                        msecsBetweenRepReq = atoi(argv[i + 1]);
                        printf("Flag -b set! Waiting %d milliseconds between receiving a reply and sending a request\n", msecsBetweenRepReq);
                        i++;
                    }
                    else
                    {
                        printf("-b flag not set, requires parameter.\n");
                    }
                }
            }
            else if(strcmp(argv[i],"-d") == 0 || strcmp(argv[i],"--datagram-size") == 0)
            {
                if(payloadSize || randSizeMinMax || randSizeAvgStd)
                {
                    printf("-d flag not set, conflicting with previously set flag.\n");
                }
                else
                {
                    if(i + 1 < argc && atoi(argv[i + 1]) > 0)
                    {
                        datagramSizeSet = true;
                        icmpPayloadLength = atoi(argv[i + 1]) - ICMP_MINLEN - IP_MINLENGTH;
                        if(icmpPayloadLength < 8)
                        {
                            printf("Error: datagram size must be greater than 50 bytes.\n");
                            printf("----------------------------------------------------------------\n");
                            return(1);
                        }
                        printf("Flag -d set! Datagram will be %d bytes large.\n", icmpPayloadLength+IP_MINLENGTH+ICMP_MINLEN);
                        i++;
                    }
                    else
                    {
                        printf("-d flag not set, requires parameter.\n");
                    }
                }
            }
            else if(strcmp(argv[i],"-p") == 0 || strcmp(argv[i],"--payload-size") == 0)
            {
                if(datagramSizeSet || randSizeMinMax || randSizeAvgStd)
                {
                    printf("-p flag not set, conflicting with previously set flag.\n");
                }
                else
                {
                    if(i + 1 < argc && atoi(argv[i + 1]) >= 0)
                    {
                        payloadSize = true;
                        icmpPayloadLength = atoi(argv[i + 1]);
                        if(icmpPayloadLength < 8)
                        {
                            printf("Error: Payload size must be 8 bytes or greater.\n");
                            printf("----------------------------------------------------------------\n");
                            return(1);
                        }
                        printf("Flag -p set! Payload size will be %d bytes large.\n", icmpPayloadLength);
                        i++;
                    }
                    else
                    {
                        printf("-p flag not set, requires parameter.\n");
                    }
                }
            }
            else if(strcmp(argv[i],"-l") == 0 || strcmp(argv[i],"--random-size-minmax") == 0)
            {
                if(datagramSizeSet || payloadSize || increasingSize || randSizeAvgStd)
                {
                    printf("-l flag not set, conflicting with previously set flag.\n");
                }
                else
                {
                    if(i + 2 < argc && atoi(argv[i + 2]) > 0)
                    {
                        randSizeMinMax = true;
                        if(atoi(argv[i + 1]) > atoi(argv[i + 2]))
                        {
                            bytesSizeMin = atoi(argv[i + 2]);
                            bytesSizeMax = atoi(argv[i + 1]);
                        }
                        else
                        {
                            bytesSizeMin = atoi(argv[i + 1]);
                            bytesSizeMax = atoi(argv[i + 2]);
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
            else if(strcmp(argv[i],"-r") == 0 || strcmp(argv[i],"--random-size-avgstd") == 0)
            {
                if(datagramSizeSet || payloadSize || increasingSize || randSizeMinMax)
                {
                    printf("-r flag not set, conflicting with previously set flag.\n");
                }
                else
                {
                    if(i + 2 < argc && atoi(argv[i + 2]) > 0)
                    {
                        randSizeAvgStd = true;
                        bytesSizeAvg = atoi(argv[i + 1]);
                        bytesSizeStd = atoi(argv[i + 2]);
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
            else if(strcmp(argv[i],"-s") == 0 || strcmp(argv[i],"--random-time-minmax") == 0)
            {
                if(timeBetweenReq || timeBetweenRepReq || randTimeAvgStd)
                {
                    printf("-s flag not set, conflicting with previously set flag.\n");
                }
                else
                {
                    if(i + 2 < argc && atoi(argv[i + 2]) > 0)
                    {
                        randTimeMinMax = true;
                        if(atoi(argv[i + 1]) > atoi(argv[i + 2]))
                        {
                            msecsTimeMin = atoi(argv[i + 2]);
                            msecsTimeMax = atoi(argv[i + 1]);
                        }
                        else
                        {
                            msecsTimeMin = atoi(argv[i + 1]);
                            msecsTimeMax = atoi(argv[i + 2]);
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
            else if(strcmp(argv[i],"-t") == 0 || strcmp(argv[i],"--random-time-avgstd") == 0)
            {
                if(timeBetweenReq || timeBetweenRepReq || randTimeMinMax)
                {
                    printf("-t flag not set, conflicting with previously set flag.\n");
                }
                else
                {
                    if(i + 2 < argc && atoi(argv[i + 2]) > 0)
                    {
                        randTimeAvgStd = true;
                        msecsTimeAvg = atoi(argv[i + 1]);
                        msecsTimeStd = atoi(argv[i + 2]);
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
            else if(strcmp(argv[i],"-i") == 0 || strcmp(argv[i],"--increasing-size") == 0)
            {
                if(datagramSizeSet || payloadSize || randSizeMinMax || randSizeAvgStd)
                {
                    printf("-i flag not set, conflicting with previously set flag.\n");
                }
                else
                {
                    if(i + 2 < argc && atoi(argv[i + 2]) > 0)
                    {
                        increasingSize = true;
                        sizeInitial = atoi(argv[i + 1]);
                        sizeGrowth = atoi(argv[i + 2]);
                        if(sizeInitial >= (IP_MINLENGTH + ICMP_MINLENGTH))
                        {
                            printf("Flag -i set! Pings will have an initial size of %d ", sizeInitial);
                            printf("and grow at a rate of %d per request.\n", sizeGrowth);
                            
                            //Subtract growth from initial once so when we ping, we can add sizeGrowth to it every time,
                            //and initialGrowth is still proper
                            icmpPayloadLength = sizeInitial - sizeGrowth - IP_MINLENGTH - ICMP_MINLENGTH + 8;
                        }
                        else
                        {
                            printf("Problem: Initial size must be greater than IP header size plus the ICMP header size (%d).\n", IP_MINLENGTH + ICMP_MINLENGTH);
                            return(1);
                        }
                        i += 2;
                    }
                    else
                    {
                        printf("-i flag not set, requires parameters.\n");
                    }
                }
            }
            else if(strcmp(argv[i],"-e") == 0 || strcmp(argv[i],"--exclude") == 0)
            {
                if(i + 1 < argc && atoi(argv[i + 1]) > 0)
                {
                    excludingPing = true;
                    pingsToExclude = atoi(argv[i + 1]);
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
            else if(strcmp(argv[i],"-n") == 0 || strcmp(argv[i],"--pings-to-send") == 0)
            {
                if(i + 1 < argc && atoi(argv[i + 1]) > 0)
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
            else if(strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--csv") == 0)
            {
                csvMode = 1;
                if(i + 1 < argc)
                {
                    csvFileName = argv[i+1];
                    i++;
                }
                else
                {
                    csvFileName = "output.csv";
                }
                /* Open the CSV file */
                csvOutput.open(csvFileName);
                printf("Flag -c set! Replies will be output to %s.\n", csvFileName.c_str());
            }
            else if(strcmp(argv[i], "-z") == 0 || strcmp(argv[i], "--timeout") == 0)
            {
                if(i + 1 < argc && atoi(argv[i + 1]) > 0)
                {
                    timeoutInput = atoi(argv[i + 1]);
                    i++;
                }
  
                printf("Flag -z set! Listening timeout will be set to %d seconds.\n", timeoutInput);
            }
            else
            {
                printf("Flag not recognized, \"%s\"\n",argv[i]);
            }
        }
        printf("Destination set to: %s\n", argv[1]);
        
        struct addrinfo *result, hints;
        hints.ai_family = AF_INET;    /* Allow IPv4 or IPv6 */
        hints.ai_socktype = SOCK_RAW; /* RAW socket */
        hints.ai_flags = AI_PASSIVE;  /* Fill in my IP address */
        int status;
        
        /* Convert address */
        if((status = getaddrinfo(destination, NULL, &hints, &result)) != 0)
        {
            printf("getaddrinfo error: %s\n", gai_strerror(status));
            printf("Double check the address that you want to ping.\n");
            exit(1);
        }
        
        whereto = (struct sockaddr_in *)result->ai_addr;
        
        
        /*
         
         WINDOWS block for setting the source and destination IP address
         
         */
#if WIN32
        sockAddress = &whereto;
        int sizeOfAddress=sizeof(IPHeader.destinationIPAddress);
        if(WSAStringToAddress((char *)destination,AF_INET,NULL,(LPSOCKADDR)&IPHeader.destinationIPAddress.S_un.S_un_W,&sizeOfAddress)!=0)
        {
            int error=WSAGetLastError();
            std::cout<<error<<std::endl;
            std::cout<<sizeOfAddress<<std::endl;
        }
        if(WSAStringToAddress((char*)destination,AF_INET,NULL,(LPSOCKADDR)&(socketAddress->sin_addr),(int*)sizeof(socketAddress->sin_addr))!=0)
        {
            int error=WSAGetLastError();
            std::cout<<error<<std::endl;
        }
#endif
        
        /*
         
         Set up the listening sockaddr_in struct
         
         */
        sockaddr_in sourceSocket;
        sourceSocket.sin_addr = srcIP;  
        sourceSocket.sin_family = AF_INET;
        
        /* We use the process ID from this program as our ICMP packet id */
        processID = getpid() & 0xFFFF;
        
        printf("----------------------------------------------------------------\n");
        
        /* Initialize some ICMP and IP header values */
        buildPing();
        
        /* Just a counting variable */
        int i = 0;
        
        /* Variables and generators for random size/time */
        double randomDatagramSize;
        double randomTime;
        std::random_device rd;
        std::default_random_engine generator( rd() );
        std::normal_distribution<double> distribution(bytesSizeAvg, bytesSizeStd);
        std::normal_distribution<double> distribution2(msecsTimeAvg, msecsTimeStd);
        
        
        /* Specify that we want two threads (one for listening, one for sending) */
        omp_set_num_threads(2);
        
        /*
         
         Execute the ping/listen functions in sequential mode (wait for reply before sending request)
         
         */
        if(timeBetweenRepReq)
        {
            while(pingsSent < pingsToSend)
            {
                
                pingICMP(socketDescriptor, icmpPayloadLength);
                listenICMP(socketDescriptor, &sourceSocket, 0, 0, timeoutInput);
                usleep(msecsBetweenRepReq * 1000);
            }
            
        }
        
        /*
         
         Execute the ping/listen functions in parallel with OpenMP threading
         
         */
        else
        {
            
#pragma omp parallel sections
            {
                
                /* Sending block */
#pragma omp section
                for (i = 0; i < pingsToSend; i++)
                {
                    /* Ping */
                    if(randSizeAvgStd)
                    {
                        randomDatagramSize = distribution(generator);
                        pingICMP(socketDescriptor, randomDatagramSize - IP_MINLENGTH - ICMP_MINLEN);
                    }
                    else if(increasingSize)
                    {
                        icmpPayloadLength += sizeGrowth;
                        pingICMP(socketDescriptor, icmpPayloadLength);
                    }
                    else
                    {
                        pingICMP(socketDescriptor, icmpPayloadLength);
                    }
                    
                    /* Wait */
                    if(randTimeAvgStd)
                    {
                        randomTime = distribution2(generator);
                        usleep(randomTime * 1000);
                    }
                    else
                    {
                        usleep(msecsBetweenReq * 1000);
                    }
                }
                
                /* Listening block */
#pragma omp section
                while(i != pingsToSend - 1 && pingsToSend != pingsReceived + packetsTimedOut && killSwitch)
                {
                    
                    /* If we're excluding some pings, listen but don't print any info */
                    if(excludingPing && pingsReceived < pingsToExclude)
                    {
                        listenICMP(socketDescriptor, &sourceSocket, 1, 1, timeoutInput);
                    }
                    
                    /* If we're outputting to a CSV file, don't print out so much on the terminal */
                    else if(csvMode)
                    {
                        listenICMP(socketDescriptor, &sourceSocket, 1, 0, timeoutInput);
                    }
                    
                    /* Otherwise, print all statistics and information to the terminal */
                    else
                    {
                        listenICMP(socketDescriptor, &sourceSocket, 0, 0, timeoutInput);
                    }
                    
                }
                
            }
        }
        
        /* Print final statistics and quit */
        report();
        if(csvMode)
        {
            csvOutput.close();
        }
#ifdef WIN32
        int result = WSACleanup();
#endif
        return(0);
    }
    
    void interrupt(int signal)
    {
		if(killSwitch)
		{
		//printf("INTERRUPT THIS, BITCH\n");
		//printf("pingsToSend = %d\npingsSent = %d\n", pingsToSend, pingsSent);
		pingsToSend = 0;
		killSwitch = false;
		//report();
		}
	}
