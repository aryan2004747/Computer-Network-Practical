#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 4096

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    signal(SIGCHLD, SIG_IGN);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        exit(1);
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        exit(1);
    }

    printf("Server is running on port %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd,
                           (struct sockaddr *)&client_addr,
                           &client_len);

        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            close(client_fd);
            continue;
        }

        if (pid == 0) {
            // Child process handles one client
            char buffer[BUFFER_SIZE] = {0};
            char student_id[50];
            char response[200] = "Record Not Found";

            close(server_fd);

            ssize_t n = read(client_fd, buffer,
                             sizeof(buffer) - 1);

            if (n > 0) {
                buffer[n] = '\0';

                char *saveptr;
                char *id = strtok_r(buffer, " \t\r\n",
                                    &saveptr);

                snprintf(student_id, sizeof(student_id),
                         "%s", id ? id : "");

                char *name = strtok_r(NULL, " \t\r\n",
                                      &saveptr);
                char *dept = strtok_r(NULL, " \t\r\n",
                                      &saveptr);

                if (id && name && dept) {
                    // Search records received from client
                    char *records = strtok_r(NULL, "",
                                             &saveptr);
                    char *line_save = NULL;
                    char *line = records ?
                        strtok_r(records, "\n", &line_save) :
                        NULL;

                    while (line) {
                        char rid[50], rname[100], rdept[100];
                        char extra;

                        if (sscanf(line, "%49s %99s %99s %c",
                                   rid, rname, rdept, &extra) == 3
                            && strcmp(rid, student_id) == 0) {
                            snprintf(response, sizeof(response),
                                     "Record Found: %s %s %s",
                                     rid, rname, rdept);
                            break;
                        }

                        line = strtok_r(NULL, "\n", &line_save);
                    }
                }
            }

            write(client_fd, response, strlen(response));
            close(client_fd);
            exit(0);
        }

        // Parent continues accepting new clients
        close(client_fd);
        client_len = sizeof(client_addr);
    }

    close(server_fd);
    return 0;
}
