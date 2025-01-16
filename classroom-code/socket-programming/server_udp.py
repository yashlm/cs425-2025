import socket

HOST = "127.0.0.1"  # Server's IP address
PORT = 12345        # Port to listen on

# Create a UDP socket
with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as server_socket:
    server_socket.bind((HOST, PORT))
    print(f"Server listening on {HOST}:{PORT}")
    
    while True:
        # Receive data and the sender's address
        data, addr = server_socket.recvfrom(1024)  # Buffer size is 1024 bytes
        print(f"Received message from {addr}: {data.decode()}")
        
        # Send a response to the sender
        response = f"Hello, {addr}. You said: {data.decode()}"
        server_socket.sendto(response.encode(), addr)

