#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "threadpool.h"


#define USAGE printf("Usage: server <port> <pool-size> <max-number-of-request>\n")
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

//bad request function
char *badRequest() {
    time_t now;
    char timeStr[128];
    now = time(NULL);
    strftime(timeStr, sizeof(timeStr), RFC1123FMT, gmtime(&now));
    char *firstLine = "HTTP/1.0 400 Bad Request\r\nServer: webserver/1.0\r\nDate: ";
    char *lastLine = "\r\nContent-Type: text/html\r\nContent-Length: 113\r\nConnection: Close\r\n\r\n<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\r\n<BODY><H4>400 Bad request</H4>\r\nBad Request.\r\n</BODY></HTML>";
    int total = strlen(timeStr) + strlen(firstLine) + strlen(lastLine);
    char *msg = calloc(total + 1, sizeof(char));
    if (msg == NULL)
        return NULL;
    strcpy(msg, firstLine);
    strcat(msg, timeStr);
    strcat(msg, lastLine);
    msg[strlen(msg)] = '\0';
    return msg;
}
//found function
char *found(char *path) {
    time_t now;
    char timeStr[128];
    now = time(NULL);
    strftime(timeStr, sizeof(timeStr), RFC1123FMT, gmtime(&now));
    char *firstLine = "HTTP/1.0 302 Found\r\nServer: webserver/1.0\r\nDate: ";
    char *secondLine = "\r\nLocation: ";
    char *lastLine = "/\r\nContent-Type: text/html\r\nContent-Length: 123\r\nConnection: Close\r\n\r\n<HTML><HEAD><TITLE>302 Found</TITLE></HEAD>\r\n<BODY><H4>302 Found</H4>\r\nDirectories must end with a slash.\r\n</BODY></HTML>";
    int total = strlen(timeStr) + strlen(firstLine) + strlen(lastLine) + strlen(path) + strlen(secondLine);
    char *msg = calloc(total + 1, sizeof(char));
    if (msg == NULL)
        return NULL;
    strcpy(msg, firstLine);
    strcat(msg, timeStr);
    strcat(msg, secondLine);
    strcat(msg, path);
    strcat(msg, lastLine);
    msg[strlen(msg)] = '\0';
    return msg;
}
//forbidden function
char *forbidden() {
    time_t now;
    char timeStr[128];
    now = time(NULL);
    strftime(timeStr, sizeof(timeStr), RFC1123FMT, gmtime(&now));
    char *firstLine = "HTTP/1.0 403 Forbidden\r\nServer: webserver/1.0\r\nDate: ";
    char *lastLine = "\r\nContent-Type: text/html\r\nContent-Length: 111\r\nConnection: Close\r\n\r\n<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\r\n<BODY><H4>403 Forbidden</H4>\r\nAccess denied.\r\n</BODY></HTML>";
    int total = strlen(timeStr) + strlen(firstLine) + strlen(lastLine);
    char *msg = calloc(total + 1, sizeof(char));
    if (msg == NULL)
        return NULL;
    strcpy(msg, firstLine);
    strcat(msg, timeStr);
    strcat(msg, lastLine);
    msg[strlen(msg)] = '\0';
    return msg;
}
//not found function
char *notFound() {
    time_t now;
    char timeStr[128];
    now = time(NULL);
    strftime(timeStr, sizeof(timeStr), RFC1123FMT, gmtime(&now));
    char *firstLine = "HTTP/1.0 404 Not Found\r\nServer: webserver/1.0\r\nDate: ";
    char *lastLine = "\r\nContent-Type: text/html\r\nContent-Length: 112\r\nConnection: Close\r\n\r\n<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\r\n<BODY><H4>404 Not Found</H4>\r\nFile not found.\r\n</BODY></HTML>";
    int total = strlen(timeStr) + strlen(firstLine) + strlen(lastLine);
    char *msg = calloc(total + 1, sizeof(char));
    if (msg == NULL)
        return NULL;
    strcpy(msg, firstLine);
    strcat(msg, timeStr);
    strcat(msg, lastLine);
    msg[strlen(msg)] = '\0';
    return msg;
}
//server error function
char *serverError() {
    time_t now;
    char timeStr[128];
    now = time(NULL);
    strftime(timeStr, sizeof(timeStr), RFC1123FMT, gmtime(&now));
    char *firstLine = "HTTP/1.0 500 Internal Server Error\r\nServer: webserver/1.0\r\nDate: ";
    char *lastLine = "\r\nContent-Type: text/html\r\nContent-Length: 144\r\nConnection: Close\r\n\r\n<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\r\n<BODY><H4>500 Internal Server Error</H4>\r\nSome server side error.\r\n</BODY></HTML>";
    int total = strlen(timeStr) + strlen(firstLine) + strlen(lastLine);
    char *msg = calloc(total + 1, sizeof(char));
    if (msg == NULL)
        return NULL;
    strcpy(msg, firstLine);
    strcat(msg, timeStr);
    strcat(msg, lastLine);
    msg[strlen(msg)] = '\0';
    return msg;
}

char *notSupported() {
    time_t now;
    char timeStr[128];
    now = time(NULL);
    strftime(timeStr, sizeof(timeStr), RFC1123FMT, gmtime(&now));
    char *firstLine = "HTTP/1.0 501 Not supported\r\nServer: webserver/1.0\r\nDate: ";
    char *lastLine = "\r\nContent-Type: text/html\r\nContent-Length: 129\r\nConnection: Close\r\n\r\n<HTML><HEAD><TITLE>501 Not supported</TITLE></HEAD>\r\n<BODY><H4>501 Not supported</H4>\r\nMethod is not supported.\r\n</BODY></HTML>";
    int total = strlen(timeStr) + strlen(firstLine) + strlen(lastLine);
    char *msg = calloc(total + 1, sizeof(char));
    if (msg == NULL)
        return NULL;
    strcpy(msg, firstLine);
    strcat(msg, timeStr);
    strcat(msg, lastLine);
    msg[strlen(msg)] = '\0';
    return msg;
}
//send file to the client
void writeFile(int file, char *path, char *extension, int sd) {
    struct stat attrib;
    stat(path, &attrib);
    char lastMod[128];
    strftime(lastMod, sizeof(lastMod), RFC1123FMT, gmtime(&attrib.st_mtime));
    int file_size = (int) attrib.st_size;

    time_t now;
    char timeStr[128];
    now = time(NULL);
    strftime(timeStr, sizeof(timeStr), RFC1123FMT, gmtime(&now));

    char header[300+128+128];
    sprintf(header, "HTTP/1.0 200 OK\r\nServer: webserver/1.0\r\nDate: %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nLast-Modified: %s\r\nConnection: Close\r\n\r\n", timeStr, extension, file_size, lastMod);
    write(sd, header, strlen(header));
    unsigned char readBuffer[512];
    memset(readBuffer, 0,sizeof(readBuffer));
    size_t bytes_read;

    while ((bytes_read = read(file, readBuffer, sizeof(readBuffer))) > 0) {
       if( (write(sd, readBuffer, bytes_read))<0){
           perror(" write here\n");
           break;
       }
        memset(readBuffer, 0, sizeof(readBuffer));
        bytes_read=0;
    }


}

//send the directory Content to the client
void dirContent(DIR *dir, char *path, int sd) {
    time_t now;
    char timeStr[128];
    now = time(NULL);
    strftime(timeStr, sizeof(timeStr), RFC1123FMT, gmtime(&now));

    struct stat attrib;
    stat(path, &attrib);
    char lastMod[128];
    strftime(lastMod, sizeof(lastMod), RFC1123FMT, gmtime(&attrib.st_mtime));

    char header[300+128+128] = {0};
    sprintf(header, "HTTP/1.0 200 OK\r\nServer: webserver/1.0\r\nDate: %s\r\nContent-Type: text/html\r\nLast-Modified: %s\r\nConnection: Close\r\n\r\n", timeStr, lastMod);
    write(sd, header, sizeof(header));
    struct stat st;
    struct dirent *dirent;
    char body[1024] = {0};
    sprintf(body, "<html><head><title>Index of %s</title></head>\r\n<body><h4>Index of %s</h4><hr><table CELLSPACING=15><tr><th>Name</th><th>Last-Modified</th><th>Size</th></tr>", path, path);
    write(sd, body, sizeof(body));

    while ((dirent = readdir(dir)) != NULL) {
        bzero(body, sizeof(body));
        char temp[strlen(path) + strlen(dirent->d_name) + 1];
        bzero(temp, sizeof(temp));
        strcat(temp, path);
        strcat(temp, dirent->d_name);
        stat(temp, &st);
        bzero(timeStr, sizeof(timeStr));
        strftime(timeStr, sizeof(timeStr), RFC1123FMT, gmtime(&st.st_mtime));

        if (!S_ISDIR(st.st_mode)) {
            sprintf(body, "<tr><td><A HREF=\"%s\">%s</A></td><td>%s</td><td>%d</td></tr>", dirent->d_name, dirent->d_name, timeStr, (int) st.st_size);
        } else {
            sprintf(body, "<tr><td><A HREF=\"%s/\">%s</A></td><td>%s</td><td>", dirent->d_name, dirent->d_name, timeStr);
        }
        write(sd, body, strlen(body));

    }

    sprintf(body, "</table><HR><ADDRESS>webserver/1.0</ADDRESS></body></html>");
    write(sd, body, strlen(body));
    closedir(dir);
}


void Handle(void *socket_id);


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
    if (argc != 4) {
        USAGE;
        exit(EXIT_FAILURE);
    }
    int port = -1, pool_size = -1, max_number_of_request = -1;
    //check arguments
    port = atoi(argv[1]);
    if (port == 0 && isDigit(argv[1]) == 1) {
        USAGE;
        exit(EXIT_FAILURE);
    }
    pool_size = atoi(argv[2]);
    if (pool_size == 0 && isDigit(argv[2]) == 1) {
        USAGE;
        exit(EXIT_FAILURE);
    }
    max_number_of_request = atoi(argv[3]);
    if (max_number_of_request == 0 && isDigit(argv[3]) == 1) {
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
    int connection;
    while (i < max_number_of_request) {
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        connection = accept(sd, (struct sockaddr *) &client, &len);
        dispatch(tp, (dispatch_fn) Handle, (void *) &connection);
        i++;
    }
    destroy_threadpool(tp);
    close(sd);
    return 0;
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

void Handle(void *socket_id) {
    int sd = *((int *) (socket_id)); // cast the void pointer to int pointer and get the socket descriptor
    char requestMsg[500];
    char *response = NULL;
    char *p;
    bzero(requestMsg, sizeof(requestMsg)); // set all elements of requestMsg to 0

    //read the request from the socket
    while ((read(sd, requestMsg, sizeof(requestMsg)) >= 0)) { // read the request message from the socket
        p = strstr(requestMsg, "\r\n");
        if (p != NULL) {
            strtok(requestMsg, "\r\n"); // break the message at the first occurrence of "\r\n"
            break;
        }
        bzero(requestMsg, sizeof(requestMsg)); // set all elements of requestMsg to 0
    }
    char *token = strtok(requestMsg, " "); // tokenize the request message by space
    char *path = NULL;
    int i = 0;
    while (token != NULL) { // while there are still tokens left
        if (i > 2)
            break;
        if (i == 0 && strcmp(token, "GET") != 0) { // check if the method is GET
            //method not supported 501
            response = notSupported();
            if (response == NULL) {
                response = serverError();
            }
            write(sd, response, strlen(response));
            goto exit;
        }if (i == 1) {
            path = calloc(strlen(token) + 2, sizeof(char)); // allocate memory for the path
            if (path == NULL) {
                response = serverError();

                write(sd, response, strlen(response));
                goto exit;
            }
            path[0] = '.';
            strncat(path, token, strlen(token)); // concatenate '.' and the path
            path[strlen(path)] = '\0';
        }if (i == 2 && (strcmp(token, "HTTP/1.0") != 0 && strcmp(token, "HTTP/1.1") != 0)) { // check if the HTTP version is supported
            //bad request 400
            response = badRequest();
            if (response == NULL) {
                response = serverError();
            }
            write(sd, response, strlen(response));
            goto exit;

        }
        token = strtok(NULL, " ");
        i++;
    }if (i != 3) { // check if the request is properly formatted
        //bad request
        response = badRequest();
        if (response == NULL) {
            response = serverError();
        }
        write(sd, response, strlen(response));
        goto exit;
    }
    i = 0;
    int k = -1;
    while (i < strlen(path)) { // find the last occurrence of '/'
        if (path[i] == '/')
            k = i;
        i++;
    }
    char *directory=NULL; // variable to hold the directory name
    char *filename=NULL;  // variable to hold the file name

    if (k != -1) {
        directory = calloc(k + 2, sizeof(char)); // allocate memory for directory name
        if (directory == NULL) {
            response = serverError();
            write(sd, response, strlen(response));
            goto exit;
        }
        strncat(directory, path, k + 1); // concatenate the directory name
        filename = calloc((strlen(path) - k + 1) + 1, sizeof(char)); // allocate memory for the file name
        if (filename == NULL) {
            response = serverError();
            write(sd, response, strlen(response));
            goto exit;
        }
        strncpy(filename, path + k + 1, i - k - 1); // copy the file name from the path
    }

    char tempPath[500];
    i = 0;
    while (i < strlen(path)) {
        while (path[i] != '/' && i < strlen(path)) {
            tempPath[i] = path[i];
            i++;
        }
        struct stat attr;
        if (stat(path, &attr) < 0) { // check the status of the file
            response = notFound();
            if (response == NULL)
                response = serverError();
            write(sd, response, strlen(response));
            goto exit;
        }if (S_ISREG(attr.st_mode)) { // if it's a regular file
            if (!((attr.st_mode & S_IROTH) && (attr.st_mode & S_IRGRP) && (attr.st_mode & S_IRUSR))) { // check if the file is readable
                response = forbidden();
                if (response == NULL)
                    response = serverError();
                write(sd, response, strlen(response));
                goto exit;
            }
        } else if (S_ISDIR(attr.st_mode)) { // if it's a directory
            if (!(attr.st_mode & S_IXOTH)) { // check if the directory is executable
                response = forbidden();
                if (response == NULL)
                    response = serverError();
                write(sd, response, strlen(response));
                goto exit;
            }
        }
        tempPath[i++] = '/';
    }
    DIR *temp = opendir(path);
    if (temp != NULL && path[strlen(path) - 1] != '/') {
        response = found(path);
        if (response == NULL)
            response = serverError();
        closedir(temp);
        write(sd, response, strlen(response));
        goto exit;
    }
    if (temp != NULL)
        closedir(temp);
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        response = notFound();
        if (response == NULL)
            response = serverError();
        write(sd, response, strlen(response));
        goto exit;
    }
    //5+6 -- check the end if the directory, and if its directory, search for index html and send it, otherwise send what its containing.
    if (directory[strlen(directory) - 1] == '/' && strcmp(filename, "") == 0) {
        char absPath[strlen(path) + 11];
        strcpy(absPath, path);
        strncat(absPath, "index.html", 11);
        int file = open(absPath, O_RDONLY);
        if (file < 0) {
            dirContent(dir, path, sd);
            goto exit;
        } else {
            writeFile(file, absPath, "text/html", sd);
            closedir(dir);
            close(file);
            goto exit;
        }
    } else {
        int file = open(path, O_RDONLY);
        if (file < 0) {
            response = notFound();
            if (response == NULL)
                response = serverError();
            closedir(dir);
            write(sd, response, strlen(response));
            goto exit;
        } else {
            writeFile(file, path, get_mime_type(filename), sd);
            closedir(dir);
            close(file);
            goto exit;
        }
    }

    exit:
    close(sd);
    if(directory!=NULL)
        free(directory);
    if(response!=NULL)
        free(response);
    if(filename != NULL)
        free(filename);
    if(path != NULL)
        free(path);
}
