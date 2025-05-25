# 🗂️ SimpleFS – Dosya Sistemi Simülatörü

Bu proje, C dili kullanılarak geliştirilmiş bir dosya sistemi simülatörüdür. Gerçek dosya sistemine müdahale etmek yerine, tüm işlemler `disk.sim` adlı sanal bir disk üzerinde sistem çağrıları (system calls) kullanılarak gerçekleştirilmiştir.

## 🚀 Amaç

- Dosya sistemlerinin temel işleyişini anlamak
- Sistem çağrıları (`open`, `read`, `write`, `lseek`, `ftruncate`, `unlink`, vs.) ile pratik kazanmak
- Metadata ve veri yönetimi yapısı oluşturarak dosya sistemi simülasyonu yapmak

---

## 🔧 Derleme ve Çalıştırma

### Linux/Ubuntu
```bash
make
./simplefs
```

### Windows (MinGW)
```bash
mingw32-make
simplefs.exe
```

> Not: `make` kullanımı için sisteminizde `make` aracı yüklü olmalıdır. Windows için `MinGW` önerilir.

---

## 📂 Proje Yapısı

```bash
.
├── main.c              # Menü ve kullanıcı arayüzü
├── fs.c                # Dosya sistemi işlevleri (fs_create, fs_write, vs.)
├── fs.h                # Header dosyası (fonksiyon tanımları)
├── Makefile            # Derleme betiği
├── disk.sim            # 1MB boyutunda sanal disk dosyası
├── fs_operations.log   # İşlem geçmişi log dosyası
└── README.md           # Bu dökümantasyon dosyası
```

---

## 📦 Desteklenen Komutlar

| Komut | Açıklama |
|-------|----------|
| `fs_create` | Yeni dosya oluşturur |
| `fs_delete` | Dosyayı siler |
| `fs_write` | Dosyaya veri yazar |
| `fs_read` | Dosyadan veri okur |
| `fs_ls` | Tüm dosyaları listeler |
| `fs_format` | Disk formatlar, her şeyi sıfırlar |
| `fs_rename` | Dosyanın adını değiştirir |
| `fs_exists` | Dosya var mı kontrol eder |
| `fs_size` | Dosya boyutunu döner |
| `fs_append` | Dosyanın sonuna veri ekler |
| `fs_truncate` | Dosyayı keser veya küçültür |
| `fs_copy` | Dosyayı başka bir dosyaya kopyalar |
| `fs_mv` | Dosyayı taşır (ileri sürümde desteklenebilir) |
| `fs_defragment` | Disk üzerindeki boşlukları birleştirir |
| `fs_check_integrity` | Tutarlılık kontrolü yapar |
| `fs_backup` | Diskin yedeğini alır |
| `fs_restore` | Yedeği geri yükler |
| `fs_cat` | Dosyanın içeriğini gösterir |
| `fs_diff` | İki dosyayı karşılaştırır |
| `fs_log` | Tüm işlemleri loglar |

---

## 🖥️ Kullanıcı Arayüzü (Menü)

```
=== SimpleFS Menu ===
 1. Create file
 2. Delete file
 3. Write to file
 4. Read from file
 5. List files
 6. Format disk
 7. Rename file
 8. Check file exists
 9. Get file size
10. Append to file
11. Truncate file
12. Copy file
13. Move file
14. Defragment disk
15. Check integrity
16. Backup disk
17. Restore backup
18. Show file contents (cat)
19. Compare two files (diff)
20. Show operation log
21. Exit
```

---

## 🧪 Test Senaryoları

- Aynı ada sahip birden fazla dosya oluşturulamaz.
- Maksimum dosya sayısı kontrol edilir.
- Disk doluyken veri yazımı engellenir.
- Format sonrası tüm dosyalar temizlenmiş olur.

---

## 🛠️ Kullanılan Sistem Çağrıları

- `open`, `read`, `write`, `lseek`, `close`, `ftruncate`, `unlink`, `calloc`, `stat` vb.

---
