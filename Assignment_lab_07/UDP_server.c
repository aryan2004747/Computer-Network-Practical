
/*
   Roll: 24643
   Service Type: B
   d = 1
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#define PORT 30643
#define BUFFER_SIZE 2048
#define EXPECTED_ROLL "24643"

void process_request(char *message, char *response)
{
    char *saveptr;
    char *operation = strtok_r(message, "|", &saveptr);
    char *count_str = strtok_r(NULL, "|", &saveptr);
    char *numbers_str = strtok_r(NULL, "|", &saveptr);

    if (!operation || !count_str || !numbers_str) {
        snprintf(response, BUFFER_SIZE, "ERROR: Invalid request format");
        return;
    }

    char *end;
    errno = 0;
    long n = strtol(count_str, &end, 10);

    if (errno || *end != '\0' || n < 1 || n > 500) {
        snprintf(response, BUFFER_SIZE, "ERROR: Invalid number count");
        return;
    }

    long long values[500];
    long long sum = 0, maximum, minimum;
    char *number_save;
    char *token = strtok_r(numbers_str, " \t\r\n", &number_save);

    for (int i = 0; i < n; i++) {
        if (!token) {
            snprintf(response, BUFFER_SIZE,
                     "ERROR: Not enough integers");
            return;
        }

        errno = 0;
        long long value = strtoll(token, &end, 10);

        if (errno || *end != '\0') {
            snprintf(response, BUFFER_SIZE,
                     "ERROR: Invalid integer");
            return;
        }

        values[i] = value;
        token = strtok_r(NULL, " \t\r\n", &number_save);
    }

    if (token != NULL) {
        snprintf(response, BUFFER_SIZE,
                 "ERROR: Too many integers");
        return;
    }

    maximum = minimum = values[0];

    for (int i = 0; i < n; i++) {
        if ((values[i] > 0 && sum > LLONG_MAX - values[i]) ||
            (values[i] < 0 && sum < LLONG_MIN - values[i])) {
            snprintf(response, BUFFER_SIZE, "ERROR: Sum overflow");
            return;
        }

        sum += values[i];

        if (values[i] > maximum)
            maximum = values[i];

        if (values[i] < minimum)
            minimum = values[i];
    }

    if (strcmp(operation, "SUM") == 0) {
        snprintf(response, BUFFER_SIZE, "%lld", sum);
    } else if (strcmp(operation, "MAX") == 0) {
        snprintf(response, BUFFER_SIZE, "%lld", maximum);
    } else if (strcmp(operation, "MIN") == 0) {
        snprintf(response, BUFFER_SIZE, "%lld", minimum);
    } else if (strcmp(operation, "AVG") == 0) {
        snprintf(response, BUFFER_SIZE, "%.2f",
                 (double)sum / n);
    } else {
        snprintf(response, BUFFER_SIZE,
                 "ERROR: Invalid operation");
    }
}

int main(void)
{
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    printf("Server running on port %d\n", PORT);
    fflush(stdout);

    while (1) {
        client_len = sizeof(client_addr);

        ssize_t bytes = recvfrom(server_fd, buffer,
                                 sizeof(buffer) - 1, 0,
                                 (struct sockaddr *)&client_addr,
                                 &client_len);

        if (bytes < 0) {
            perror("recvfrom");
            continue;
        }

        buffer[bytes] = '\0';

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr,
                  ip, sizeof(ip));

        printf("\nClient: %s:%d\n", ip,
               ntohs(client_addr.sin_port));

        char *separator = strchr(buffer, '#');

        if (!separator) {
            printf("Received message: %s\n", buffer);
            snprintf(response, sizeof(response),
                     "ERROR: Unauthorized client");
        } else {
            *separator = '\0';
            char *roll = buffer;
            char *message = separator + 1;

            printf("Received message: %s\n", message);

            if (strcmp(roll, EXPECTED_ROLL) != 0) {
                snprintf(response, sizeof(response),
                         "ERROR: Unauthorized client");
            } else if (strcmp(message, "EXIT") == 0) {
                snprintf(response, sizeof(response),
                         "Server shutting down");

                sendto(server_fd, response, strlen(response), 0,
                       (struct sockaddr *)&client_addr, client_len);

                printf("Sent response: %s\n", response);
                printf("Server shutting down...\n");
                break;
            } else {
                process_request(message, response);
            }
        }

        if (sendto(server_fd, response, strlen(response), 0,
                   (struct sockaddr *)&client_addr,
                   client_len) < 0) {
            perror("sendto");
        } else {
            printf("Sent response: %s\n", response);
        }

        fflush(stdout);
    }

    close(server_fd);
    return 0;
}
