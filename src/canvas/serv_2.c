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

void * handle_clnt(void * arg);
void send_msg(char * msg, int len);
void * ctrl_canvas(void * arg);
void error_handling(char * msg);

const char * bird_keyword[31] = 
	{"apple", "흑기러기", "고니", "원앙", "청둥오리", "희줄박이오리", 
	"뿔논병아리", "논병아리", "꿩", "메추라기", "괭이갈매기",
	"마도요", "깝작도요","노랑발도요","물꿩", "바다오리",
	"호사도요", "두루미", "흑두루미", "뜸부기", "물닭",
	"홍학", "쇠딱따구리", "오색딱따구리", "새홀리기",
	"황조롱이", "멧비둘기", "검은등뻐꾸기", "따오기", "저어새"};
int quizNum;

int clnt_cnt=0;
int clnt_socks[MAX_CLNT];
int presenter; //출제자
pthread_mutex_t mutx;
int valread;

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id, ctrl_canvas_thread;
	if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
  
	pthread_mutex_init(&mutx, NULL);
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET; 
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));
	
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error");
	quizNum=0;
	presenter=0;
	
	while(1)
	{
		clnt_adr_sz=sizeof(clnt_adr);
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, (socklen_t *)&clnt_adr_sz);
		
		pthread_mutex_lock(&mutx);
		clnt_socks[clnt_cnt++]=clnt_sock;
		pthread_mutex_unlock(&mutx);
	
		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
        pthread_create(&ctrl_canvas_thread, NULL, ctrl_canvas, (void*)&clnt_sock);

		pthread_detach(t_id);
        pthread_detach(ctrl_canvas_thread);

		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
	}
	close(serv_sock);
	return 0;
}
int start;
int turn;
	
void * handle_clnt(void * arg)
{
	int clnt_sock=*((int*)arg);
	int str_len=0, i;
	char msg[BUF_SIZE];
	char keyword_msg[BUF_SIZE];
	int before_presenter=99;

	start=0;
	
	while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0)
	{
		if (start) {
			send_msg(msg, str_len);
		}
		else if ((strstr(msg,"start")!=NULL || strstr(msg,"sss")!=NULL) && clnt_cnt >= 2 && !start){
			start=1;
			for(i=0; i<clnt_cnt; i++){
				write(clnt_socks[i], "Game Start!\n", strlen("Game Start!\n"));
				printf("start: %d %s\n", clnt_cnt, msg);
			}
			
		}
		else if ((before_presenter!=presenter)){
			sprintf(keyword_msg, "Keyword: %s!\n", bird_keyword[quizNum]);
			write(clnt_socks[presenter], keyword_msg, strlen(keyword_msg));
			before_presenter = presenter;
		}

	}

	if (clnt_cnt< 2) start=0;
	
	pthread_mutex_lock(&mutx);
	for(i=0; i<clnt_cnt; i++)   // remove disconnected client
	{
		if(clnt_sock==clnt_socks[i])
		{
			while(i++<clnt_cnt-1)
				clnt_socks[i]=clnt_socks[i+1];
			break;
		}
	}
	clnt_cnt--;
	pthread_mutex_unlock(&mutx);
	close(clnt_sock);
	return NULL;
}

void send_msg(char * msg, int len)   // send to all
{
	int i;
	char send[BUF_SIZE];
	printf("send msg: %s", msg);
	
	strcpy(send, msg); //msg 저장
	msg[strlen(msg)-1]=0;

	printf("strs %s", strstr(bird_keyword[quizNum], msg));

	if (strstr(msg, bird_keyword[quizNum]) != NULL){
		sprintf(send, "%s:O\n", msg);
		quizNum++;
		
		if(presenter > clnt_cnt) presenter=0;
		else presenter++;
	}
	else sprintf(send, "%s:X\n", msg);

	printf("answer: %s\n", bird_keyword[quizNum]);
	
	pthread_mutex_lock(&mutx);
	for(i=0; i<clnt_cnt; i++)
		write(clnt_socks[i], send, strlen(send));
	pthread_mutex_unlock(&mutx);

}

void * ctrl_canvas(void * arg) 
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

        // send drawing data to other users except for the user who is drawing
        pthread_mutex_lock(&mutx);

        size = data.size();
        // printf("clnt number : %d\n", clnt_cnt);
        for (i = 0; i < clnt_cnt; i++) {
            if (clnt_sock != clnt_socks[i]) {
                write(clnt_socks[i], &size, sizeof(size));

                const int chunkSize = 4096;
                int bytesSent = 0;
                while (bytesSent < size)
                {
                    int bytesToSend = std::min(chunkSize, size - bytesSent);
                    write(clnt_socks[i], data.data() + bytesSent, bytesToSend);
                    bytesSent += bytesToSend;
                }
            }
        }

        pthread_mutex_unlock(&mutx);
    }
    
    return NULL;
}

void error_handling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}