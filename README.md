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

- `#define MAX_BUFFER 4096` → Ukuran buffer komunikasi.
- Fungsi `reverse_string()` → Membalik string.
- Fungsi `read_file()` → Membaca isi file `.txt` untuk dikirim.
- Fungsi `write_file()` → Menyimpan file JPEG hasil download.

### Fungsi `upload_file()`

- Membaca file `.txt` terenkripsi.
- Melakukan reverse string.
- Mengirim perintah `UPLOAD filename\n<reversed_hex>` ke server.
- Menerima respons `OK:<nama_output.jpeg>`.

### Fungsi `download_file()`

- Mengirim perintah `DOWNLOAD <nama_file>\n`.
- Menerima stream JPEG dari server.
- Menyimpannya di direktori `client/`.

### Fungsi `main()`

- Menampilkan menu:
  - Upload File
  - Download File
  - Keluar
- Menghubungkan ke server menggunakan TCP.
- Meneruskan input ke fungsi `upload_file()` / `download_file()`.

---

##  Penjelasan `image_server.c`

### Fungsi `daemonize()`

- Membuat server berjalan di background.
- Menutup terminal (STDIN, STDOUT, STDERR).
- Menulis log ke syslog.

### Fungsi `log_action()` dan `create_directory()`

- `log_action()` → Mencatat aktivitas ke `server/server.log`.
- `create_directory()` → Membuat direktori jika belum ada.

### Fungsi `decrypt_data()`

- Melakukan:
  - Reverse string.
  - Decode setiap 2 karakter hex menjadi byte asli (menggunakan `sscanf()`).
- Hasil: string biner JPEG.

### Fungsi `handle_client()`

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
