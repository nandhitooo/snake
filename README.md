# Snake (raylib GUI)

Game Snake untuk desktop dengan tampilan grafis penuh memakai [raylib](https://www.raylib.com/). Kode C++17 dipisah menurut peran, tanpa aset eksternal: tidak ada gambar, font, atau suara.

## Fitur

- Home menu dengan tombol **PLAY**, **FULLSCREEN**, dan **QUIT**.
- UI 16:9 (960×540 px), dengan arena 30×20 sel (600×400 px), HUD, dan bar kontrol.
- Tembus tepi arena (wrap-around) dengan efek ghost saat pindah sisi.
- Boost tahan SPACE: tick lebih cepat, tetapi ekor memendek tiap 3 tick boost.
- Obstacle muncul berkala dengan peringatan berkedip sebelum aktif, lalu hilang setelah beberapa saat.
- **Special food** dengan 7 efek acak yang menguntungkan atau merugikan.
- Mode fullscreen (F11 atau tombol menu) dengan letterbox scaling, tetap tajam di resolusi apa pun.
- Kontrol GUI: tombol arah, boost, restart, dan quit bisa diklik.
- High score per sesi dan overlay game over.

## Kebutuhan

- Compiler C++17 (g++ / clang++)
- raylib (diuji dengan raylib 6.2)
- `pkg-config` (Linux/macOS)

### Instalasi raylib

```bash
# Debian/Ubuntu
sudo apt install libraylib-dev

# Arch
sudo pacman -S raylib

# macOS
brew install raylib
```

## Build & Jalankan

```bash
mkdir -p build
g++ -std=c++17 -Wall -Wextra -Isrc src/main.cpp src/game/game.cpp src/ui/input.cpp src/ui/draw.cpp src/render/renderer.cpp -o build/snake_gui $(pkg-config --cflags --libs raylib)
./build/snake_gui
```

Windows (MSYS2/MinGW), setelah `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-raylib`:

```bash
mkdir -p build
g++ -std=c++17 -Isrc src/main.cpp src/game/game.cpp src/ui/input.cpp src/ui/draw.cpp src/render/renderer.cpp -o build/snake_gui.exe $(pkg-config --cflags --libs raylib)
```

Salin `libraylib.dll` (dan DLL runtime MinGW bila ada) ke `build/` jika game tidak mau jalan.

### Build `.exe` otomatis (GitHub Actions)

Workflow `.github/workflows/windows.yml` membangun `snake_gui.exe` di runner Windows pada setiap push ke `main`, dan bisa dijalankan manual dari tab **Actions** → **Build Windows EXE** → **Run workflow**. Unduh `snake_gui-windows-x64.zip` di bagian **Artifacts**, ekstrak, lalu jalankan `snake_gui.exe`. Workflow membundel DLL MinGW yang dibutuhkan.

## Menu Utama

Game dibuka di home menu. **PLAY** / `ENTER` memulai ronde baru, **FULLSCREEN** / `F11` mengganti mode layar, dan **QUIT** / `ESC` keluar. Saat sedang bermain, `ESC` kembali ke menu (skor berjalan disimpan sebagai high score sesi), lalu **PLAY** memulai ronde baru.

## Kontrol

| Aksi | Keyboard | GUI |
|---|---|---|
| Belok | `WASD` / panah | Tombol `<` `^` `v` `>` |
| Boost | tahan `SPACE` | tahan tombol `BOOST` |
| Restart | `R` (saat game over) | tombol `RESTART` |
| Fullscreen | `F11` | tombol `FULLSCREEN` di menu |
| Kembali ke menu | `ESC` saat bermain | — |
| Keluar | `ESC` di menu utama | tombol `QUIT` |

Balik arah 180° diabaikan, dan hanya satu belokan yang dibuffer per tick.

## Aturan Main

- Makan food: **+10 skor** dan **+1 panjang**.
- Skor bertambah **+1 per detik** selama bertahan hidup.
- Menabrak badan sendiri atau obstacle solid = game over.
- Tepi arena tidak mematikan: kepala muncul kembali di sisi seberang.
- Boost memakai interval tick 0.08 s (normal 0.16 s) dan memotong 1 segmen ekor setiap 3 tick boost, dengan panjang minimum 3.
- Gelombang obstacle muncul tiap 8 detik berisi 1–3 sel, berkedip sebagai peringatan selama 1 detik, maksimal 15 obstacle (yang tertua dihapus), dan tidak pernah muncul di badan, food, atau dalam radius 4 sel dari kepala.
- Setiap obstacle solid bertahan 12 detik, memudar pada 1,5 detik terakhir, lalu hilang otomatis. Efek `CLEANSE` tetap bisa menghapus semuanya sekaligus.

## Special Food

Muncul dengan peluang 30% setiap kali food normal baru muncul (maksimal satu di arena), berbentuk lingkaran ungu bercincin emas. Efeknya dipilih acak dan baru diketahui setelah dimakan. Selain efek, special food tetap memberi +10 skor dan +1 panjang.

| Efek | Jenis | Hasil |
|---|---|---|
| `BONUS +50` | Menguntungkan | Skor +50 langsung |
| `SLOW` | Menguntungkan | Interval tick ×1.45 selama 5 detik |
| `TRIM -2` | Menguntungkan | Potong 2 segmen ekor (minimum 3) |
| `CLEANSE` | Menguntungkan | Hapus semua obstacle dan peringatan |
| `HASTE` | Merugikan | Interval tick ×0.72 selama 5 detik |
| `GROW +3` | Merugikan | Tambah 3 segmen tubuh |
| `CURSE +3` | Merugikan | Munculkan 3 obstacle solid acak, minimal 4 sel dari kepala |

Efek bertimer ditampilkan beserta hitungan mundur di baris `FX:` pada HUD.

## Struktur Proyek

```
.
├── .github/workflows/windows.yml   # build .exe otomatis
├── src/
│   ├── game/
│   │   ├── config.hpp               # ukuran arena, aturan, dan warna
│   │   ├── game.hpp                 # state dan antarmuka game
│   │   └── game.cpp                 # aturan dan simulasi game
│   ├── render/
│   │   ├── color.hpp                # helper warna untuk renderer dan UI
│   │   └── renderer.cpp             # gambar arena, HUD, dan overlay
│   ├── ui/
│   │   ├── draw.cpp                 # menu dan tombol
│   │   ├── input.cpp                # input keyboard dan mouse
│   │   └── layout.hpp               # posisi tombol UI
│   └── main.cpp                     # window dan game loop
├── README.md
└── .gitignore
```

Hasil build lokal masuk `build/`; artifact GitHub Actions masuk `dist/`. Keduanya diabaikan Git.

## Lisensi

Belum ada lisensi. Tambahkan file `LICENSE` (misalnya MIT) sebelum mempublikasikan repo jika ingin memberi izin penggunaan.
