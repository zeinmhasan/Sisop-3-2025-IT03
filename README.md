<table>
  <thead>
    <tr>
      <th>No</th>
      <th>Nama</th>
      <th>NRP</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>1</td>
      <td>Zein muhammad hasan</td>
      <td>5027241035</td>
    </tr>
    <tr>
      <td>2</td>
      <td>Afriza Tristan Calendra Rajasa</td>
      <td>5027241104</td>
    </tr>
    <tr>
      <td>3</td>
      <td>Jofanka Al-kautsar Pangestu Abady</td>
      <td>5027241107</td>
    </tr>
  </tbody>
</table>




<nav>
  <ul>
    <li><a href="#soal1">Soal1</a></li>
    <li><a href="#soal2">Soal2</a></li>
    <li><a href="#soal3">Soal3</a></li>
    <li><a href="#soal4">Soal4</a></li>
  </ul>
</nav>


  <h2 id="soal1">Soal1</h2>

  # Encrypted Image Transfer System (Client-Server RPC)

Proyek ini terdiri dari dua program C: `image_client.c` dan `image_server.c`, yang membentuk sistem client-server untuk **mengirim file terenkripsi** ke server, **menyimpannya sebagai gambar JPEG**, dan **mengunduhnya kembali** atas permintaan.

---

## Struktur Proyek

```
project-root/
├── client/
│ ├── image_client # Binary client
│ ├── secrets/ # Folder tempat file .txt terenkripsi berada
│ └── *.jpeg # File hasil download dari server
├── server/
│ ├── image_server # Binary server (daemon)
│ ├── database/ # Tempat penyimpanan file JPEG hasil upload
│ └── server.log # Log aktivitas server
├── image_client.c # Kode sumber client
└── image_server.c # Kode sumber server
```


---

## Mekanisme Umum

- **Client**:
  - Menyediakan menu interaktif.
  - Mengirim file `.txt` yang berisi hex terenkripsi ke server.
  - File di-reverse dan didecode oleh server menjadi gambar JPEG.
  - Client dapat meminta file JPEG berdasarkan nama.

- **Server**:
  - Berjalan sebagai daemon.
  - Menerima dan memproses permintaan `UPLOAD` dan `DOWNLOAD`.
  - Menyimpan hasil dekripsi sebagai JPEG di folder `database/`.
  - Menulis log semua aktivitas ke `server.log`.

---

##  Format Enkripsi dan Dekripsi

1. File `.txt` berisi **hex string** (misalnya: `FFD8FFE000104A...`)
2. Sebelum decode, hex string di-**reverse**.
3. Decode hex menghasilkan data biner JPEG, lalu disimpan.

---

##  Penjelasan `image_client.c`

### Header dan Utility

```c
void reverse_string(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; ++i) {
        char tmp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = tmp;
    }
}
```

- `#define MAX_BUFFER 4096` → Ukuran buffer komunikasi.
- Fungsi `reverse_string()` → Membalik string.
- Fungsi `read_file()` → Membaca isi file `.txt` untuk dikirim.
- Fungsi `write_file()` → Menyimpan file JPEG hasil download.

### Fungsi `upload_file()`

```c
void upload_file(int sockfd) {
    char filename[256];
    printf("Masukkan nama file (.txt) terenkripsi: ");
    scanf("%s", filename);
    
    char path[512];
    snprintf(path, sizeof(path), "secrets/%s", filename);
    
    char *data = read_file(path);
    reverse_string(data);

    dprintf(sockfd, "UPLOAD %s\n%s", filename, data);
    ...
}
```

- Membaca file `.txt` terenkripsi.
- Melakukan reverse string.
- Mengirim perintah `UPLOAD filename\n<reversed_hex>` ke server.
- Menerima respons `OK:<nama_output.jpeg>`.

### Fungsi `download_file()`

```c
void download_file(int sockfd) {
    char filename[256];
    printf("Masukkan nama file JPEG yang ingin didownload: ");
    scanf("%s", filename);
    
    dprintf(sockfd, "DOWNLOAD %s\n", filename);
    ...
}
```
- Mengirim perintah `DOWNLOAD <nama_file>\n`.
- Menerima stream JPEG dari server.
- Menyimpannya di direktori `client/`.

### Fungsi `main()`

```c
int main() {
    ...
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ...
    while (1) {
        printf("\nMenu:\n1. Upload File\n2. Download File\n3. Keluar\nPilihan: ");
        scanf("%d", &choice);
        ...
    }
}
```

- Menampilkan menu:
  - Upload File
  - Download File
  - Keluar
- Menghubungkan ke server menggunakan TCP.
- Meneruskan input ke fungsi `upload_file()` / `download_file()`.

---

##  Penjelasan `image_server.c`

### Fungsi `daemonize()`

```c
void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    setsid();
    ...
    close(STDIN_FILENO); close(STDOUT_FILENO); close(STDERR_FILENO);
}
```

- Membuat server berjalan di background.
- Menutup terminal (STDIN, STDOUT, STDERR).
- Menulis log ke syslog.

### Fungsi `log_action()` dan `create_directory()`

```c
void log_action(const char *message) {
    FILE *log = fopen("server/server.log", "a");
    ...
}
void create_directory(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir(path, 0700);
    }
}
```

- `log_action()` → Mencatat aktivitas ke `server/server.log`.
- `create_directory()` → Membuat direktori jika belum ada.

### Fungsi `decrypt_data()`

```c
char *decrypt_data(const char *input) {
    size_t len = strlen(input);
    char *reversed = strdup(input);
    reverse_string(reversed);

    unsigned char *decoded = malloc(len / 2);
    for (size_t i = 0; i < len; i += 2) {
        sscanf(reversed + i, "%2hhx", &decoded[i / 2]);
    }

    free(reversed);
    return (char *)decoded;
}
```

- Melakukan:
  - Reverse string.
  - Decode setiap 2 karakter hex menjadi byte asli (menggunakan `sscanf()`).
- Hasil: string biner JPEG.

### Fungsi `handle_client()`

```c
void handle_client(int client_sock) {
    char buffer[MAX_BUFFER];
    memset(buffer, 0, sizeof(buffer));
    read(client_sock, buffer, sizeof(buffer) - 1);

    if (strncmp(buffer, "UPLOAD", 6) == 0) {
        ...
        char *jpeg_data = decrypt_data(hex_data);
        ...
        write_file(filepath, jpeg_data, strlen(jpeg_data));
        write(client_sock, response, strlen(response));
    } else if (strncmp(buffer, "DOWNLOAD", 8) == 0) {
        ...
        write(client_sock, file_data, file_size);
    }
}
```
Menangani permintaan dari client:

#### UPLOAD
- Format permintaan: `UPLOAD <filename>\n<reversed_hex_data>`
- Memanggil `decrypt_data()`.
- Menyimpan hasil decode sebagai `timestamp.jpeg` ke `server/database/`.
- Mengirim `OK:<nama_file.jpeg>` ke client.
- Mencatat log.

#### DOWNLOAD
- Format permintaan: `DOWNLOAD <filename>\n`
- Membaca file dari `server/database/`.
- Mengirim file sebagai biner ke client.
- Jika gagal, kirim `"ERROR: File not found\n"`.

### Fungsi `main()`

```c
int main() {
    daemonize();
    create_directory("server/database");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ...
    while (1) {
        int client_sock = accept(sockfd, ...);
        if (fork() == 0) {
            handle_client(client_sock);
            close(client_sock);
            exit(0);
        }
        close(client_sock);
    }
}
```
- Membuka socket TCP di port `12345`.
- Melakukan bind dan listen.
- Menerima koneksi menggunakan `accept()`.
- Fork process:
  - Anak proses → menangani client.
  - Proses utama → lanjut menerima koneksi baru.

---

##  Komunikasi

- **Protocol**: Socket TCP sederhana.
- **RPC-style interaction** (client mengirim perintah spesifik, server merespon sesuai perintah).
- **Format komunikasi**: berbasis teks untuk perintah, biner untuk data gambar.

---

##  Kelebihan

- Komunikasi client-server efisien via TCP.
- Data aman karena dikirim dalam bentuk terenkripsi.
- Server berjalan sebagai daemon, cocok untuk layanan background.
- Logging sistematis untuk debugging dan audit.

---


##  Cara Menjalankan

### 1. Kompilasi

```bash
gcc -o image_client image_client.c
gcc -o image_server image_server.c
```

Jalankan Server (daemon)
```bash
cd server
./image_server
```

Jalankan Client
```bash
cd client
./image_client

```

##  output
![Image](https://github.com/user-attachments/assets/9219cfff-d1a1-49e6-9f8c-b0718ef162c4)
![Image](https://github.com/user-attachments/assets/c274d27b-911f-4d21-aa0c-a8c5f3738a9d)

<h2 id="soal2">Soal2</h2>

#  Delivery Management System

Sistem manajemen pengiriman berbasis shared memory dan multithreading dengan dua program utama: `delivery_agent` dan `dispatcher`.

---

##  Penjelasan `delivery_agent.c`

###  Header dan Utility

```c
void log_delivery(const char *agent, const char *type, const char *name, const char *address) {
    FILE *log = fopen("delivery.log", "a");
    if (!log) return;
    flock(fileno(log), LOCK_EX); // Lock file
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log, "[%02d/%02d/%04d %02d:%02d:%02d] [%s] %s package delivered to %s in %s\n",
        t->tm_mday, t->tm_mon+1, t->tm_year+1900, t->tm_hour, t->tm_min, t->tm_sec,
        agent, type, name, address);
    fflush(log);
    flock(fileno(log), LOCK_UN);
    fclose(log);
}
```


```c
void* agent_thread(void *arg) {
    char *agent = (char*) arg;
    while (1) {
        pthread_mutex_lock(&shared_data->lock);
        for (int i = 0; i < shared_data->count; i++) {
            if (shared_data->orders[i].type == EXPRESS && shared_data->orders[i].status == PENDING) {
                shared_data->orders[i].status = DELIVERED;
                strcpy(shared_data->orders[i].agent, agent);
                log_delivery(agent, "Express", shared_data->orders[i].name, shared_data->orders[i].address);
                break;
            }
        }
        pthread_mutex_unlock(&shared_data->lock);
        sleep(1);
    }
    return NULL;
}
```

- `#define MAX_ORDERS 100` → Batas maksimum pesanan yang dapat diproses.
- Menggunakan **Shared Memory** (`shmget`, `shmat`) untuk berbagi data antar proses dengan `dispatcher`.
- Menggunakan **Thread** (`pthread_create`) untuk menangani pengiriman otomatis oleh agen.

###  Fungsi `read_csv()`

```c
void read_csv_and_store(const char *filename) {
    FILE *f = fopen(filename, "r");
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *name = strtok(line, ",");
        char *address = strtok(NULL, ",");
        char *type_str = strtok(NULL, "\n");
        Order *o = &shared_data->orders[shared_data->count++];
        strncpy(o->name, name, MAX_NAME);
        strncpy(o->address, address, MAX_ADDRESS);
        o->type = parse_type(type_str);
        o->status = PENDING;
        strcpy(o->agent, "-");
    }
    fclose(f);
}
```
- Membaca file `delivery_order.csv`.
- Menyimpan data pesanan ke shared memory.

###  Fungsi `*express_delivery()`

- Setiap thread agen (`AGENT A`, `AGENT B`, `AGENT C`) akan:
  - Mengambil pesanan bertipe **Express**.
  - Melakukan simulasi pengiriman.
  - Mengubah status menjadi `DELIVERED`.
  - Menulis entri log ke `delivery.log`.

###  Fungsi `main()`

```c
int main() {
    key_t key = ftok("delivery_agent.c", 65);
    int shmid = shmget(key, sizeof(SharedData), 0666 | IPC_CREAT);
    shared_data = (SharedData*) shmat(shmid, NULL, 0);

    shared_data->count = 0;
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shared_data->lock, &mattr);

    read_csv_and_store("delivery_order.csv");

    pthread_t t1, t2, t3;
    pthread_create(&t1, NULL, agent_thread, "AGENT A");
    pthread_create(&t2, NULL, agent_thread, "AGENT B");
    pthread_create(&t3, NULL, agent_thread, "AGENT C");

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    shmdt(shared_data);
    return 0;
}
```
- Membuat segmen shared memory.
- Membaca pesanan dari file CSV.
- Membuat 3 thread untuk pengiriman otomatis oleh agen.
- Menunggu semua thread selesai (`pthread_join`).

---

##  Penjelasan `dispatcher.c`

### Mode Operasi

Program dijalankan dengan argumen tertentu:

```c
if (strcmp(argv[1], "-deliver") == 0 && argc == 3) {
    char *nama = argv[2];
    for (int i = 0; i < shared_data->count; i++) {
        if (strcmp(shared_data->orders[i].name, nama) == 0 && shared_data->orders[i].type == REGULER) {
            if (shared_data->orders[i].status == DELIVERED) {
                printf("Order sudah dikirim.\n");
                return 0;
            }
            shared_data->orders[i].status = DELIVERED;
            strcpy(shared_data->orders[i].agent, get_username());
            log_delivery(shared_data->orders[i].agent, "Reguler", nama, shared_data->orders[i].address);
            printf("Order untuk %s telah dikirim.\n", nama);
            return 0;
        }
    }
    printf("Order tidak ditemukan.\n");
}
```

- `-deliver <Nama>`
  → Menandai pesanan **Reguler** sebagai dikirim.  
  → Menulis log dengan username (`getlogin()`).
  → Akan gagal jika:
    - Nama tidak ditemukan.
    - Tipe pesanan bukan Reguler.
    - Sudah dikirim sebelumnya.

```c
else if (strcmp(argv[1], "-status") == 0 && argc == 3) {
    char *nama = argv[2];
    for (int i = 0; i < shared_data->count; i++) {
        if (strcmp(shared_data->orders[i].name, nama) == 0) {
            if (shared_data->orders[i].status == DELIVERED)
                printf("Status for %s: Delivered by %s\n", nama, shared_data->orders[i].agent);
            else
                printf("Status for %s: Pending\n", nama);
            return 0;
        }
    }
    printf("Order tidak ditemukan.\n");
}
```
- `-status <Nama>`  
  → Menampilkan status pengiriman berdasarkan nama.

```c
else if (strcmp(argv[1], "-list") == 0) {
    for (int i = 0; i < shared_data->count; i++) {
        printf("%s - %s\n", shared_data->orders[i].name,
            shared_data->orders[i].status == DELIVERED ? "Delivered" : "Pending");
    }
}
```
- `-list`  
  → Menampilkan seluruh daftar pesanan dan statusnya.

###  Shared Memory

- Mengakses shared memory yang telah dibuat oleh `delivery_agent.c`.
- Menggunakan `ftok("delivery_agent.c", 65)` agar key konsisten antara kedua program.

---

##  Interaksi Proses

- Komunikasi antar proses dilakukan melalui **Shared Memory IPC**.
- Kedua program (`delivery_agent` dan `dispatcher`) membaca dan menulis ke area memori yang sama.

---

##  Format Log `delivery.log`

```text
[08/05/2025 12:34:56] [AGENT A] Express package delivered to Alice in Surabaya
[08/05/2025 12:35:00] [zain] Reguler package delivered to Bob in Malang
```

##  output
![Image](https://github.com/user-attachments/assets/2dab3d25-e746-4026-a467-3f34b229485b)
![Image](https://github.com/user-attachments/assets/f7b6eb0a-470a-4187-a90f-8fe0989a4368)

<h2 id="soal3">Soal3</h2>

Pada soal ini, praktikan diminta untuk membuat tiga file, yaitu `dungeon.c` sebagai server, `shop.c`, dan `player.c` sebagai client.

### a. dungeon.c
**1) Library**

Untuk library yang digunakan yaitu library standar, `<arpa/inet.h>`, `<time.h>`, dan `"shop.c"` untuk mengambil data dari file `shop.c`. Untuk kodenya seperti ini

    #include <stdio.h>
    #include <unistd.h>
    #include <string.h>
    #include <stdlib.h>
    #include <time.h>
    #include <arpa/inet.h>
    #include "shop.c"

**2) Main Menu**

Fungsi ini digunakan untuk menampilkan UI main menu. Untuk kodenya seperti ini
    
    void display_menu(int client_socket)
    {
        // mengirimkan text ke client
        char menu[] = 
            "\033c\033[3J"
            "\033[38;2;255;0;0m  Welcome to My Hero Academia   \e[0m\n"
            "\033[38;2;255;250;0m僕のヒーローアカデミアへようこそ\e[0m\n\n"
            "\033[38;2;105;210;82m=== Main Menu ===\e[0m\n"
            "1. Show Player Stats\n"
            "2. View Inventory\n"
            "3. Shop\n"
            "4. Equip Weapon\n"
            "5. Battle Mode\n"
            "6. About\n"
            "x. Exit\e[0m\n"
            "\033[38;2;105;210;82m=================\n"
            "Choice: \e[0m";
        send(client_socket, menu, strlen(menu), 0);
    }

dimana untuk cara kerjanya seperti ini.
1. `"\033c\033[3J"` adalah kode untuk clear screen agar tampilan lebih rapih. Kode ini berlaku di beberapa fungsi lain.
2. `\033[38;2;255;0;0m` adalah kode warna, dimana output akan memuncukan warna yang berbeda dan diakhiri dengan `\e[0m`.
3. Untuk mengirimkan output ke client, diguankan fungsi `send()`.

**3) Struct**

Struct digunakan untuk memuat artibut data player dan musuh. Untuk kodenya seperti ini

**struct Player**

    typedef struct
    {
        int gold;
        Weapon inventory[20];
        int inventory_count;
        Weapon equipped;
        int base_damage;
        int enemies_defeated;
        int frozen;
    } Player;

**struct Enemy**

    typedef struct
    {
        char name[50];
        int health;
        int damage;
    } Enemy;

**4) Fungsi `trim_trailing_whitespace`**

Fungsi ini digunakan untuk menghapus karakter tertentu, seperti `"\n"`, `"\r"`, `" "`, dan `"\t"` yang akan digunakan untuk case 5. Untuk kodenya seperti ini

    void trim_trailing_whitespace(char *str)
    {
        int len = strlen(str);
        while(len > 0 && (str[len-1] == '\n' || str[len-1] == '\r' || str[len-1] == ' ' || str[len-1] == '\t'))
        {
            str[len-1] = '\0';
            len--;
        }
    }

**5) Fungsi `handle_client`**

Fungsi ini digunakan untuk menjalankan keseluruhan fungsi UI dan cara program bekerja. Untuk kodenya seperti ini

    void handle_client(int client_socket)
    {
        Player player;
        player.gold = 100;
        player.inventory_count = 0;
        player.inventory[player.inventory_count++] = fists;
        player.equipped = fists;
        player.base_damage = player.equipped.damage;
        player.enemies_defeated = 0;
        player.frozen = 0;
    
        char buffer[4096];
        while (1)
        {
            display_menu(client_socket);
    
            char buf[100] = {0};
            memset(buf, 0, sizeof(buf));
            int valread = read(client_socket, buf, sizeof(buf));
            if(valread <= 0) {
                // client disconnect
                break;
            }
            trim_trailing_whitespace(buf);
    
            int option = buf[0];
    
            switch (option)
            {
                case '1':
                {
                    ...
                }
                case '2':
                {
                    ...
                }
                case '3':
                {
                    ...
                }
                case '4':
                {
                    ...
                }
                case '5':
                {
                    ...
                }
                case '6':
                {
                    ...
                }
                case 'x':
                {
                    ...
                }
                default:
                {
                    ...
                }
            }
        }
    }

dimana cara kerjanya sebagai berikut.
1. Player akan mendapatkan atribut default.
2. Program akan memunculkan UI kepada client.
3. Setelah server menerima input dari client, maka input dari client akan diproses menggunakan switch case.
4. Jika player salah input ke server, maka switch case default dijalankan.

**6) Show Player Stats**

Show player stats adalah informasi untuk melihat atribut yang digunakan oleh player. Untuk kodenya seperti ini

    case '1':
    {
        char msg[4096];
        snprintf(msg, sizeof(msg),
            "\033c\033[3J"
            "\033[38;2;105;210;82m=== PLAYER STATS ===\e[0m\n\n"
            "\033[38;2;243;177;68mGold: %d\e[0m\n"
            "Equipped: %s\n"
            "\033[38;2;226;79;65mBase damage: %d\e[0m\n"
            "\033[38;2;127;217;241mEnemies defeated: %d\e[0m\n"
            "Passive: %s\n\n"
            "\033[38;2;243;177;68mPress enter to return...\e[0m ",
            player.gold, player.equipped.name,
            player.base_damage, player.enemies_defeated,
            player.equipped.passive);
    
        send(client_socket, msg, strlen(msg), 0);
        memset(buf, 0, sizeof(buf));
        valread = read(client_socket, buf, sizeof(buf)); // tunggu ENTER
        break;
    }

dimana cara kerjanya sebagai berikut.
1. Program membuat array.
2. `snprinf` digunakan untuk menginput data tertentu seperti UI dan informasi player.
3. Server mengirimkan array tadi ke client menggunakan send.
4. `memset` diperlukan untuk menghindari input maupun output yang tidak diinginkan.
5. Setelah itu, jika player menginputkan `ENTER`, maka setelah itu switch case akan berhenti.

**7) Inventory**

Sama seperti shoe player stats, inventory digunakan untuk menampilkan persediaan yang dimiliki player. Untuk kodenya seperti ini

    case '2':
    {
        char msg[4096];
        msg[0] = '\0'; // inisialisasi string kosong
    
        snprintf(msg, sizeof(msg), "\033c\033[3J\033[38;2;105;210;82m=== INVENTORY ===\e[0m\n\n");
        for (int i = 0; i < player.inventory_count; i++)
        {
            char temp[100];
            snprintf(temp, sizeof(temp), "%d. %s - \033[38;2;226;79;65mDamage: %d\e[0m - Passive: %s\n",
                i+1, player.inventory[i].name, player.inventory[i].damage,
                player.inventory[i].passive);
            strncat(msg, temp, sizeof(msg) - strlen(msg) - 1);
        }
        strncat(msg, "\n\033[38;2;243;177;68mPress enter to return..\e[0m ", sizeof(msg) - strlen(msg) - 1);
    
        send(client_socket, msg, strlen(msg), 0);
        memset(buf, 0, sizeof(buf));
        valread = read(client_socket, buf, sizeof(buf)); // tunggu ENTER
        break;
    }

dimana cara kerjanya sebagai berikut.
1. Program membuat array.
2. `snprinf` digunakan untuk menginput data tertentu seperti UI dan informasi persediaan player.
3. Server mengirimkan array tadi ke client menggunakan send.
4. `memset` diperlukan untuk menghindari input maupun output yang tidak diinginkan.
5. Setelah itu, jika player menginputkan `ENTER`, maka setelah itu switch case akan berhenti.

**8) Shop**

Pada switch case shop digunakan untuk memunculkan weapon di shop yang dapat dibeli oleh player. Untuk kodenya seperti ini

    case '3':
    {
        char msg[4096];
        snprintf(msg, sizeof(msg), "\033c\033[3J\033[38;2;105;210;82mWelcome to the shop!\e[0m\n\033[38;2;243;177;68mYour gold:\e[0m %d\n\n", player.gold);
        for (int i = 0; i < total_weapons; i++)
        {
            char weapon_msg[256];
            snprintf (weapon_msg, sizeof(weapon_msg), "%d. %s | \033[38;2;243;177;68mPrice: %d Gold\e[0m | \033[38;2;226;79;65mDamage: %d\e[0m | Passive: %s\n",
                i + 1, weapons[i].name, weapons[i].price, weapons[i].damage, weapons[i].passive);
            strncat(msg, weapon_msg, sizeof(msg) - strlen(msg) - 1);
        }
        strncat(msg, "\n\033[38;2;243;177;68mEnter the weapon you want to purchase (or 0 to cancel): \e[0m", sizeof(msg) - strlen(msg) - 1);
    
        send(client_socket, msg, strlen(msg), 0);
    
        memset(buf, 0, sizeof(buf));
        valread = read(client_socket, buf, sizeof(buf));
        if(valread <= 0) break; // client disconnect
    
        int choice = atoi(buf);
        if (choice == 0) break;
    
        Weapon selected = buy_weapon(choice);
    
        if (strlen(selected.name) == 0)
        {
            const char error[] = "\033[38;2;226;79;65mInvalid choice. Press enter to return...\e[0m ";
            send(client_socket, error, strlen(error), 0);
            memset(buf, 0, sizeof(buf));
            valread = read(client_socket, buf, sizeof(buf));
            break;
        }
    
        if (player.gold >= selected.price)
        {
            player.gold -= selected.price;
            player.inventory[player.inventory_count++] = selected;
    
            char success[256];
            snprintf(success, sizeof(success),
                "\033[38;2;243;177;68mSuccessfully purchased %s for %d gold!\nPress enter to return...\e[0m ",
                selected.name, selected.price);
            send(client_socket, success, strlen(success), 0);
        }
        else
        {
            const char failed[] = "\033[38;2;226;79;65mNot enough gold! Press enter to return...\e[0m ";
            send(client_socket, failed, strlen(failed), 0);
        }
        memset(buf, 0, sizeof(buf));
        valread = read(client_socket, buf, sizeof(buf)); // tunggu ENTER
        break;
    }

dimana cara kerja dari kodenya seperti ini.
1. Server mengirimkan tampilan shop kepada client dan meminta untuk client jika ingin membeli weapon.
2. Jika client memasukkan nomor weapon tertentu, maka server akan mengecek apakah gold yang dimiliki player sudah cukup.
3. Jika gold mencukupi, maka weapon dapat dibeli oleh player dan memasukkannnya ke dalam inventory.
4. Jika gold tidak mencukupi, maka server mengirim pesan kepada client bahwa gold tidak mencukupi.
5. Jika player ingin keluar dari shop, maka player dapat menginput `0` dan `ENTER` kepada server.  

**9) Equip Weapon**
Fungsi equip weapon digunakan agar player dapat memilih senjata yang ia punya. Untuk kodenya seperti ini

    case '4':
    {
        char msg[4096];
        msg[0] = '\0';
    
        if (player.inventory_count == 0)
        {
            const char empty_inv[] = "\033[38;2;226;79;65mInventory is empty. Press enter to return..\e[0m ";
            send(client_socket, empty_inv, strlen(empty_inv), 0);
            memset(buf, 0, sizeof(buf));
            valread = read(client_socket, buf, sizeof(buf));
            break;
        }
    
        snprintf(msg, sizeof(msg), "\033c\033[3J\033[38;2;105;210;82mPlease choose your weapon\e[0m\n\n");
        for (int i = 0; i < player.inventory_count; i++)
        {
            char temp[200];
            snprintf(temp, sizeof(temp), "%d. %s - \033[38;2;226;79;65mDamage: %d\e[0m - Passive: %s\n",
                i+1, player.inventory[i].name, player.inventory[i].damage,
                player.inventory[i].passive);
            strncat(msg, temp, sizeof(msg) - strlen(msg) - 1);
        }
        strncat(msg, "\n\033[38;2;243;177;68mSelect a weapon number to use (or press 0 to cancel): \e[0m", sizeof(msg) - strlen(msg) - 1);
    
        send(client_socket, msg, strlen(msg), 0);
    
        memset(buf, 0, sizeof(buf));
        valread = read(client_socket, buf, sizeof(buf));
        if (valread <= 0) break;
    
        int choice = atoi(buf);
        if(choice > 0 && choice <= player.inventory_count)
        {
            player.equipped = player.inventory[choice - 1];
            player.base_damage = player.equipped.damage;
    
            char confirm[128];
            snprintf(confirm, sizeof(confirm), "\033[38;2;243;177;68mWeapon %s berhasil digunakan.\nPress enter to return...\e[0m ", player.equipped.name);
            send(client_socket, confirm, strlen(confirm), 0);
        }
        else
        {
            const char cancel[] = "\033[38;2;243;177;68mCancel weapon selection.\nPress enter to return...\e[0m ";
            send(client_socket, cancel, strlen(cancel), 0);
        }
    
        memset(buf, 0, sizeof(buf));
        valread = read(client_socket, buf, sizeof(buf));
        break;
    }

dimana cara kerjanya sebagai berikut.
1. Server mengirimkan data berupa inventory yang dimiliki player.
2. Player memasukkan angka tertentu untuk memilih weapon yang diinginkan.
3. Program akan set weapon dan mengirim pesan kepada client jika pemilihan berhasil.
4. Jika input salah, maka akan dianggap cancel untuk memilih wepon dan player dapat menekan `ENTER` untuk keluar dari tampilan tersebut.

**10) Battle Mode**
Untuk battle mode, pertama-tama server menggunakan fungsi `battle_mode()` lalu mengirimkan ke client. Untuk kode `case '5'` seperti ini.

    case '5':
    {
        battle_mode(&player, client_socket);
        char dummy[100] = {0};
        memset(buf, 0, sizeof(buf));
        valread = read(client_socket, dummy, sizeof(dummy));
        break;
    }
    
Untuk fungsi `battle_mode()` seperti ini

    void battle_mode(Player *player, int client_socket) 
    {
        srand(time(NULL));
        int enemy_hp = rand() % 50 + 500;
        int enemy_max_hp = enemy_hp;
        char buffer[4096], hpbar[128];
    
        // Tampilkan musuh muncul dan bar HP awal
        snprintf(buffer, sizeof(buffer), "\033c\033[3J\033[38;2;226;79;65m============ BATTLE MODE ============\e[0m\n\nEnemy appeared! HP: %d\n", enemy_hp);
        display_hp_bar(enemy_hp, enemy_max_hp, hpbar, sizeof(hpbar));
        strncat(buffer, hpbar, sizeof(buffer) - strlen(buffer) - 1);
        send(client_socket, buffer, strlen(buffer), 0);
    
        // Kirim prompt untuk menunggu input sebelum memasuki loop
        const char *prompt = "Enter command: '\033[38;2;105;210;82mattack\e[0m' or '\033[38;2;226;79;65mexit\e[0m' ";
        send(client_socket, prompt, strlen(prompt), 0);
    
        int battle_active = 1;
        while (battle_active) 
        {
            // Reset buffer untuk pesan penuh
            char full_msg[8192];
            size_t len = 0;
            full_msg[0] = '\0';
    
            // Baca input dari client
            memset(buffer, 0, sizeof(buffer));
            int valread = read(client_socket, buffer, sizeof(buffer) - 1);
            if (valread <= 0) break; // Koneksi putus
            buffer[valread] = '\0';
    
            // Bersihkan trailing whitespace
            trim_trailing_whitespace(buffer);
    
            if (strcmp(buffer, "exit") == 0) 
            {
                snprintf(full_msg, sizeof(full_msg), "Exited battle mode. Press ENTER to exit.\n");
                send(client_socket, full_msg, strlen(full_msg), 0);
                battle_active = 0;
                break;
            } 
            else if (strcmp(buffer, "attack") == 0) 
            {
                if (player->frozen > 0) 
                {
                    snprintf(full_msg, sizeof(full_msg), "Enemy is frozen and skips a turn!\n");
                    send(client_socket, full_msg, strlen(full_msg), 0);
                    player->frozen--;
                    continue; // Kembali ke awal loop untuk menunggu input lagi
                }
    
                int dmg = player->base_damage + (rand() % 6 - 2);
                if (dmg < 0) dmg = 0;
    
                // Hit kritikal
                if (rand() % 10 < 2) 
                {
                    dmg *= 2;
                    len += snprintf(full_msg + len, sizeof(full_msg) - len, "Critical Hit!\n");
                }
    
                enemy_hp -= dmg;
                if (enemy_hp < 0) enemy_hp = 0;
    
                len += snprintf(full_msg + len, sizeof(full_msg) - len,
                    "\033c\033[3J\033[38;2;226;79;65m============ BATTLE MODE ============\e[0m\n\nYou dealt %d damage! Enemy HP now: %d\n", dmg, enemy_hp);
    
                // Tambahkan bar HP
                display_hp_bar(enemy_hp, enemy_max_hp, hpbar, sizeof(hpbar));
                len += snprintf(full_msg + len, sizeof(full_msg) - len, "%s", hpbar);
    
                // Efek passive senjata
                if (strcmp(player->equipped.passive, "\033[38;2;158;253;209mDetroit smash\e[0m") == 0 && rand() % 100 < 30) 
                {
                    int full_punch = rand() % 700;
                    enemy_hp -= full_punch;
                    if (enemy_hp < 0) enemy_hp = 0;
                    len += snprintf(full_msg + len, sizeof(full_msg) - len,
                        "\033[38;2;158;253;209mデトロイトスマッシュ\e[0m Extra %d damage. Enemy HP now: %d\n", full_punch, enemy_hp);
                    display_hp_bar(enemy_hp, enemy_max_hp, hpbar, sizeof(hpbar));
                    len += snprintf(full_msg + len, sizeof(full_msg) - len, "%s", hpbar);
                } 
                else if (strcmp(player->equipped.passive, "\033[38;5;222mAcid Sweat\e[0m") == 0 && rand() % 100 < 25) 
                {
                    int acid_sweat = rand() % 500;
                    enemy_hp -= acid_sweat;
                    if (enemy_hp < 0) enemy_hp = 0;
                    len += snprintf(full_msg + len, sizeof(full_msg) - len,
                        "\033[38;5;222mドゥアール!\e[0m Extra %d damage. Enemy HP now: %d\n", acid_sweat, enemy_hp);
                    display_hp_bar(enemy_hp, enemy_max_hp, hpbar, sizeof(hpbar));
                    len += snprintf(full_msg + len, sizeof(full_msg) - len, "%s", hpbar);
                } 
                else if (strcmp(player->equipped.passive, "\033[38;5;153mFreeze\e[0m") == 0 && rand() % 100 < 20) 
                {
                    player->frozen = 1;
                    len += snprintf(full_msg + len, sizeof(full_msg) - len,
                        "\033[38;5;153mFreeze effect! Enemy skips next turn!\n\e[0m");
                }
    
                if (enemy_hp == 0) 
                {
                    int gold_reward = rand() % 75 + (enemy_max_hp / 2);
                    player->gold += gold_reward;   
                    player->enemies_defeated++;
                    len += snprintf(full_msg + len, sizeof(full_msg) - len,
                        "Enemy defeated! You gained \033[38;2;243;177;68m%d gold.\e[0m\n", gold_reward);
    
                    enemy_hp = rand() % 100 + enemy_max_hp;
                    enemy_max_hp = enemy_hp;
                    len += snprintf(full_msg + len, sizeof(full_msg) - len,
                        "New Enemy appeared! HP: %d\n", enemy_hp);
                    display_hp_bar(enemy_hp, enemy_max_hp, hpbar, sizeof(hpbar));
                    len += snprintf(full_msg + len, sizeof(full_msg) - len, "%s", hpbar);
                }
    
                // Akhiri dengan prompt supaya client tahu menunggu input
                len += snprintf(full_msg + len, sizeof(full_msg) - len, "Enter command: '\033[38;2;105;210;82mattack\e[0m' or '\033[38;2;226;79;65mexit\e[0m' ");
                send(client_socket, full_msg, len, 0);
            } 
            else 
            {
                snprintf(full_msg, sizeof(full_msg),
                    "Unknown command. Use '\033[38;2;105;210;82mattack\e[0m' or '\033[38;2;226;79;65mexit\e[0m' ");
                send(client_socket, full_msg, strlen(full_msg), 0);
            }
        }
    }

dimana cara kerjanya seperti ini.
1. Server setting hp musuh sebesar 500 - 550 HP secara random.
2. Server mengampikan UI Battle Mode kepada client berupa HP musuh dan perintah kepada client untuk memasukkan input.
3. Jika player memasukkan input berupa `attack`, maka hp musuh berkurang sesuai weapon yang digunakan.
4. Jika player mempunyai weapon dengan passive tertentu, maka passive skill akan bekerja secara random dengan tambahan damage.
6. Jika musuh telah dikalahkan, maka akan muncul musuh baru dengan hp lebih tinggi dari sebelumnya.
7. Jika player salah memasukkan input, maka server akan mengirim pesan bahwa input salah.
8. JIka player ingin keluar dari Battle Mode, maka player dapat mengetik `exit` dan menekan `ENTER`.

**10) `int main()`**

Fungsi `int main()` digunakan sebagai program utamanya. Untuk kodenya seperti ini
    
    int main()
    {
        int new_socket;
        int sfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sfd == -1)
        {
            perror("Socket failed");
            return -1;
        }
    
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        socklen_t addrlen = sizeof(addr);
        
        if(bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        {
            perror("Bind failed");
            return -1;
        }
    
        if (listen(sfd, 1) == -1)
        {
            perror("Listen failed");
            return -1;
        }
    
        printf("The server is up on port %d\n", PORT);
    
        while(1)
        {
            new_socket = accept(sfd, (struct sockaddr *)&addr, &addrlen);
            if (new_socket < 0)
            {
                perror("Accept");
                return -1;
            }
    
            printf("Player has connected to the server.\n");
    
            pid_t pid = fork();
            if(pid == 0)
            {
                close(sfd);
                handle_client(new_socket);
                exit(0);
            }
            else if (pid < 0)
            {
                perror("Fork failed");
            }
            close(new_socket);
        }
    }
    
Untuk cara kerjanya sebagai berikut.
1. Program menjalankan fungsi socket sebagai server secara RPC.
2. Jika dalam prosesnya ada yang tidak berhasil, maka muncul pesan sesuai proses yang gagal tersebut.
3. Jika berhasil, maka muncul pesan `The server is up on port %d` di server.
4. Jika client berhasil connect, maka muncul pesan `Player has connected to the server.`.
5. Setelah itu, fungsi `handle_client()` dijalankan menggunakn pid.
6. Jika client berhasil disconnect, maka muncul pesan `Player has disconnected to the server.`.

### b. Shop.c
Shop.c digunakan sebagai database toko. Untuk atribut yang digunakan menggunakan struct. Untuk kodenya seperti ini

    #include <stdio.h>
    #include <string.h>
    
    typedef struct 
    {
        char name[100];
        int price;
        int damage;
        char passive[50];
    } Weapon;
    
    Weapon weapons[] = 
    {
        {"\033[38;5;203m硬化 (hardening)\e[0m", 400, 200, "None"},
        {"\033[38;5;98m黒影 (dark Shadow)\e[0m", 650, 350, "None"},
        {"\033[38;5;153m半冷半燃\e[0m \033[38;5;209m(Half-cool, half-burn)\e[0m", 750, 450, "\033[38;5;153mFreeze\e[0m"},
        {"\033[38;5;222m爆破 (Exploison)\e[0m", 850, 475, "\033[38;5;222mAcid Sweat\e[0m"},
        {"\033[38;2;158;253;209mワン・フォー・オール (One For All)\e[0m", 1000, 500, "\033[38;2;158;253;209mDetroit smash\e[0m"}
    };
    
    int total_weapons = 5;
    
    void show_shop() 
    {
        printf("\n=== Weapon Shop ===\n");
        for (int i = 0; i < total_weapons; i++) {
            printf("%d. %s - \033[38;2;243;177;68mPrice: %d Gold\e[0m - Damage: %d - Passive: %s\n",
                   i+1, weapons[i].name, weapons[i].price, weapons[i].damage, weapons[i].passive);
        }
        printf("===================\n");
    }
    
    Weapon buy_weapon(int choice) 
    {
        if (choice < 1 || choice > total_weapons) 
        {
            Weapon empty = {"", 0, 0, ""};
            return empty;
        }
        return weapons[choice-1];
    }

dimana utnuk fungsi buy_weapon akan bekerja ketika player sedang membeli weapon.

### player.c
Player.c bekerja sebagai client. Untuk kodenya seperti ini

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
    
    
            // meminta input player
            char input[400] = {0};
            fgets(input, sizeof(input), stdin);
            send(sock, input, strlen(input), 0);
            if (input[0] == 'x') break;
        }
    
    
        close(sock);
        return 0;
    }
    
dimana cara kerjanya sebagai berikut.
1. Client mengguakan sock untuk bekerja secara RPC.
2. Client connect ke server, jika berhasil, maka tampilan UI dari server langsung muncul.
3. Jika player telah exit menggunakan `x`, maka program akan berhenti.


### d. Output
1) Output dari server selama proses berlangsung seperti ini

![Image](https://github.com/user-attachments/assets/8f4bc83c-bfd3-4290-8135-dbd5b313d7bd)

2) Untuk tampilan main menu seperti ini

![Image](https://github.com/user-attachments/assets/5893dfe9-cf7d-4610-bdf3-efecfc83bce7)

3) Untuk show player stats seperti ini

![Image](https://github.com/user-attachments/assets/35371760-f528-4a6c-b486-93166e22ad1c)

4) Untuk inventory seperti ini

![Image](https://github.com/user-attachments/assets/76691d25-bd35-4405-9a1a-26bc150f1200)

5) Untuk tampilan shop seperti ini

![Image](https://github.com/user-attachments/assets/27887e07-af3d-43ca-93cc-64b11e8330fb)

6) Untuk tampilan equip weapon seperti ini

![Image](https://github.com/user-attachments/assets/bb354eab-aefc-4257-8215-2365b39d5dde)

7) Untuk tampilan battle mode (awal) seperti ini

![Image](https://github.com/user-attachments/assets/d36d3476-5247-4755-b6e5-b97a9ec4019a)

8) Jika hp musuh habis, maka tempilannya seperti ini

![Image](https://github.com/user-attachments/assets/296c6126-660b-4dbd-b700-1a2d91014855)

9) Jika passive aktif, muncul tampilan seperti ini

![Image](https://github.com/user-attachments/assets/be829f64-94a7-43de-b3f5-59b08a5ed161)

10) Untuk about (tambahan) muncul seperti ini

![Image](https://github.com/user-attachments/assets/e556071c-e93a-4d8f-a99c-82c61de1f501)

<h2 id="soal4">Soal4</h2>
