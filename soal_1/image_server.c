#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <syslog.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>

#define PORT 9090
#define MAX_BUFFER 4096
#define DATABASE_DIR "server/database"
#define LOG_FILE "server/server.log"

void create_directory(const char *path) {
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

void log_action(const char *source, const char *action, const char *info) {
    time_t now;
    time(&now);
    struct tm *tm_info = localtime(&now);
    
    char timestamp[20];
    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    
    create_directory("server");
    
    FILE *log = fopen(LOG_FILE, "a");
    if (log) {
        fprintf(log, "[%s][%s]: [%s] [%s]\n", source, timestamp, action, info);
        fclose(log);
    }
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);
    if (setsid() < 0) exit(EXIT_FAILURE);
    
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);
    
    umask(0);
    chdir("/");
    
    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
        close(x);
    }
    
    openlog("image_server", LOG_PID, LOG_DAEMON);
}

char* decrypt_data(const char *input) {
    char *clean_input = strdup(input);
    if (!clean_input) return NULL;
    
    char *ptr = clean_input;
    while (*ptr) {
        if (*ptr == '\n' || *ptr == '\r') *ptr = '\0';
        ptr++;
    }

    int len = strlen(clean_input);
    if (len % 2 != 0) {
        free(clean_input);
        return NULL;
    }

    char *reversed = malloc(len + 1);
    if (!reversed) {
        free(clean_input);
        return NULL;
    }
    
    for (int i = 0; i < len; i++) {
        reversed[i] = clean_input[len - 1 - i];
    }
    reversed[len] = '\0';
    free(clean_input);

    char *decoded = malloc(len / 2 + 1);
    if (!decoded) {
        free(reversed);
        return NULL;
    }
    
    for (int i = 0; i < len / 2; i++) {
        if (sscanf(reversed + i * 2, "%2hhx", &decoded[i]) != 1) {
            free(reversed);
            free(decoded);
            return NULL;
        }
    }
    decoded[len / 2] = '\0';
    free(reversed);

    return decoded;
}

void handle_client(int client_sock) {
    char buffer[MAX_BUFFER];
    ssize_t bytes_read;
    
    bytes_read = read(client_sock, buffer, MAX_BUFFER - 1);
    if (bytes_read <= 0) {
        log_action("Server", "ERROR", "Failed to read from client");
        close(client_sock);
        return;
    }
    buffer[bytes_read] = '\0';

    if (strncmp(buffer, "UPLOAD", 6) == 0) {
        char *filename = buffer + 7;
        char *data_start = strchr(filename, '\n');
        if (!data_start) {
            write(client_sock, "ERROR: Invalid upload format", 28);
            log_action("Server", "ERROR", "Invalid upload format");
            close(client_sock);
            return;
        }
        *data_start = '\0';
        char *file_data = data_start + 1;

        char *decrypted_data = decrypt_data(file_data);
        if (!decrypted_data) {
            write(client_sock, "ERROR: Decryption failed", 25);
            log_action("Server", "ERROR", "Decryption failed");
            close(client_sock);
            return;
        }

        time_t now = time(NULL);
        char db_filename[256];
        snprintf(db_filename, sizeof(db_filename), "%s/%ld.jpeg", DATABASE_DIR, now);

        create_directory(DATABASE_DIR);
        
        FILE *file = fopen(db_filename, "wb");
        if (file) {
            size_t data_len = strlen(decrypted_data);
            size_t written = fwrite(decrypted_data, 1, data_len, file);
            fclose(file);
            
            if (written == data_len) {
                char response[256];
                snprintf(response, sizeof(response), "OK:%ld.jpeg", now);
                write(client_sock, response, strlen(response));
                log_action("Server", "SAVE", db_filename);
            } else {
                write(client_sock, "ERROR: Incomplete write", 23);
                log_action("Server", "ERROR", "Incomplete file write");
                remove(db_filename);
            }
        } else {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "ERROR: Failed to save file (%s)", strerror(errno));
            write(client_sock, error_msg, strlen(error_msg));
            
            char debug_msg[512];
            snprintf(debug_msg, sizeof(debug_msg), "Failed to open %s: %s (database dir exists: %d)", 
                    db_filename, strerror(errno), access(DATABASE_DIR, F_OK) == 0);
            log_action("Server", "DEBUG", debug_msg);
        }
        
        free(decrypted_data);
    } else if (strncmp(buffer, "DOWNLOAD", 8) == 0) {
        char *filename = buffer + 9;
        filename[strcspn(filename, "\n")] = '\0';
        
        char filepath[256];
        snprintf(filepath, sizeof(filepath), "%s/%s", DATABASE_DIR, filename);
        
        FILE *file = fopen(filepath, "rb");
        if (file) {
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);
            
            char *file_data = malloc(file_size);
            if (file_data) {
                fread(file_data, 1, file_size, file);
                fclose(file);
                
                write(client_sock, file_data, file_size);
                free(file_data);
                
                log_action("Server", "SEND", filename);
            } else {
                fclose(file);
                write(client_sock, "ERROR: Memory allocation failed", 30);
            }
        } else {
            write(client_sock, "ERROR: File not found", 21);
            log_action("Server", "ERROR", "File not found");
        }
    } else {
        write(client_sock, "ERROR: Unknown command", 22);
        log_action("Server", "ERROR", "Unknown command");
    }
    
    close(client_sock);
}

int main() {
    // Untuk debugging, bisa di-comment
    // daemonize();
    
    create_directory(DATABASE_DIR);
    create_directory("server");
    
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        syslog(LOG_ERR, "Failed to create socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    // Set socket option to reuse address
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        syslog(LOG_ERR, "Setsockopt failed: %s", strerror(errno));
        close(server_sock);
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT)
    };
    
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        syslog(LOG_ERR, "Failed to bind socket: %s", strerror(errno));
        close(server_sock);
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_sock, 5) < 0) {
        syslog(LOG_ERR, "Failed to listen: %s", strerror(errno));
        close(server_sock);
        exit(EXIT_FAILURE);
    }
    
    syslog(LOG_INFO, "Server started on port %d", PORT);
    log_action("Server", "START", "Server initialized");
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_sock < 0) {
            syslog(LOG_ERR, "Accept failed: %s", strerror(errno));
            continue;
        }
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "Connection accepted from %s", client_ip);
        
        handle_client(client_sock);
    }
    
    close(server_sock);
    return 0;
}

