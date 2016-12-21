#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <termios.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <netdb.h>
#include <mraa/aio.h>

sig_atomic_t volatile run_flag = 1;
int sock_fd, freq = 3, isFahr = 1, isOff = 0, start = 1;
pthread_t thr_id;
char buffer[256];

void do_when_interrupted(int sig) {
    if (sig == SIGINT)
        run_flag = 0;
}

void* threadFunc() {
    int n;
    while(run_flag) {
        n = read(sock_fd, buffer, 255);
        if (n < 0) {
            perror("ERROR reading from socket!\n");
            exit(EXIT_FAILURE);
        }
        if (strstr(buffer, "OFF") != NULL) {
            if (strcmp(buffer, "OFF") == 0) {
                fprintf(stdout, "OFF\n");
                fflush(stdout);
                exit(0);
            }
            else {
                fprintf(stdout, "%s I\n", buffer);
                fflush(stdout);
            }
        }
        else if (strstr(buffer, "STOP") != NULL) {
            if (strcmp(buffer, "STOP") == 0) {
                start = 0;
                fprintf(stdout, "STOP\n");
                fflush(stdout);
            }
            else {
                fprintf(stdout, "%s I\n", buffer);
                fflush(stdout);
            }
        }
        else if (strstr(buffer, "START") != NULL) {
            if (strcmp(buffer, "START") == 0) {
                start = 1;
                fprintf(stdout, "START\n");
                fflush(stdout);
            }
            else {
                fprintf(stdout, "%s I\n", buffer);
                fflush(stdout);
            }
        }
        else if (strstr(buffer, "SCALE") != NULL) {
            if (strcmp(buffer, "SCALE=C") == 0) {
                isFahr = 0;
                fprintf(stdout, "SCALE=C\n");
                fflush(stdout);
            }
            else if (strcmp(buffer, "SCALE=F") == 0) {
                isFahr = 1;
                fprintf(stdout, "SCALE=F\n");
                fflush(stdout);
            }
            else {
                fprintf(stdout, "%s I\n", buffer);
                fflush(stdout);
            }
        }
        else if (strstr(buffer, "FREQ") != NULL) {
            char strFreq[5];
            int i;
            for (i = 0; i < 4; i++) {
                strFreq[i] = buffer[5 + i];
            }
            if (atoi(strFreq) >= 1 && atoi(strFreq) <= 3600) {
                freq = atoi(strFreq);
                fprintf(stdout, "%s\n", buffer);
                fflush(stdout);
            }
            else {
                fprintf(stdout, "%s I\n", buffer);
                fflush(stdout);
            }
        }
        else if (strstr(buffer, "DISP") != NULL) {
            fprintf(stdout, "%s I\n", buffer);
        }
        else {
            fprintf(stdout, "%s I\n", buffer);
            continue;
        }
        
        memset(buffer, 0, sizeof(buffer));
    }
}

int main() {
    uint16_t value;
    int port_no = 16000;
    struct sockaddr_in srv_addr;
    struct hostent *srv;
    int B = 4275;
    
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Error opening the socket!\n");
        exit(EXIT_FAILURE);
    }
    srv = gethostbyname("lever.cs.ucla.edu");
    if (srv == NULL) {
        fprintf(stderr, "No such host detected!\n");
        exit(0);
    }
    
    memset((char *) &srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    memcpy((char *)&srv_addr.sin_addr.s_addr,
           (char *)srv->h_addr,
           srv->h_length);
    srv_addr.sin_port = htons(port_no);
    if (connect(sock_fd,(struct sockaddr *)&srv_addr,sizeof(srv_addr)) < 0) {
        perror("Connection error!");
        exit(EXIT_FAILURE);
    }
    
    memset(buffer, 0, 256);
    strcpy(buffer, "Port request 404464206");
    if (write(sock_fd, buffer, sizeof(buffer)) < 0) {
        perror("ERROR writing to socket!\n");
        exit(EXIT_FAILURE);
    }

    memset(buffer, 0, 256);
    int new_port_num = 0;
    if (read(sock_fd, &new_port_num, 2) < 0) {
        perror("ERROR reading from socket!\n");
        exit(EXIT_FAILURE);
    }
    close(sock_fd);
    
    port_no = new_port_num;
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Error opening the socket!\n");
        exit(EXIT_FAILURE);
    }
    memset((char *) &srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    memcpy((char *)&srv_addr.sin_addr.s_addr,
           (char *)srv->h_addr,
           srv->h_length);
    srv_addr.sin_port = htons(port_no);
    if (connect(sock_fd,(struct sockaddr *)&srv_addr,sizeof(srv_addr)) < 0) {
        perror("Connection error!");
        exit(EXIT_FAILURE);
    }
    
    signal(SIGINT, do_when_interrupted);
    pthread_create(&thr_id, NULL, threadFunc, NULL);
    
    while (run_flag) {
        if (start) {
            // declare rotary as an analog I/O context
            mraa_aio_context temperature;
            temperature = mraa_aio_init(0);
            
            // read the rotary sensor value
            value = mraa_aio_read(temperature);
            double R = 1023.0/value - 1.0;
            R = 100000.0*R;
            double tmpInCels = 1.0/(log(R/100000.0)/B + 1/298.15) - 273.15;
            double tmpInFahr = (tmpInCels * 1.80) + 32.0;
            
            // timing code
            char formatted_time[20];
            time_t epoch_time;
            time(&epoch_time);
            struct tm *local_time = localtime(&epoch_time);
            strftime(formatted_time, 80, "%H:%M:%S", local_time);
            memset(buffer, 0, 256);
        
            if (isFahr) {
                fprintf(stdout, "%s %04.1f F\n", formatted_time, tmpInFahr);
                snprintf(buffer, 256, "404464206 TEMP=%04.1f\n", tmpInFahr);
            }
            else {
                fprintf(stdout, "%s %04.1f C\n", formatted_time, tmpInCels);
                snprintf(buffer, 256, "404464206 TEMP=%04.1f\n", tmpInCels);
            }
            fflush(stdout);
            if (write(sock_fd, buffer, sizeof(buffer)) < 0) {
                perror("ERROR writing to socket!\n");
                exit(EXIT_FAILURE);
            }
            memset(buffer, 0, 256);
            mraa_aio_close(temperature);
        }
        sleep(freq);
    }
    close(sock_fd);
    return 0;
}
