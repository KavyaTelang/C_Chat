# Multi-User Chat System in C

A real-time multi-user chat application built in C using TCP sockets and POSIX threads.

## Features
- Multi-client support (up to 100 concurrent users)
- Real-time message broadcasting
- Join/leave notifications
- Timestamped messages
- Thread-safe client management

## Compilation
```bash
make
```

## Usage

Start server:
```bash
./server 8080
```

Connect clients:
```bash
./client localhost 8080
```

## Technologies
- TCP Sockets
- POSIX Threads
- Mutex Synchronization
- Linux/WSL Environment

## Commands
- `/quit` - Leave the chat
- `/users` - Show active users
