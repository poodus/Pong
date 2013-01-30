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
socket(AF_NET, SOCK_RAW, ICMPPROTO_ICMP);
/*
	IP HEADER
*/
typedef struct tagIPHDR
{
	u_char VIHL;
	u_char TOS;
	short TotLen;
	short ID;
	short FlagOff;
	u_char TTL;
	u_char Protocol;
	u_short Checksum;
	struct in_addr iaSrc;
	struct in_addr iaDst;
} IPHDR, *PIPHDR;

/* 
	ICMP HEADER 
*/
typedef struct tagICMPHDR{
	u_char type;
	u_char code;
	u_short checksum;
	u_short identifier;
	u_short sequence number;
} ICMPHDR, *PICMPHDR

typedef struct icmpEchoRequest{
	icmpHeader header;
	int time;
	charfillData;
} 
echo_req.icmpHdr.Type = ICMP_ECHOREQ;
echo_req.icmpHdr.Code = 0;
echo_req.icmpHdr.Checksum = 0;
echo_req.icmpHdr.ID = id++;
echo_req.icmpHdr.Seq = seq++;




/*

	Main

*/
main(argc, argv)
char *argv[];
{
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
}


/*

	Listen()

*/
listen()
{
	/* Wait for reply... or timeout */
	
	/* Receive reply */
	/* Lost packets: did you get a sequence without missing numbers? */
 	
}


/*

	Report()

*/
report()
{

}

