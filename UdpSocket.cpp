// Project:      CSS432 UDP Socket Class
// Professor:    Munehiro Fukuda
// Organization: CSS, University of Washington, Bothell
// Date:         March 5, 2004
// Updated by Yang Peng on 12/10/2019

#include "UdpSocket.h"

// Constructor ----------------------------------------------------------------
UdpSocket::UdpSocket( const char* port ) : port( port ), sd( NULL_SD ) {

	struct addrinfo hints, *res, *p;
	memset(&hints, 0, sizeof(hints));		// Zero-initialize hints

	hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_DGRAM;  // use TCP
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	// call getaddrinfo to update res
	if (getaddrinfo(NULL, port, &hints, &res) < 0) {
		cerr << "Cannot resolve server info." << endl;
		return;
	}
    for(p = res; p != NULL; p = p->ai_next) {
        if ((sd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
			cerr << "server: socket" << endl;
            continue;
        }

        if (bind(sd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sd);
			sd = NULL_SD;
            cerr << "server: bind" << endl;
            continue;
        }
        break;
    }
	freeaddrinfo(res);
	if (sd < 0)  {
        cerr << "server: failed to bind" << endl;
    }

}

// Destructor -----------------------------------------------------------------
UdpSocket::~UdpSocket( ) {
	// Close the socket being used
	if ( sd != NULL_SD ) {
		close( sd );
		sd = NULL_SD;
	}
}

// Set the IP addr given a destination IP name ----------------------
bool UdpSocket::setDestAddress( const char* ipName ) {

	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints) );
	hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_DGRAM;  // use UDP

	if (getaddrinfo(ipName, port, &hints, &res) < 0) {
		cerr << "Cannot find hostname: " << ipName << endl;
		return false;                                // set in failure
	}
	
	memset(&destAddr, 0, sizeof(destAddr));
    memcpy(&destAddr, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);
	return true;                                   // set in success
}

// Check if this socket has data to receive -----------------------------------
int UdpSocket::pollRecvFrom( ) {
  struct pollfd pfd[1];
  pfd[0].fd = sd;             // declare I'll check the data availability of sd
  pfd[0].events = POLLRDNORM; // declare I'm interested in only reading from sd

  // check it immediately and return a positive number if sd is readable,
  // otherwise return 0 or a negative number
  return poll( pfd, 1, 0 );
}

// Send msg[] of length size through the sd socket ----------------------------
int UdpSocket::sendTo( char msg[], int length ) {

  // return the number of bytes sent
  return sendto( sd, msg, length, 0, &destAddr, sizeof( destAddr ) );

}

// Receive data through the sd socket and store it in msg[] of lenth size -----
int UdpSocket::recvFrom( char msg[], int length ) {
  
  // zero-initialize the srcAddr structure so that it can be filled out with
  // the address of the source computer that has sent msg[]
  socklen_t addrlen = sizeof( srcAddr );
  memset(&srcAddr, 0, addrlen );

  // return the number of bytes received
  return recvfrom( sd, msg, length, 0, &srcAddr, &addrlen );
}

// Send through the sd socket an acknowledgment in msg[] whose size is length -
int UdpSocket::ackTo( char msg[], int length ) {

  // assume that srcAddress has be filled out upon the previous recvFrom( )
  // method.

  // return the number of bytes sent
  return sendto( sd, msg, length, 0, &srcAddr, sizeof( srcAddr ) );
}
