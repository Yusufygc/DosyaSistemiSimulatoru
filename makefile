# Derleyici ve bayraklar
CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11

# Hedef dosya
TARGET  := simplefs

# Kaynak ve nesne dosyaları
SRCS    := main.c fs.c disk.c
OBJS    := $(SRCS:.c=.o)

# Varsayılan hedef
all: $(TARGET)

# Hedefin nasıl derleneceği
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Her .c için .o oluşturma kuralı
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Temizlik
clean:
	rm -f $(OBJS) $(TARGET) disk.sim fs_operations.log

# Yardım mesajı (isteğe bağlı)
help:
	@echo "Kullanılabilir komutlar:"
	@echo "  make        - Derlemeyi yapar"
	@echo "  make clean  - Nesne ve çıktı dosyalarını temizler"
	@echo "  make help   - Yardım mesajını gösterir"
