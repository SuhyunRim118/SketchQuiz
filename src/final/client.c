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

using namespace cv;

void * send_msg(void * arg);
void * recv_msg(void * arg);
void send_canvas(int sock);
void error_handling(char * msg);

char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];
int flag = 1;

///
Mat temp;
Point p;
bool isDrawing;
int clear_flag=-2;
Mat recv_canvas;
Point prevPoint(-1, -1);
Point currPoint(-1,-1);

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
///


int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void * thread_return;
    if(argc!=4) {
        printf("Usage : %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }

    sprintf(name, "[%s]", argv[3]);
    sock=socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_addr.sin_port=htons(atoi(argv[2]));
    
    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
        error_handling("connect() error");

    fputs("When there are more than two users, enter [Start] to start.\n", stdout);
    fputs("Press any key to view the number of users.\n", stdout);

    pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
    pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
    pthread_join(snd_thread, &thread_return);
    pthread_join(rcv_thread, &thread_return);

    // while(1) {
    //     if(canvasUpdated) {
    //         // Display the received drawing
    //         printf("here\n");
    //         imshow("Received Drawing", canvas);
    //         waitKey(1);
    //         canvasUpdated = false;
    //     }
    // }

    close(sock);
    return 0;
}

void * send_msg(void * arg)   // send thread main
{
    int sock=*((int*)arg);
    char name_msg[NAME_SIZE+BUF_SIZE];
    while(1)
    {
        if(flag==1) { // guess
            fgets(msg, BUF_SIZE, stdin);
            if(!strcmp(msg,"q\n")||!strcmp(msg,"Q\n"))
            {
                close(sock);
                exit(0);
            }
            if (msg[0] != '2') sprintf(name_msg,"%s %s", name, msg);
            else sprintf(name_msg,"%s", msg);
            write(sock, name_msg, strlen(name_msg));
            memset(msg, 0, sizeof(msg));
        }
        
    }
    return NULL;
}

void * recv_msg(void * arg)   // read thread main
{
    int sock=*((int*)arg);
    char name_msg[NAME_SIZE+BUF_SIZE];
    char notice[NAME_SIZE+BUF_SIZE];
    int str_len;

    namedWindow("Received Drawing", WINDOW_NORMAL);
    recv_canvas = Mat::zeros(600, 800, CV_8UC3);
    // Point prevPoint(-1, -1);

    while(1)
    {
        str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);
        if(str_len==-1)
        return (void*)-1;
        name_msg[str_len]=0;
        
        if (flag==1 && name_msg[0]=='2') { // read canvas
            int x=0;
            int y=0;
            int count=0;
            char trash = ' ';

            // x, y 좌표값 얻기
            char *ptr = strtok(name_msg, ",");
            memset(name_msg, 0, strlen(name_msg));

            while (ptr != NULL)
            {
                if(count==1) x = atoi(ptr);
                else if(count==2) y = atoi(ptr);
                else trash = ptr[0];

                ptr = strtok(NULL, ",");
                count++;
            }

            // printf("x: %d and y: %d\n", x, y);

            if(x==-20 && y==-20) {
                recv_canvas = Scalar(0,0,0);
            }
            else if(x==-10 && y==-10) {
                prevPoint.x = -1;
                prevPoint.y = -1;
            }
            else {
                currPoint = Point(x, y);
                printf("curr x: %d, y: %d // prev x: %d, y: %d\n", currPoint.x, currPoint.y, prevPoint.x, prevPoint.y);

                    // line(recv_canvas, prevPoint, currPoint, Scalar(255, 255, 255), 3);
                    // circle(recv_canvas, currPoint, 5, Scalar(255,255,255),1,8,0);
                if (prevPoint.x != -1 && prevPoint.y != -1) {
                    // printf("here\n\n");
                    line(recv_canvas, prevPoint, currPoint, Scalar(255, 255, 255), 3);
                }

                prevPoint = currPoint;
            }

            // canvasUpdated = true;
            // Display the received drawing
            imshow("Received Drawing", recv_canvas);
            waitKey(30);
        }
        else {
            if (!strcmp(name_msg, "draw")) {
                flag = 0;
                printf("draw: %d\n", flag);
                sprintf(name_msg, "[Draw the Keyword!]\n");
                send_canvas(sock);
                // continue;
            }
            else if (!strcmp(name_msg, "guess")) {
                flag = 1;
                printf("guess: %d\n", flag);
                sprintf(name_msg, "[Guess the Keyword!]\n");
                // recv_canvas(sock);
                // continue;
            }
            fputs(name_msg, stdout);
            // fputs(notice, stdout);
        }
        
    }
    return NULL;
}

void send_canvas(int sock)
{
    int x, y;
    char coordinates[BUF_SIZE];

    namedWindow("Canvas", WINDOW_NORMAL);
    Mat canvas = Mat::zeros(600, 800, CV_8UC3);
    setMouseCallback("Canvas", onMouse, &canvas);

    while (1)
    {
        // Display the canvas with the drawing
        imshow("Canvas", canvas);

        if(clear_flag==1) {
            sprintf(coordinates, "2,-20,-20,");
            write(sock, coordinates, strlen("2,-20,-20,"));
        }
        else if(clear_flag==-1) {
            sprintf(coordinates, "2,-10,-10,");
            write(sock, coordinates, strlen("2,-20,-20,"));
        }
        else if(clear_flag==0) {
            // Send the x and y coordinates of the mouse
            x = p.x;
            y = p.y;
            sprintf(coordinates, "2,%d,%d,", x, y);
            write(sock, coordinates, strlen(coordinates));
        }
        
        printf("%s\n", coordinates);
        memset(coordinates, 0, sizeof(coordinates));

        // Break the loop if 'q' is pressed
        if (waitKey(30) == 'q')
            break;
    }
}

// void recv_canvas(int sock)
// {
//     char msg[BUF_SIZE];

//     namedWindow("Received Drawing", WINDOW_NORMAL);
//     Mat canvas = Mat::zeros(600, 800, CV_8UC3);
//     Point prevPoint(-1, -1);

//     while((read(sock, msg, sizeof(msg)))!=0){
        
//         int x=0;
//         int y=0;
//         int count=0;
//         char trash = ' ';

//         char *ptr = strtok(msg, ",");
//         memset(msg, 0, strlen(msg));

//         while (ptr != NULL)
//         {
//             if(count==1) x = atoi(ptr);
//             else if(count==2) y = atoi(ptr);
//             else trash = ptr[0];

//             ptr = strtok(NULL, ",");
//             count++;
//         }

//         printf("x: %d and y: %d\n", x, y);

//         if(x==-20 && y==-20) {
//             canvas = Scalar(0,0,0);
//         }
//         else if(x==-10 && y==-10) {
//             prevPoint.x = -1;
//             prevPoint.y = -1;
//         }
//         else {
//             currPoint = Point(x, y);

//             if (prevPoint.x != -1 && prevPoint.y != -1) {
//                 line(canvas, prevPoint, currPoint, Scalar(255, 255, 255), 3);
//             }

//             prevPoint = currPoint;
//         }

//         // Display the received drawing
//         imshow("Received Drawing", canvas);
//         waitKey(15);
//     }
// }

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
