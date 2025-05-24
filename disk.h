#ifndef DISK_H
#define DISK_H

#include <stdint.h>   // uint32_t gibi sabit boyutlu tamsayılar
#include <time.h>     // time_t zaman türü

#define DISK_NAME       "disk.sim"            // Sanal disk dosya adı
#define DISK_SIZE       (1024 * 1024)         // 1 MB = 1024 * 1024 byte
#define METADATA_SIZE   (4 * 1024)            // 4 KB metadata alanı
#define BLOCK_SIZE      512                   // Sabit blok boyutu (byte)

typedef struct {
    char     name[32];        // Dosya ismi (maks. 31 karakter + null)
    uint32_t size;            // Dosya boyutu (byte)
    uint32_t start_block;     // Veri bloğundaki başlangıç indeksi
    time_t   created;         // Oluşturulma zamanı (Unix zaman damgası)
} FileEntry;

#define MAX_FILES ((METADATA_SIZE - sizeof(uint32_t)) / sizeof(FileEntry))
// Dosya sayısı = metadata'dan kalan alan / bir dosya kaydının boyutu

typedef struct {
    uint32_t    file_count;               // Toplam dosya sayısı
    FileEntry   entries[MAX_FILES];       // Dosya kayıtları
} DiskMetadata;

extern DiskMetadata metadata;  // Diğer .c dosyalarında kullanılacak global metadata

// Fonksiyon prototipleri
int  disk_open(void);                                      // disk.sim dosyasını aç
int  disk_read_metadata(void);                             // metadata'yı oku
int  disk_write_metadata(void);                            // metadata'yı diske yaz
int  disk_read_block(uint32_t block_index, void *buffer);  // belirli bloktan veri oku
int  disk_write_block(uint32_t block_index, const void *buffer); // belirli bloğa veri yaz

#endif // DISK_H
