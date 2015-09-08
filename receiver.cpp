#include "receiver.h"

void init_receiver() {
    
    int sockfd;
    struct sockaddr_in rcvraddr;
    size_t recvlen;
    char buffer[UDP_RCV_BUFFER_SIZE];
    //char sbuffer[UDP_RCV_BUFFER_SIZE];
    int i,j;
    int file_size;
    int id;
    socklen_t sndraddr_len;
    struct sockaddr_in sndraddr;
    int total_chunks;
    FILE *fp;
    unsigned char file_buffer[FSCP_UDP_DATA_BYTES];
    int write_len;
    fprintf(stdout, "Begin receiving file\n");
    
    memset(&rcvraddr, 0, sizeof(rcvraddr));
    rcvraddr.sin_family = AF_INET;
    rcvraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    rcvraddr.sin_port = htons(FSCP_UDP_PORT);
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        fprintf(stderr, "Error: cannot create UDP socket in init_receiver()\n");
        exit(1);
    }
    
    int so_rcvbuf_size = SOCKET_RCVBUFFER;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &so_rcvbuf_size, sizeof(so_rcvbuf_size)) < 0) {
        fprintf(stderr, "Error: cannot set SO_RCVBUF in init_receiver()\n");
        exit(2);
    }
    
    if (::bind(sockfd, (struct sockaddr *) &rcvraddr, sizeof(rcvraddr)) < 0) {
        fprintf(stderr, "Error: cannot bind UDP socket port %d in init_receiver()\n", FSCP_UDP_PORT);
        exit(3);
    }
    
    memset(buffer, 0, sizeof(buffer));
    for(i=0;i<4;i++){
        recvlen = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&sndraddr, &sndraddr_len);
        memcpy(&id,buffer,FSCP_UDP_ID_BYTES);
        memcpy(&file_size,buffer+FSCP_UDP_ID_BYTES,4);
        total_chunks = file_size / FSCP_UDP_DATA_BYTES + ((file_size % FSCP_UDP_DATA_BYTES > 0)?1:0);
        if (recvlen > 0 && id ==0) {
            memcpy(&file_size,buffer+FSCP_UDP_ID_BYTES,4);
            printf("%lu\n",recvlen);
            fprintf(stderr, "The given file is of size = %d and has id = %d\n",file_size,id);
            for(j=0;j<4;j++){
                printf("x\n");
                sendto(sockfd, buffer, FSCP_UDP_ID_BYTES, 0, (struct sockaddr *)&sndraddr, sndraddr_len);
            }
            break;
        }
        
    }
    
    fp = fopen("output.txt","w");
    while(1) {
        recvlen = recvfrom(sockfd, buffer,UDP_DATA_MAX_LENGTH, 0, (struct sockaddr *)&sndraddr, &sndraddr_len);
        memcpy(&id,buffer,FSCP_UDP_ID_BYTES);
        if(id>0) {
            //fp = fopen("output.txt","a");
            fseek(fp,(id-1)*FSCP_UDP_DATA_BYTES , SEEK_SET);
            write_len=fwrite(buffer+FSCP_UDP_ID_BYTES, recvlen-FSCP_UDP_ID_BYTES, 1, fp);
            fflush(fp);
            printf("write=%d, %lu %.2x%.2x%.2x\n",write_len, recvlen, buffer[2], buffer[1], buffer[0]);
            sendto(sockfd, buffer, FSCP_UDP_ID_BYTES, 0, (struct sockaddr *)&sndraddr, sndraddr_len);
	    sendto(sockfd, buffer, FSCP_UDP_ID_BYTES, 0, (struct sockaddr *)&sndraddr, sndraddr_len);
            sendto(sockfd, buffer, FSCP_UDP_ID_BYTES, 0, (struct sockaddr *)&sndraddr, sndraddr_len);
            //fclose(fp);
        }
        else {
	    sendto(sockfd, buffer, FSCP_UDP_ID_BYTES, 0, (struct sockaddr *)&sndraddr, sndraddr_len);
        }
    }
}

