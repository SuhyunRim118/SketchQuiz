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
void * send_canvas(void * arg);
void * recv_canvas(void * arg);
void error_handling(char * msg);
	
char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];
int valread;
int flag;

Mat temp;
Point p;
bool isDrawing;

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
    }
    else if (event == cv::EVENT_MOUSEMOVE && isDrawing)
    {
        temp = *(Mat*)param;

        // Draw a line from the previous point to the current point
        cv::Point currPt(x, y);
        cv::line(temp, p, currPt, cv::Scalar(255, 255, 255), 3);
        p = currPt;
    }
    else if (event == cv::EVENT_RBUTTONDOWN)
    {
        // Clear the canvas
        temp = cv::Scalar(0, 0, 0);
    }
}
	
int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread, snd_canvas_thread, rcv_canvas_thread;
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
	
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
    pthread_create(&snd_canvas_thread, NULL, send_canvas, (void*)&sock);
	pthread_create(&rcv_canvas_thread, NULL, recv_canvas, (void*)&sock);

	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);
    pthread_join(snd_canvas_thread, &thread_return);
	pthread_join(rcv_canvas_thread, &thread_return);
	close(sock);  
	return 0;
}
	
void * send_msg(void * arg)   // send thread main
{
	int sock=*((int*)arg);
	char name_msg[NAME_SIZE+BUF_SIZE];
	while(1) 
	{
		fgets(msg, BUF_SIZE, stdin);
		if(!strcmp(msg,"q\n")||!strcmp(msg,"Q\n")) 
		{
			close(sock);
			exit(0);
		}
		sprintf(name_msg,"%s %s", name, msg);
		write(sock, name_msg, strlen(name_msg));
		strcpy(msg, "");
	}
	return NULL;
}
	
void * recv_msg(void * arg)   // read thread main
{
	int sock=*((int*)arg);
	char name_msg[NAME_SIZE+BUF_SIZE];
	int str_len;
	while(1)
	{
		str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);
		if(str_len==-1) 
			return (void*)-1;
		name_msg[str_len]=0;
		fputs(name_msg, stdout);
	}
	return NULL;
}

void * send_canvas(void * arg)
{
    if(flag == 1)
    {
        int sock = *((int*)arg);
        char name_msg[NAME_SIZE + BUF_SIZE];

        namedWindow("Canvas", WINDOW_NORMAL);
        Mat canvas = Mat::zeros(600, 800, CV_8UC3);
        setMouseCallback("Canvas", onMouse, &canvas);

        while (1)
        {
            // Display the canvas with the drawing
            imshow("Canvas", canvas);

            // Convert the canvas to a byte array
            std::vector<uchar> drawingData;
            imencode(".jpg", canvas, drawingData);

            // Send the size of the drawing data to the server
            int size = drawingData.size();
            write(sock, &size, sizeof(size));

            // Send the drawing data to the server in smaller chunks
            const int chunkSize = 4096;
            int bytesSent = 0;
            while (bytesSent < size)
            {
                int bytesToSend = std::min(chunkSize, size - bytesSent);
                write(sock, drawingData.data() + bytesSent, bytesToSend);
                bytesSent += bytesToSend;
            }
        }
    }
    return NULL;
}

void * recv_canvas(void * arg)
{
	if(flag == 0)
    {
        int clnt_sock = *((int *)arg);
        int str_len = 0, i;
        char msg[BUF_SIZE];

        namedWindow("Received Drawing", WINDOW_NORMAL);

        while(1){
            // Receive the size of the drawing data from the client
            int size;
            valread = recv(clnt_sock, &size, sizeof(size), 0);

            //printf("size: %d\n", size);

            // Receive the drawing data from the client
            std::vector<uchar> data;
            data.resize(size);
            int bytesReceived = 0;
            while (bytesReceived < size)
            {
                valread = recv(clnt_sock, &data[bytesReceived], size - bytesReceived, 0);
                bytesReceived += valread;
            }

            // Decode the received drawing data into an OpenCV Mat object
            Mat drawing = imdecode(data, IMREAD_COLOR);

            // Display the received drawing
            imshow("Received Drawing", drawing);

            waitKey(1);
        }
    }
    return NULL;
}
	
void error_handling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
