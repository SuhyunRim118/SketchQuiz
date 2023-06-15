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
void draw_xy(int x, int y);
void error_handling(char * msg);

const char * bird_keyword[31] =
{"apple", "흑기러기", "고니", "원앙", "청둥오리", "흰줄박이오리",
"뿔논병아리", "논병아리", "꿩", "메추라기", "괭이갈매기",
"마도요", "깝작도요","노랑발도요","물꿩", "바다오리",
"호사도요", "두루미", "흑두루미", "뜸부기", "물닭",
"홍학", "쇠딱따구리", "오색딱따구리", "새홀리기",
"황조롱이", "멧비둘기", "검은등뻐꾸기", "따오기", "저어새"};

const char * english_keyword[31] =
{"apple", "banana", "grape", "bat", "cat", "dog",
"bird", "pan", "straw", "movie", "series",
"egg", "corn", "water", "wine", "milk",
"horse", "monkey", "flog", "mouse", "rizard",
"fox", "cow", "ape", "barrrow", "bear"
"salmon", "boar", "buffalo", "rion", "tiger"};
int quizNum;

int clnt_cnt=0;
int clnt_socks[MAX_CLNT];
int presenter; //출제자
pthread_mutex_t mutx;

Mat canvas;

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    pthread_t t_id;
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
    char buf[30];

    while(1)
    {
        clnt_adr_sz=sizeof(clnt_adr);
        clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,(socklen_t *)&clnt_adr_sz);

        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++]=clnt_sock;
        pthread_mutex_unlock(&mutx);

        pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
        pthread_detach(t_id);
        printf("Connected client IP: %s (%d)\n", inet_ntoa(clnt_adr.sin_addr), clnt_sock);
    }
    close(serv_sock);
    return 0;
}

int start;

void * handle_clnt(void * arg)
{
    int clnt_sock=*((int*)arg);
    int str_len=0, i;
    char msg[BUF_SIZE];
    char keyword_msg[BUF_SIZE];
    char buf[BUF_SIZE];

    start=0;

    namedWindow("Received Drawing", WINDOW_NORMAL);
    canvas = Mat::zeros(600, 800, CV_8UC3);

    while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0)
	{
        //좌표
        if (msg[0]=='2'){
            // draw_xy(x, y);
            for(i=0; i<clnt_cnt; i++){
				if (clnt_socks[i] != clnt_socks[presenter]) {
					write(clnt_socks[i], msg, sizeof(msg));
				}
			}
        }
		else if (start) {
			send_msg(msg, str_len);
		}
		else if ((strstr(msg,"start")!=NULL || strstr(msg,"sss")!=NULL) && clnt_cnt >= 2 && !start){
			start=1;
			for(i=0; i<clnt_cnt; i++){
				sprintf(buf, "Game Start!\n");
				write(clnt_socks[i], buf, strlen(buf));
				printf("start: %d [%d] %s\n", clnt_cnt, clnt_socks[i], msg);
			}
			
			for(i=0; i<clnt_cnt; i++){
				if (clnt_socks[i] != clnt_socks[presenter]) {
                    if(presenter >= clnt_cnt-1) presenter=0;
                    // else presenter++;
					write(clnt_socks[i], "guess", strlen("guess"));
					printf("G %d\n", i);

				}
				else {
					write(clnt_socks[i], "draw", strlen("draw"));
					printf("D %d\n", i);
				}
			}
            sprintf(keyword_msg, "Keyword: %s!\n", english_keyword[quizNum]);
			write(clnt_socks[presenter], keyword_msg, strlen(keyword_msg));
		}
		else {
				sprintf(buf, "Currently logged on: %d people\n", clnt_cnt);
				write(clnt_sock, buf, strlen(buf));
		}
		memset(msg, 0, strlen(msg));
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
    char keyword_msg[BUF_SIZE];
    printf("send msg: %s\n", msg);


    strcpy(send, msg); //msg 저장
    send[strlen(msg)] = 0;
    msg[strlen(msg)-1]=0;


    if (strstr(msg, english_keyword[quizNum]) != NULL){
        sprintf(send, "%s:O\n", msg);
        quizNum++;
        printf("P %d, %d\n", presenter, clnt_socks[presenter]);

        if(presenter >= clnt_cnt-1) presenter=0;
        else presenter++;
        printf("ㅖ %d, %d\n", presenter, clnt_socks[presenter]);


        for(i=0; i<clnt_cnt; i++){
			if (clnt_socks[i] != clnt_socks[presenter]) {
				write(clnt_socks[i], "guess", strlen("guess"));
				printf("G %d\n", i);
			}
			else {
				write(clnt_socks[i], "draw", strlen("draw"));
				printf("D %d\n", i);
			}
		}
        sprintf(keyword_msg, "Keyword: %s!\n", english_keyword[quizNum]);
        write(clnt_socks[presenter], keyword_msg, strlen(keyword_msg));
    }
    else sprintf(send, "%s:X\n", msg);

    printf("answer: %s\n", english_keyword[quizNum]);

    pthread_mutex_lock(&mutx);
    for(i=0; i<clnt_cnt; i++)
        write(clnt_socks[i], send, strlen(send));
    pthread_mutex_unlock(&mutx);
}


void draw_xy(int x, int y)
{

    
}

void error_handling(char * msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
