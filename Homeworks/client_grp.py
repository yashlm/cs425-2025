import socket
import threading

# Receive messages from the server
def receive_messages(client_socket):
    while True:
        try:
            message = client_socket.recv(1024).decode()
            print(message)
        except:
            print("Disconnected from server.")
            client_socket.close()
            break

# Connect to the server
def main():
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect(("127.0.0.1", 12345))

    # Start thread to receive messages
    threading.Thread(target=receive_messages, args=(client_socket,), daemon=True).start()

    # Send messages to the server
    while True:
        message = input()
        if message.lower() == "/exit":
            client_socket.close()
            break
        client_socket.sendall(message.encode())

if __name__ == "__main__":
    main()

