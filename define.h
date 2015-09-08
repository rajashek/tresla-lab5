//
//  define.h
//  fscp
//

#ifndef fscp_define_h
#define fscp_define_h

#define FSCP_UDP_PORT 45559

#define SOCKET_RCVBUFFER 212992     // net.core.rmem_max
#define SOCKET_SNDBUFFER 212992     // net.core.wmem_max

#define UDP_RCV_BUFFER_SIZE 1600

#define UDP_DATA_MAX_LENGTH 1472
#define FSCP_UDP_ID_BYTES 3
#define FSCP_UDP_DATA_BYTES 1469

#define FSCP_DEFAULT_NUMBERS_OF_ACK 1

//#define _DEBUG
//#define _DEBUG_VERBOSE

#endif
