#define _POSIX_C_SOURCE 200809L

#include "fs.h"
#include "disk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>    // ← stat için
#ifdef _WIN32
    #include <io.h>
    #define ftruncate _chsize
#else
    #include <unistd.h>  // Bu satır kesinlikle burada olmalı
#endif


#define LOG_FILENAME  "fs_operations.log"

// Format (initialize) the disk
int fs_format(void) {
    int fd = open(DISK_NAME, O_CREAT | O_TRUNC | O_RDWR, 0666);
    if (fd < 0) { perror("fs_format: open"); return -1; }
    if (ftruncate(fd, DISK_SIZE) < 0) { perror("fs_format: ftruncate"); close(fd); return -1; }

    void *zero = calloc(1, METADATA_SIZE);
    if (!zero) { perror("fs_format: calloc"); close(fd); return -1; }
    if (lseek(fd, 0, SEEK_SET) < 0 || write(fd, zero, METADATA_SIZE) != METADATA_SIZE) {
        perror("fs_format: write metadata"); free(zero); close(fd); return -1;
    }
    free(zero);

    if (lseek(fd, DISK_SIZE - 1, SEEK_SET) < 0 || write(fd, "\0", 1) != 1) {
        perror("fs_format: allocate data"); close(fd); return -1;
    }
    close(fd);
    return 0;
}

// Create a new file in metadata
int fs_create(const char *filename) {
    if (!filename || !*filename || strlen(filename) >= sizeof(metadata.entries[0].name)) {
        fprintf(stderr, "fs_create: invalid name\n");
        return -1;
    }
    if (disk_read_metadata() < 0) { fprintf(stderr, "fs_create: read_meta\n"); return -1; }

    for (uint32_t i = 0; i < metadata.file_count; ++i) {
        if (strcmp(metadata.entries[i].name, filename) == 0) {
            fprintf(stderr, "fs_create: '%s' exists\n", filename);
            return -1;
        }
    }
    if (metadata.file_count >= MAX_FILES) {
        fprintf(stderr, "fs_create: max files reached\n");
        return -1;
    }

    FileEntry *e = &metadata.entries[metadata.file_count];
    memset(e, 0, sizeof(*e));
    strncpy(e->name, filename, sizeof(e->name)-1);
    e->size = 0;
    e->start_block = 0;
    e->created = time(NULL);
    metadata.file_count++;

    if (disk_write_metadata() < 0) { fprintf(stderr, "fs_create: write_meta\n"); return -1; }
    printf("fs_create: '%s' created\n", filename);
    return 0;
}

// Delete a file from metadata
int fs_delete(const char *filename) {
    if (disk_read_metadata() < 0) {
        fprintf(stderr, "fs_delete: read_meta\n");
        return -1;
    }
    int idx = -1;
    for (uint32_t i = 0; i < metadata.file_count; ++i) {
        if (strcmp(metadata.entries[i].name, filename) == 0) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        fprintf(stderr, "fs_delete: '%s' not found\n", filename);
        return -1;
    }
    // shift entries
    for (uint32_t i = idx; i + 1 < metadata.file_count; ++i) {
        metadata.entries[i] = metadata.entries[i + 1];
    }
    metadata.file_count--;
    memset(&metadata.entries[metadata.file_count], 0, sizeof(FileEntry));

    if (disk_write_metadata() < 0) {
        fprintf(stderr, "fs_delete: write_meta\n");
        return -1;
    }
    printf("fs_delete: '%s' deleted\n", filename);
    return 0;
}

// Overwrite data into a file
ssize_t fs_write(const char *filename, const void *data, size_t size) {
    if (size == 0) return 0;
    if (disk_read_metadata() < 0) { fprintf(stderr, "fs_write: read_meta\n"); return -1; }

    FileEntry *e = NULL;
    for (uint32_t i = 0; i < metadata.file_count; ++i) {
        if (strcmp(metadata.entries[i].name, filename) == 0) {
            e = &metadata.entries[i];
            break;
        }
    }
    if (!e) {
        fprintf(stderr, "fs_write: '%s' not found\n", filename);
        return -1;
    }

    int fd = open(DISK_NAME, O_RDWR);
    if (fd < 0) { perror("fs_write: open disk"); return -1; }

    off_t abs_off = METADATA_SIZE + (off_t)e->start_block * BLOCK_SIZE;
    if (lseek(fd, abs_off, SEEK_SET) < 0) { perror("fs_write: lseek"); close(fd); return -1; }
    ssize_t written = write(fd, data, size);
    if (written < 0) { perror("fs_write: write"); close(fd); return -1; }
    close(fd);

    if ((uint32_t)written > e->size) {
        e->size = (uint32_t)written;
        if (disk_write_metadata() < 0) {
            fprintf(stderr, "fs_write: write_meta\n");
            return -1;
        }
    }
    printf("fs_write: '%s' -> %zd bytes\n", filename, written);
    return written;
}
//************************************************************************************************ */
// Read data from a file
ssize_t fs_read(const char *filename, uint32_t offset, size_t size, void *buffer) {
    if (size == 0) return 0;
    if (disk_read_metadata() < 0) { fprintf(stderr, "fs_read: read_meta\n"); return -1; }

    FileEntry *e = NULL;
    for (uint32_t i = 0; i < metadata.file_count; ++i) {
        if (strcmp(metadata.entries[i].name, filename) == 0) {
            e = &metadata.entries[i];
            break;
        }
    }
    if (!e) {
        fprintf(stderr, "fs_read: '%s' not found\n", filename);
        return -1;
    }
    if (offset >= e->size) {
        fprintf(stderr, "fs_read: offset beyond size\n");
        return -1;
    }

    int fd = open(DISK_NAME, O_RDONLY);
    if (fd < 0) { perror("fs_read: open disk"); return -1; }

    off_t abs_off = METADATA_SIZE + (off_t)e->start_block * BLOCK_SIZE + offset;
    if (lseek(fd, abs_off, SEEK_SET) < 0) { perror("fs_read: lseek"); close(fd); return -1; }
    memset(buffer, 0, size);
    ssize_t rd = read(fd, buffer, size);
    if (rd < 0) { perror("fs_read: read"); close(fd); return -1; }
    close(fd);

    printf("fs_read: '%s' <- %zd bytes\n", filename, rd);
    return rd;
}
// Tüm dosyayı okuyup buffer'a yazar
ssize_t fs_read_all(const char *filename, void *buffer) {
    if (disk_read_metadata() < 0) {
        fprintf(stderr, "fs_read_all: read_meta\n");
        return -1;
    }

    FileEntry *e = NULL;
    for (uint32_t i = 0; i < metadata.file_count; ++i) {
        if (strcmp(metadata.entries[i].name, filename) == 0) {
            e = &metadata.entries[i];
            break;
        }
    }
    if (!e) {
        fprintf(stderr, "fs_read_all: '%s' not found\n", filename);
        return -1;
    }

    return fs_read(filename, 0, e->size, buffer);  // Offset = 0, size = tüm dosya
}
//***************************************************************************************** */
// Copy stub
// fs.c — implement fs_copy

#define COPY_CHUNK_SIZE BLOCK_SIZE

int fs_copy(const char *src_filename, const char *dest_filename) {
    // 1) kaynak dosya var mı?
    if (!fs_exists(src_filename)) {
        fprintf(stderr, "fs_copy: source '%s' not found\n", src_filename);
        return -1;
    }
    // 2) hedef zaten varsa hata
    if (fs_exists(dest_filename)) {
        fprintf(stderr, "fs_copy: destination '%s' already exists\n", dest_filename);
        return -1;
    }
    // 3) yeni dosya oluştur
    if (fs_create(dest_filename) < 0) {
        return -1;
    }
    // 4) kaynak boyutunu al
    uint32_t total_size;
    if (fs_size(src_filename, &total_size) < 0) {
        return -1;
    }

    // 5) blok blok kopyala
    uint32_t offset = 0;
    ssize_t n;
    char *buffer = malloc(COPY_CHUNK_SIZE);
    if (!buffer) {
        perror("fs_copy: malloc");
        return -1;
    }

    // İlk blokta fs_write, kalanlarda fs_append
    int first = 1;
    while (offset < total_size) {
        size_t to_read = (total_size - offset > COPY_CHUNK_SIZE)
                         ? COPY_CHUNK_SIZE
                         : (total_size - offset);
        n = fs_read(src_filename, offset, to_read, buffer);
        if (n < 0) {
            free(buffer);
            return -1;
        }
        if (first) {
            if (fs_write(dest_filename, buffer, (size_t)n) < 0) {
                free(buffer);
                return -1;
            }
            first = 0;
        } else {
            if (fs_append(dest_filename, buffer, (size_t)n) < 0) {
                free(buffer);
                return -1;
            }
        }
        offset += (uint32_t)n;
    }

    free(buffer);
    printf("fs_copy: '%s' -> '%s' complete (%u bytes)\n",
           src_filename, dest_filename, total_size);
    return 0;
}


// Move stub
//  After your fs_copy implementation, add:

int fs_mv(const char *old_path, const char *new_path) {
    // 1) Kaynak dosya var mı?
    if (!fs_exists(old_path)) {
        fprintf(stderr, "fs_mv: source '%s' not found\n", old_path);
        return -1;
    }
    // 2) Hedef zaten varsa hata
    if (fs_exists(new_path)) {
        fprintf(stderr, "fs_mv: destination '%s' already exists\n", new_path);
        return -1;
    }
    // 3) Kopyalama
    if (fs_copy(old_path, new_path) < 0) {
        return -1;
    }
    // 4) Orijinali sil
    if (fs_delete(old_path) < 0) {
        fprintf(stderr, "fs_mv: copied but failed to delete '%s'\n", old_path);
        return -1;
    }
    printf("fs_mv: '%s' moved to '%s'\n", old_path, new_path);
    return 0;
}

// Remaining stubs...
int fs_ls(void) {
    if (disk_read_metadata() < 0) {
        fprintf(stderr, "fs_ls: metadata okunamadı\n");
        return -1;
    }
    printf("=== Files on disk ===\n");
    for (uint32_t i = 0; i < metadata.file_count; ++i) {
        FileEntry *e = &metadata.entries[i];
        printf("%2u: %-32s  %10u bytes\n",
               i+1, e->name, e->size);
    }
    if (metadata.file_count == 0) {
        printf("(no files)\n");
    }
    return 0;
}

// 2) Rename a file in metadata
int fs_rename(const char *old_name, const char *new_name) {
    if (!old_name || !new_name || strlen(new_name) >= sizeof(metadata.entries[0].name)) {
        fprintf(stderr, "fs_rename: geçersiz isim\n");
        return -1;
    }
    if (disk_read_metadata() < 0) {
        fprintf(stderr, "fs_rename: metadata okunamadı\n");
        return -1;
    }
    // check new_name not already used
    for (uint32_t i = 0; i < metadata.file_count; ++i) {
        if (strcmp(metadata.entries[i].name, new_name) == 0) {
            fprintf(stderr, "fs_rename: '%s' zaten mevcut\n", new_name);
            return -1;
        }
    }
    // find old_name
    for (uint32_t i = 0; i < metadata.file_count; ++i) {
        if (strcmp(metadata.entries[i].name, old_name) == 0) {
            // rename
            memset(metadata.entries[i].name, 0, sizeof(metadata.entries[i].name));
            strncpy(metadata.entries[i].name, new_name, sizeof(metadata.entries[i].name)-1);
            if (disk_write_metadata() < 0) {
                fprintf(stderr, "fs_rename: metadata yazılamadı\n");
                return -1;
            }
            printf("fs_rename: '%s' -> '%s'\n", old_name, new_name);
            return 0;
        }
    }
    fprintf(stderr, "fs_rename: '%s' bulunamadı\n", old_name);
    return -1;
}

// 3) Check if a file exists
int fs_exists(const char *filename) {
    if (!filename) return 0;
    if (disk_read_metadata() < 0) {
        fprintf(stderr, "fs_exists: metadata okunamadı\n");
        return 0;
    }
    for (uint32_t i = 0; i < metadata.file_count; ++i) {
        if (strcmp(metadata.entries[i].name, filename) == 0) {
            return 1;
        }
    }
    return 0;
}
// fs.c içinde uygun yere ekleyin:

// 4) Get file size from metadata
int fs_size(const char *filename, uint32_t *size_out) {
    if (!filename || !size_out) {
        fprintf(stderr, "fs_size: invalid arguments\n");
        return -1;
    }
    // Eğer log dosyası isteniyorsa host FS'ten oku
    if (strcmp(filename, LOG_FILENAME) == 0) {
        struct stat st;
        if (stat(LOG_FILENAME, &st) < 0) {
            perror("fs_size: stat log file");
            return -1;
        }
        *size_out = (uint32_t)st.st_size;
        return 0;
    }
    // Aksi halde virtual FS metadata’dan oku
    if (disk_read_metadata() < 0) {
        fprintf(stderr, "fs_size: metadata okunamadı\n");
        return -1;
    }
    for (uint32_t i = 0; i < metadata.file_count; ++i) {
        if (strcmp(metadata.entries[i].name, filename) == 0) {
            *size_out = metadata.entries[i].size;
            return 0;
        }
    }
    fprintf(stderr, "fs_size: '%s' not found\n", filename);
    return -1;
}

// 5) Append data to end of file (preserve existing content)
ssize_t fs_append(const char *filename, const void *data, size_t size) {
    if (!filename || !data || size == 0) {
        fprintf(stderr, "fs_append: invalid arguments\n");
        return -1;
    }
    if (disk_read_metadata() < 0) {
        fprintf(stderr, "fs_append: metadata okunamadı\n");
        return -1;
    }
    FileEntry *e = NULL;
    for (uint32_t i = 0; i < metadata.file_count; ++i) {
        if (strcmp(metadata.entries[i].name, filename) == 0) {
            e = &metadata.entries[i];
            break;
        }
    }
    if (!e) {
        fprintf(stderr, "fs_append: '%s' bulunamadı\n", filename);
        return -1;
    }

    int fd = open(DISK_NAME, O_RDWR);
    if (fd < 0) {
        perror("fs_append: open disk");
        return -1;
    }
    // Hesap: veri bölgesinin başı + mevcut boyut
    off_t abs_off = METADATA_SIZE + (off_t)e->start_block * BLOCK_SIZE + e->size;
    if (lseek(fd, abs_off, SEEK_SET) < 0) {
        perror("fs_append: lseek");
        close(fd);
        return -1;
    }
    ssize_t written = write(fd, data, size);
    if (written < 0) {
        perror("fs_append: write");
        close(fd);
        return -1;
    }
    close(fd);

    // Metadata boyut güncellemesi
    e->size += (uint32_t)written;
    if (disk_write_metadata() < 0) {
        fprintf(stderr, "fs_append: metadata yazılamadı\n");
        return -1;
    }
    printf("fs_append: '%s' dosyasına %zd byte eklendi\n", filename, written);
    return written;
}

// 6) Truncate (or extend with zeros) a file
int fs_truncate(const char *filename, uint32_t new_size) {
    if (!filename) {
        fprintf(stderr, "fs_truncate: invalid argument\n");
        return -1;
    }
    if (disk_read_metadata() < 0) {
        fprintf(stderr, "fs_truncate: metadata okunamadı\n");
        return -1;
    }
    FileEntry *e = NULL;
    for (uint32_t i = 0; i < metadata.file_count; ++i) {
        if (strcmp(metadata.entries[i].name, filename) == 0) {
            e = &metadata.entries[i];
            break;
        }
    }
    if (!e) {
        fprintf(stderr, "fs_truncate: '%s' bulunamadı\n", filename);
        return -1;
    }

    // Eğer küçültme ise sadece metadata boyutu değişir
    if (new_size <= e->size) {
        e->size = new_size;
    } else {
        // Uzatma: araya sıfır dolduralım
        int fd = open(DISK_NAME, O_RDWR);
        if (fd < 0) {
            perror("fs_truncate: open disk");
            return -1;
        }
        off_t abs_off = METADATA_SIZE + (off_t)e->start_block * BLOCK_SIZE + e->size;
        if (lseek(fd, abs_off, SEEK_SET) < 0) {
            perror("fs_truncate: lseek");
            close(fd);
            return -1;
        }
        // Yeni kısma sıfır yaz
        size_t pad = new_size - e->size;
        void *zeros = calloc(1, pad);
        if (!zeros) {
            perror("fs_truncate: calloc");
            close(fd);
            return -1;
        }
        if (write(fd, zeros, pad) != (ssize_t)pad) {
            perror("fs_truncate: write zeros");
            free(zeros);
            close(fd);
            return -1;
        }
        free(zeros);
        close(fd);
        e->size = new_size;
    }

    // Metadata kaydet
    if (disk_write_metadata() < 0) {
        fprintf(stderr, "fs_truncate: metadata yazılamadı\n");
        return -1;
    }
    printf("fs_truncate: '%s' boyutu %u byte olarak ayarlandı\n", filename, new_size);
    return 0;
}

int fs_defragment(void) {
    if (disk_read_metadata() < 0) {
        fprintf(stderr, "fs_defragment: metadata okunamadı\n");
        return -1;
    }

    // Yeni blok indeksini takip et
    uint32_t next_block = 0;
    char *buffer = malloc(BLOCK_SIZE);
    if (!buffer) { perror("fs_defragment: malloc"); return -1; }

    // Geçici kopya metadata
    DiskMetadata old_meta = metadata;

    for (uint32_t i = 0; i < old_meta.file_count; ++i) {
        FileEntry *e_old = &old_meta.entries[i];
        FileEntry *e_new = &metadata.entries[i];

        uint32_t remaining = e_old->size;
        uint32_t read_offset = 0;
        off_t write_base = METADATA_SIZE + (off_t)next_block * BLOCK_SIZE;

        // Her dosya için start_block güncelle
        e_new->start_block = next_block;

        // Boş blok tanımlandı, bellekte data bölgesini silmemek için sparse skip
        int fd = open(DISK_NAME, O_RDWR);
        if (fd < 0) { perror("fs_defragment: open"); free(buffer); return -1; }

        // Parça parça oku ve yeniden yaz
        while (remaining > 0) {
            uint32_t chunk = remaining < BLOCK_SIZE ? remaining : BLOCK_SIZE;
            // Kaynaktan oku
            if (lseek(fd, METADATA_SIZE + (off_t)e_old->start_block * BLOCK_SIZE + read_offset, SEEK_SET) < 0 ||
                read(fd, buffer, chunk) != (ssize_t)chunk) {
                perror("fs_defragment: read");
                close(fd);
                free(buffer);
                return -1;
            }
            // Yeni konuma yaz
            if (lseek(fd, write_base + read_offset, SEEK_SET) < 0 ||
                write(fd, buffer, chunk) != (ssize_t)chunk) {
                perror("fs_defragment: write");
                close(fd);
                free(buffer);
                return -1;
            }
            read_offset += chunk;
            remaining  -= chunk;
        }
        close(fd);

        // Bir sonraki dosya için blokları atla
        uint32_t blocks_used = (e_old->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
        next_block += blocks_used;
    }

    free(buffer);

    // Yeni metadata’yı diske yaz
    if (disk_write_metadata() < 0) {
        fprintf(stderr, "fs_defragment: metadata yazılamadı\n");
        return -1;
    }
    printf("fs_defragment: tamamlandı, %u blok kullanıldı\n", next_block);
    return 0;
}
int fs_check_integrity(void) {
    if (disk_read_metadata() < 0) {
        fprintf(stderr, "fs_check_integrity: metadata okunamadı\n");
        return -1;
    }

    int fd = open(DISK_NAME, O_RDONLY);
    if (fd < 0) {
        perror("fs_check_integrity: open disk");
        return -1;
    }

    int errors = 0;
    for (uint32_t i = 0; i < metadata.file_count; ++i) {
        FileEntry *e = &metadata.entries[i];
        off_t data_off = METADATA_SIZE + (off_t)e->start_block * BLOCK_SIZE;
        // Try to lseek past end of file region
        off_t end_off = data_off + e->size;
        if (lseek(fd, end_off, SEEK_SET) < 0) {
            fprintf(stderr, "fs_check_integrity: '%s' data region invalid (start_block %u, size %u)\n",
                    e->name, e->start_block, e->size);
            errors++;
        }
    }
    close(fd);

    if (errors) {
        fprintf(stderr, "fs_check_integrity: %d bozuk dosya bulundu\n", errors);
        return -1;
    }
    printf("fs_check_integrity: tüm dosyalar tutarlı\n");
    return 0;
}

// 11) Backup: copy entire disk.sim into backup_filename
int fs_backup(const char *backup_filename) {
    if (!backup_filename) {
        fprintf(stderr, "fs_backup: geçersiz hedef dosya adı\n");
        return -1;
    }

    int src = open(DISK_NAME, O_RDONLY);
    if (src < 0) {
        perror("fs_backup: open disk");
        return -1;
    }
    int dst = open(backup_filename, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (dst < 0) {
        perror("fs_backup: open backup");
        close(src);
        return -1;
    }

    char *buf = malloc(BLOCK_SIZE);
    if (!buf) {
        perror("fs_backup: malloc");
        close(src);
        close(dst);
        return -1;
    }

    ssize_t n;
    while ((n = read(src, buf, BLOCK_SIZE)) > 0) {
        if (write(dst, buf, n) != n) {
            perror("fs_backup: write backup");
            free(buf);
            close(src);
            close(dst);
            return -1;
        }
    }
    if (n < 0) perror("fs_backup: read disk");

    free(buf);
    close(src);
    close(dst);
    printf("fs_backup: disk '%s' dosyasına yedeklendi\n", backup_filename);
    return (n < 0) ? -1 : 0;
}

// 12) Restore: overwrite disk.sim from backup_filename
int fs_restore(const char *backup_filename) {
    if (!backup_filename) {
        fprintf(stderr, "fs_restore: geçersiz kaynak dosya adı\n");
        return -1;
    }

    int src = open(backup_filename, O_RDONLY);
    if (src < 0) {
        perror("fs_restore: open backup");
        return -1;
    }
    int dst = open(DISK_NAME, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (dst < 0) {
        perror("fs_restore: open disk");
        close(src);
        return -1;
    }

    char *buf = malloc(BLOCK_SIZE);
    if (!buf) {
        perror("fs_restore: malloc");
        close(src);
        close(dst);
        return -1;
    }

    ssize_t n;
    while ((n = read(src, buf, BLOCK_SIZE)) > 0) {
        if (write(dst, buf, n) != n) {
            perror("fs_restore: write disk");
            free(buf);
            close(src);
            close(dst);
            return -1;
        }
    }
    if (n < 0) perror("fs_restore: read backup");

    free(buf);
    close(src);
    close(dst);
    printf("fs_restore: '%s' geri yüklendi\n", backup_filename);
    return (n < 0) ? -1 : 0;
}
// 13) Print file contents to stdout
int fs_cat(const char *filename) {
    if (!filename) {
        fprintf(stderr, "fs_cat: invalid filename\n");
        return -1;
    }
    // Eğer log dosyası isteniyorsa host FS'ten oku
    if (strcmp(filename, LOG_FILENAME) == 0) {
        FILE *f = fopen(LOG_FILENAME, "r");
        if (!f) {
            perror("fs_cat: fopen log file");
            return -1;
        }
        printf("=== %s ===\n", LOG_FILENAME);
        int c;
        while ((c = fgetc(f)) != EOF) {
            putchar(c);
        }
        fclose(f);
        return 0;
    }
    // Aksi halde virtual FS içinde çalış
    uint32_t size;
    if (fs_size(filename, &size) < 0) {
        fprintf(stderr, "fs_cat: cannot get size for '%s'\n", filename);
        return -1;
    }
    char *buffer = malloc(size + 1);
    if (!buffer) {
        perror("fs_cat: malloc");
        return -1;
    }
    ssize_t rd = fs_read(filename, 0, size, buffer);
    if (rd < 0) {
        free(buffer);
        return -1;
    }
    buffer[rd] = '\0';
    printf("=== %s contents ===\n%s\n", filename, buffer);
    free(buffer);
    return 0;
}

// 14) Compare two files and print diff-like output
int fs_diff(const char *file1, const char *file2) {
    if (!file1 || !file2) {
        fprintf(stderr, "fs_diff: invalid arguments\n");
        return -1;
    }
    uint32_t sz1, sz2;
    if (fs_size(file1, &sz1) < 0 || fs_size(file2, &sz2) < 0) {
        fprintf(stderr, "fs_diff: cannot get sizes\n");
        return -1;
    }
    uint32_t maxsz = sz1 > sz2 ? sz1 : sz2;
    char *buf1 = malloc(maxsz);
    char *buf2 = malloc(maxsz);
    if (!buf1 || !buf2) {
        perror("fs_diff: malloc");
        free(buf1); free(buf2);
        return -1;
    }
    ssize_t r1 = fs_read(file1, 0, maxsz, buf1);
    ssize_t r2 = fs_read(file2, 0, maxsz, buf2);
    if (r1 < 0 || r2 < 0) {
        free(buf1); free(buf2);
        return -1;
    }
    int diffs = 0;
    uint32_t limit = r1 < r2 ? r1 : r2;
    for (uint32_t i = 0; i < limit; ++i) {
        if (buf1[i] != buf2[i]) {
            printf("Difference at byte %u: '%c' vs '%c'\n", i,
                   buf1[i], buf2[i]);
            diffs++;
        }
    }
    if (r1 != r2) {
        printf("Files have different lengths: %zd vs %zd bytes\n", r1, r2);
        diffs++;
    }
    if (diffs == 0) {
        printf("fs_diff: files are identical\n");
    }
    free(buf1); free(buf2);
    return diffs == 0 ? 0 : 1;
}

// 15) Log operations to a persistent file
int fs_log(const char *operation, const char *filename) {
    FILE *f = fopen(LOG_FILENAME, "a");
    if (!f) {
        perror("fs_log: fopen");
        return -1;
    }
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timestr[64];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", tm);
    if (filename)
        fprintf(f, "[%s] %s('%s')\n", timestr, operation, filename);
    else
        fprintf(f, "[%s] %s\n", timestr, operation);
    fclose(f);
    return 0;
}
