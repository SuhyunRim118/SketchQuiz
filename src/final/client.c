#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <opencv2/opencv.hpp>

#define BUF_SIZE 100
#define NAME_SIZE 20

using namespace cv;

void * send_msg(void * arg);
void * recv_msg(void * arg);
void send_canvas(int sock);
void recv_canvas(int sock, char * msg);
void error_handling(char * msg);

char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];
int prev_flag = -1;
int flag = 1;

// canvas related variables
Mat temp;
Point p;
bool isDrawing;
int clear_flag=-2;
Mat get_canvas;
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

    close(sock);
    return 0;
}

void * send_msg(void * arg)   // send thread main
{
    int sock=*((int*)arg);
    char name_msg[NAME_SIZE+BUF_SIZE];
    while(1)
    {
        if(flag==1) {
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

    // namedWindow("Received Drawing", WINDOW_NORMAL);
    get_canvas = Mat::zeros(500, 500, CV_8UC3);

    while(1)
    {
        str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);
        if(str_len==-1)
        return (void*)-1;
        name_msg[str_len]=0;
        
        if (flag==1 && name_msg[0]=='2')  {
            recv_canvas(sock, name_msg);
            memset(name_msg, 0, sizeof(name_msg));
        }
        // if(prev_flag==1 && flag==0){
        //     destroyWindow("Received Drawing");
        //     prev_flag=0;
        // }

        if (!strcmp(name_msg, "draw")) { // enter draw mode
            memset(name_msg, 0, sizeof(name_msg));
            printf("draw: %d...%d\n", flag, prev_flag);
            flag = 0;
            prev_flag = 1;
            // printf("draw: %d\n", flag);
            sprintf(name_msg, "[Draw the Keyword!]\n");
            fputs(name_msg, stdout);
        }
        else if (!strcmp(name_msg, "guess")) { // enter guess mode
            memset(name_msg, 0, sizeof(name_msg));
            if(flag==0) {
                destroyWindow("Canvas");
            }
            flag = 1;
            prev_flag = 0;
            // printf("guess: %d\n", flag);
            sprintf(name_msg, "[Guess the Keyword!]\n");
            fputs(name_msg, stdout);
        }
        else if(flag==0 && name_msg[7]==':') { // receive keyword (only for draw mode)
            // fputs(name_msg, stdout);
            send_canvas(sock);
        }
        else fputs(name_msg, stdout);

        memset(name_msg, 0, sizeof(name_msg));

    }
    return NULL;
}

void send_canvas(int sock) // draw and send canvas
{
    clock_t start, end;

    int x, y;
    char coordinates[BUF_SIZE];

    namedWindow("Canvas", WINDOW_NORMAL);
    Mat canvas = Mat::zeros(500, 500, CV_8UC3);
    setMouseCallback("Canvas", onMouse, &canvas);

    // start timing
    start = clock();

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
            write(sock, coordinates, strlen("2,-10,-10,"));
        }
        else if(clear_flag==0) {
            // Send the x and y coordinates of the mouse
            x = p.x;
            y = p.y;
            sprintf(coordinates, "2,%d,%d,", x, y);
            write(sock, coordinates, strlen(coordinates));
        }
        
        // printf("%s\n", coordinates);
        memset(coordinates, 0, sizeof(coordinates));

        waitKey(1);

        //for 루프 끝난 시간
        end = clock();
        printf("Timer:%lf\n", (double)(end-start)/CLOCKS_PER_SEC);
        if(((double)(end-start)/CLOCKS_PER_SEC)>70) break;
    }
}

void recv_canvas(int sock, char * msg) // recv canvas
{
    int x=0;
    int y=0;
    int count=0;
    char trash = ' ';

    // x, y 좌표값 얻기
    char *ptr = strtok(msg, ",");
    memset(msg, 0, strlen(msg));

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
        get_canvas = Scalar(0,0,0);
    }
    else if(x==-10 && y==-10) {
        prevPoint.x = -1;
        prevPoint.y = -1;
    }
    else {
        currPoint = Point(x, y);
        // printf("curr x: %d, y: %d // prev x: %d, y: %d\n", currPoint.x, currPoint.y, prevPoint.x, prevPoint.y);

        if (prevPoint.x != -1 && prevPoint.y != -1) {
            line(get_canvas, prevPoint, currPoint, Scalar(255, 255, 255), 3);
        }

        prevPoint = currPoint;
    }

    // Display the received drawing
    imshow("Received Drawing", get_canvas);
    waitKey(1);


}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
