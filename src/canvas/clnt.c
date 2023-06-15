#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <opencv2/opencv.hpp>

#define BUF_SIZE 100
#define NAME_SIZE 20
#define PORT 8080

using namespace cv;

void *send_canvas(void *arg);
void *recv_canvas(void *arg);
void error_handling(char *msg);

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];

Mat temp;
Point p;
bool isDrawing;
int clear_flag=-2;

void onMouse(int event, int x, int y, int flags, void* param)
{
    if (event == cv::EVENT_LBUTTONDOWN)
    {
        // Start drawing
        isDrawing = true;
        p = cv::Point(x, y);
    }
    else if (event == cv::EVENT_LBUTTONUP)
    {
        // Stop drawing
        isDrawing = false;
        clear_flag=-1;
    }
    else if (event == cv::EVENT_MOUSEMOVE && isDrawing)
    {
        temp = *(Mat*)param;

        // Draw a line from the previous point to the current point
        cv::Point currPt(x, y);
        cv::line(temp, p, currPt, cv::Scalar(255, 255, 255), 3);
        p = currPt;
        clear_flag = 0;
    }
    else if (event == cv::EVENT_RBUTTONDOWN)
    {
        // Clear the canvas
        temp = cv::Scalar(0, 0, 0);
        clear_flag = 1;
    }
}

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void *thread_return;
    if (argc != 4)
    {
        printf("Usage : %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }

    sprintf(name, "[%s]", argv[3]);
    sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");
    pthread_create(&snd_thread, NULL, send_canvas, (void*)&sock);
    // pthread_create(&rcv_thread, NULL, recv_canvas, (void*)&sock);
    pthread_join(snd_thread, &thread_return);
    // pthread_join(rcv_thread, &thread_return);
    close(sock);
    //printf("closed\n");
    return 0;
}

void *send_canvas(void *arg)   // send thread main
{
    int sock = *((int*)arg);
    int x, y;

    namedWindow("Canvas", WINDOW_NORMAL);
    Mat canvas = Mat::zeros(600, 800, CV_8UC3);
    setMouseCallback("Canvas", onMouse, &canvas);

    while (1)
    {
        // Display the canvas with the drawing
        imshow("Canvas", canvas);

        if(clear_flag==1) {
            x = -20;
            y = -20;
            write(sock, &x, sizeof(x));
            write(sock, &y, sizeof(y));
        }
        else if(clear_flag==-1) {
            x = -10;
            y = -10;
            write(sock, &x, sizeof(x));
            write(sock, &y, sizeof(y));
        }
        else if(clear_flag==0) {
            // Send the x and y coordinates of the mouse
            x = p.x;
            y = p.y;
            write(sock, &x, sizeof(x));
            write(sock, &y, sizeof(y));
        }
        

        waitKey(15);
    }
    return NULL;
}

void *recv_canvas(void *arg)   // read thread main
{
    int sock = *((int*)arg);
    char name_msg[NAME_SIZE + BUF_SIZE];
    int str_len;
    while (1)
    {
        str_len = read(sock, name_msg, NAME_SIZE + BUF_SIZE - 1);
        if (str_len == -1)
            return (void*)-1;
        name_msg[str_len] = 0;
        fputs(name_msg, stdout);
    }
    return NULL;
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
