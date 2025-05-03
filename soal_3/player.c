#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define PORT 8080

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    // struct sockaddr_in serv = {AF_INET, htons(PORT)};

    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORT);
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t addrlen = sizeof(serv);
    
    inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr);
    connect(sock, (struct sockaddr*)&serv, sizeof(serv));

    char buf[4096] = {0};
    while(1)
    {
        memset(buf, 0, sizeof(buf));
        int bytes_read = read(sock, buf, sizeof(buf));
        printf("%s\n", buf);


        // meminta input user
        char input[400] = {0};
        fgets(input, sizeof(input), stdin);
        send(sock, input, strlen(input), 0);
        if (input[0] == 'x') break;
    }


    close(sock);
    return 0;
}


