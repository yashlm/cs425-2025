import socket

HOST = "127.0.0.1"  # Server IP
PORT = 12345        # Server port

# Create a TCP socket
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
    server_socket.bind((HOST, PORT))
    server_socket.listen()
    print(f"Server listening on {HOST}:{PORT}")
    
    # Accept a connection
    conn, addr = server_socket.accept()  # addr contains client's IP and port
    with conn:
        print(f"Connected by {addr}")
        while True:
            data = conn.recv(1024)
            if not data:
                break
            print(f"Received: {data.decode()}")
            conn.sendall(data)  # Echo the data back to the client

