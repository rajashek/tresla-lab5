//
//  sender.cpp
//  fscp
//

#include "sender.h"

void init_sender(char *host, FILE *file, long file_size, const char *file_name, unsigned char required_acks) {

    int sockfd, i;
    struct sockaddr_in sndraddr, rcvraddr;
    socklen_t rcvraddr_len;
    size_t recvlen;
    
    unsigned long long chunk_id = 0L, total_chunks, chunks_ack_count = 0L, j;
    unsigned long long chunk_id_first_unacked = 1L;
    unsigned long long chunk_id_last_cached = 0L;
    unsigned char *chunks_ack;
    
    unsigned char buffer[UDP_DATA_MAX_LENGTH];
    size_t file_read_len;
    
    struct timeval tv;
    
    memset(&sndraddr, 0, sizeof(sndraddr));
    sndraddr.sin_family=AF_INET;
    sndraddr.sin_addr.s_addr=htonl(INADDR_ANY);
    sndraddr.sin_port=htons(0);
    
    memset(&rcvraddr, 0, sizeof(rcvraddr));
    rcvraddr.sin_family=AF_INET;
    rcvraddr.sin_addr.s_addr=htonl(INADDR_ANY);
    rcvraddr.sin_port=htons(FSCP_UDP_PORT);

    fprintf(stdout, "        Filename: %s\n", file_name);
    fprintf(stdout, "        Filesize: %ld Bytes\n", file_size);
    fprintf(stdout, "   Required ACKs: %d\n", required_acks);
    
    // Calculate the number of chunks
    total_chunks = (file_size / FSCP_UDP_DATA_BYTES) + ((file_size % FSCP_UDP_DATA_BYTES > 0)?1:0);
    fprintf(stdout, "  Size of chunks: %d Bytes\n", FSCP_UDP_DATA_BYTES);
    fprintf(stdout, "Number of chunks: %llu (0x%.6llx)\n", total_chunks, total_chunks);
    
    // Chunk cache declaration
    unsigned char *chunk_cache[total_chunks];
    pthread_mutex_t *chunk_cache_mutex;
    chunk_cache_mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t) * total_chunks);
    
    #ifdef _DEBUG
    fprintf(stdout, "          Status: Connecting\n");
    #else
    fprintf(stdout, "          Status: Connecting");
    fflush(stdout);
    #endif
    
    // Initialize chuck acknowledgement bitmap
    chunks_ack = (unsigned char *) malloc(total_chunks/8 + 1);
    memset(chunks_ack, 0, total_chunks/8 + 1);
    
    // Initialize buffer
    //   Structure: CHUNK_ID|FILE_SIZE|REQUIRED_ACKS|FILE_NAME
    memset(buffer, 0, UDP_DATA_MAX_LENGTH);
    memcpy(buffer + FSCP_UDP_ID_BYTES, &file_size, sizeof(file_size)); // Add filesize info into the buffer
    memcpy(buffer + FSCP_UDP_ID_BYTES + sizeof(file_size), &required_acks, 1); // Add required number of acks into the buffer
    memcpy(buffer + FSCP_UDP_ID_BYTES + sizeof(file_size) + 1, file_name, strlen(file_name)); // Add file name into the buffer

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        fprintf(stderr, "Error: cannot create UDP socket in init_sender()\n");
        exit(1);
    }
    
    // Set Socket Options
    int so_sndbuf_size = SOCKET_SNDBUFFER;
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &so_sndbuf_size, sizeof(so_sndbuf_size)) < 0) {
        fprintf(stderr, "Error: cannot set SO_SNDBUF in init_sender()\n");
        exit(2);
    }
    int so_rcvbuf_size = SOCKET_RCVBUFFER;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &so_rcvbuf_size, sizeof(so_rcvbuf_size)) < 0) {
        fprintf(stderr, "Error: cannot set SO_RCVBUF in init_sender()\n");
        exit(2);
    }

    tv.tv_sec = 0;
    tv.tv_usec = 300*1000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval));

    // Bind socket
    if(::bind(sockfd ,(struct sockaddr *) &sndraddr, sizeof(sndraddr)) <0) {
        fprintf(stderr, "Error bind udp socket failed in init_sender()\n");
        exit(3);
    }
    
    inet_pton(AF_INET, host, &rcvraddr.sin_addr.s_addr);
    
    // Send the file information to the receiver
    while (true) {

        for(i=0; i<required_acks+1; i++) {
            
            sendto(sockfd, buffer, FSCP_UDP_ID_BYTES + sizeof(file_size) + 1 + strlen(file_name) + 1, 0, (struct sockaddr *)&rcvraddr, sizeof(rcvraddr));
            
            #ifdef _DEBUG
            fprintf(stdout, "[DEBUG] Send: 0x");
            for (unsigned int p=0; p<FSCP_UDP_ID_BYTES+sizeof(file_size) + 1 + strlen(file_name) + 1; p++) {
                if (p==FSCP_UDP_ID_BYTES) fprintf(stdout, " ");
                fprintf(stdout, "%.2x", buffer[p]);
            }
            fprintf(stdout, "\n");
            #endif
        }
        
        // Wait for ack (with receive timeout)
        recvlen = recvfrom(sockfd, &chunk_id, FSCP_UDP_ID_BYTES, 0, (struct sockaddr *)&rcvraddr, &rcvraddr_len);
        if (recvlen != -1UL) {
            if (recvlen == FSCP_UDP_ID_BYTES) {
                
                #ifdef _DEBUG
                fprintf(stdout, "[DEBUG] Recv: ACK 0x%.6llx\n", chunk_id);
                #endif
                
                if (chunk_id==0ULL) break;
            }
        }
        else {
            #ifdef _DEBUG
            fprintf(stdout, "[DEBUG] Timeout!\n");
            #endif
        }
    }
    
    #ifdef _DEBUG
    fprintf(stdout, "          Status: Connected\n");
    fprintf(stdout, "[DEBUG] -------------------------\n");
    #else
    fprintf(stdout, "\r          Status: Connected \n");
    fprintf(stdout, "        Progress:   0%%");
    fflush(stdout);
    #endif
    
    
    // Seek to the beginning of the file
    fseek(file, 0, SEEK_SET);
    
    // Start ACK receiver thread
    struct ack_receiver_thread_parameter params;
    params.sockfd = &sockfd;
    params.chunks_ack_count = &chunks_ack_count;
    params.total_chunks = &total_chunks;
    params.chunk_id_first_unacked = &chunk_id_first_unacked;
    params.rcvraddr = &rcvraddr;
    params.chunks_ack = &chunks_ack;
    params.chunk_cache = &chunk_cache[0];
    params.chunk_cache_mutex = &chunk_cache_mutex;
    
//    params.chunk_cache_mutex = &chunk_cache_mutex[0];
    pthread_t thread;
    if(pthread_create(&thread , NULL,  ack_receiver_thread, (void*) &params) < 0) {
        fprintf(stderr, "Error: Can not create a thread for the ack_receiver_thread in init_sender()\n");
        exit(4);
    }
    
    // Sending data packets
    while(chunks_ack_count<total_chunks) {
        //printf("loop~\n");
        // Loop from the first unacked to the last chunk
        for(j=chunk_id_first_unacked; j<=total_chunks && j<=chunk_id_first_unacked+210000L; j++) {
            
            // Send only chunk whose ACK has not been received
            if (!is_ack(&chunks_ack, j)) {
            
                if (chunk_id_last_cached < j) {
                    // Remember id of the last cached chunk
                    chunk_id_last_cached = j;
                    
                    // Create cache for a data chunk with structure: file_read_len|chunk_id|chunk_data
                    chunk_cache[j-1] = (unsigned char*) malloc(sizeof(file_read_len) + sizeof(unsigned char)*UDP_DATA_MAX_LENGTH);
                    // Put Chunk ID into cache
                    memcpy(chunk_cache[j-1] + sizeof(file_read_len), &j, FSCP_UDP_ID_BYTES);
                    // Read file into cache
                    file_read_len = fread(chunk_cache[j-1] + sizeof(file_read_len) + FSCP_UDP_ID_BYTES, sizeof(unsigned char), FSCP_UDP_DATA_BYTES, file);
                    // Put data length of the chunk into cache
                    memcpy(chunk_cache[j-1], &file_read_len, sizeof(file_read_len));
                    
                    // Init mutex
                    pthread_mutex_init(&chunk_cache_mutex[j-1], NULL);
                }
                else {
                    // Try lock to load cache
                    if (pthread_mutex_trylock(&chunk_cache_mutex[j-1]) != 0) {
                        // Skip if cannot lock because ack_receiver_thread probably received ack
                        continue;
                    }
                    
                    // Load cache
                    memcpy(&file_read_len, chunk_cache[j-1], sizeof(file_read_len));
                }

                #ifdef _DEBUG
                fprintf(stdout, "[DEBUG] Send %3llu%%: Chunk 0x%.2x%.2x%.2x", 100*chunks_ack_count/total_chunks, chunk_cache[j-1][10], chunk_cache[j-1][9], chunk_cache[j-1][8]);
                    #ifdef _DEBUG_VERBOSE
                    fprintf(stdout, " : ");
                    for (size_t k=0; k<file_read_len; k++) {
                        fprintf(stdout, "%.2x", chunk_cache[j-1][sizeof(file_read_len)+FSCP_UDP_ID_BYTES+k]);
                    }
                    #endif
                fprintf(stdout, "\n");
                #endif

                sendto(sockfd, chunk_cache[j-1] + sizeof(file_read_len), FSCP_UDP_ID_BYTES+file_read_len, 0, (struct sockaddr *)&rcvraddr, sizeof(rcvraddr));
                pthread_mutex_unlock(&chunk_cache_mutex[j-1]);
                
            }

        }
        
    }
    
    pthread_join(thread, NULL);
    
    close(sockfd);
    fclose(file);
    
    fprintf(stdout, "\n");
}

void *ack_receiver_thread(void *params) {
    
    struct ack_receiver_thread_parameter *p = (struct ack_receiver_thread_parameter *) params;
    int *sockfd = p->sockfd;
    unsigned long long *chunks_ack_count = p->chunks_ack_count;
    unsigned long long *total_chunks = p->total_chunks;
    unsigned long long *chunk_id_first_unacked = p->chunk_id_first_unacked;
    struct sockaddr_in *rcvraddr = p->rcvraddr;
    unsigned char **chunks_ack = p->chunks_ack;
    unsigned char **chunk_cache = p->chunk_cache;
    pthread_mutex_t **chunk_cache_mutex = p->chunk_cache_mutex;
    
    unsigned long long chunk_id = 0L;
    socklen_t rcvraddr_len;
    size_t recvlen;
    
    unsigned long long progress = 101;
    
    // Receive ACK
    while (*chunks_ack_count < *total_chunks) {
        recvlen = recvfrom(*sockfd, &chunk_id, FSCP_UDP_ID_BYTES, 0, (struct sockaddr *)rcvraddr, &rcvraddr_len);
        if (recvlen == FSCP_UDP_ID_BYTES) {
            if (chunk_id>0) {
                
                if (chunk_id == (1<<(FSCP_UDP_ID_BYTES*8))-1) {
                    // Receive FIN
                    *chunks_ack_count = *total_chunks;
                    fprintf(stdout, "\b\b\b\b100%%");
                    break;
                }
                
                if (!is_ack(chunks_ack, chunk_id)) {
                    
                    set_ack(chunks_ack, chunk_id, true);
                    *chunks_ack_count = *chunks_ack_count + 1;
                    
                    // Advance to the next unacked id
                    if (chunk_id == *chunk_id_first_unacked) {
                        do {
                            *chunk_id_first_unacked = *chunk_id_first_unacked + 1;
                        } while (is_ack(chunks_ack, *chunk_id_first_unacked));
                    }
                    
                    // Remove cache of the chunk
                    pthread_mutex_lock(*chunk_cache_mutex+(chunk_id-1));
                    free(chunk_cache[chunk_id-1]);
                    pthread_mutex_destroy(*chunk_cache_mutex+(chunk_id-1));
                    
                    #ifdef _DEBUG
                    fprintf(stdout, "[DEBUG] Recv %3llu%%: ACK 0x%.6llx\n", 100*(*chunks_ack_count)/(*total_chunks), chunk_id);
                    #else
                    if (progress != 100*(*chunks_ack_count)/(*total_chunks)) {
                        progress = 100*(*chunks_ack_count)/(*total_chunks);
                        fprintf(stdout, "\b\b\b\b%3llu%%", progress);
                        fflush(stdout);
                    }
                    #endif
                    
                }
                
            }
        }
    }
    
    return 0;
}

