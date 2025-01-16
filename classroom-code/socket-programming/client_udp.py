import socket

SERVER_HOST = "127.0.0.1"  # Server's IP address
SERVER_PORT = 12345        # Server's port

# Create a UDP socket
with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as client_socket:
    message = "Hello, Server!"
    
    # Send a message to the server
    client_socket.sendto(message.encode(), (SERVER_HOST, SERVER_PORT))
    
    # Wait for the server's response
    data, server = client_socket.recvfrom(1024)  # Buffer size is 1024 bytes
    print(f"Received from server: {data.decode()}")

