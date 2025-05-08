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
