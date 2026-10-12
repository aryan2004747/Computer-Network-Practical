#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8081
#define BUFFER_SIZE 1024

int main()
{
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    char input[BUFFER_SIZE];

    /* Create Socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);

    /* Initialize server address */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    /* Connect to server */
    connect(sock,
            (struct sockaddr *)&serv_addr,
            sizeof(serv_addr));

    while (1)
    {
        printf("Enter request: ");

        fgets(input, BUFFER_SIZE, stdin);

        input[strcspn(input, "\n")] = 0;

        /* Send request */
        send(sock, input, strlen(input), 0);

        /* Exit condition */
        if (strcmp(input, "bye") == 0)
        {
            printf("Connection terminated.\n");
            break;
        }

        /* Receive response */
        memset(buffer, 0, BUFFER_SIZE);

        read(sock, buffer, BUFFER_SIZE);

        printf("Server Response: %s\n", buffer);
    }

    close(sock);

    return 0;
}
