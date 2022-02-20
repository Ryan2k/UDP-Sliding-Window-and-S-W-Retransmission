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
 *
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
 *
 * void serverReliable(UdpSocket &sock, const int max, int message[]) {
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
*
int clientSlidingWindow(UdpSocket &sock, const int max, int message[], int windowSize) {
    int count = 0; // counts the number of retransmissions and returns it at the end
    int inTrans = 0; // counts the number of unacknowledged messages - decrements every time we get an ack, used to compare to window size
    int lastAck= 0; // stores the sequence number of the last acked message

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
                // check to see if any data has been recieved from the server
                if (sock.pollRecvFrom() > 0) {
                    // read in the message from the server - results now stored in message[0]
                    sock.recvFrom((char*)message, MSGSIZE);

                    // if the return value is -1, the server shut down, so just break out of everything as there is an error
                    if (message[0] < 0) {
                        i = max;
                        break;
                    }

                    // if the return value has the correct ack number in the first byte, it was successful so we can move on
                    if (message[0] == lastAck) {
                        lastAck++; // one more packet has been acked
                        inTrans--; // there is now one less packet in transmission as its done
                        break; // we can move on
                    }

                    // if the timer gets to 1500usec without recieving any response, there was a timeout and need to retransmit
                    if (t.lap() > 1500) {
                        int currWindowRetrans = (windowSize - lastAck) + i;
                        count += currWindowRetrans; // add the current number of retransmissions needed to the total count

                        i = lastAck; // need to go back and retransmit all the packets starting after the last acked one (the loop will increment by one)
                        inTrans = 0; // now there are no more packets in transmission
                        break; // can now continue
                    }
                }
            }
        }
    }

    return count;
}

/**
 * Receives a message[] and returns the acknowledgment number to the client
 * Sliding window program for the server side. Every time the server recieves a message,
 * It will save the messages sequence number into an array and return an acknowledgment
 * The acknowledment is the sequence number of the last recieved in order message 
 *
void serverEarlyRetrans(UdpSocket &sock, const int max, int message[], int windowSize) {
    int currACK; // keeps track of the last messages ack sequence number to send as an acknowledment
    int acks[max]; // holds all the sequence numbers of the packets recieved

    while (currACK < max) {
        // Step 1 - Recieve the message from the socket
        sock.recvFrom((char*)message, MSGSIZE);

        if (message[0] == currACK) {
            acks[currACK] = message[0]; 
        }
        
        sock.ackTo((char*)currACK, sizeof(currACK));
        currACK++;
    }
}*/

#include <iostream>
#include "udphw3.h"

#define TIMEOUT 1500
using namespace std;

/**
 * sends message[] and receives an acknowledgment from the server max
 * If the client cannot receive an acknowledgement immedietly
 * it should start a Timer. If a timeout occurs (i.e., no response after 1500 usec)
 * the client must resend the same message. The function must count the number of messages retransmitted and
 * return it to the main function as its return value.
 * @param socket the udp socket send data from
 * @param int the maximum number of time (messages) to send
 * @param message the message buffer
 * @return number of retransmissions
 **/
int clientStopWait( UdpSocket &sock, const int max, int message[] )
{
 
    cout << "beginning clientStopWait" << endl;
    int resubmissions = 0;
    for(int i = 0; i < max; i++){ // loop for the 20000 times
        message[0] = i; //insert the message into message[0]
        Timer timer;
        bool response = false;
        int sent;
        sent = sock.sendTo((char*) message, MSGSIZE); //send the message to the server
        cout << "data sent to server" << endl;
        timer.start();
        while(true) //while I have not receieved an response
        {
            int responseData = sock.pollRecvFrom(); //Any data been recieved?
            if(responseData > 0)
                break; //response is true and then cut the while loop

            if(timer.lap() > TIMEOUT && !response) //If we go over the 1500 Timeout 
            {
                cout << "response found " << endl;
                response = true;
                resubmissions++; //add to resubmissions count
                i--; //take away from the for loop total     
                break;
            }
        }
        if(response) //if the response is found dont recieve the message
            continue; //skip the iteration

        sock.recvFrom((char*) message, MSGSIZE); //receieve the message from ther server
        if(message[0] != i) //If it is a bad message
        {
            i--;    
            resubmissions++;
            continue; //Restart
        }
    }
    cout << "finishing" << endl;
    return resubmissions;
}

/**
 * repeats receiving message[] and sending an acknowledgment at a server side max (=20,000) times using the sock object.
 * @param socket the udp socket send data to
 * @param int the maximum number of time (messages) to receive
 * @param message the message buffer to receive 
**/
void serverReliable( UdpSocket &sock, const int max, int message[] )
{
    cout << "inside serverReliable" << endl;    
    for(int i = 0; i < max; i++)    //loop through the 20000 messages
    {
        while(true)
        {
            int recievedData = 0;
            recievedData = sock.pollRecvFrom(); //Any data been recieved?
            if(recievedData > 0)
            {
                cout << "message recieved" << endl;
                sock.recvFrom((char*) message, MSGSIZE); //recieve the information
                if(message[0] == i) //only if its the correct one
                {
                    cout << "ack sent back to client" << endl;
                    sock.ackTo((char *) &i, sizeof(i)); //if data has been receievd then I need to send it acknoledge it 
                    break;                
                }
            }            
         }
    }
}

/**
 *sends message[] and receiving an acknowledgment from a server max (=20,000) times using the sock object.
 * As described above, the client can continuously send a new message[] and increasing the sequence number as long as the number of in-transit messages (i.e., # of unacknowledged messages) is less than "windowSize." 
 * That number should be decremented every time the client receives an acknowledgment. If the number of unacknowledged messages reaches "windowSize," the client should start a Timer. If a timeout occurs (i.e., no response after 1500 usec), 
 * it must resend the message with the minimum sequence number among those which have not yet been acknowledged. 
 * The function must count the number of messages (not bytes) re-transmitted and return it to the main function as its return value. 
 * @param socket the udp socket send data from
 * @param int the maximum number of time (messages) to send
 * @param message the message buffer
 * @param windowsize the windowSize to the send the data
 * @return number of retransmission
 **/
int clientSlidingWindow( UdpSocket &sock, const int max, int message[], int windowSize )
{
    cout << "beginning clientSlidingWindow" << endl;
    int resubmissions = 0;
    int unacknowledged = 0;
    int acknowledgements = 0;
    for(int i = 0; i < max; i++)
    {
        cerr << "unacknowledged: " << unacknowledged << " windowsize: " << windowSize << endl;
        if(unacknowledged < windowSize)//if the unacknowledged messages is less than windowsize 
        {
            message[0] = i; //insert the message into message[0]
            sock.sendTo((char*) message, MSGSIZE); //send the message to the server
            unacknowledged++;
            cerr << " i " << i << " unacknolwedged: " << unacknowledged << endl;
        }

        if(unacknowledged == windowSize) //has to be another if statment here otherwise breaks
        {
            Timer timer;
            timer.start();
            while(true)
            {
                if(sock.pollRecvFrom() > 0)
                {
                    sock.recvFrom((char *)message, MSGSIZE);
                    if(message[0] < 0) {//The server is done
                        i = max; 
                        break;
                    }
                    cerr << "recieved data " << message[0] << " acknowledgements " << acknowledgements << endl;
                    if(message[0] == acknowledgements)
                    {
                        cout << "correct message couting as acknolegement" << endl;
                        acknowledgements++; //increase the amount acknowledged
                        unacknowledged--; //take away from the for loop total     
                        break; //response is true and then cut the while loop
                    }
                }
                if(timer.lap() > TIMEOUT && unacknowledged == windowSize) //If we go over the 1500 Timeout 
                {
                    cerr << "Timed Out resending message" << endl;
                    resubmissions = resubmissions + (i + windowSize - acknowledgements); //add to resubmissions count
                    i = acknowledgements; //resetting back to the last correctly submitted ack 
                    unacknowledged = 0; //go back to last valid ack
                    cerr << "after acknolegements i: " << i << endl;
                    break;
                }       
            }
        }
    }
    cout << "finishing" << endl;
    return resubmissions;
}

/**
 * receives message[] and sends an acknowledgment to the client max (=20,000) times using the sock object. 
 * Every time the server receives a new message[], it must save the message's sequence number in an array and return a cumulative acknowledgment, 
 * i.e.,* the last received message in order.
 * @param socket the udp socket retrieve data from
 * @param int the maximum number of time (messages) to retrieve
 * @param message the message buffer
 * @param windowsize the windowSize to the retrieve the data
 **/
void serverEarlyRetrans( UdpSocket &sock, const int max, int message[], int windowSize )
{
    cout << "inside Server EarlyRetrans" << endl;
    int cumAck[max];
    int count = 0;
    while(count != max)
    {
        if(sock.pollRecvFrom() > 0)
        {
            //cout << "message recieved" << endl;
            sock.recvFrom((char*) message, MSGSIZE)  ; //recieve the information
            cout << "message: " << message[0] << " count: "  << count << endl;
            if(message[0] == count){
                cout << "acknolegment sent" << endl; //never gets in here
                sock.ackTo((char *) &count, sizeof(count)); //if data has been receievd then I need to send it acknoledge it 
                cumAck[count] = message[0];
                count++;
            }
            else
            {
                sock.ackTo((char *) &count, sizeof(count)); //if message[0] != count 2 != 3
                count++; //not sure
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