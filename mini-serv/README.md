# mini_serv (Exam 06)

A simple chat server implementation in C that listens for clients on a specific port (127.0.0.1) and broadcasts messages between them. This project demonstrates the use of `select()` for non-blocking I/O multiplexing.

## Features
- Manages multiple client connections simultaneously.
- Broadcasts messages from one client to all other connected clients.
- Notifies when a client joins or leaves the chat.
- Handles partial messages and multiple lines correctly.
- No memory leaks (Proper cleanup on disconnect).

## Compilation
Compile the program using `gcc`:

```bash
gcc -Wall -Wextra -Werror mini_serv.c -o mini_serv
```

## Usage
Run the server by specifying a port number:

```bash
./mini_serv <port>
```

Example:
```bash
./mini_serv 8080
```

## Testing

### 1. Manual Testing with Netcat
You can open multiple terminal windows to simulate different clients.

**Terminal 1 (Server):**
```bash
./mini_serv 8080
```

**Terminal 2 (Client 1):**
```bash
nc 127.0.0.1 8080
```

**Terminal 3 (Client 2):**
```bash
nc 127.0.0.1 8080
```
*Anything typed in Client 1 should appear in Client 2, and vice versa.*

### 2. Automated Testing Suite
A comprehensive test script is included to verify all functionalities (Notification, Broadcasting, Partial messages, etc.).

**How to run:**
```bash
chmod +x comprehensive_test.sh
./comprehensive_test.sh
```

**What it tests:**
1.  **Connection Notifications:** Checks if existing clients are notified when a new client joins.
2.  **Broadcasting:** Verifies that messages sent by one client are received by others.
3.  **Disconnect Notifications:** Checks if clients are notified when someone leaves.
4.  **Partial Messages:** Ensures the server buffers split messages correctly.
5.  **Multi-line Messages:** Ensures the server processes multiple newline-separated messages correctly.

## Implementation Details
- Uses `select()` to handle multiple file descriptors without blocking.
- `FD_ISSET`, `FD_SET`, `FD_CLR`, `FD_ZERO` macros are used for managing the file descriptor sets.
- `SO_REUSEADDR` is used to allow immediate restart of the server on the same port.