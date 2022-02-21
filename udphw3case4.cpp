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
#include <iostream>
using namespace std;

/**
 * Sends the message and recieves an acknowledgment from the server (max = 20,000 times in test)
 * using the sock object. If the client cannot recieve an acknowledgement immdediately,
 * it should start a timer. If a timeout occurs (no response after 1500 usec), the
 * client must resend the message. The function will count the number of messages it
 * had to retransmit and that will be the return value
 * 
 */
int clientStopWait( UdpSocket &sock, const int max, int message[] )
{
    cout << "Entered the clientStopWait" << endl;
    int count = 0; // total number of packets we had to resend

    // Loop 20000 times to send 20000 data buffers (packets)
    for(int i = 0; i < max; i++){ 
        message[0] = i; // the sequence number will be put into the first byte of the message
        Timer t;
        bool recievedResponse = false;
        int bytesSent;
        bytesSent = sock.sendTo((char*) message, MSGSIZE); //send the message to the server

        cout << "data sent to server" << endl;
        t.start();

        // loop infinitely until we have to manually break out via a response recieved or a timeout
        // a timeout again is when we havent recieved any data back from the server in 1500 microseconds
        while(true) {
            // break case 1 - we have recieved a response
            // in this case, we can move on and send the next packet
            int bytesRecieved = sock.pollRecvFrom(); 

            if(bytesRecieved > 0){
                recievedResponse = true;
                break;
            }

            // break case 2 - we have spent over 1500 microseconds looking for the ack and no luck
            // in this case, we should increment the count of dropped packets and decrement the
            // loop because we will have to resend the data
            if(t.lap() > 1500 && !recievedResponse) 
            {
                count++; 
                i--;   
                break;
            }
        }

        // if we have recieved a response (break case 1), we must make sure it is valid
        // it is valid if the first byte matches our sequence number - the servers ack
        if(recievedResponse) {
            // read in the message from the server
            sock.recvFrom((char*) message, MSGSIZE);

            // if the first byte is bad, do the same thing as we did in break case 2
            // increment the dropped packet cont and add another iteration in the loop 
            if(message[0] != i) 
            {
                i--;    
                count++;

                // move onto the same iteration pretty much
                continue;
            }
        }

        
    }
    cout << "finishing" << endl;
    return count;
}

/**
 * The server side code for stop and wait
 * Repeats recieving message[] and sending an acknowledgment to the client
 */
void serverReliable( UdpSocket &sock, const int max, int message[] )
{
    cout << "inside serverReliable" << endl; 

    // loop 20000 times to read the 20000 buffers
    // wont have to add any iterations this time because it wont move on until it recieves a good one  
    for(int i = 0; i < max; i++) {
        while(true) {
            // first, check to see if we have got any data from the client
            int bytesRecieved = 0;
            bytesRecieved = sock.pollRecvFrom(); //Any data been recieved?
            if(bytesRecieved > 0)
            {
                // if so, check to see if the message is the correct one by checking the sequence number
                // given by the client to our current iteration (they should be the same as the window size is 1)
                sock.recvFrom((char*) message, MSGSIZE);

                if(message[0] == i)
                {
                    // if data has been recieved, send an acknowledgment with our iteration number
                    sock.ackTo((char *) &i, sizeof(i));
                    break;                
                }
            }            
         }
    }
}

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
int clientSlidingWindow( UdpSocket &sock, const int max, int message[], int windowSize )
{
    int count = 0; // the total number of retransmitted packets
    int numUnacked = 0; // the number of packets currently unacknowledged
    int lastAcked = 0; // the sequence number of the last packet that got acknowledged by the server

    // loop 20000 times to send 20000 data buffers (packets)
    for(int i = 0; i < max; i++)
    {
        // we can only send more data if we have not surpassed the window size
        // the window size is the maximum number of currently unacknowledged packets we can have
        if(numUnacked < windowSize)
        {
            // in this case, we send another packet, and increment the number of unacked packets
            // because it obviously has not been acknowledged yet but is now on its way
            numUnacked++;
            message[0] = i; // the first byte again is the sequence number
            sock.sendTo((char*) message, MSGSIZE); //send the message to the server
        }

        // if we ever have more unacknowledged packets than the allowed window size, we need to retransmit
        // that is the whole purpose of the sliding window
        if(numUnacked >= windowSize) //has to be another if statment here otherwise breaks
        {
            // start the timer immediately after sending the packet
            Timer t;
            t.start();

            // same as stop and wait, start unlimited loop until good case or timeout
            while(true)
            {
                // if we have recieved data
                if(sock.pollRecvFrom() > 0)
                {
                    sock.recvFrom((char *)message, MSGSIZE);
                    
                    // break case 1 - good case (correct ack recieved)
                    // if the first byte of the message is 1 greater than the last one we have acknowledged
                    // (the next one we are trying to acknowledge), then increment the next ack we are looking for
                    // and decrement the number of unacknowledged packets. Then we can skip the rest
                    if(message[0] == lastAcked)
                    {
                        lastAcked++; 
                        numUnacked--;    
                        break; 
                    }
                }

                // break case 2 - timout 
                // break when we havent recieved a response within 1500 microseconds
                if(t.lap() > 1500 && numUnacked == windowSize)
                {
                    // have to retransmit the entire window size on a timeout
                    int numDropped = count + (i + windowSize - lastAcked); 
                    count += numDropped;

                    // go back to the loop where we sent our first unacked packet
                    // as we need to resend all of them
                    i = lastAcked;
                    
                    // also now we obviously have no unacked packets
                    numUnacked = 0;
                    break;
                }       
            }
        }
    }
    cout << "done sending data from window client" << endl;
    return count; // return the total number of unacknowledged packets
}

/**
 * Receives a message[] and returns the acknowledgment number to the client
 * Sliding window program for the server side. Every time the server recieves a message,
 * It will save the messages sequence number into an array and return an acknowledgment
 * The acknowledment is the sequence number of the last recieved in order message 
 */
void serverEarlyRetrans( UdpSocket &sock, const int max, int message[], int windowSize )
{
    int acks[max];
    int count = 0;

    while(count != max) {
        if(sock.pollRecvFrom() > 0){
            sock.recvFrom((char*) message, MSGSIZE); 

            int chance = rand() % 10;

            if (chance < 1) {
                continue;
            }
    
            if(message[0] == count){
                
                sock.ackTo((char *) &count, sizeof(count)); 
                acks[count] = message[0];
                count++;
            }
            else
            {
                sock.ackTo((char *) &count, sizeof(count)); 
                count++;
            }
        }   
    }
    count = -1;
    sock.ackTo((char* ) &count, sizeof(count));
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
 * recvFrom()
 *    1. Calls the recvfrom() function from socket.cpp
 *       a) We give it only the size and the message
 *       b) However, the actual function needs a socket descriptor, flags, and the sending address
 *       c) Therefor, we use the professors custom function
 * 
 *   2. Output is stored in the message[] buffer we pass into it
 *      a) The result should be the number of bytes it read from the buffer if successful
 *      b) If it was not able to read any message,s it will return 0
 *      c) If the server shutdown or there was some other error, it will return -1
 * 
 */