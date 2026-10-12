#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 4096

int main()
{
    int sock;
    struct sockaddr_in server_addr;
    char file_data[BUFFER_SIZE] = "";
    char student_id[50];
    char request[BUFFER_SIZE];
    char response[200] = {0};

    FILE *fp = fopen("data.txt", "r");

    if (fp == NULL) {
        perror("data.txt");
        return 1;
    }

    size_t len = fread(file_data, 1,
                       sizeof(file_data) - 1, fp);
    file_data[len] = '\0';
    fclose(fp);

    printf("Enter Student ID to search: ");
    if (scanf("%49s", student_id) != 1)
        return 1;

    int written = snprintf(request, sizeof(request),
                           "%s\n%s", student_id, file_data);

    if (written < 0 || written >= sizeof(request)) {
        printf("File data is too large.\n");
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    if (write(sock, request, strlen(request)) < 0) {
        perror("write");
        close(sock);
        return 1;
    }

    ssize_t n = read(sock, response, sizeof(response) - 1);

    if (n > 0) {
        response[n] = '\0';
        printf("Server Response: %s\n", response);
    } else {
        printf("No response from server.\n");
    }

    close(sock);
    return 0;
}
