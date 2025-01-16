#include <iostream>
#include <cstring>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

void start_tcp_server() {
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock < 0) {
        perror("TCP socket creation failed");
        return;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // Initialize with zeros
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(tcp_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("TCP bind failed");
        close(tcp_sock);
        return;
    }

    if (listen(tcp_sock, 5) < 0) {
        perror("TCP listen failed");
        close(tcp_sock);
        return;
    }

    std::cout << "TCP server listening on port " << SERVER_PORT << "...\n";

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sock = accept(tcp_sock, (struct sockaddr *)&client_addr, &client_len);
    if (client_sock < 0) {
        perror("TCP accept failed");
    } else {
        char buffer[BUFFER_SIZE] = {0};
        ssize_t bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received < 0) {
            perror("TCP receive failed");
        } else {
            std::cout << "TCP: Received " << bytes_received << " bytes: " << buffer << "\n";
            std::cout << "TCP: Packet size (including headers) is " << (bytes_received + 20) << " bytes\n";
        }
        close(client_sock);
    }

    close(tcp_sock);
    std::cout << "TCP server closed.\n";
}

void start_udp_server() {
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        perror("UDP socket creation failed");
        return;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // Initialize with zeros
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("UDP bind failed");
        close(udp_sock);
        return;
    }

    std::cout << "UDP server listening on port " << SERVER_PORT << "...\n";

    char buffer[BUFFER_SIZE] = {0};
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    ssize_t bytes_received = recvfrom(udp_sock, buffer, BUFFER_SIZE, 0,
                                      (struct sockaddr *)&client_addr, &client_len);
    if (bytes_received < 0) {
        perror("UDP receive failed");
    } else {
        std::cout << "UDP: Received " << bytes_received << " bytes: " << buffer << "\n";
        std::cout << "UDP: Packet size (including headers) is " << (bytes_received + 8) << " bytes\n";
    }

    close(udp_sock);
    std::cout << "UDP server closed.\n";
}

int main() {
    std::thread tcp_thread(start_tcp_server); // Thread for TCP server
    std::thread udp_thread(start_udp_server); // Thread for UDP server

    tcp_thread.join(); // Wait for TCP server to finish
    udp_thread.join(); // Wait for UDP server to finish

    return 0;
}
