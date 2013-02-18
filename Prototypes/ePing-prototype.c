/*  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*                    ePing: Flexible Ping Utility Prototype
*  			
*                             Ericsson                   
*                                                                       
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*   
*              Name: ePing-prototype.c
*     Creation Date: 1/2013
*            Author: Shane Reetz + Co.
*  
*       Description: This is just me learning about the network programming
*			libraries we will be using. This document attempts
*			but does not rigorously follow coding standards.
*  
* 
*	Code Review:
*  
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


/*
*
* Imports
*
*/



/*
	IP Header
*/
typedef struct tagIPHeader
{
	u_char 	versionHeaderLength;
	u_char 	typeOfService;
	short 	totalLength;
	short 	ID;
	short 	flagsFragOff;
	u_char 	timeToLive;
	u_char 	protocol;
	u_short checksum;
	struct 	in_addr srcIpAddress;
	struct 	in_addr destIpAddress;
};
/*
	Address Info Struct
*/

struct addrinfo {
	int 	ai_flags; // AI_Passive
	int 	ai_family; // Inet? Inet6? Unspecified?
	int 	ai_socktype; // Stream or Datagram: SOCK_STREAM or SOCK_DIAGRAM
	int 	ai_protocol; //
	size_t 	ai_addrlen;  // Size of ai_addr in bytes
	struct 	sockaddr *ai_addr; // other struct
	char 	*ai_canonname; //full canonical hostname
	struct 	addrinfo *ai_next; // linked list, next node
};


/* 
	ICMP HEADER 
*/
typedef struct tagICMPHeader{
	u_char 	type;
	u_char 	code;
	u_short checksum;
	u_short identifier;
	u_short sequencenumber;
};

typedef struct tagICMPEchoRequest{
	ICMPHeader 	header;
	int		time;
	char 		charfillData[REQ_DATASIZE];
};
/*
	Initialize Structs
*/

typedef struct tagICMPHeader ICMPHeader;
typedef struct tagICMPEchoRequest ICMPEchoRequest;
echo_req.icmpHdr.type = ICMP_ECHOREQ;
echo_req.icmpHdr.code = 0;
echo_req.icmpHdr.checksum = 0;
echo_req.icmpHdr.ID = id++;
echo_req.icmpHdr.Seq = seq++;

// Create a socket
socket(AF_NET, SOCK_RAW, ICMPPROTO_ICMP);

/*

	Main

*/
main(argc, argv)
char *argv[];
{
	// Grab arguments from command line and set flags
	// Number of Pings
	// Packet Size
	// How does runSequence work into here?
	buildPacket(); // get a packet ready to send. will call computeChecksum()
	send(); // send will call listen() and printPacket()
	printStatistics();
}

/*

	buildPacket()
	
*/
buildPacket()
{
	// Put everything together
}

/*

	computeChecksum()
	
*/
computeChecksum()
{
	// Compute it!
}
/*

	Send()

*/
send()
{
	// Fill in some data to send
	memset(echo_req.cData, ' ', REQ_DATASIZE);

	// Save tick count when sent (milliseconds)
	echo_req.dwTime = gettime ...;

	// Put data in packet and compute checksum
	echo_req.icmpHdr.Checksum = in_cksum(...);
	
	readfds.fd_count = 1; // set size
	readfds.fd_array[0] = raw; // socket set
	timeout.tv_sec = 10; // timeout (s)
	timeout.tv_usec = 0; // timeout (us)

	if((rc = select(1, &readfds, NULL, NULL, &timeout)) == SOCKET_ERROR){
		errexit("select() failed %d\n", perror());
	}
	/* blah = sendto(s, msg, len, flags, to, tolen) */
	sendTo(      ); // actually sends the packet
	
	// Increment sequence number
	// SET ALARM
	setAlarm();
	listen();
}


/*

	Listen()

*/
listen()
{
	/* Wait for reply... or timeout */
	
	/* Receive reply */
	
	// Get the info out of it
	
	// Was it an error packet? Uh oh!
	
	/* Lost packets: was this packet in order with the sequence? */
	printPacket();
 	
}


/*

	PrintPacket()

*/
printPacket()
{
	// Print a single line about the current packet received
	// Print it!
	

}
/*

	PrintStatistics()

*/
printStatistics()
{
	// Any missing packets?
	// Delays for each packet
	// Print it!
	

}

