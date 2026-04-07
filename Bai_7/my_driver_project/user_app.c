#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define DEVICE_PATH "/dev/my_gpio_driver"

int main() {
    int fd;
    int i;

    printf("--- CHUONG TRINH TEST BLINK LED TREN BBB ---\n");

    /* Open Driver */
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        printf("Loi: Khong the mo %s.\n", DEVICE_PATH);
        return -1;
    }
    printf("Mo file thiet bi thanh cong! Bat dau test...\n");

    /* 2. Nhấp nháy CHẬM (Tần số 1 Hz - Sáng 0.5s, tắt 0.5s) */
    printf("\n[1] Slow Blink LED...\n");
    for (i = 0; i < 5; i++) {
        write(fd, "1", 1); // Kích hoạt my_write trong Driver
        usleep(500000);    // Hàm usleep tính bằng micro giây (500000 = 0.5s)
        
        write(fd, "0", 1);
        usleep(500000);
    }

    /* 3. Nhấp nháy NHANH (Tần số 5 Hz - Sáng 0.1s, tắt 0.1s) */
    printf("\n[2] Fast Blink LED...\n");
    for (i = 0; i < 15; i++) {
        write(fd, "1", 1);
        usleep(100000);
        
        write(fd, "0", 1);
        usleep(100000);
    }

    /* 4. Nhấp nháy CHỚP NHOÁNG (Tần số 20 Hz - Sáng 0.025s, tắt 0.025s) */
    printf("\n[3] Faster Blink LED...\n");
    for (i = 0; i < 30; i++) {
        write(fd, "1", 1);
        usleep(25000);
        
        write(fd, "0", 1);
        usleep(25000);
    }

    /* 5. Mua hàng xong thì đi về (Kích hoạt my_release) */
    printf("\nHoan thanh! Dong file thiet bi.\n");
    close(fd);

    return 0;
}
