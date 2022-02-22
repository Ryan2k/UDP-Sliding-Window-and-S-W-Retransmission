/**
 * This file is created to run two test cases which will be ran from hw3.cpp
 * The first test case is created in udp.cpp so this file will run cases 2 and 3
 * 
 * Test Case 2 - Implements the stop and wair algorithm where
 *  1. The client writes a message sequence number in the first bits of the message
 *  2. Sends the message
 *  3. Waits until it recieves an integer acknowledment of that sequence number from a server
 *     - While the server is recieving the message, it copies the sequence number from
 *       the client and returns it as an acknowledgment message
 *  Does this in the 2 methods: clientStopWait, and serverReliable
 */

#include "udphw3.h"

/**
 * Sends the message and recieves an acknowledgment from the server (max = 20,000 times in test)
 * using the sock object. If the client cannot recieve an acknowledgement immdediately,
 * it should start a timer. If a timeout occurs (no response after 1500 usec), the
 * client must resend the message. The function will count the number of messages it
 * had to retransmit and that will be the return value
 * 
 */
int clientStopWait(UdpSocket &sock, const int max, int message[]) {
    int count = 0; // the number of retransmittions (what we will return)

    // each loop sends a packet and waits for a response before it can continue
    for (int i = 0; i < max; i++){
        message[0] = i; // the message starts with the sequence number

        bool gotResponse = false; // set to true if we have got a response (stays false on timeout)

        Timer t; // create a new timer at each iteration as it is easier this way

        // send the message array to the socket (which will have the destination already set in hw3.cpp)
        // it will return the number of bytes sent in the message
        // The third variable is the size of the message (defined as 1460 bytes in udpsocket.h)
        int numBytesSent = sock.sendTo((char*) message, MSGSIZE);

        t.start(); // immediately after starting to send data, start the timer (timout if goes over 1500 usec)

        // break this once we have recieved a response or gone over 1500usec in the timer
        while (true) {
            // Break Case 1 - If we have recieved data
            int dataSize = sock.pollRecvFrom();

            if (dataSize > 0) {
                gotResponse = true; // we have successfully recieved a response
                break; // there are written bytes from the other side of the socket
            }

            // Break Case 2 - timed out and need to resubmit

            if (t.lap() > 1500) {
                count++; // we need to retransmit this packet
                i--; // we need another loop to resend this packet and still send the rest
                break;
            }
        }

        // if we got a response, we dont have to retransmit this packet
        if (gotResponse) {
            continue;
        }
    }

    return count;

}

/**
 * The server side code for stop and wait
 * Repeats recieving message[] and sending an acknowledgment to the client
 */
void serverReliable(UdpSocket &sock, const int max, int message[]) {
    // for each of the 20,000 packets
    for (int i = 0; i < max; i++) {

        // infinite loop breaking when we recieve data from the client
        // this way we only have to loop exactly 20,000 times in the outside loop
        // because we dont break and the retransmission is done on the client side
        while (true) {
            int bytesRecieved = 0;

            bytesRecieved = sock.pollRecvFrom();

            // if there is any recieved data
            if (bytesRecieved > 0) {
                sock.recvFrom((char*) message, MSGSIZE);

                // send the acknowledement if the data has been recieved
                if (message[0] == i) {
                    sock.ackTo((char*) &i, sizeof(i));
                    break;
                }
            }
        }
    }
}

/**
* Case 3 - Sliding window
* First number in message holds the sequence number on the clients side
* and first number in the message holds the acknowledgment number on the servers

* Client - keeps writing message sequence number in message[0] and sending message[]
* as long as the number of in transit messages is less than a given window size
*
* Server - recieves the message[], saves the sequence number of the message in a
* local buffer (which it gathered from message[0]), and returns an acknowledgement,
* which includes the sequence number of the last recieved in-order message
*/

/**
* Sends the message[] and recieves an ack from the server using sock. 
* Does this "max" times. In the test cases, we will pass in 20,000 as max.
* The client can keep sending messages as long as there are less packets in
* transmission than the windowSize value. The inTrans variable keeps track of this
* as it contains the number of unacknowledged packets. This number is incremented every
* time a packet is sent and decremented every time an acknowledgement is recieved.
* If this number serpasses the window size, the client will start a timer, and if the timer
* passes 1500 usec, a timeout has occured and it should resend the message with the minimum 
* sequence number amongst those who have not yet been acked. It returns the number of
* messages which got retransmitted.
*/
int clientSlidingWindow(UdpSocket &sock, const int max, int message[], int windowSize) {
    int count = 0; // counts the number of retransmissions and returns it at the end
    int inTrans = 0; // counts the number of unacknowledged messages - decrements every time we get an ack, used to compare to window size
    int ACKed = 0; // counts the number of acknowledged messages

    for (int i = 0; i < max; i++) {
        // Step 1 - If the number of messages in transmission is less than the window size, send a new message
        if (inTrans < windowSize) {
            message[0] = i; // the sequence number of the current packet will be the number of packets sent before it
            sock.sendTo((char*) message, MSGSIZE); // send the message to the server
            inTrans++; // another message has been sent but we obviously havent recieved an ack for it so increment
        }

        // Step 2 - If the number of in transmission packets is now equal or greater than the window size,
        // should start a timer, and if it passes 1500usec, a timeout has occured

        if (inTrans >= windowSize) {
            // start the timer
            Timer t;
            t.start();

            // keep an infinite loop, and break if it meets the condition where it has been 1500usec and no response from server
            while (true) {
                // check to see if there are any bytes written from the server (i.e, message length is less than 1)
                int bytesRecieved = sock.recvFrom();

                if (bytesRecieved < 1) {

                }
            }
        }
    }
}





/**
 *          NOTES
 * 
 * UdpSocket() Class
 *    1. It is a custom class given by the professor
 *       a) Allows us to send packets that are like UDP in real life
 *       b) 
 * 
 *    2. Constructor
 *       a) Uses that addrinfo object and socket we have been using to create a connection
 *       b) Instead of using a sock stream like in the past, we are delcaring it as a datagram
 *       c) If successful, holds on to variables of port number and socket descriptor
 *       d) Sets up like the server side did telling the kernel to bind the ip and port to the sd
 * 
 *    3. SetDestAddress
 *       a) Sets up the socket to have a connection between this process and the one we are sending it to
 *    
 *    4. 
 * 
 */

