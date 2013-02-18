# include <winsock.h>
/*
 * ePing
 *
 * NOTE: This is just me playing around with the general structure
 * of the program based on some textbook examples. As of yet, it does 
 * not necessarily follow coding standards and it probably won't even 
 * compile. Don't shoot me.
 *
 * Using the Internet Control Message Protocol (ICMP) "ECHO" facility,
 * measure round-trip-delays and packet loss across network paths.
 *
*/


/* Questions

   what do the statements after the structs mean?
*/

/*
NOTES
high level:

send ping. start listener to get incoming pings.

create socket
IPheader = 20 bytes
ICMP message = variable length
	TYPE(of ICMP message) CODE (indicate a specific condition)  CHECKSUM(depends on TYPE and CODE)
	MESSAGE

	IDENTIFIER	SEQUENCE NUMBER(this is how you identify lost packets)

TYPE 8 = request
TYPE 0 = echo reply
TYPE 3 = host unreachable
*/

/*
	IP HEADER
*/
typedef struct tagIPHeader
{
	u_char versionHeaderLength;
	u_char typeOfService;
	short totalLength;
	short ID;
	short flagsFragOff;
	u_char timeToLive;
	u_char protocol;
	u_short checksum;
	struct in_addr srcIpAddress;
	struct in_addr destIpAddress;
};
/*
	Address Info Struct
*/

struct addrinfo {
	int ai_flags; // AI_Passive
	int ai_family; // Inet? Inet6? Unspecified?
	int ai_socktype; // Stream or Datagram: SOCK_STREAM or SOCK_DIAGRAM
	int ai_protocol; //
	size_t ai_addrlen;  // Size of ai_addr in bytes
	struct sockaddr *ai_addr; // other struct
	char *ai_canonname; //full canonical hostname
	struct addrinfo *ai_next; // linked list, next node
}


/* 
	ICMP HEADER 
*/
typedef struct tagICMPHeader{
	u_char type;
	u_char code;
	u_short checksum;
	u_short identifier;
	u_short sequencenumber;
};

typedef struct tagICMPEchoRequest{
	ICMPHeader header;
	int time;
	char charfillData[REQ_DATASIZE];
};
/*
	Initialize Structs
*/

typedef struct tagICMPHeader ICMPHeader;
typedef struct tagICMPEchoRequest ICMPEchoRequest;





/*

	Main

*/
main(argc, argv)
char *argv[];
{
	socket(AF_NET, SOCK_RAW, ICMPPROTO_ICMP);
	echo_req.icmpHdr.type = ICMP_ECHOREQ;
	echo_req.icmpHdr.code = 0;
	echo_req.icmpHdr.checksum = 0;
	echo_req.icmpHdr.ID = id++;
	echo_req.icmpHdr.Seq = seq++;
	// Grab arguments from command line and set flags
	// Number of Pings
	// Packet Size
	ping();
	listen();
	report();

}

/*

	Ping()

*/
ping()
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
	// 
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
	
 	
}


/*

	Report()

*/
report()
{
	// Any missing packets?
	// Delays for each packet
	// Print it!
	

}

