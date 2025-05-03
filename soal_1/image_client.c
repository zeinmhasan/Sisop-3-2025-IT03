#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#define PORT 9090
#define MAX_BUFFER 4096
#define SECRETS_DIR "client/secrets"
#define DOWNLOAD_DIR "client/downloads"

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

void print_menu() {
    printf("\n╔══════════════════════════════════╗\n");
    printf("║      IMAGE CLIENT MENU          ║\n");
    printf("╠══════════════════════════════════╣\n");
    printf("║ 1. Upload and decrypt text file ║\n");
    printf("║ 2. Download file from server    ║\n");
    printf("║ 3. List available secret files  ║\n");
    printf("║ 4. List downloaded files        ║\n");
    printf("║ 5. Exit                        ║\n");
    printf("╚══════════════════════════════════╝\n");
    printf("Enter your choice: ");
}

void list_files(const char *dir) {
    DIR *d = opendir(dir);
    if (!d) {
        printf("Cannot open directory %s: %s\n", dir, strerror(errno));
        return;
    }
    
    printf("\nFiles in %s:\n", dir);
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_name[0] != '.') {
            printf("- %s\n", entry->d_name);
        }
    }
    closedir(d);
}

char* read_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) return NULL;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *content = malloc(size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    fread(content, 1, size, file);
    content[size] = '\0';
    fclose(file);
    
    return content;
}

int connect_to_server() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Socket error: %s\n", strerror(errno));
        return -1;
    }
    
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT)
    };
    
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        printf("Invalid address\n");
        close(sock);
        return -1;
    }
    
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connection failed: %s\n", strerror(errno));
        close(sock);
        return -1;
    }
    
    return sock;
}

void upload_file() {
    list_files(SECRETS_DIR);
    printf("\nEnter filename to upload: ");
    
    char filename[256];
    if (scanf("%255s", filename) != 1) {
        printf("Invalid input\n");
        return;
    }
    getchar();
    
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", SECRETS_DIR, filename);
    
    printf("Trying to read file: %s\n", path);  // Debug
    
    if (access(path, F_OK) == -1) {
        printf("File doesn't exist\n");
        return;
    }
    if (access(path, R_OK) == -1) {
        printf("No read permission\n");
        return;
    }
    
    char *content = read_file(path);
    if (!content) {
        printf("Error reading %s: %s\n", path, strerror(errno));
        return;
    }
    
    int sock = connect_to_server();
    if (sock < 0) {
        free(content);
        return;
    }
    
    char request[MAX_BUFFER];
    snprintf(request, sizeof(request), "UPLOAD %s\n%s", filename, content);
    free(content);
    
    if (write(sock, request, strlen(request)) < 0) {
        printf("Send error: %s\n", strerror(errno));
        close(sock);
        return;
    }
    
    char response[MAX_BUFFER];
    ssize_t bytes = read(sock, response, sizeof(response) - 1);
    if (bytes > 0) {
        response[bytes] = '\0';
        if (strncmp(response, "OK:", 3) == 0) {
            printf("Success! Saved as: %s\n", response + 3);
        } else {
            printf("Server error: %s\n", response);
        }
    } else {
        printf("No server response\n");
    }
    close(sock);
}

void download_file() {
    printf("Enter filename to download (e.g. 1234567890.jpeg): ");
    char filename[256];
    if (scanf("%255s", filename) != 1) {
        printf("Invalid input\n");
        return;
    }
    getchar();
    
    int sock = connect_to_server();
    if (sock < 0) return;
    
    char request[MAX_BUFFER];
    snprintf(request, sizeof(request), "DOWNLOAD %s\n", filename);
    
    if (write(sock, request, strlen(request)) < 0) {
        printf("Send error: %s\n", strerror(errno));
        close(sock);
        return;
    }
    
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", DOWNLOAD_DIR, filename);
    
    FILE *file = fopen(path, "wb");
    if (!file) {
        printf("Cannot create %s: %s\n", path, strerror(errno));
        close(sock);
        return;
    }
    
    char buffer[MAX_BUFFER];
    ssize_t bytes;
    while ((bytes = read(sock, buffer, sizeof(buffer))) > 0) {
        if (fwrite(buffer, 1, bytes, file) != bytes) {
            printf("Write error\n");
            break;
        }
    }
    
    fclose(file);
    close(sock);
    
    if (bytes < 0) {
        printf("Download error: %s\n", strerror(errno));
        remove(path);
    } else {
        printf("Downloaded to: %s\n", path);
    }
}

int main() {
    create_directory("client");
    create_directory(SECRETS_DIR);
    create_directory(DOWNLOAD_DIR);
    
    int choice;
    do {
        print_menu();
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            choice = 0;
        }
        getchar();
        
        switch (choice) {
            case 1: upload_file(); break;
            case 2: download_file(); break;
            case 3: list_files(SECRETS_DIR); break;
            case 4: list_files(DOWNLOAD_DIR); break;
            case 5: printf("Goodbye!\n"); break;
            default: printf("Invalid choice\n");
        }
    } while (choice != 5);
    
    return 0;
}

