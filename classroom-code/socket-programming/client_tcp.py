import socket

HOST = "127.0.0.1"  # Server's IP address
PORT = 12345        # Server's port

# Create a TCP socket
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
    client_socket.connect((HOST, PORT))
    message = "Hello, Server!"
    client_socket.sendall(message.encode())
    data = client_socket.recv(1024)
    print(f"Received from server: {data.decode()}")

