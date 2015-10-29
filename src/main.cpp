#include <iostream>
#include <conio.h>
#include <string>
#include <thread>
#include <mutex>
#include "rs232.h"

#define MAX_BUFFER                  1024

#if     defined(WIN32)
#define PORTNO_TRANSLATOR(port)     (port - 1)
#elif   defined(linux)
#define PORTNO_TRANSLATOR(port)     (port)
#endif

int         iPort;
bool        bEcho;
std::mutex  oMutex;

class UARTCloser {
public:
    ~UARTCloser() {
        RS232_CloseComport(iPort);
    }
};

UARTCloser oCloser;

void rssend(void) {
    while (true) {
        unsigned char c = _getch();
        std::lock_guard<std::mutex> oLocker(oMutex);
        if (c == '\n' || c == '\r') {
            unsigned char aEOL[] = "\r\n";
            RS232_SendBuf(iPort, aEOL, 2);
            if (bEcho) {
                putchar('\n');
            }
        } else {
            RS232_SendByte(iPort, c);
            if (bEcho) {
                putchar(c);
            }
        }
    }
}

void rsread(void) {
    while (true) {
        unsigned char aBuffer[MAX_BUFFER];
        int iLen = RS232_PollComport(iPort, aBuffer, MAX_BUFFER);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (iLen) {
            aBuffer[iLen] = '\0';
            std::lock_guard<std::mutex> oLocker(oMutex);
            std::cout << aBuffer;
            std::cout.flush();
        }
    }
}

int main(void) {
    int iBaud;
    std::string szTransmissionMode, szHexSwitch;
    std::cout << "Enter port number:" << std::endl;
    std::cin >> iPort;
    iPort = PORTNO_TRANSLATOR(iPort);
    std::cout << "Enter baudrate:" << std::endl;
    std::cout << "0 - 9600" << std::endl;
    std::cout << "1 - 19200" << std::endl;
    std::cout << "2 - 38400" << std::endl;
    std::cout << "3 - 57600" << std::endl;
    std::cout << "4 - 115200" << std::endl;
    std::cin >> iBaud;
    std::cout << "Enter transmission mode:" << std::endl;
    std::cout << "[local echo n|e][bits][parity n|e|o][stopbits]" << std::endl;
    std::cin >> szTransmissionMode;
    if (szTransmissionMode.size() != 4) {
        std::cout << "Invalid transmission mode" << std::endl;
        return 1;
    }
    switch (szTransmissionMode[0]) {
    case 'n':
        bEcho = false;
        break;
    case 'e':
        bEcho = true;
        break;
    default:
        std::cout << "Invalid transmission mode" << std::endl;
        return 1;
        break;
    }
    int iOpenResult;
    switch (iBaud) {
    case 0:
        iOpenResult = RS232_OpenComport(iPort, 9600, szTransmissionMode.c_str() + 1);
        break;
    case 1:
        iOpenResult = RS232_OpenComport(iPort, 19200, szTransmissionMode.c_str() + 1);
        break;
    case 2:
        iOpenResult = RS232_OpenComport(iPort, 38400, szTransmissionMode.c_str() + 1);
        break;
    case 3:
        iOpenResult = RS232_OpenComport(iPort, 57600, szTransmissionMode.c_str() + 1);
        break;
    case 4:
        iOpenResult = RS232_OpenComport(iPort, 115200, szTransmissionMode.c_str() + 1);
        break;
    default:
        iOpenResult = RS232_OpenComport(iPort, iBaud, szTransmissionMode.c_str() + 1);
        break;
    }
    if (iOpenResult) {
        return 1;
    }
    std::thread oReadThd(rsread);
    std::thread oSendThd(rssend);
    oReadThd.join();
    oSendThd.join();
    return 0;
}
