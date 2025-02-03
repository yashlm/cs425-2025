#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <condition_variable>

#define BUFFER_SIZE 1024

// Mutexes for thread safety
std::mutex clients_mutex;
std::mutex groups_mutex;

// Data structures
std::unordered_map<int, std::string> clients; // Client socket -> username
std::unordered_map<std::string, std::string> users; // Username -> password
std::unordered_map<std::string, std::unordered_set<int>> groups; // Group -> client sockets

// Function to load users from the file
std::unordered_map<std::string, std::string> load_users(const std::string& file_name) {
    std::unordered_map<std::string, std::string> users;
    std::ifstream file(file_name);
    std::string line;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string username, password;
        if (std::getline(ss, username, ':') && std::getline(ss, password)) {
            users[username] = password;
        }
    }

    return users;
}

// Function to send messages to the client
void send_message(int client_socket, const std::string& message) {
    send(client_socket, message.c_str(), message.size(), 0);
}

// Function to handle client commands
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    std::string username;

    // Receive username and password
    send_message(client_socket, "Enter username: ");
    memset(buffer, 0, sizeof(buffer));
    recv(client_socket, buffer, sizeof(buffer), 0);
    username = buffer;
    username = username.substr(0, username.find_first_of("\r\n\0")); // Clean up the username

    send_message(client_socket, "Enter password: ");
    memset(buffer, 0, sizeof(buffer));
    recv(client_socket, buffer, sizeof(buffer), 0);
    std::string password = buffer;
    password = password.substr(0, password.find_first_of("\r\n\0")); // Clean up the password

    // Authenticate the user
    bool authenticated = false;
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        if (users.find(username) != users.end() && users[username] == password) {
            authenticated = true;
            send_message(client_socket, "Welcome to the chat server!\n");
            clients[client_socket] = username; // Store the client's socket and username

            // Notify all other clients that a new client has joined
            for (const auto& [socket, user] : clients) {
                if (socket != client_socket) { // Don't send to the client themselves
                    std::string join_message = username + " has joined the chat\n";
                    send_message(socket, join_message);
                }
            }
            std::cout << "User " << username << " authenticated successfully.\n";
        } else {
            send_message(client_socket, "Authentication failed.\n");
            std::cout << "Authentication failed for user " << username << "\n";
            close(client_socket);
            return;
        }
    }

    if (!authenticated) {
        close(client_socket);
        return;
    }

    // Handle incoming messages from the client
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int recv_size = recv(client_socket, buffer, sizeof(buffer), 0);
        if (recv_size <= 0) {
            break;
        }

        std::string message(buffer);

        // Handle /msg command (private message)
        if (message.starts_with("/msg")) {
            size_t space1 = message.find(' ');
            size_t space2 = message.find(' ', space1 + 1);
            if (space1 != std::string::npos && space2 != std::string::npos) {
                std::string recipient = message.substr(space1 + 1, space2 - space1 - 1);
                std::string private_message = message.substr(space2 + 1);

                bool user_found = false;
                for (auto& [socket, user] : clients) {
                    if (user == recipient) {
                        send_message(socket, "[" + username + "]: " + private_message + "\n");
                        user_found = true;
                        break;
                    }
                }

                if (!user_found) {
                    send_message(client_socket, "User not found.\n");
                }
            } else {
                send_message(client_socket, "Invalid /msg syntax. Use: /msg <username> <message>\n");
            }
        } 
        // Handle /broadcast command
        else if (message.starts_with("/broadcast")) {
            std::lock_guard<std::mutex> lock(clients_mutex);
            std::string broadcast_message = message.substr(11); // Skip "/broadcast" part

            // Send the broadcast message to all connected clients
            for (auto& [socket, user] : clients) {
                if (socket != client_socket) {
                    send_message(socket, "[" + username + "]: " + broadcast_message + "\n");
                }
            }
        } 
        // Handle /create_group command
        else if (message.starts_with("/create_group")) {
            size_t space = message.find(' ', 14);
            if (space != std::string::npos) {
                std::string group_name = message.substr(14);
                std::lock_guard<std::mutex> lock(groups_mutex);
                groups[group_name] = {client_socket}; // Add the creator to the group
                send_message(client_socket, "Group " + group_name + " created.\n");
            }
        } 
        // Handle /join_group command
        else if (message.starts_with("/join_group")) {
            size_t space = message.find(' ', 12);
            if (space != std::string::npos) {
                std::string group_name = message.substr(12);
                std::lock_guard<std::mutex> lock(groups_mutex);
                if (groups.find(group_name) != groups.end()) {
                    groups[group_name].insert(client_socket); // Add client to group
                    send_message(client_socket, "You joined the group " + group_name + ".\n");
                } else {
                    send_message(client_socket, "Group " + group_name + " does not exist.\n");
                }
            }
        } 
        // Handle /leave_group command
        else if (message.starts_with("/leave_group")) {
            size_t space = message.find(' ', 13);
            if (space != std::string::npos) {
                std::string group_name = message.substr(13);
                std::lock_guard<std::mutex> lock(groups_mutex);
                if (groups.find(group_name) != groups.end()) {
                    groups[group_name].erase(client_socket); // Remove client from group
                    send_message(client_socket, "You left the group " + group_name + ".\n");
                } else {
                    send_message(client_socket, "Group " + group_name + " does not exist.\n");
                }
            }
        } 
        // Handle /group_msg command
        else if (message.starts_with("/group_msg")) {
            size_t space1 = message.find(' ');
            size_t space2 = message.find(' ', space1 + 1);
            if (space1 != std::string::npos && space2 != std::string::npos) {
                std::string group_name = message.substr(space1 + 1, space2 - space1 - 1);
                std::string group_message = message.substr(space2 + 1);

                std::lock_guard<std::mutex> lock(groups_mutex);
                if (groups.find(group_name) != groups.end()) {
                    for (int socket : groups[group_name]) {
                        if (socket != client_socket) { // Ensure message is not sent back to the sender
                            send_message(socket, "[" + username + "][Group " + group_name + "]: " + group_message + "\n");
                        }
                    }
                } else {
                    send_message(client_socket, "Group " + group_name + " does not exist.\n");
                }
            }
        } else {
            send_message(client_socket, "Unknown command.\n");
        }
    }

    // Close the client socket when done
    close(client_socket);
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(client_socket); // Remove client from list
    }

    // Notify all other clients that the client has left
    for (auto& [socket, user] : clients) {
        std::string leave_message = username + " has left the chat\n";
        send_message(socket, leave_message);
    }
}

// Main server function
int main() {
    // Load users from file
    users = load_users("users.txt");

    // Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Error creating socket.\n";
        return 1;
    }

    sockaddr_in server_address {};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(1234);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Error binding socket.\n";
        return 1;
    }

    if (listen(server_socket, 5) < 0) {
        std::cerr << "Error listening for connections.\n";
        return 1;
    }

    std::cout << "Server listening on port 1234...\n";

    // Accept and handle clients in separate threads
    while (true) {
        sockaddr_in client_address {};
        socklen_t client_address_size = sizeof(client_address);
        int client_socket = accept(server_socket, (sockaddr*)&client_address, &client_address_size);
        if (client_socket < 0) {
            std::cerr << "Error accepting connection.\n";
            continue;
        }

        std::thread client_thread(handle_client, client_socket);
        std::cout << "New connection accepted. Waiting for authentication...\n";
        client_thread.detach();  // Detach thread to handle multiple clients
    }

    close(server_socket);
    return 0;
}
