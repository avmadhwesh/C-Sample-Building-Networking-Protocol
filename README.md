📄 Repository Description
Reliable Networking Protocol Implementation

This project simulates the transport layer of a network stack by implementing a custom Reliable Transport Protocol (RTP) using the Stop-and-Wait protocol. The system ensures reliable message delivery over an unreliable network by handling packet segmentation, checksum validation, acknowledgment (ACK/NACK) handling, and message reassembly. The client sends encrypted (ROT13) messages to a server, which decrypts and returns the message after successful transmission.

🛠️ What I worked on
`src/rtp.c` & 
`src/rtp.h`

- Implemented the packetization process to divide large messages into fixed-size packets, marking the last packet appropriately.

- Developed a custom checksum algorithm to detect packet corruption by swapping character pairs and computing a weighted sum of ASCII values.

- Built the Stop-and-Wait protocol logic, handling ACK/NACK acknowledgments and retransmissions.

- Managed multi-threaded communication using thread synchronization (mutexes and condition variables) to ensure thread-safe queue operations.

- Designed the receive thread to validate packet integrity, request retransmissions if needed, and reassemble complete messages.

- Handled robust resource management, including memory allocation and deallocation, to prevent memory leaks.

-Ensured clean compilation with no warnings, well-documented code, and adherence to concurrency best practices.

