# udp_tools
udp_echo measures RTT(round trip time) for a packet traveling between client and server over UDP. 

All packets except Report Packet are 1400 bytes. 

Functionalities: 
#### Echo
* Client sends an Echo Packet with a sequence number to the server.
* Server sends back an Echo Packet with an 
and server responds with an Echo Packet with the same sequence number. 
* Client calculates the RTT(round trip time) when received the Echo Packet from server. 

#### Interarrival Time
* Client sends SEND_NUM packets and the last packet has a sequence number -1
* Server receives packets until the last packet with a sequence number -1
* Server calculates interarrival time and bandwidth, and sends REPORT_NUM report packets to client to avoid packet loss 
* Then the server sends SEND_NUM packets to client and client calculates interarrival time and bandwidth. Note that the server obtained SEND_NUM by counting number of received packets.

#### Byte Stream Measurement
* Client sends a bytes stream with a speed of N Mbits/s for T seconds. Similarly, the last packet is marked with a sequence number -1. 
* Server receives the bytes steam and calculate the speed. 
* Server sends REPORT_NUM packets to client.