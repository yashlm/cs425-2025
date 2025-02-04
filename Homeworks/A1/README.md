# Chat Server Implementation

A multi-threaded chat server implementation supporting private messages, group communication, and user authentication. This project demonstrates concurrent programming, socket communication, and thread synchronization in C++.

## Features

- User authentication with username/password
- Private messaging between users
- Broadcast messages to all users
- Group chat functionality
  - Create groups
  - Join/leave groups
  - Send messages to groups
- Real-time notifications for user join/leave events
- Thread-safe logging system
- Concurrent client handling
- Stress testing capabilities

## Building the Project

The project uses a Makefile for compilation. To build all components:

```bash
make clean  # Clean previous builds
make        # Compile server, client, and stress test
```

This will create three executables:
- `server_grp`: The chat server
- `client_grp`: The chat client
- `stress_test`: Stress testing utility

## Running the Server

Start the server:
```bash
./server_grp
```

The server will:
1. Load user credentials from `users.txt`
2. Listen on port 12345
3. Accept incoming client connections
4. Handle client authentication
5. Process messages and group operations

## Client Commands

The following commands are supported:

- `/msg <username> <message>`: Send private message
- `/broadcast <message>`: Send message to all users
- `/create_group <group_name>`: Create a new group
- `/join_group <group_name>`: Join an existing group
- `/leave_group <group_name>`: Leave a group
- `/group_msg <group_name> <message>`: Send message to group members

## Implementation Details

### Server Architecture

1. **Thread Management**
   - Main thread accepts incoming connections
   - Each client gets a dedicated thread
   - Thread-safe data structures using mutex locks

2. **Data Structures**
   ```cpp
   std::unordered_map<int, std::string> clients;        // Socket -> Username
   std::unordered_map<std::string, std::string> users;  // Username -> Password
   std::unordered_map<std::string, std::unordered_set<int>> groups;  // Group -> Members
   ```

3. **Synchronization**
   - `clients_mutex`: Protects client list
   - `groups_mutex`: Protects group operations
   - `log_mutex`: Ensures thread-safe logging

4. **Message Processing**
   - Command parsing using string tokenization
   - Message routing based on command type
   - Error handling for invalid commands

### Stress Testing

The project includes a stress testing utility (`stress_test.cpp`) to evaluate server performance under load.

#### Stress Test Features

- Simulates multiple concurrent clients
- Randomized message patterns
- Authentication testing
- Group operation testing
- Configurable parameters:
  ```cpp
  #define NUM_CLIENTS 100       // Number of concurrent clients
  #define TEST_DURATION 60      // Test duration in seconds
  #define BUFFER_SIZE 1024      // Message buffer size
  ```

#### Running Stress Tests

1. Start the server:
   ```bash
   ./server_grp
   ```

2. In a separate terminal, run stress test:
   ```bash
   ./stress_test
   ```

The stress test will:
- Create multiple client connections
- Authenticate with random valid credentials
- Send various types of messages
- Perform group operations
- Monitor connection success/failure

#### Test Messages

The stress test includes various message types:
```cpp
std::vector<std::string> test_messages = {
    "/broadcast Hello everyone!",
    "/create_group TestGroup",
    "/join_group TestGroup",
    "/group_msg TestGroup Test message",
    "/leave_group TestGroup",
    "/msg alice Hello there!"
};
```

## Error Handling

The implementation includes robust error handling for:
- Connection failures
- Authentication errors
- Invalid commands
- Group operation errors
- Network communication issues

## Logging System

Thread-safe logging system with two levels:
```cpp
Logger::log_info("Informational message");
Logger::log_error("Error message");
```

Logs include timestamps and message types.


