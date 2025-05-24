// disk.c
#include "disk.h"
#include <fcntl.h>
#include <unistd.h>     // open, read, write, lseek, close
#include <stdio.h>      // perror, fprintf
#include <stdlib.h>     // malloc, calloc, free
#include <string.h>     // memcpy
#include <stdint.h>     // uint8_t

#define META_BUF_SIZE METADATA_SIZE

DiskMetadata metadata;
static int disk_fd = -1;

// Disk dosyasını açar (yoksa hata verir)
int disk_open() {
    if (disk_fd >= 0) return 0;

    disk_fd = open(DISK_NAME, O_RDWR);
    if (disk_fd < 0) {
        perror("Failed to open disk file");
        return -1;
    }
    return 0;
}

// Metadata’yı diskin başından belleğe okur
int disk_read_metadata() {
    if (disk_open() < 0) return -1;

    if (lseek(disk_fd, 0, SEEK_SET) < 0) {
        perror("lseek metadata read failed");
        return -1;
    }

    uint8_t *buf = malloc(META_BUF_SIZE);
    if (!buf) {
        perror("malloc metadata buffer");
        return -1;
    }

    ssize_t bytes = read(disk_fd, buf, META_BUF_SIZE);
    if (bytes != META_BUF_SIZE) {
        fprintf(stderr, "Incomplete metadata read: %zd bytes\n", bytes);
        free(buf);
        return -1;
    }

    memcpy(&metadata, buf, sizeof(DiskMetadata));
    free(buf);
    return 0;
}

// Metadata’yı bellekteki halinden diske yazar
int disk_write_metadata() {
    if (disk_open() < 0) return -1;

    if (lseek(disk_fd, 0, SEEK_SET) < 0) {
        perror("lseek metadata write failed");
        return -1;
    }

    uint8_t *buf = calloc(1, META_BUF_SIZE);
    if (!buf) {
        perror("calloc metadata buffer");
        return -1;
    }

    memcpy(buf, &metadata, sizeof(DiskMetadata));

    ssize_t bytes = write(disk_fd, buf, META_BUF_SIZE);
    if (bytes != META_BUF_SIZE) {
        fprintf(stderr, "Incomplete metadata write: %zd bytes\n", bytes);
        free(buf);
        return -1;
    }

    free(buf);
    return 0;
}

// Belirtilen bloktan veri okur (BLOCK_SIZE kadar)
int disk_read_block(uint32_t block_index, void *buffer) {
    if (disk_open() < 0) return -1;

    off_t offset = METADATA_SIZE + (off_t)block_index * BLOCK_SIZE;
    if (lseek(disk_fd, offset, SEEK_SET) < 0) {
        perror("lseek block read failed");
        return -1;
    }

    ssize_t bytes = read(disk_fd, buffer, BLOCK_SIZE);
    if (bytes != BLOCK_SIZE) {
        fprintf(stderr, "Incomplete block read: %zd bytes\n", bytes);
        return -1;
    }

    return 0;
}

// Belirtilen bloğa veri yazar (BLOCK_SIZE kadar)
int disk_write_block(uint32_t block_index, const void *buffer) {
    if (disk_open() < 0) return -1;

    off_t offset = METADATA_SIZE + (off_t)block_index * BLOCK_SIZE;
    if (lseek(disk_fd, offset, SEEK_SET) < 0) {
        perror("lseek block write failed");
        return -1;
    }

    ssize_t bytes = write(disk_fd, buffer, BLOCK_SIZE);
    if (bytes != BLOCK_SIZE) {
        fprintf(stderr, "Incomplete block write: %zd bytes\n", bytes);
        return -1;
    }

    return 0;
}

// Disk dosyasını kapatır
static void disk_close() {
    if (disk_fd >= 0) {
        close(disk_fd);
        disk_fd = -1;
    }
}

// Program başladığında fd’yi başlat
__attribute__((constructor))
static void init_disk() {
    disk_fd = -1;
}

// Program bittiğinde diski kapat
__attribute__((destructor))
static void cleanup_disk() {
    disk_close();
}
