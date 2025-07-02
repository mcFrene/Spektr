#include "priemnik.h"
#include <QDebug>
#include <uhp_rx_eth.h>
#include <uhp_iq_stream.h>
#include <fftw3.h>
#include <math.h>
#include <winsock2.h>
#include <ws2tcpip.h>

const short interval = 200;
const short pointsQty = 1024;
const short averageQty = 10;
const short partSize = 256;


Priemnik::Priemnik(QObject *parent) : QObject{parent}
{
}

void Priemnik::priemnikFinish(){
    isRunning.store(false);
}

bool Priemnik::getIsRunning(){
    return isRunning;
}

double complexTOgraph(fftw_complex z);

void Priemnik::socketWork(std::string ip, int port, int freq){
    isRunning.store(true);

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
    tcp_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    tcp_addr.sin_port = htons(port);


    if(::connect(socketTCP, (sockaddr *) &tcp_addr, sizeof(tcp_addr)) == SOCKET_ERROR){
        qDebug() << "Failed to connect (tcp): " << WSAGetLastError() << Qt::endl;
        // Выкинуть ошибку?
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
    udp_addr.sin_addr.s_addr = 0;
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
    setFreq.carrier_freq_Hz = freq*1000; //минимальное значение


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
    //UDP_IQ::header_t udp_header;
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


    //std::vector<char> buffer(4096);

    //MOLOTILKA
    int packetsCount = 0;
    int graphCount = 0;
    int currentTime = GetTickCount64();
    BYTE buffer[3072];
    fftw_complex rawComplexNumbers[pointsQty];

    fftw_plan plan = fftw_plan_dft_1d(pointsQty, rawComplexNumbers, rawComplexNumbers, FFTW_FORWARD, FFTW_ESTIMATE);

    std::vector<std::vector<double>> numbersForAverage(averageQty, std::vector<double>(pointsQty));
    std::vector<double> numbersForDraw(pointsQty);

    while(isRunning.load()){
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
                //udp_header = *(UDP_IQ::header_t*)(buffer);
                int start = 256*(packetsCount % 4);
                for(int i=0; i<partSize; i++){
                    rawComplexNumbers[start+i][0] = *(int32_t*)(buffer + sizeof(UDP_IQ::header_t) + i*8);
                    rawComplexNumbers[start+i][1] = *(int32_t*)(buffer + sizeof(UDP_IQ::header_t) + i*8 + sizeof(int32_t));
                    //int seq = udp_header.seqnum;
                    //qDebug()<< seq << Qt::endl;
                }
            }
            if(packetsCount > 0 && packetsCount % 4 == 0){
                fftw_execute(plan);

                if(graphCount < averageQty){
                    for(int i=0; i<pointsQty; i++){
                        numbersForAverage[graphCount][i] = complexTOgraph(rawComplexNumbers[i]);
                    }
                    graphCount++;
                }
                else{
                    for(int i=0; i<averageQty-1; i++){
                        numbersForAverage[i] = numbersForAverage[i+1];
                    }

                    for(int i=0; i<pointsQty; i++){
                        numbersForAverage[averageQty-1][i] = complexTOgraph(rawComplexNumbers[i]);
                    }
                }

                if(graphCount == averageQty && GetTickCount64() - currentTime > interval){
                    for(int i=0; i<pointsQty; i++){
                        int sum = 0;
                        for(int j=0; j<averageQty; j++){
                            sum += numbersForAverage[j][i];
                        }
                        numbersForDraw[i] = sum / averageQty;
                    }
                    emit sendData(numbersForDraw);
                    currentTime = GetTickCount64();
                }
            }
            packetsCount++;
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
