#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <sstream>
#include <fstream>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

#define PORT 12345
#define BUFFER_SIZE 1024

std::unordered_map<int, std::string> clients;                    // Client socket -> username
std::unordered_map<std::string, std::string> users;              // Username -> password
std::unordered_map<std::string, std::unordered_set<int>> groups; // Group -> client sockets
std::mutex clients_mutex, groups_mutex;

// Function to load user credentials from users.txt
void load_users()
{
    std::ifstream file("users.txt");
    if (!file)
    {
        std::cerr << "Error: users.txt file not found!\n";
        return;
    }
    std::string line, username, password;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        if (std::getline(iss, username, ':') && std::getline(iss, password))
        {
            users[username] = password;
        }
    }
    file.close();
}

// Function to send a message to a specific client
void send_message(int client_socket, const std::string &message)
{
    send(client_socket, message.c_str(), message.size(), 0);
}

// Function to handle private messages
void private_message(int sender_socket, const std::string &recipient, const std::string &message)
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto &pair : clients)
    {
        if (pair.second == recipient)
        {
            send_message(pair.first, "[Private] " + clients[sender_socket] + ": " + message + "\n");
            return;
        }
    }
    send_message(sender_socket, "User not found!\n");
}

// Function to broadcast messages to all clients
void broadcast_message(int sender_socket, const std::string &message)
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto &pair : clients)
    {
        if (pair.first != sender_socket)
        {
            send_message(pair.first, "[Broadcast] " + clients[sender_socket] + ": " + message + "\n");
        }
    }
}

// Function to send group messages
void group_message(int sender_socket, const std::string &group_name, const std::string &message)
{
    std::lock_guard<std::mutex> lock(groups_mutex);
    if (groups.find(group_name) == groups.end())
    {
        send_message(sender_socket, "Group does not exist!\n");
        return;
    }
    if (groups[group_name].find(sender_socket) == groups[group_name].end())
    {
        send_message(sender_socket, "You are not part of this group!\n");
        return;
    }
    for (int socket : groups[group_name])
    {
        if (socket != sender_socket)
        {
            send_message(socket, "[Group " + group_name + "] " + clients[sender_socket] + ": " + message + "\n");
        }
    }
}

// Function to handle client commands
void handle_client(int client_socket)
{   
    std::string username, password;
    char buffer[BUFFER_SIZE];

    // Send username prompt
    std::string prompt = "Enter username: ";
    send(client_socket, prompt.c_str(), prompt.size(), 0);

    memset(buffer, 0, BUFFER_SIZE);
    recv(client_socket, buffer, BUFFER_SIZE, 0);
    username = buffer;

    // Send password prompt
    prompt = "Enter password: ";
    send(client_socket, prompt.c_str(), prompt.size(), 0);

    memset(buffer, 0, BUFFER_SIZE);
    recv(client_socket, buffer, BUFFER_SIZE, 0);
    password = buffer;

    // Simple authentication (modify as needed)
    if (username == "admin" && password == "password123") {
        std::string success = "Welcome to the server";
        send(client_socket, success.c_str(), success.size(), 0);
    } else {
        std::string failure = "Authentication failed";
        send(client_socket, failure.c_str(), failure.size(), 0);
        close(client_socket);
        return;
    }
    char buffer[BUFFER_SIZE];
    std::string username;

    // Authenticate user
    recv(client_socket, buffer, BUFFER_SIZE, 0);
    std::istringstream iss(buffer);
    std::string user, pass;
    iss >> user >> pass;

    if (users.find(user) == users.end() || users[user] != pass)
    {
        send_message(client_socket, "Authentication failed!\n");
        close(client_socket);
        return;
    }

    { // Lock and add client
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients[client_socket] = user;
    }

    send_message(client_socket, "Welcome to the chat server!\n");

    while (true)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0)
            break;

        std::string message(buffer);
        if (message.starts_with("/msg "))
        {
            size_t space1 = message.find(' ', 5);
            if (space1 != std::string::npos)
            {
                std::string recipient = message.substr(5, space1 - 5);
                std::string msg = message.substr(space1 + 1);
                private_message(client_socket, recipient, msg);
            }
        }
        else if (message.starts_with("/broadcast "))
        {
            broadcast_message(client_socket, message.substr(11));
        }
        else if (message.starts_with("/create_group "))
        {
            std::string group_name = message.substr(14);
            std::lock_guard<std::mutex> lock(groups_mutex);
            groups[group_name] = {client_socket};
            send_message(client_socket, "Group " + group_name + " created!\n");
        }
        else if (message.starts_with("/join_group "))
        {
            std::string group_name = message.substr(12);
            std::lock_guard<std::mutex> lock(groups_mutex);
            groups[group_name].insert(client_socket);
            send_message(client_socket, "Joined group " + group_name + "!\n");
        }
        else if (message.starts_with("/group_msg "))
        {
            size_t space = message.find(' ', 11);
            if (space != std::string::npos)
            {
                std::string group_name = message.substr(11, space - 11);
                std::string msg = message.substr(space + 1);
                group_message(client_socket, group_name, msg);
            }
        }
    }

    { // Lock and remove client
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(client_socket);
    }
    close(client_socket);
}

int main()
{
    load_users();

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    // sockaddr_in server_addr{AF_INET, htons(PORT), INADDR_ANY};
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));
    bind(server_socket, (sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, 10);

    std::cout << "Server running on port " << PORT << "...\n";

    while (true)
    {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (sockaddr *)&client_addr, &client_len);

        std::thread(handle_client, client_socket).detach();
    }

    return 0;
}
