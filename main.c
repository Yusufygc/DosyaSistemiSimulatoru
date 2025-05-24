#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>   // access() için
#ifdef _WIN32
#include <windows.h>
#endif
#include "fs.h"

#define MAX_DATA_SIZE 1024
#define LOG_FILE      "fs_operations.log"

void print_menu() {
    printf("\n=== SimpleFS Menu ===\n");
    printf(" 1. Create file\n");
    printf(" 2. Delete file\n");
    printf(" 3. Write to file\n");
    printf(" 4. Read from file\n");
    printf(" 5. List files\n");
    printf(" 6. Format disk\n");
    printf(" 7. Rename file\n");
    printf(" 8. Check file exists\n");
    printf(" 9. Get file size\n");
    printf("10. Append to file\n");
    printf("11. Truncate file\n");
    printf("12. Copy file\n");
    printf("13. Move file\n");
    printf("14. Defragment disk\n");
    printf("15. Check integrity\n");
    printf("16. Backup disk\n");
    printf("17. Restore backup\n");
    printf("18. Show file contents (cat)\n");
    printf("19. Compare two files (diff)\n");
    printf("20. Show operation log\n");
    printf("21. Exit\n");
    printf("Choice: ");
}

void handle_read_file() {
    char filename[256];
    printf("Enter filename to read: ");
    scanf("%255s", filename);

    int choice;
    printf("Read mode:\n");
    printf("1. Read full file\n");
    printf("2. Read partial (offset + size)\n");
    printf("Choose option: ");
    scanf("%d", &choice);

    char *buffer = malloc(BLOCK_SIZE * 10); // Gerekirse arttırılabilir
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    ssize_t result = -1;

    if (choice == 1) {
        result = fs_read_all(filename, buffer);
        if (result > 0) {
            printf("=== File Content ===\n");
            fwrite(buffer, 1, result, stdout);
            printf("\n====================\n");
        }
    } else if (choice == 2) {
        uint32_t offset;
        size_t size;
        printf("Enter offset: ");
        scanf("%u", &offset);
        printf("Enter number of bytes to read: ");
        scanf("%zu", &size);

        result = fs_read(filename, offset, size, buffer);
        if (result > 0) {
            printf("Read data: ");
            fwrite(buffer, 1, result, stdout);
            printf("\n");
        }
    } else {
        printf("Invalid option.\n");
    }
    /****************************** */
    if (result > 0) {
        fs_log("read", filename);
    }
/*********************************** */
    free(buffer);
}

int main() {
    int choice;
    char filename[256], newname[256], src[256], dst[256], backup[256];
    char data[MAX_DATA_SIZE];
    //uint32_t offset, size, filesize;
    ssize_t res;
    uint32_t size, filesize;




    // ✅ disk.sim varsa sadece metadata yükle, yoksa formatla
    if (access(DISK_NAME, F_OK) != 0) {
        if (fs_format() != 0) {
            fprintf(stderr, "Disk format failed. Exiting.\n");
            return EXIT_FAILURE;
        }
        fs_log("format", NULL);
        printf("disk.sim created and formatted successfully.\n");
    } else {
        if (disk_read_metadata() != 0) {
            fprintf(stderr, "Failed to read existing disk metadata.\n");
            return EXIT_FAILURE;
        }
        printf("disk.sim found. Loaded existing disk.\n");
    }

    while (1) {
        print_menu();
        if (scanf("%d", &choice) != 1) {
            fprintf(stderr, "Invalid input!\n");
            int c; while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }
        switch (choice) {
            case 1:
                printf("Enter file name to create: ");
                scanf("%s", filename);
                if (fs_create(filename) == 0) fs_log("create", filename);
                break;
            case 2:
                printf("Enter file name to delete: ");
                scanf("%s", filename);
                if (fs_delete(filename) == 0) fs_log("delete", filename);
                break;
            case 3:
                printf("Enter file name: ");
                scanf("%s", filename);
                printf("Enter data to write (max %d chars): ", MAX_DATA_SIZE-1);
                getchar();
                fgets(data, MAX_DATA_SIZE, stdin);
                data[strcspn(data, "\n")] = '\0';
                size = (uint32_t)strlen(data);
                if ((res = fs_write(filename, data, size)) >= 0) {
                    fs_log("write", filename);
                }
                break;
            case 4:
                handle_read_file(); 
                break;
            case 5:
                if (fs_ls() == 0) fs_log("ls", NULL);
                break;
            case 6:
                if (fs_format() == 0) fs_log("format", NULL);
                break;
            case 7:
                printf("Enter old file name: ");
                scanf("%s", filename);
                printf("Enter new file name: ");
                scanf("%s", newname);
                if (fs_rename(filename, newname) == 0) fs_log("rename", filename);
                break;
            case 8:
                printf("Enter file name to check: ");
                scanf("%s", filename);
                printf(fs_exists(filename) ? "File exists.\n" : "File does not exist.\n");
                fs_log("exists", filename);
                break;
            case 9:
                printf("Enter file name to get size: ");
                scanf("%s", filename);
                if (fs_size(filename, &filesize) == 0) {
                    printf("%s size: %u bytes\n", filename, filesize);
                    fs_log("size", filename);
                }
                break;
            case 10:
                printf("Enter file name: ");
                scanf("%s", filename);
                printf("Enter data to append (max %d chars): ", MAX_DATA_SIZE-1);
                getchar();
                fgets(data, MAX_DATA_SIZE, stdin);
                data[strcspn(data, "\n")] = '\0';
                size = (uint32_t)strlen(data);
                if ((res = fs_append(filename, data, size)) >= 0) fs_log("append", filename);
                break;
            case 11:
                printf("Enter file name to truncate: ");
                scanf("%s", filename);
                printf("Enter new size (bytes): ");
                scanf("%u", &size);
                if (fs_truncate(filename, size) == 0) fs_log("truncate", filename);
                break;
            case 12:
                printf("Source file: ");
                scanf("%s", src);
                printf("Destination file: ");
                scanf("%s", dst);
                if (fs_copy(src, dst) == 0) fs_log("copy", src);
                break;
            case 13:
                printf("Source file: ");
                scanf("%s", src);
                printf("Destination file: ");
                scanf("%s", dst);
                if (fs_mv(src, dst) == 0) fs_log("move", src);
                break;
            case 14:
                if (fs_defragment() == 0) fs_log("defragment", NULL);
                break;
            case 15:
                if (fs_check_integrity() == 0) fs_log("check_integrity", NULL);
                break;
            case 16:
                printf("Enter backup file name: ");
                scanf("%s", backup);
                if (fs_backup(backup) == 0) fs_log("backup", backup);
                break;
            case 17:
                printf("Enter backup to restore: ");
                scanf("%s", backup);
                if (fs_restore(backup) == 0) fs_log("restore", backup);
                break;
            case 18:
                printf("Enter file name to display: ");
                scanf("%s", filename);
                if (fs_cat(filename) == 0) fs_log("cat", filename);
                break;
            case 19:
                printf("First file to compare: ");
                scanf("%s", src);
                printf("Second file to compare: ");
                scanf("%s", dst);
                if (fs_diff(src, dst) == 0) fs_log("diff", src);
                break;
            case 20:
                // Show the log file
                fs_cat(LOG_FILE);
                break;
            case 21:
                printf("Exiting.\n");
                fs_log("exit", NULL);
                return EXIT_SUCCESS;
            default:
                printf("Invalid choice!\n");
        }
    }

    return EXIT_SUCCESS;
}
