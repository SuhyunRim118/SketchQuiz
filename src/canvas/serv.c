#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "opencv4/opencv2/opencv.hpp"

using namespace cv;

#define BUF_SIZE 100
#define MAX_CLNT 256

void *handle_clnt(void *arg);
void send_msg(char *msg, int len);
void error_handling(char *msg);

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;

Mat canvas;

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    pthread_t t_id;
    
    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    pthread_mutex_init(&mutx, NULL);
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    while (1)
    {
        // Accept client connections
        clnt_adr_sz=sizeof(clnt_adr);
        if ((clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, (socklen_t *)&clnt_adr_sz)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mutx);

        pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock);
        pthread_detach(t_id);
        printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
    }
    close(serv_sock);
    return 0;
}

void *handle_clnt(void *arg)
{
    int clnt_sock = *((int *)arg);

    namedWindow("Received Drawing", WINDOW_NORMAL);
    canvas = Mat::zeros(600, 800, CV_8UC3);
    Point prevPoint(-1, -1);

    while(1){
        int x;
        int y;

        // 클라이언트로부터 x, y 좌표 읽기
        if (read(clnt_sock, &x, sizeof(x)) == -1)
            return (void *)-1;

        if (read(clnt_sock, &y, sizeof(y)) == -1)
            return (void *)-1;

        // printf("x: %d and y: %d\n", x, y);

        if(x==-20 && y==-20) {
            canvas = Scalar(0,0,0);
        }
        else if(x==-10 && y==-10) {
            prevPoint.x = -1;
            prevPoint.y = -1;
        }
        else {
            Point currPoint(x, y);

            if (prevPoint.x != -1 && prevPoint.y != -1) {
                line(canvas, prevPoint, currPoint, Scalar(255, 255, 255), 3);
            }

            prevPoint = currPoint;
        }

        // Display the received drawing
        imshow("Received Drawing", canvas);
        waitKey(15);
    }
    
    return NULL;
}

void send_msg(char *msg, int len) // send to all
{
    int i;
    pthread_mutex_lock(&mutx);
    for (i = 0; i < clnt_cnt; i++)
        write(clnt_socks[i], msg, len);
    pthread_mutex_unlock(&mutx);
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
/*void show_rank() {
    for (i = 0; i < clnt_cnt; i++)
    {
        printf("===== RANK =====");
        for (i = 0; i < clnt_cnt; i++)
        {
            print()
        }
        printf("================");
    }
}*/
