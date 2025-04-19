#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")  // 加上 Winsock2 lib

#define PORT 2368
#define PACKET_SIZE 1212
#define DIST_RESOLUTION 0.4f

const int VERTICAL_ANGLES[16] = {
    -16, 0, -14, 2, -12, 4, -10, 6,
    -8, 8, -6, 10, -4, 12, -2, 14
};

auto start_time = std::chrono::steady_clock::now();

float get_elapsed_time() {
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsed = now - start_time;
    return elapsed.count();
}

int main() {
    // 初始化 Winsock
    WSADATA wsaData;
    int wsaerr = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaerr != 0) {
        std::cerr << "WSAStartup failed: " << wsaerr << std::endl;
        return 1;
    }

    // 建立 UDP socket
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed." << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed." << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "Listening on UDP port " << PORT << "..." << std::endl;

    std::ofstream outfile("C:/Users/sgrc-325/Desktop/git/lidar/lslidar/lidar_output.csv");
    outfile << "time,azimuth,vert_angle,distance,intensity\n";

    char buffer[PACKET_SIZE];
    sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);

    while (true) {
        int recv_len = recvfrom(sock, buffer, PACKET_SIZE, 0, (sockaddr*)&client_addr, &addr_len);
        if (recv_len != PACKET_SIZE) continue;

        float timestamp = get_elapsed_time();

        for (int block = 0; block < 12; ++block) {
            int base = 100 * block;
            if ((unsigned char)buffer[base] != 0xFF || (unsigned char)buffer[base + 1] != 0xEE)
                continue;

            uint16_t azimuth_raw = *(uint16_t*)&buffer[base + 2];
            float azimuth = azimuth_raw / 100.0f;

            for (int ch = 0; ch < 16; ++ch) {
                int offset = base + 4 + ch * 3;
                uint16_t dist_raw = *(uint16_t*)&buffer[offset];
                uint8_t intensity = buffer[offset + 2];
                float distance = dist_raw * DIST_RESOLUTION;

                if (distance == 0) continue;
                int vert_angle = VERTICAL_ANGLES[ch];

                outfile << timestamp << "," << azimuth << ","
                        << vert_angle << "," << distance << ","
                        << (int)intensity << "\n";
            }
        }

        outfile.flush();  // 可以關掉提升速度
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
