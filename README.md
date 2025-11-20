# Multi-User Chat System in C

A real-time multi-user chat application built in C using TCP sockets and POSIX threads.

DEMO - 
Server : 
<img width="983" height="527" alt="image" src="https://github.com/user-attachments/assets/9d652f7b-f53c-4f7e-aac1-c709e3a10dfe" />

Multi - Client View : 
<img width="1118" height="481" alt="image" src="https://github.com/user-attachments/assets/fcf062b2-30f0-489b-bfe4-7724aad91c9b" />
<img width="1119" height="461" alt="image" src="https://github.com/user-attachments/assets/8d0da84b-8f2f-4a75-a6b0-8e12a33a57a4" />
<img width="1084" height="296" alt="image" src="https://github.com/user-attachments/assets/144b9a23-d16b-4182-949a-e9b7128567e0" />





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
