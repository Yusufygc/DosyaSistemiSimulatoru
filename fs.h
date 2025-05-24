#ifndef FS_H
#define FS_H

#include <stdint.h>     // uint32_t gibi sabit boyutlu tamsayılar
#include <time.h>       // time_t türü
#include <sys/types.h>  // ssize_t türü
#include "disk.h"       // Disk yapısı ve metadata

// Disk formatla (boş metadata ve veri alanı oluştur)
int fs_format(void);

// Yeni bir dosya oluştur
int fs_create(const char *filename);

// Dosyayı sil
int fs_delete(const char *filename);

// Dosyaya veri yaz (üzerine yazar)
ssize_t fs_write(const char *filename, const void *data, size_t size);

// Dosyadan veri oku (belirli offset’ten)
ssize_t fs_read(const char *filename, uint32_t offset, size_t size, void *buffer);

// fs.h
void handle_read_file();
ssize_t fs_read_all(const char *filename, void *buffer);

// Dosya listesini ve boyutlarını yazdır
int fs_ls(void);

// Dosya adını değiştir
int fs_rename(const char *old_name, const char *new_name);

// Dosya var mı kontrolü (1: var, 0: yok)
int fs_exists(const char *filename);

// Dosya boyutunu al
int fs_size(const char *filename, uint32_t *size_out);

// Dosya sonuna veri ekle
ssize_t fs_append(const char *filename, const void *data, size_t size);

// Dosyanın boyutunu kes veya uzat (truncate gibi)
int fs_truncate(const char *filename, uint32_t new_size);

// Dosyayı başka adla kopyala
int fs_copy(const char *src_filename, const char *dest_filename);

// Dosyayı taşı (yeniden adlandırma ile benzer)
int fs_mv(const char *old_path, const char *new_path);

// Diskteki parçalı blokları birleştir (defragmentation)
int fs_defragment(void);

// Dosya sistemi bütünlüğünü kontrol et (metadata + veri blokları)
int fs_check_integrity(void);

// Disk dosyasının yedeğini al
int fs_backup(const char *backup_filename);

// Yedekten disk dosyasını geri yükle
int fs_restore(const char *backup_filename);

// Dosyanın içeriğini stdout’a yaz
int fs_cat(const char *filename);

// İki dosyayı karşılaştır (byte düzeyinde)
int fs_diff(const char *file1, const char *file2);

// İşlem günlüğüne log yaz (örn. "create", "delete" vs.)
int fs_log(const char *operation, const char *filename);

#endif // FS_H
