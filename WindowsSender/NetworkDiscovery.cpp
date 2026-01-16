#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

#define DISCOVERY_PORT 5050
#define CONTROL_PORT 5051
#define MAGIC_ID 0x5649524D

struct DiscoveryPacket {
    uint32_t magic;
    uint8_t type; // 1=Req, 2=Resp
    uint16_t port;
    char name[32];
};

class NetworkDiscovery {
    SOCKET m_Socket;
    bool m_Running;
    std::thread m_Thread;

public:
    NetworkDiscovery() : m_Socket(INVALID_SOCKET), m_Running(false) {}

    bool Start() {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        m_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_Socket == INVALID_SOCKET) return false;

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(DISCOVERY_PORT);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(m_Socket, (sockaddr*)&addr, sizeof(addr)) != 0) return false;

        m_Running = true;
        m_Thread = std::thread(&NetworkDiscovery::Run, this);
        return true;
    }

    void Run() {
        char buffer[1024];
        sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);

        while (m_Running) {
            int len = recvfrom(m_Socket, buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr, &clientLen);
            if (len >= 5) { // Min size
                DiscoveryPacket* pkt = (DiscoveryPacket*)buffer;
                if (pkt->magic == htonl(MAGIC_ID) && pkt->type == 1) {
                    // Send Response
                    DiscoveryPacket resp;
                    resp.magic = htonl(MAGIC_ID);
                    resp.type = 2; // Response
                    resp.port = htons(CONTROL_PORT);
                    strcpy_s(resp.name, "WindowsPC");

                    sendto(m_Socket, (char*)&resp, sizeof(resp), 0, (sockaddr*)&clientAddr, clientLen);
                    std::cout << "Responded to discovery from " << inet_ntoa(clientAddr.sin_addr) << std::endl;
                }
            }
        }
    }

    void Stop() {
        m_Running = false;
        closesocket(m_Socket);
        if (m_Thread.joinable()) m_Thread.join();
        WSACleanup();
    }
};
