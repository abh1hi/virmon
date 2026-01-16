#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <iostream>
#include <thread>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define CONTROL_PORT 5051
#define VIDEO_PORT 5052
#define DISCOVERY_PORT 55555

class NetworkTransport {
    SOCKET m_TcpServerSocket;
    SOCKET m_TcpClientSocket;
    SOCKET m_UdpSocket;
    
    // Discovery
    SOCKET m_DiscoverySocket;
    std::thread m_DiscoveryThread;
    bool m_Broadcasting = false;

    sockaddr_in m_ClientUdpAddr;
    bool m_Connected;
    uint32_t m_Sequence;

public:
    NetworkTransport() : m_TcpServerSocket(INVALID_SOCKET), m_TcpClientSocket(INVALID_SOCKET), 
                         m_UdpSocket(INVALID_SOCKET), m_DiscoverySocket(INVALID_SOCKET),
                         m_Connected(false), m_Sequence(0) {
        
        // Initialize Winsock if not already (assuming main might not have)
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }

    ~NetworkTransport() {
        m_Broadcasting = false;
        if (m_DiscoveryThread.joinable()) m_DiscoveryThread.join();
        
        if (m_TcpClientSocket != INVALID_SOCKET) closesocket(m_TcpClientSocket);
        if (m_TcpServerSocket != INVALID_SOCKET) closesocket(m_TcpServerSocket);
        if (m_UdpSocket != INVALID_SOCKET) closesocket(m_UdpSocket);
        if (m_DiscoverySocket != INVALID_SOCKET) closesocket(m_DiscoverySocket);
        WSACleanup();
    }

    bool Start() {
        // 1. TCP Server for Control/Connection
        m_TcpServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_TcpServerSocket == INVALID_SOCKET) return false;

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(CONTROL_PORT);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(m_TcpServerSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) return false;
        if (listen(m_TcpServerSocket, 1) == SOCKET_ERROR) return false;

        // 2. UDP Socket for Video
        m_UdpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_UdpSocket == INVALID_SOCKET) return false;
        
        // Bind UDP to ANY (or specific if needed) to ensure we can send/receive
        // bind is optional for sending, but good practice if we expect return traffic.
        
        // 3. Start Discovery Broadcast
        StartDiscoveryBroadcast();

        // 4. Start Accept Thread
        std::thread([this]() { AcceptLoop(); }).detach();
        return true;
    }

    void StartDiscoveryBroadcast() {
        m_DiscoverySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        char broadcast = 1;
        if (setsockopt(m_DiscoverySocket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
            std::cerr << "Failed to set broadcast option" << std::endl;
            return;
        }

        m_Broadcasting = true;
        m_DiscoveryThread = std::thread([this]() {
            sockaddr_in destAddr;
            destAddr.sin_family = AF_INET;
            destAddr.sin_port = htons(DISCOVERY_PORT);
            destAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

            std::string beacon = "VIRMON_SERVER_BEACON";

            std::cout << "Broadcasting Discovery Beacon on Port " << DISCOVERY_PORT << "..." << std::endl;

            while (m_Broadcasting) {
                // Only broadcast if not connected, or continue broadcasting to allow reconnection?
                // For now, continue to allow other devices to seeing it (though we handle 1 client)
                if (!m_Connected) {
                     sendto(m_DiscoverySocket, beacon.c_str(), (int)beacon.length(), 0, (sockaddr*)&destAddr, sizeof(destAddr));
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    }

    void AcceptLoop() {
        std::cout << "Waiting for Client Connection on TCP " << CONTROL_PORT << "..." << std::endl;
        
        sockaddr_in clientAddr;
        int len = sizeof(clientAddr);
        
        // Blocking accept
        m_TcpClientSocket = accept(m_TcpServerSocket, (sockaddr*)&clientAddr, &len);
        
        if (m_TcpClientSocket != INVALID_SOCKET) {
             std::cout << "Client Connected: " << inet_ntoa(clientAddr.sin_addr) << std::endl;
             
             // Prepare UDP destination (Client IP, port 5052)
             m_ClientUdpAddr = clientAddr;
             m_ClientUdpAddr.sin_port = htons(VIDEO_PORT);
             
             m_Connected = true;
             
             // Stop broadcasting to reduce noise? Or keep it.
             // m_Broadcasting = false; 
             
             // Start Read Loop for Control/Input
             ReceiveLoop();
        }
    }

    void ReceiveLoop() {
        char buffer[1024];
        while (m_Connected) {
            int ret = recv(m_TcpClientSocket, buffer, sizeof(buffer), 0);
            if (ret <= 0) {
                std::cout << "Client Disconnected." << std::endl;
                m_Connected = false;
                
                // Allow re-accept? 
                // Simple implementation: Close socket and restart Accept??
                // For MVP, we just set connected=false. Ideally we loop in AcceptLoop again.
                // Since AcceptLoop finished, we need a way to restart it.
                // FIX: Real implementation needs a loop in AcceptLoop.
                closesocket(m_TcpClientSocket);
                m_TcpClientSocket = INVALID_SOCKET;
                
                // Restart Accept Loop logic ideally
                std::thread([this]() { AcceptLoop(); }).detach();
                break;
            }
            // Handle Input data...
        }
    }

    bool SendVideoPacket(const std::vector<uint8_t>& data, uint64_t timestamp, bool keyframe) {
        if (!m_Connected) return false;

        // Header: [Seq:4][TS:8][Flags:1] + Data
        // Total UDP payload size check?
        // If data > 60KB, UDP will likely drop. NVENC slices or simple fragmentation needed for reliability.
        // For MVP on LAN, we hope for Jumbo frames or small packets.
        
        // Simple fragmentation:
        const int MAX_UDP_PAYLOAD = 1400; // Safe MTU
        int remaining = (int)data.size();
        int offset = 0;
        
        // NOTE: Real video streaming requires RTP or similar.
        // Sending raw NAL units in UDP fragments without reassembly logic on receiver is risky.
        // Receiver (Java) needs to handle this.
        // For Simplest MVP: Send whole blob and rely on IP Fragmentation (up to 64k).
        // Java DatagramPacket default buffer might be small.
        
        if (data.size() > 60000) {
            // Warn?
        }

        std::vector<uint8_t> packet;
        packet.resize(13 + data.size());
        
        uint32_t* seq = (uint32_t*)&packet[0];
        *seq = htonl(m_Sequence++);
        
        uint64_t* ts = (uint64_t*)&packet[4];
        // Custom swap or use htons structure
        // Simple little endian to big endian logic
        *ts = timestamp; // Assuming receiver handles endianness or we agree on LE.
        // Let's stick to LE for simplicity if both are LE architecture (x86/ARM usually are).
        
        packet[12] = keyframe ? 0x02 : 0x00;
        memcpy(&packet[13], data.data(), data.size());

        int sent = sendto(m_UdpSocket, (const char*)packet.data(), (int)packet.size(), 0, (sockaddr*)&m_ClientUdpAddr, sizeof(m_ClientUdpAddr));
        return (sent > 0);
    }
};
