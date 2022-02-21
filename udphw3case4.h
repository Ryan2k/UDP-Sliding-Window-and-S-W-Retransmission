#include "Timer.h"
#include "UdpSocket.h"

using namespace std;

class udphw3 {
    public:
        int clientStopWait(UdpSocket &sock, const int max, int message[]);
        void serverReliable(UdpSocket &sock, const int max, int message[]);
        int clientSlidingWindow(UdpSocket &sock, const int max, int message[], int windowSize);
        void serverEarlyRetrans(UdpSocket &sock, const int max, int message[], int windowSize);
};