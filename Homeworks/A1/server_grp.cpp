#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <condition_variable>
#include <chrono>
#include <ctime>
#include <algorithm>

// Define buffer size for client-server messages
#define BUFFER_SIZE 1024

// Logger namespace to provide thread-safe logging functionality
namespace Logger {
    std::mutex log_mutex;
    void log_info(const std::string& message) {
        std::lock_guard<std::mutex> lock(log_mutex);
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
        std::string time_str = std::string(std::ctime(&currentTime));
        if (!time_str.empty() && time_str.back() == '\n') {
            time_str.pop_back();
        }
        std::cout << "[" << time_str << "] [INFO] " << message << std::endl;
    }
    void log_error(const std::string& message) {
        std::lock_guard<std::mutex> lock(log_mutex);
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
        std::string time_str = std::string(std::ctime(&currentTime));
        if (!time_str.empty() && time_str.back() == '\n') {
            time_str.pop_back();
        }
        std::cerr << "[" << time_str << "] [ERROR] " << message << std::endl;
    }
}

// Global mutexes for thread safety over shared data structures
std::mutex clients_mutex;
std::mutex groups_mutex;

// Global data structures:
// clients: mapping of client socket to username
// users: allowed username-password pairs loaded from file
// groups: mapping of group names to a set of client sockets (its members)
std::unordered_map<int, std::string> clients;
std::unordered_map<std::string, std::string> users;
std::unordered_map<std::string, std::unordered_set<int>> groups;

// Helper function to split a string by whitespace
std::vector<std::string> split(const std::string& s) {
    std::istringstream iss(s);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// Function to load users from a file
std::unordered_map<std::string, std::string> load_users(const std::string& file_name) {
    std::unordered_map<std::string, std::string> loaded_users;
    std::ifstream file(file_name);
    std::string line;
    if (!file.is_open()) {
        Logger::log_error("Failed to open user file: " + file_name);
        return loaded_users;
    }
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string username, password;
        if (std::getline(ss, username, ':') && std::getline(ss, password)) {
            loaded_users[username] = password;
        }
    }
    Logger::log_info("Loaded " + std::to_string(loaded_users.size()) + " users from " + file_name);
    return loaded_users;
}

// Utility function to send a message to a client socket with error checking
void send_message(int client_socket, const std::string& message) {
    ssize_t sent = send(client_socket, message.c_str(), message.size(), 0);
    if (sent < 0) {
        Logger::log_error("Failed to send message to socket " + std::to_string(client_socket));
    }
}

// --- Command processing functions ---
void processPrivateMessage(int client_socket, const std::string& message, const std::string& username) {
    size_t space1 = message.find(' ');
    size_t space2 = message.find(' ', space1 + 1);
    if (space1 == std::string::npos || space2 == std::string::npos) {
        send_message(client_socket, "Invalid /msg syntax. Use: /msg <username> <message>\n");
        Logger::log_error("Invalid /msg syntax from user " + username);
        return;
    }
    std::string recipient = message.substr(space1 + 1, space2 - space1 - 1);
    std::string private_message = message.substr(space2 + 1);

    bool user_found = false;
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (const auto& [sock, user] : clients) {
            if (user == recipient) {
                send_message(sock, "[" + username + "]: " + private_message + "\n");
                user_found = true;
                Logger::log_info("Private message from " + username + " to " + recipient);
                break;
            }
        }
    }
    if (!user_found) {
        send_message(client_socket, "User not found.\n");
        Logger::log_error("User " + recipient + " not found for private message from " + username);
    }
}

void processBroadcastMessage(int client_socket, const std::string& message, const std::string& username) {
    std::string broadcast_message = message.substr(11); // Skip the command part
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (const auto& [sock, user] : clients) {
            if (sock != client_socket) {
                send_message(sock, "[" + username + "]: " + broadcast_message + "\n");
            }
        }
    }
    Logger::log_info("Broadcast message from " + username);
}

void processCreateGroup(int client_socket, const std::string& message, const std::string& username) {
    size_t space = message.find(' ');
    if (space == std::string::npos) {
        send_message(client_socket, "Invalid syntax. Use: /create_group <group_name>\n");
        Logger::log_error("Invalid /create_group syntax from " + username);
        return;
    }
    std::string group_name = message.substr(space + 1);
    group_name = group_name.substr(0, group_name.find_first_of("\r\n\0"));
    {
        std::lock_guard<std::mutex> lock(groups_mutex);
        if (groups.find(group_name) != groups.end()) {
            send_message(client_socket, "Group " + group_name + " already exists.\n");
            Logger::log_error("Group creation failed: " + group_name + " already exists. User: " + username);
        } else {
            groups[group_name] = {client_socket}; // Add creator to the group
            send_message(client_socket, "Group " + group_name + " created.\n");
            Logger::log_info("Group " + group_name + " created by " + username);
        }
    }
}

void processJoinGroup(int client_socket, const std::string& message, const std::string& username) {
    size_t space = message.find(' ');
    if (space == std::string::npos) {
        send_message(client_socket, "Invalid syntax. Use: /join_group <group_name>\n");
        Logger::log_error("Invalid /join_group syntax from " + username);
        return;
    }
    std::string group_name = message.substr(space + 1);
    group_name = group_name.substr(0, group_name.find_first_of("\r\n\0"));
    {
        std::lock_guard<std::mutex> lock(groups_mutex);
        if (groups.find(group_name) != groups.end()) {
            if (groups[group_name].find(client_socket) != groups[group_name].end()) {
                send_message(client_socket, "You are already in group " + group_name + ".\n");
                Logger::log_info(username + " attempted to rejoin group " + group_name);
            } else {
                groups[group_name].insert(client_socket);
                send_message(client_socket, "You joined the group " + group_name + ".\n");
                Logger::log_info(username + " joined group " + group_name);
                // Notify existing group members about the new member
                for (int member_socket : groups[group_name]) {
                    if (member_socket != client_socket) {
                        send_message(member_socket, username + " has joined the group " + group_name + ".\n");
                    }
                }
            }
        } else {
            send_message(client_socket, "Group " + group_name + " does not exist.\n");
            Logger::log_error("Join group failed: Group " + group_name + " does not exist for user " + username);
        }
    }
}

void processLeaveGroup(int client_socket, const std::string& message, const std::string& username) {
    size_t space = message.find(' ');
    if (space == std::string::npos) {
        send_message(client_socket, "Invalid syntax. Use: /leave_group <group_name>\n");
        Logger::log_error("Invalid /leave_group syntax from " + username);
        return;
    }
    std::string group_name = message.substr(space + 1);
    group_name = group_name.substr(0, group_name.find_first_of("\r\n\0"));
    {
        std::lock_guard<std::mutex> lock(groups_mutex);
        if (groups.find(group_name) != groups.end()) {
            if (groups[group_name].find(client_socket) == groups[group_name].end()) {
                send_message(client_socket, "You are not in group " + group_name + ".\n");
                Logger::log_error(username + " attempted to leave group " + group_name + " but was not a member");
            } else {
                groups[group_name].erase(client_socket);
                send_message(client_socket, "You left the group " + group_name + ".\n");
                Logger::log_info(username + " left group " + group_name);
                // Notify remaining members in the group
                for (int member_socket : groups[group_name]) {
                    send_message(member_socket, username + " has left the group " + group_name + ".\n");
                }
                // Remove group if it becomes empty
                if (groups[group_name].empty()) {
                    groups.erase(group_name);
                    Logger::log_info("Group " + group_name + " removed as it became empty.");
                }
            }
        } else {
            send_message(client_socket, "Group " + group_name + " does not exist.\n");
            Logger::log_error("Leave group failed: Group " + group_name + " does not exist for user " + username);
        }
    }
}

void processGroupMessage(int client_socket, const std::string& message, const std::string& username) {
    size_t space1 = message.find(' ');
    size_t space2 = message.find(' ', space1 + 1);
    if (space1 == std::string::npos || space2 == std::string::npos) {
        send_message(client_socket, "Invalid syntax. Use: /group_msg <group_name> <message>\n");
        Logger::log_error("Invalid /group_msg syntax from " + username);
        return;
    }
    std::string group_name = message.substr(space1 + 1, space2 - space1 - 1);
    group_name = group_name.substr(0, group_name.find_first_of("\r\n\0"));
    std::string group_message = message.substr(space2 + 1);
    group_message = group_message.substr(0, group_message.find_first_of("\r\n\0"));

    {
        std::lock_guard<std::mutex> lock(groups_mutex);
        if (groups.find(group_name) != groups.end()) {
            if (groups[group_name].find(client_socket) == groups[group_name].end()) {
                send_message(client_socket, "You are not a member of group " + group_name + ".\n");
                Logger::log_error(username + " attempted to send a group message to " + group_name + " but is not a member");
            } else {
                for (int sock : groups[group_name]) {
                    if (sock != client_socket) {
                        send_message(sock, "[" + username + "][Group " + group_name + "]: " + group_message + "\n");
                    }
                }
                Logger::log_info(username + " sent a group message to group " + group_name);
            }
        } else {
            send_message(client_socket, "Group " + group_name + " does not exist.\n");
            Logger::log_error("Group message failed: Group " + group_name + " does not exist for user " + username);
        }
    }
}

// Handling client authentication
bool authenticate_client(int client_socket, std::string & username) {
    char buffer[BUFFER_SIZE];
    // Prompt for username
    send_message(client_socket, "Enter username: ");
    memset(buffer, 0, sizeof(buffer));
    if (recv(client_socket, buffer, sizeof(buffer), 0) <= 0) {
        Logger::log_error("Failed to read username from socket " + std::to_string(client_socket));
        return false;
    }
    username = std::string(buffer);
    username = username.substr(0, username.find_first_of("\r\n\0"));

    // Prompt for password
    send_message(client_socket, "Enter password: ");
    memset(buffer, 0, sizeof(buffer));
    if (recv(client_socket, buffer, sizeof(buffer), 0) <= 0) {
        Logger::log_error("Failed to read password for user " + username);
        return false;
    }
    std::string password(buffer);
    password = password.substr(0, password.find_first_of("\r\n\0"));

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        if (users.find(username) != users.end() && users[username] == password) {
            // First add the client to our list
            clients[client_socket] = username;
            Logger::log_info("User " + username + " authenticated successfully.");
            
            // Send welcome message to the new client
            send_message(client_socket, "Welcome to the chat server!\n");
            
            // Send the list of currently online users to the new client
            if (clients.size() > 1) {  // If there are other users online
                std::string online_users = "Currently online users: ";
                for (const auto& [_, user] : clients) {
                    if (user != username) {  // Don't include the new user
                        online_users += user + ", ";
                    }
                }
                // Remove the last comma and space
                if (online_users.length() > 2) {
                    online_users = online_users.substr(0, online_users.length() - 2);
                }
                send_message(client_socket, online_users + "\n");
            }
            
            // Notify other clients that a new user has joined
            for (const auto& [sock, user] : clients) {
                if (sock != client_socket) {
                    send_message(sock, username + " has joined the chat\n");
                }
            }
            return true;
        } else {
            send_message(client_socket, "Authentication failed.\n");
            Logger::log_error("Authentication failed for user " + username);
            return false;
        }
    }
}

// New function to process a client message using token splitting
void processClientMessage(int client_socket, const std::string & message, const std::string & username) {
    std::vector<std::string> tokens = split(message);
    if (tokens.empty()) {
        send_message(client_socket, "Empty command received.\n");
        Logger::log_error("Empty command received from " + username);
        return;
    }
    // Dispatch based on the first token which is the command
    if (tokens[0] == "/msg") {
        processPrivateMessage(client_socket, message, username);
    } else if (tokens[0] == "/broadcast") {
        processBroadcastMessage(client_socket, message, username);
    } else if (tokens[0] == "/create_group") {
        processCreateGroup(client_socket, message, username);
    } else if (tokens[0] == "/join_group") {
        processJoinGroup(client_socket, message, username);
    } else if (tokens[0] == "/leave_group") {
        processLeaveGroup(client_socket, message, username);
    } else if (tokens[0] == "/group_msg") {
        processGroupMessage(client_socket, message, username);
    } else {
        send_message(client_socket, "Unknown command.\n");
        Logger::log_error("Unknown command received from " + username + ": " + message);
    }
}

// Main client handler function
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    std::string username;

    // Use the new authentication helper
    if (!authenticate_client(client_socket, username)) {
        close(client_socket);
        return;
    }

    // Handle incoming messages
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int recv_size = recv(client_socket, buffer, sizeof(buffer), 0);
        if (recv_size <= 0) {
            break;
        }
        std::string message(buffer);
        processClientMessage(client_socket, message, username);
    }

    // Cleanup after the client disconnects
    close(client_socket);
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(client_socket);
    }
    // Notify remaining clients about the disconnection
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (const auto& [sock, user] : clients) {
            send_message(sock, username + " has left the chat\n");
        }
    }
    Logger::log_info("User " + username + " disconnected.");
}

// Main server function to accept and handle incoming connections
int main() {
    // Load allowed users from the file
    users = load_users("users.txt");

    // Create the server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        Logger::log_error("Error creating socket.");
        return 1;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(12345);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (sockaddr*)&server_address, sizeof(server_address)) < 0) {
        Logger::log_error("Error binding socket.");
        return 1;
    }

    if (listen(server_socket, 5) < 0) {
        Logger::log_error("Error listening for connections.");
        return 1;
    }

    Logger::log_info("Server listening on port 12345...");

    // Accept and handle clients in separate threads
    while (true) {
        sockaddr_in client_address{};
        socklen_t client_address_size = sizeof(client_address);
        int client_socket = accept(server_socket, (sockaddr*)&client_address, &client_address_size);
        if (client_socket < 0) {
            Logger::log_error("Error accepting connection.");
            continue;
        }
        Logger::log_info("New connection accepted. Waiting for authentication...");
        std::thread client_thread(handle_client, client_socket);
        client_thread.detach(); 
    }

    close(server_socket);
    return 0;
}
