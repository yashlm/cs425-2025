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
  - *Create Group (/create_group <group_name>)*: Any user can create a new group
  - *Join Group (/join_group <group_name>)*: Users can join existing groups
  - *Leave Group (/leave_group <group_name>)*: Members can leave groups
  - *Group Messages (/group_msg <group_name> <message>)*: Send to all group members
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
### *Key Data Structures*
```cpp
// Client management
std::unordered_map<int, std::string> clients;        // Socket -> Username
std::unordered_map<std::string, std::string> users;  // Username -> Password
std::unordered_map<std::string, std::unordered_set<int>> groups;  // Group -> Member sockets
```

### *Core Functions*
- *Server Operations*:
  - `main()`: Server initialization and client acceptance
  - `handle_client()`: Per-client message processing
  - `authenticate_client()`: User validation
- *Message Processing*:
  - `processClientMessage()`: Command routing
  - `processPrivateMessage()`: Direct messages
  - `processBroadcastMessage()`: All-user messages
  - `processGroupMessage()`: Group communication
- *Group Management*:
  - `processCreateGroup()`: New group creation
  - `processJoinGroup()`: Group membership
  - `processLeaveGroup()`: Member removal
- *Utility Functions*:
  - `send_message()`: Reliable message delivery
  - `split()`: Command parsing
  - `load_users()`: Configuration loading

### *Message Flow*
1. Client connects and authenticates
2. Server creates dedicated client thread
3. Thread processes incoming messages
4. Messages are routed based on command
5. Responses sent to appropriate recipients
6. Logging captures all operations
7. Cleanup on client disconnect

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
                               +----------------------------------+
                                               |
                                               v
                               +----------------------------------+
                               |         Process messages         |
                               +----------------------------------+
                                               |
                                               v
                          +--------------------------------------------+
                          |                 Message Type               |
                          +--------------------------------------------+
                            /                     |                        \
                           v                      v                         v
               +----------------+        +------------------+       +-------------------+
               | Private Message |       |   Group Message  |       | Broadcast Message |
               |     (/msg)      |       |   (/group_msg)   |       |   (/broadcast)    |
               +----------------+        +------------------+       +-------------------+
                                                   |
                                                   v
                                        +---------------------+
                                        |     Log activity    |
                                        +---------------------+
                                                   |
                                                   v
                                        +---------------------+
                                        |  Client disconnects |
                                        +---------------------+
                                                   |
                                                   v
                                        +---------------------+
                                        |   Cleanup & remove  |
                                        +---------------------+

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