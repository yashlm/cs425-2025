#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

void send_via_tcp(const std::string &server_ip, const std::string &message) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("TCP socket creation failed");
        return;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // Initialize with zeros
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("TCP connection failed");
        close(sockfd);
        return;
    }

    // Using C++11's std::chrono for measuring time
    auto start_time = std::chrono::high_resolution_clock::now();
    ssize_t sent_bytes = send(sockfd, message.c_str(), message.length(), 0);
    auto end_time = std::chrono::high_resolution_clock::now();

    if (sent_bytes < 0) {
        perror("TCP send failed");
    } else {
        std::cout << "TCP: Sent " << sent_bytes << " bytes in "
                  << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count()
                  << " microseconds.\n";
        std::cout << "TCP: Packet size (including headers) is " << (sent_bytes + 20) << " bytes\n";
    }

    close(sockfd);
}

void send_via_udp(const std::string &server_ip, const std::string &message) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("UDP socket creation failed");
        return;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // Initialize with zeros
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    // Using C++11's std::chrono for measuring time
    auto start_time = std::chrono::high_resolution_clock::now();
    ssize_t sent_bytes = sendto(sockfd, message.c_str(), message.length(), 0,
                                (struct sockaddr *)&server_addr, sizeof(server_addr));
    auto end_time = std::chrono::high_resolution_clock::now();

    if (sent_bytes < 0) {
        perror("UDP send failed");
    } else {
        std::cout << "UDP: Sent " << sent_bytes << " bytes in "
                  << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count()
                  << " microseconds.\n";
        std::cout << "UDP: Packet size (including headers) is " << (sent_bytes + 8) << " bytes\n";
    }

    close(sockfd);
}

int main() {
    std::string server_ip = "127.0.0.1"; // Loopback address
    std::string message = "Hello, Network Programming!";

    std::cout << "Message length: " << message.length() << " bytes\n";

    // Send the message via TCP
    send_via_tcp(server_ip, message);

    // Send the message via UDP
    send_via_udp(server_ip, message);

    return 0;
}
