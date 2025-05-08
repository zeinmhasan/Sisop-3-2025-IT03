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

##  Contoh Format File `.txt`
FFDB0043000604040504040506050505060706060708090E09090...


---

##  Kelebihan

- Komunikasi client-server efisien via TCP.
- Data aman karena dikirim dalam bentuk terenkripsi.
- Server berjalan sebagai daemon, cocok untuk layanan background.
- Logging sistematis untuk debugging dan audit.

---

##  Catatan Keamanan

- Protokol tidak mengenkripsi komunikasi (bisa pakai TLS di masa depan).
- Validasi nama file masih minimal (rentan path traversal jika tidak dibatasi).

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

<h2 id="soal1">Soal1</h2>

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

### ⚙ Mode Operasi

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

### 🔗 Shared Memory

- Mengakses shared memory yang telah dibuat oleh `delivery_agent.c`.
- Menggunakan `ftok("delivery_agent.c", 65)` agar key konsisten antara kedua program.

---

## 📡 Interaksi Proses

- Komunikasi antar proses dilakukan melalui **Shared Memory IPC**.
- Kedua program (`delivery_agent` dan `dispatcher`) membaca dan menulis ke area memori yang sama.

---

## 📝 Format Log `delivery.log`

```text
[08/05/2025 12:34:56] [AGENT A] Express package delivered to Alice in Surabaya
[08/05/2025 12:35:00] [zain] Reguler package delivered to Bob in Malang
```
