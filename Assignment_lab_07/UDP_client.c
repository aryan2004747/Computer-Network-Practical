
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define PORT 30643
#define BUFFER_SIZE 2048

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in server_addr;
    char input[BUFFER_SIZE];
    char request[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    if (argc != 3) {
        printf("Usage: %s <server_ip> <roll>\n", argv[0]);
        return 1;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct timeval timeout = {5, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
               &timeout, sizeof(timeout));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1],
                  &server_addr.sin_addr) != 1) {
        printf("Invalid server IP address\n");
        close(sock);
        return 1;
    }

    printf("UDP client started. Server: %s:%d\n",
           argv[1], PORT);
    printf("Enter requests such as SUM|5|1 2 3 4 5\n");
    printf("Type EXIT to stop the server and client.\n");

    while (1) {
        printf("\nEnter request: ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            strcpy(input, "EXIT");
        } else {
            input[strcspn(input, "\r\n")] = '\0';
        }

        int is_exit = (strcasecmp(input, "EXIT") == 0);

        int len = snprintf(request, sizeof(request),
                           "%s#%s", argv[2], input);

        if (len < 0 || len >= sizeof(request)) {
            printf("Request too long\n");
            continue;
        }

        if (sendto(sock, request, strlen(request), 0,
                   (struct sockaddr *)&server_addr,
                   sizeof(server_addr)) < 0) {
            perror("sendto");
            break;
        }

        socklen_t addr_len = sizeof(server_addr);
        ssize_t bytes = recvfrom(sock, response,
                                 sizeof(response) - 1, 0,
                                 (struct sockaddr *)&server_addr,
                                 &addr_len);

        if (bytes < 0) {
            perror("recvfrom");
            if (is_exit)
                break;
            continue;
        }

        response[bytes] = '\0';
        printf("Server Response: %s\n", response);

        if (is_exit)
            break;
    }

    close(sock);
    printf("Client terminated.\n");
    return 0;
}
