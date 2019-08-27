#include <iostream>
#include <string>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>

using namespace std;

const int BUF_SIZE = 2048;
const int SAMLL_BUFF = 100;

void *request_handler(void *arg);
void send_data(FILE *fp, char *ct, char *file_name);
char *content_type(char *file);
void send_error(FILE *fp);
void error_handling(char *msg);

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;

    char buf(BUF_SIZE);
    pthread_t t_id;

    if (argc != 2)
    {
        cout << "Please usage: " << argv[0] << " <IP>" << endl;
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));

    // time_wait
    int option;
    socklen_t optlen = sizeof(option);
    option = 1;
    setsockopt(serv_sock, SOL_SOCKET,
               SO_REUSEADDR, (void *)&option, optlen);

    if (-1 == bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)))
        error_handling("bind() error");

    if (-1 == listen(serv_sock, 5))
        error_handling("listen() error");

    while (1)
    {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        cout << "client from " << inet_ntoa(clnt_addr.sin_addr)
             << " port " << ntohs(clnt_addr.sin_port) << " connection..." << endl;
        pthread_create(&t_id, NULL, request_handler, &clnt_sock);
        pthread_detach(t_id);
    }
    close(serv_sock);
    exit(1);
}

void *request_handler(void *arg)
{
    int clnt_sock = *((int *)arg);
    char req_line[SAMLL_BUFF];
    FILE *clnt_read;
    FILE *clnt_write;

    char method[10];
    char ct[15];
    char file_name[30];

    clnt_read = fdopen(clnt_sock, "r");
    clnt_write = fdopen(dup(clnt_sock), "w");
    fgets(req_line, SAMLL_BUFF, clnt_read);

    if (strstr(req_line, "HTTP/") == NULL)
    {
        send_error(clnt_write);
        fclose(clnt_read);
        fclose(clnt_write);
        return NULL;
    }
    strcpy(method, strtok(req_line, " /"));
    strcpy(file_name, strtok(NULL, " /"));
    strcpy(ct, content_type(file_name));
    if (strcmp(method, "GET") != 0)
    {
        send_error(clnt_write);
        fclose(clnt_read);
        fclose(clnt_write);
        return NULL;
    }

    fclose(clnt_read);
    cout << ct << file_name << endl;
    send_data(clnt_write, ct, file_name);

}

void send_data(FILE *fp, char *ct, char *file_name)
{

    cout << file_name << endl;
    char protocl[] = "HTTP/1.0 200 ok \r\n";
    char server[] = "Server:Linux Web Server\r\n";
    char cnt_len[] = "Content-Length:2048\r\n";
    char cnt_type[SAMLL_BUFF];
    char buf[BUF_SIZE];
    FILE *send_file;

    sprintf(cnt_type, "Content-type:%s\r\n\r\n", ct);
    send_file = fopen(file_name, "r");
    if (send_file == NULL)
    {
        send_error(fp);
        return;
    }

    //传输头信息
    fputs(protocl, fp);
    fputs(server, fp);
    fputs(cnt_len, fp);
    fputs(cnt_type, fp);
    //传输请求数据
    while (fgets(buf, BUF_SIZE, send_file) != NULL)
    {
        fputs(buf, fp);
        fflush(fp);
    }
    fflush(fp);
    fclose(fp);
}

char *content_type(char *file)
{
    char extension[SAMLL_BUFF];
    char file_name[SAMLL_BUFF];
    strcpy(file_name, file);
    strtok(file_name, ".");
    strcpy(extension, strtok(NULL, "."));

    if (!strcmp(extension, "html") || !strcmp(extension, "htm"))
    {
        return "text/html";
    }
    else
    {
        return "text/plain";
    }
}

void send_error(FILE *fp)
{
    char protocl[] = "HTTP/1.0 200 ok \r\n";
    char server[] = "Server:Linux Web Server\r\n";
    char cnt_len[] = "Content-Length:2048\r\n";
    char cnt_type[] = "Content-type:text/html\r\n\r\n";
    char content[] = "<html><head><title>NETWORK</title></head>"
                     "<body>"
                     "<font size=+5><br>发生错误！请查看请求文件名或请求方式或者与管理员联系！"
                     "</body>"
                     "</html>";
    fputs(protocl, fp);
    fputs(server, fp);
    fputs(cnt_len, fp);
    fputs(cnt_type, fp);
    fputs(content, fp);
    fflush(fp);
}

void error_handling(char *msg)
{
    cout << msg << endl;
    exit(1);
}