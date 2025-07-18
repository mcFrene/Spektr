#include "priemnik.h"
#include <uhp_rx_eth.h>
#include <uhp_iq_stream.h>
#include <fftw3.h>
#include <math.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <algorithm>

const short pointsQty = 1024;
const short partSize = 256;
const short partsQty = pointsQty/partSize;
const short averageQty = 10;
const short maxPacketSize = 3*1024;

Priemnik::Priemnik(QObject *parent) : QObject{parent}
{
}

double complexLog(fftw_complex z);
void recvTCPPacket(SOCKET socketTCP);
void saveData(std::vector<std::vector<double>>& numbersForAverage, fftw_complex* numbersForFFT, int graphCount);
void drawData(std::vector<std::vector<double>>& numbersForAverage, int graphCount, Priemnik* p);
void accumulateForFFT(BYTE* buffer, fftw_complex* numbersForFFT, int packetsCount);

void Priemnik::socketWork(std::string ip, int port, int freq){
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "Failed to find Winsock 2.2 or better." << std::endl;
        WSACleanup();
        return;
    }

    //TCP INIT
    SOCKET socketTCP = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketTCP == INVALID_SOCKET) {
        std::cout << "Failed to create tcp_socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in tcp_addr;
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET; //AF_UNSPEC;
    tcp_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    tcp_addr.sin_port = htons(port);


    if(::connect(socketTCP, (sockaddr *) &tcp_addr, sizeof(tcp_addr)) == SOCKET_ERROR){
        std::cout << "Failed to connect (tcp): " << WSAGetLastError() << std::endl;
        emit throwError("Failed to connect (tcp): " + std::to_string(WSAGetLastError()));
        WSACleanup();
        return;
    }

    ULONG i_mode = 1;
    if(ioctlsocket(socketTCP, FIONBIO, &i_mode) == SOCKET_ERROR){
        std::cout << "Failed to configure socket (tcp): " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }


    //UDP INIT
    SOCKET socketUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (socketUDP == INVALID_SOCKET) {
        std::cout << "Failed to create udp_socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in udp_addr;
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = 0;
    udp_addr.sin_port = htons(42000);

    if (bind(socketUDP, (sockaddr *) &udp_addr, sizeof(udp_addr)) == SOCKET_ERROR) {
        std::cout << "Failed to bid udp_socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    if(ioctlsocket(socketUDP, FIONBIO, &i_mode) == SOCKET_ERROR){
        std::cout << "Failed to configure socket (udp): " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    //SET HZ
    ETH_RX_CTRL::set_freq setFreq;
    setFreq.head.cmd_type = 0x2;
    setFreq.head.messid = 1;
    setFreq.head.size = sizeof(ETH_RX_CTRL::set_freq);
    setFreq.carrier_freq_Hz = freq*1000; //минимальное значение

    if(send(socketTCP, (char*)&setFreq, sizeof(setFreq), 0) == SOCKET_ERROR){
        std::cout << "Failed status request: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    //SET UDP STREAM
    ETH_RX_CTRL::ctrl_iq_stream_now startStream;
    startStream.head.messid = 2;
    startStream.head.cmd_type = 0xC;
    startStream.head.size = sizeof(ETH_RX_CTRL::ctrl_iq_stream_now);
    startStream.IP_stream = 0;
    startStream.port_stream = 42000;
    startStream.preset_num = ETH_RX_CTRL::pres_default;

    fd_set read_s;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    if(send(socketTCP, (char*)&startStream, sizeof(startStream), 0) == SOCKET_ERROR){
        std::cout << "Failed srart stream: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    //MOLOTILKA
    short packetsCount = 0;
    short graphCount = 0;
    fftw_complex numbersForFFT[pointsQty];
    BYTE buffer[maxPacketSize];
    fftw_plan plan = fftw_plan_dft_1d(pointsQty, numbersForFFT, numbersForFFT, FFTW_FORWARD, FFTW_ESTIMATE);
    std::vector<std::vector<double>> numbersForAverage(averageQty, std::vector<double>(pointsQty));
    int fromlen_udp = sizeof(udp_addr);

    while(isRunning.load()){
        FD_ZERO(&read_s);
        FD_SET(socketTCP, &read_s);
        FD_SET(socketUDP, &read_s);
        select(0, &read_s, NULL, NULL, &tv);

        // //TCP RECV
        if(FD_ISSET(socketTCP, &read_s)){
            recvTCPPacket(socketTCP);
        }

        //UDP RECV
        if(FD_ISSET(socketUDP, &read_s)){
            int len = 1;
            while(len != SOCKET_ERROR){
                len = recvfrom(socketUDP, (char *) &buffer, sizeof(buffer), 0, (sockaddr*)&udp_addr, &fromlen_udp);
                if(len == 2066){
                    accumulateForFFT(buffer, numbersForFFT, packetsCount);
                    packetsCount = (packetsCount + 1) % 4;
                    if(packetsCount == 0){
                        fftw_execute(plan);
                        saveData(numbersForAverage, numbersForFFT, graphCount);
                        drawData(numbersForAverage, graphCount, this);
                        graphCount++;

                        if(graphCount == 20){
                            graphCount -= 10;
                        }
                    }
                }

                if(len == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK){
                    std::cout << "Failed to recvfromUDP: " << WSAGetLastError() << std::endl;
                    WSACleanup();
                    return;
                }
            }
        }
    }

    ETH_RX_CTRL::stop_iq_stream stopStream;
    stopStream.head.messid = 0;
    stopStream.head.cmd_type = 0xE;
    stopStream.head.size = sizeof(ETH_RX_CTRL::stop_iq_stream);

    if(send(socketTCP, (char*)&stopStream, sizeof(stopStream), 0) == SOCKET_ERROR){
        std::cout << "Failed stop stream: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    fftw_destroy_plan(plan);
    WSACleanup();
    emit priemnikFinished();
}

double complexLog(fftw_complex z){
    return 10*log10(z[0]*z[0] + z[1]*z[1]);
}

void recvTCPPacket(SOCKET socketTCP){
    BYTE buffer[maxPacketSize];
    int len = 1;

    while(len != SOCKET_ERROR){
        len = recv(socketTCP, (char *) &buffer, sizeof(buffer), 0);

        if (len > 0) {
            ETH_RX_CTRL::status tcp_answer = *(ETH_RX_CTRL::status*)(buffer);
            std::cout<<
                "Answer to command : "
                      <<ETH_RX_CTRL::cmdCOMMANDtoStr(tcp_answer.head.cmd_ack_type)
                      <<" status: "
                      << tcp_answer.head.cmd_complete << std::endl;
        }

        if(len == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK){
            std::cout << "Failed to recvfromTCP: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return;
        }
    }
}

void saveData(std::vector<std::vector<double>>& numbersForAverage, fftw_complex* fftCompexNumbers, int graphCount){
    for(int i=0; i<pointsQty; i++){
        numbersForAverage[graphCount % averageQty][i] = complexLog(fftCompexNumbers[i]);
    }
}

void drawData(std::vector<std::vector<double>>& numbersForAverage, int graphCount, Priemnik* p){
    std::vector<double> numbersForDraw(pointsQty);

    for(int i=0; i<pointsQty; i++){
        int sum = 0;
        for(int j=0; j<averageQty; j++){
            sum += numbersForAverage[j][i];
        }

        numbersForDraw[i] = sum / (graphCount < averageQty ? graphCount+1 : averageQty);
    }
    rotate(numbersForDraw.begin(), numbersForDraw.begin() + pointsQty/2, numbersForDraw.end());

    emit p->sendData(numbersForDraw);
}

void accumulateForFFT(BYTE* buffer, fftw_complex* numbersForFFT, int packetsCount){
    int start = partSize * (packetsCount % partsQty);
    for(int i=0; i<partSize; i++){
        numbersForFFT[start+i][0] = *(int32_t*)(buffer + sizeof(UDP_IQ::header_t) + i*8);
        numbersForFFT[start+i][1] = *(int32_t*)(buffer + sizeof(UDP_IQ::header_t) + i*8 + sizeof(int32_t));
    }
}
