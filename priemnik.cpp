#include "priemnik.h"
#include <QDebug>
#include <uhp_rx_eth.h>
#include <uhp_iq_stream.h>
#include <fftw3.h>
#include <math.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>



Priemnik::Priemnik(QObject *parent) : QObject{parent}
{
}
double complexTOgraph(fftw_complex z);

void Priemnik::socketWork(){
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        qDebug() << "Failed to find Winsock 2.2 or better." << Qt::endl;
        WSACleanup();
        return;
    }

    //TCP INIT
    SOCKET socketTCP = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketTCP == INVALID_SOCKET) {
        qDebug() << "Failed to create tcp_socket: " << WSAGetLastError() << Qt::endl;
        WSACleanup();
        return;
    }

    sockaddr_in tcp_addr;
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET; //AF_UNSPEC;
    tcp_addr.sin_addr.s_addr = inet_addr("192.168.21.1");
    tcp_addr.sin_port = htons(5050);


    if(::connect(socketTCP, (sockaddr *) &tcp_addr, sizeof(tcp_addr)) == SOCKET_ERROR){
        qDebug() << "Failed to connect (tcp): " << WSAGetLastError() << Qt::endl;
        WSACleanup();
        return;
    }

    ULONG i_mode = 1;
    if(ioctlsocket(socketTCP, FIONBIO, &i_mode) == SOCKET_ERROR){
        qDebug() << "Failed to configure socket (tcp): " << WSAGetLastError() << Qt::endl;
        WSACleanup();
        return;
    }



    //UDP INIT
    SOCKET socketUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (socketUDP == INVALID_SOCKET) {
        qDebug() << "Failed to create udp_socket: " << WSAGetLastError() << Qt::endl;
        WSACleanup();
        return;
    }

    sockaddr_in udp_addr;
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = 0; //inet_addr("192.168.21.111"); //0;
    udp_addr.sin_port = htons(42000);

    if (bind(socketUDP, (sockaddr *) &udp_addr, sizeof(udp_addr)) == SOCKET_ERROR) {
        qDebug() << "Failed to bid udp_socket: " << WSAGetLastError() << Qt::endl;
        WSACleanup();
        return;
    }

    if(ioctlsocket(socketUDP, FIONBIO, &i_mode) == SOCKET_ERROR){
        qDebug() << "Failed to configure socket (udp): " << WSAGetLastError() << Qt::endl;
        WSACleanup();
        return;
    }

    //SET HZ
    ETH_RX_CTRL::set_freq setFreq;
    setFreq.head.cmd_type = 0x2;
    setFreq.head.messid = 1;
    setFreq.head.size = sizeof(ETH_RX_CTRL::set_freq);
    setFreq.carrier_freq_Hz = 3900000; //минимальное значение


    if(send(socketTCP, (char*)&setFreq, sizeof(setFreq), 0) == SOCKET_ERROR){
        qDebug() << "Failed status request: " << WSAGetLastError() << Qt::endl;
        WSACleanup();
        return;
    }


    //SET UDP STREAM
    ETH_RX_CTRL::ctrl_iq_stream_now stream;
    stream.head.messid = 2;
    stream.head.cmd_type = 0xC;
    stream.head.size = sizeof(ETH_RX_CTRL::ctrl_iq_stream_now);

    stream.IP_stream = 0; //inet_addr("192.168.21.111"); //inet_addr("192.168.21.1");
    stream.port_stream = 42000;//htons(42000);
    stream.preset_num = ETH_RX_CTRL::pres_default;


    ETH_RX_CTRL::status tcp_answer;
    UDP_IQ::header_t udp_header;
    fd_set read_s;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    int fromlen_udp = sizeof(udp_addr);


    if(send(socketTCP, (char*)&stream, sizeof(stream), 0) == SOCKET_ERROR){
        qDebug() << "Failed stream: " << WSAGetLastError() << Qt::endl;
        WSACleanup();
        return;
    }


    //std::vector<char> buffer(2048);
    unsigned char buffer[4096]{'0'};

    //MOLOTILKA
    int count = 0;
    int interval = GetTickCount64();
    UDP_IQ::cint32_t rawNums[1024];
    fftw_complex in[1024];
    fftw_complex out[1024];
    fftw_plan plan = fftw_plan_dft_1d(1024, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    double graph[1024];

    for(int i=0; i<1024; i++){
        rawNums[i].im = 0;
        rawNums[i].re = 0;
    }
    while(true){
        FD_ZERO(&read_s);
        FD_SET(socketTCP, &read_s);
        FD_SET(socketUDP, &read_s);
        select(0, &read_s, NULL, NULL, &tv);


        //TCP RECV
        if(FD_ISSET(socketTCP, &read_s)){
            int len = recv(socketTCP, (char *) &buffer, sizeof(buffer), 0);

            if(len == SOCKET_ERROR){
                qDebug() << "Failed to recvfromUDP: " << WSAGetLastError() << Qt::endl;
                WSACleanup();
                return;
            }

            if (len > 0) {
                memcpy(&tcp_answer, &buffer, sizeof(ETH_RX_CTRL::status));
                qDebug()
                    <<"Answer to command : "
                    <<ETH_RX_CTRL::cmdCOMMANDtoStr(tcp_answer.head.cmd_ack_type)
                    <<" status: "
                    << tcp_answer.head.cmd_complete << Qt::endl;
            }

            ;//qDebug() << "recvfrom socketTCP";
        }

        //UDP RECV

        if(FD_ISSET(socketUDP, &read_s)){
            int len = recvfrom(socketUDP, (char *) &buffer, sizeof(buffer), 0, (sockaddr*)&udp_addr, &fromlen_udp);
            if(len == SOCKET_ERROR){
                qDebug() << "Failed to recvfromUDP: " << WSAGetLastError() << Qt::endl;
                WSACleanup();
                return;
            }

            if (len > 0) {
                memcpy(&udp_header, &buffer, sizeof(UDP_IQ::header_t));
                memcpy(&rawNums[256*(count % 4)], &buffer[sizeof(UDP_IQ::header_t)], len - sizeof(UDP_IQ::header_t));
                //if(udp_header.iq_format == UDP_IQ::iq_format_t::iq_complex_int32
            }
            if(count > 0 && count % 4 == 0){

                for(int i=0; i<1024; i++){
                    in[i][0] = rawNums[i].re;
                    in[i][1] = rawNums[i].im;
                }

                fftw_execute(plan);

                for(int i=0; i<1024; i++){
                    graph[i] = complexTOgraph(out[i]);
                }

                if(GetTickCount64() - interval > 1000){
                    emit sendData(graph);
                    interval = GetTickCount64();
                }


                //break;
                //qDebug() << "recvfrom socketUDP";
            }
            count++;
        }
    }
    fftw_destroy_plan(plan);
    WSACleanup();
}

double complexTOgraph(fftw_complex z){
    return 10*log10(z[0]*z[0] + z[1]*z[1]);
}

// bool Priemnik::getIsRunning(){
//     return isRunning;
// }

// void Priemnik::setIsRunning(bool newIsRunning){
//     isRunning = newIsRunning;
// }


// void Priemnik::run(){
//     //qDebug() << "hello"<<Qt::endl;
//     //while (isRunning){
//     //    qDebug() << "hello"<<Qt::endl;
//     //}

//     // while (isRunning){
//     //     std::vector<int> nums;
//     //     for(int i=0; i<1024; i++){
//     //         nums.push_back(rand() % 1025);
//     //     }
//     //     emit sendData(nums);
//     //     emit finished();
//     // }


//     emit finished();
// }
