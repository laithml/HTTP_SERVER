#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "threadpool.h"


#define USAGE printf("Usage: server <port> <pool-size> <max-number-of-request>\n")
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"


char *badRequest() {
    time_t now;
    char timeStr[128];
    now = time(NULL);
    strftime(timeStr, sizeof(timeStr), RFC1123FMT, gmtime(&now));
    char *firstLine = "HTTP/1.0 400 Bad Request\r\nServer: webserver/1.0\r\nDate: ";
    char *lastLine = "\r\nContent-Type: text/html\r\nContent-Length: 113\r\nConnection: Close\r\n\r\n<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\r\n<BODY><H4>400 Bad request</H4>\r\nBad Request.\r\n</BODY></HTML>";
    int total = strlen(timeStr) + strlen(firstLine) + strlen(lastLine);
    char *msg = malloc(sizeof(char) * total);
    strcpy(msg, firstLine);
    strcat(msg, timeStr);
    strcat(msg, lastLine);
    msg[strlen(msg)] = '\0';
    printf("%s\n\n\n\n",msg);
    return msg;
}
char *found() {
    time_t now;
    char timeStr[128];
    now = time(NULL);
    strftime(timeStr, sizeof(timeStr), RFC1123FMT, gmtime(&now));
    char *firstLine = "HTTP/1.0 302 Found\r\nServer: webserver/1.0\r\nDate: ";
    char *lastLine = "\r\nContent-Type: text/html\r\nContent-Length: 123\r\nConnection: Close\r\n\r\n<HTML><HEAD><TITLE>302 Found</TITLE></HEAD>\r\n<BODY><H4>302 Found</H4>\r\nDirectories must end with a slash.\r\n</BODY></HTML>";
    int total = strlen(timeStr) + strlen(firstLine) + strlen(lastLine);
    char *msg = malloc(sizeof(char) * total);
    strcpy(msg, firstLine);
    strcat(msg, timeStr);
    strcat(msg, lastLine);
    msg[strlen(msg)] = '\0';
    printf("%s\n\n\n\n",msg);
    return msg;
}char *forbidden() {
    time_t now;
    char timeStr[128];
    now = time(NULL);
    strftime(timeStr, sizeof(timeStr), RFC1123FMT, gmtime(&now));
    char *firstLine = "HTTP/1.0 403 Forbidden\r\nServer: webserver/1.0\r\nDate: ";
    char *lastLine = "\r\nContent-Type: text/html\r\nContent-Length: 111\r\nConnection: Close\r\n\r\n<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\r\n<BODY><H4>403 Forbidden</H4>\r\nAccess denied.\r\n</BODY></HTML>";
    int total = strlen(timeStr) + strlen(firstLine) + strlen(lastLine);
    char *msg = malloc(sizeof(char) * total);
    strcpy(msg, firstLine);
    strcat(msg, timeStr);
    strcat(msg, lastLine);
    msg[strlen(msg)] = '\0';
    printf("%s\n\n\n\n",msg);
    return msg;
}char *notFound() {
    time_t now;
    char timeStr[128];
    now = time(NULL);
    strftime(timeStr, sizeof(timeStr), RFC1123FMT, gmtime(&now));
    char *firstLine = "HTTP/1.0 404 Not Found\r\nServer: webserver/1.0\r\nDate: ";
    char *lastLine = "\r\nContent-Type: text/html\r\nContent-Length: 112\r\nConnection: Close\r\n\r\n<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\r\n<BODY><H4>404 Not Found</H4>\r\nFile not found.\r\n</BODY></HTML>";
    int total = strlen(timeStr) + strlen(firstLine) + strlen(lastLine);
    char *msg = malloc(sizeof(char) * total);
    strcpy(msg, firstLine);
    strcat(msg, timeStr);
    strcat(msg, lastLine);
    msg[strlen(msg)] = '\0';
    printf("%s\n\n\n\n",msg);
    return msg;
}char *serverError() {
    time_t now;
    char timeStr[128];
    now = time(NULL);
    strftime(timeStr, sizeof(timeStr), RFC1123FMT, gmtime(&now));
    char *firstLine = "HTTP/1.0 500 Internal Server Error\r\nServer: webserver/1.0\r\nDate: ";
    char *lastLine = "\r\nContent-Type: text/html\r\nContent-Length: 144\r\nConnection: Close\r\n\r\n<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\r\n<BODY><H4>500 Internal Server Error</H4>\r\nSome server side error.\r\n</BODY></HTML>";
    int total = strlen(timeStr) + strlen(firstLine) + strlen(lastLine);
    char *msg = malloc(sizeof(char) * total);
    strcpy(msg, firstLine);
    strcat(msg, timeStr);
    strcat(msg, lastLine);
    msg[strlen(msg)] = '\0';
    printf("%s\n\n\n\n",msg);
    return msg;
}char *notSupported() {
    time_t now;
    char timeStr[128];
    now = time(NULL);
    strftime(timeStr, sizeof(timeStr), RFC1123FMT, gmtime(&now));
    char *firstLine = "HTTP/1.0 500 501 Not supported\r\nServer: webserver/1.0\r\nDate: ";
    char *lastLine = "\r\nContent-Type: text/html\r\nContent-Length: 129\r\nConnection: Close\r\n\r\n<HTML><HEAD><TITLE>501 Not supported</TITLE></HEAD>\r\n<BODY><H4>501 Not supported</H4>\r\nMethod is not supported.\r\n</BODY></HTML>";
    int total = strlen(timeStr) + strlen(firstLine) + strlen(lastLine);
    char *msg = malloc(sizeof(char) * total);
    strcpy(msg, firstLine);
    strcat(msg, timeStr);
    strcat(msg, lastLine);
    msg[strlen(msg)] = '\0';
    printf("%s\n\n\n\n",msg);
    return msg;
}



int Handle(void *socket_id);


int isDigit(char *string) {
    int i = 0;
    while (i < strlen(string)) {
        if (isdigit(string[i]) != 0)
            return 1;
        i++;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 5) {
        USAGE;
        exit(EXIT_FAILURE);
    }
    int port = -1, pool_size = -1, max_number_of_request = -1;
    //check arguments
    port = atoi(argv[2]);
    if (port == 0 && isDigit(argv[2]) == 1) {
        USAGE;
        exit(EXIT_FAILURE);
    }
    pool_size = atoi(argv[3]);
    if (pool_size == 0 && isDigit(argv[3]) == 1) {
        USAGE;
        exit(EXIT_FAILURE);
    }
    max_number_of_request = atoi(argv[4]);
    if (max_number_of_request == 0 && isDigit(argv[4]) == 1) {
        USAGE;
        exit(EXIT_FAILURE);
    }




    //open socket
    int sd;
    if ((sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);


    if (bind(sd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind\n");
        exit(-1);
    }

    //initialize threadpool
    threadpool *tp = create_threadpool(pool_size);
    if (tp == NULL) {
        fprintf(stderr, "create_threadpool failed\n");
        exit(EXIT_FAILURE);
    }

    if (listen(sd, max_number_of_request) < 0) {
        perror("listen\n");
        exit(-1);
    }


    int i = 0;
    while (i < max_number_of_request) {
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        int connection;
        if ((connection = accept(sd, (struct sockaddr *) &client, &len)) == -1) {
            perror("accept\n");
            exit(EXIT_FAILURE);
        } else
            //TODO: SEND FN
            dispatch(tp, Handle, (void *) &connection);

        i++;
    }
    close(sd);
    destroy_threadpool(tp);
}

char *get_mime_type(char *name) {
    char *ext = strrchr(name, '.');
    if (!ext) return NULL;
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".au") == 0) return "audio/basic";
    if (strcmp(ext, ".wav") == 0) return "audio/wav";
    if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
    if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return "video/mpeg";
    if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
    return NULL;
}

int Handle(void *socket_id) {
    int sd = *((int *) (socket_id));
    printf("\n\tthread[%d]: using socket %d start to handle\n", pthread_self(), sd);
    char requestMsg[500];
    char *response=NULL;
    char *p;
    bzero(requestMsg, sizeof(requestMsg));

    //read the request from the socket
    while ((read(sd, requestMsg, sizeof(requestMsg)) >= 0)) {
        p = strstr(requestMsg, "\r\n");
        if (p != NULL) {
            strtok(requestMsg, "\r\n");
            break;
        }
        bzero(requestMsg, sizeof(requestMsg));
    }
    printf("request=%s\r\n", requestMsg);
    char *token = strtok(requestMsg, " ");
    char *path = NULL;
    int i = 0;
    while (token != NULL) {
        if (i > 2)
            break;
        if (i == 0 && strcmp(token, "GET") != 0) {
            //method not supported 501
        }
        if (i == 1) {
            path= malloc(strlen(token)* sizeof(char ));
            strcpy(path, token);
            path[strlen(path)] = '\0';
        }
        if (i == 2 && (strcmp(token, "HTTP/1.0") != 0 || strcmp(token, "HTTP/1.1") != 0))
            //bad request 400
            response =badRequest();


        token = strtok(NULL, " ");
        i++;
    }
    if (i != 3)
        //bad request
        response=badRequest();


    int total = 0;
    if(response!=NULL)
     total= strlen(response);
    int done;
    while (total != 0) {
        if ((done = write(sd, response, total)) < 0) {
            perror("write failed\n");
            pthread_exit(NULL);
        }
        total -= done;
    }

    close(sd);
    printf("thread[%d] exiting\n", pthread_self());

    free(response);

    return 0;
}


