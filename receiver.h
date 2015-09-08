//
//  receiver.h
//  fscp
//

#ifndef __fscp__receiver__
#define __fscp__receiver__

#include <stdio.h>
#include <iostream>
#include <stdint.h>
#include <cstring>
#include <string.h>
#include <cstdlib>
#include <unistd.h> 

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <fcntl.h>

#include <pthread.h>

#include "define.h"

using namespace std;

void init_receiver();

void set_ack(unsigned char **chunks_ack, unsigned long long chunk_id, bool is_ack);
bool is_ack(unsigned char **chunks_ack, unsigned long long chunk_id);

#endif /* defined(__fscp__receiver__) */

