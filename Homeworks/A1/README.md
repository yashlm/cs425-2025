# *Multi-Client Chat Server - README*

## *Project Overview*
This project implements a *multi-client chat server* using C++ and *POSIX sockets*, supporting:
- *User authentication* with username/password from users.txt
- *Private messaging* between authenticated users
- *Group messaging and management* with create/join/leave functionality
- *Broadcast messaging* to all connected users
- *Logging and error handling* with thread-safe implementation
- *Multi-threaded architecture* for concurrent client handling

Additionally, a *stress testing tool* is included to simulate *100 concurrent clients* concurrent clients for evaluating server performance.

---

## *Features Implemented*
- *User Authentication*: 
  - Validates users against credentials stored in users.txt
  - Secure password checking
  - Notifies users of successful/failed authentication
  - Shows currently online users upon login
  - Notifies all users when someone joins/leaves
- *Private Messaging (/msg)*:
  - Direct messaging between users
  - Format: `/msg <username> <message>`
  - Error handling for non-existent users
  - Thread-safe message delivery
- *Broadcast Messaging (/broadcast)*:
  - Sends messages to all connected clients
  - Format: `/broadcast <message>`
  - Efficient delivery using client socket map
- *Group Management*:
  - *Create Group (`/create_group <group_name>`)*: Any user can create a new group
  - *Join Group (`/join_group <group_name>`)*: Users can join existing groups
  - *Leave Group (`/leave_group <group_name>`)*: Members can leave groups
  - *Group Messages (`/group_msg <group_name> <message>`)*: Send to all group members
  - Groups are automatically cleaned up when empty
- *Thread-Safe Logging*: 
  - Info and error level logging
  - Timestamps on all log entries
  - Mutex-protected log operations
- *Concurrent Client Handling*:
  - Each client runs in a dedicated thread
  - Thread-safe data structures
  - Proper cleanup on disconnection
- *Graceful Error Handling*:
  - Connection failures
  - Authentication errors
  - Invalid commands
  - Non-existent users/groups
  - Network issues

---

## *Design Decisions*
### *Threading Model*
- *One Thread Per Client*:
  - Dedicated thread for each connected client
  - Main thread handles new connections
  - Clean thread detachment after client disconnects
- *Thread Safety*:
  - Mutex protection for shared resources
  - Lock guards to prevent deadlocks
  - Thread-safe message queues

### *Synchronization Mechanisms*
| *Shared Resource* | *Protection Mechanism* | *Purpose* |
|------------------|------------------------|-----------|
| clients map | clients_mutex | Protects client list modifications |
| groups map | groups_mutex | Ensures atomic group operations |
| logging system | log_mutex | Prevents log corruption |
| socket operations | Per-socket locking | Ensures atomic message sending |

### *Error Handling & Logging*
- Errors are logged using Logger::log_error.
- Successful operations are recorded using Logger::log_info.
- *Comprehensive Logging*:
  - Connection events
  - Authentication attempts
  - Message delivery status
  - Group operations
  - Client disconnections
- *Error Categories*:
  - Network errors (connection, send, receive)
  - Authentication failures
  - Invalid commands
  - Resource access errors
  - Group operation failures
  - Errors include *failed connections, authentication failures, message delivery failures*.

---

## *Implementation Details*
#### *Core Data Structures*
```cpp
// Thread-safe client management
std::mutex clients_mutex;
std::unordered_map<int, std::string> clients;        // Socket -> Username
std::unordered_map<std::string, std::string> users;  // Username -> Password

// Thread-safe group management
std::mutex groups_mutex;
std::unordered_map<std::string, std::unordered_set<int>> groups;  // Group -> Member sockets

// Thread-safe logging
std::mutex log_mutex;
```

#### *Key Design Patterns*
1. *Thread-Per-Client Pattern*
   - Each client connection runs in its own thread
   - Allows concurrent handling of multiple clients
   - Thread detachment for automatic cleanup

2. *Resource Protection Pattern*
   - Mutex locks for shared resources
   - RAII-style lock guards
   - Prevents race conditions

3. *Message Router Pattern*
   - Command parsing and routing
   - Dedicated handler functions
   - Clean separation of concerns

4. *Observer Pattern*
   - Clients notified of user join/leave events
   - Group members notified of messages
   - Broadcast capabilities

#### *Core Functions*

1. *Server Operations*:
   ```cpp
   int main()                 // Server initialization, socket setup, client acceptance
   void handle_client()       // Per-client message processing thread
   bool authenticate_client() // User validation against users.txt
   ```

2. *Message Processing*:
   ```cpp
   void processClientMessage()    // Command parsing and routing
   void processPrivateMessage()   // Direct user-to-user messages
   void processBroadcastMessage() // Messages to all users
   void processGroupMessage()     // Messages to group members
   ```

3. *Group Management*:
   ```cpp
   void processCreateGroup() // Create new group
   void processJoinGroup()   // Add user to group
   void processLeaveGroup()  // Remove user from group
   ```

4. *Thread-Safe Operations*:
   ```cpp
   void send_message()       // Reliable message delivery
   std::vector<std::string> split() // Command parsing
   void Logger::log_info()   // Thread-safe logging
   void Logger::log_error()  // Error logging
   ```

#### *Error Handling*
1. *Network Errors*
   - Socket creation failures
   - Connection drops
   - Send/receive errors

2. *Authentication Errors*
   - Invalid credentials
   - Duplicate logins
   - File access issues

3. *Message Errors*
   - Invalid commands
   - Missing recipients
   - Group not found

4. *Resource Errors*
   - Memory allocation
   - Thread creation
   - Mutex deadlocks

#### *Message Flow Details*

1. *Connection Phase*
   ```
   Client -> Server: TCP Connection request
   Server -> Client: Accept connection
   Server: Create dedicated client thread
   ```

2. *Authentication Phase*
   ```
   Server -> Client: "Enter username:"
   Client -> Server: username
   Server -> Client: "Enter password:"
   Client -> Server: password
   Server: Validate credentials
   Server -> Client: Welcome/Error message
   ```

3. *Message Processing Phase*
   ```
   Client -> Server: Command message
   Server: Parse command
   Server: Acquire necessary locks
   Server: Process message
   Server -> Recipients: Deliver message
   Server: Release locks
   Server: Log activity
   ```

4. *Disconnection Phase*
   ```
   Client: Connection closes
   Server: Detect disconnection
   Server: Remove from clients map
   Server: Remove from all groups
   Server: Notify other clients
   Server: Clean up resources
   Server: Log disconnection
   ```

### *Code Flow Diagram*
```cpp
                                    +---------------------+
                                    |     Start Server    |
                                    +---------------------+
                                               |
                                               v
                                    +---------------------+
                                    |    Load users.txt   |
                                    +---------------------+
                                               |
                                               v
                                    +----------------------+
                                    | Listen on port 12345 |
                                    +----------------------+
                                               |
                                               v
                                    +---------------------+
                                    |     Accept client   |
                                    +---------------------+
                                               |
                                               v
                              +-------------------------------------+
                              |     Create thread for each client   |
                              +-------------------------------------+
                                               |
                                               v
                               +----------------------------------+
                               |        Authenticate client       |
                               |   1. Request username/password   |
                               |   2. Validate against users.txt  |
                               |   3. Add to clients map if OK    |
                               +----------------------------------+
                                               |
                                               v
                               +----------------------------------+
                               |      Send Welcome Messages       |
                               |   1. Welcome message to client   |
                               |   2. Show online users list      |
                               |   3. Notify others of new join   |
                               +----------------------------------+
                                               |
                                               v
                               +----------------------------------+
                               |      Process Client Messages     |
                               |   1. Receive client message      |
                               |   2. Parse command & arguments   |
                               |   3. Route to handler function   |
                               +----------------------------------+
                                               |
                                               v
                          +--------------------------------------------+
                          |              Message Handlers              |
                          +--------------------------------------------+
                            /                   |                   \
                           v                    v                    v
               +-----------------+     +-------------------+    +-------------------+
               | Private Message |     |   Group Message   |    | Broadcast Message |
               |   1. Find user  |     | 1. Verify group   |    |  1. Send to all   |
               |   2. Send msg   |     | 2. Check member   |    |  2. Log activity  |
               |   3. Log event  |     | 3. Send to group  |    +-------------------+
               +-----------------+     +-------------------+
                                               |
                                               v
                                +--------------------------------+
                                |        Thread-Safe Ops         |
                                | 1. Mutex-protected access      |
                                | 2. Lock guards for resources   |
                                | 3. Safe message delivery       |
                                +--------------------------------+
                                               |
                                               v
                                +--------------------------------+
                                |      Client Disconnection      |
                                | 1. Close socket                |
                                | 2. Remove from clients map     |
                                | 3. Remove from groups          |
                                | 4. Notify other clients        |
                                | 5. Log disconnection          |
                                +--------------------------------+
```

---

## *Building and Running*
### *Compilation*
```bash
make clean  # Remove old binaries
make        # Build server, client, and stress test
```

### *Running Components*
1. *Start Server*:
   ```bash
   ./server_grp
   ```
2. *Connect Clients*:
   ```bash
   ./client_grp
   ```
3. *Run Stress Test*:
   ```bash
   ./stress_test
   ```
#### The code was run and tested on WSL Ubuntu Enviornment (5.15.167.4-microsoft-standard-WSL2, Ubuntu 22.04.3 LTS).

---

## *Stress Testing*
A *stress test tool* (stress_test.cpp) was implemented to simulate *100 concurrent clients* connecting and interacting with the chat server. The Makefile was modified to include its execution.

### *Test Parameters*
- *Number of Clients*: 100
- *Test Duration*: 200 seconds
- *Message Types*:
  - Private messages
  - Group messages
  - Broadcasts
  - Random delays to mimic real usage

### *Running the Stress Test*
1. *Compile the Stress Test Code*
   bash
   g++ -o stress_test stress_test.cpp -pthread
   
2. *Run the Stress Test*
   bash
   ./stress_test
   
3. *Monitor Performance*
   bash
   htop    # CPU & Memory usage
   netstat -anp | grep 12345  # Active connections
   

### *Observed Performance*
| *Metric* | *Result* |
|-----------|------------|
| Max concurrent users | 100 |
| Server CPU usage | 35-45% |
| Server memory usage | ~200MB |
| Average response time | 15-30ms |

---

## *Challenges & Solutions*
| *Challenge* | *Solution Implemented* |
|--------------|-------------------------|
| *Race conditions in shared data* | Used *mutex locks* to synchronize access |
| *Handling client disconnections* | Removed clients from groups when they disconnected |
| *Overlapping messages causing corruption* | Used memset(buffer, 0, sizeof(buffer)) before recv() calls |
| *Authentication failure handling* | Sent an error message and closed the connection |

---

## *Server Restrictions*
| *Limitation* | *Reason* |
|--------------|---------|
| Max concurrent clients | Limited by system memory & threads |
| Max groups | No enforced limit but affected by memory |
| Max group members | No enforced limit but impacted by performance |
| Max message size | *1024 bytes* due to buffer constraint |

---

## *Individual Contributions*
| *Member* | *Contributions* | *Percentage* |
|---------|--------------------|-------------|
| *Pushkar Aggarwal(220839)* | Server setup, authentication, threading | 33.33% |
| *Yash Chauhan(221217)* | Messaging logic, group management, testing scripts, logging | 33.33% |
| *Ansh Agarwal(220165)* | Debugging, stress testing, documentation, README | 33.33% |

All members have eagerly took part in the assignment and done their best.

---

## *Sources Referred*
- *Books*: "Computer Networking: A Top-Down Approach"
- *Blogs*: Stack Overflow, GeeksforGeeks (Socket Programming)
- *Websites*: Cplusplus.com, Beej’s Guide to Network Programming
- *Course Stuff*: Lectures slides and the piazza for doubts. 

---

## *Declaration*
We hereby declare that *this project is our original work*. We did not indulge in plagiarism, and any external resources used have been duly credited.

---

## *Feedback*
### *Our Implemententation*
✔ Efficient *multi-threaded* implementation.  
✔ Proper *logging* and *error handling*.  
✔ *Scalable design* supporting multiple clients.

---