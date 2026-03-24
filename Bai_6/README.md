
# BÁO CÁO: HỆ ĐIỀU HÀNH NHÚNG - ỨNG DỤNG TỔNG HỢP

---

## BÀI 1: GIAO TIẾP VỚI DEVICE DRIVER BẰNG LỆNH COMMAND LINE

### 1. Mục tiêu
Tương tác trực tiếp với Device Driver của GPIO trên BeagleBone Black thông qua các lệnh `cat` và `echo` trong thư mục Sysfs (`/sys/class/gpio/`) để điều khiển ngoại vi (Blink LED).

### 2. Các bước thực hiện
Sử dụng chân **P9_12** (**GPIO 60** trong Linux) để kết nối với cực dương của LED. Truy cập vào mạch qua cổng Serial với quyền `root` và thực hiện lần lượt các lệnh:

**Kích hoạt (Export) Device Driver:**
Ra lệnh cho Kernel nhường quyền điều khiển chân GPIO 60 cho User Space.
```bash
echo 60 > /sys/class/gpio/export
```

**Cấu hình hướng tín hiệu (Direction):**
Thiết lập chân GPIO 60 hoạt động ở chế độ đầu ra (Output).
```bash
echo out > /sys/class/gpio/gpio60/direction
```

**Điều khiển LED (Value) và kiểm tra trạng thái:**
```bash
# Bật LED (Cấp mức logic 1)
echo 1 > /sys/class/gpio/gpio60/value

# Đọc và in trạng thái hiện tại của chân GPIO ra màn hình
cat /sys/class/gpio/gpio60/value

# Tắt LED (Kéo về mức logic 0)
echo 0 > /sys/class/gpio/gpio60/value

# Đọc và in trạng thái hiện tại của chân GPIO ra màn hình
cat /sys/class/gpio/gpio60/value
```

**Giải phóng tài nguyên (Unexport):**
Sau khi hoàn tất, trả lại quyền quản lý chân GPIO cho Kernel để tránh xung đột hệ thống.
```bash
echo 60 > /sys/class/gpio/unexport
```

### 3. Kết quả nghiệm thu
<img width="786" height="353" alt="image" src="https://github.com/user-attachments/assets/dbe7d185-9fdf-4d5d-a518-bccd218ea5e4" />

---

## BÀI 2: VIẾT CHƯƠNG TRÌNH GIAO TIẾP VÀ ĐÓNG GÓI VÀO BUILDROOT

### 1. Mục tiêu
Sử dụng C/C++ tự động hóa tương tác với Device Driver. Đóng gói chương trình thành một Package của Buildroot để tự động biên dịch chéo và tích hợp sẵn vào file `sdcard.img`.
### 2.  Các bước thực hiện
**Bước 1: Viết mã nguồn C cho chương trình Blink LED**
Tạo một Package mới tên là blink_led
```bash
mkdir -p package/blink_led/src
```

Viết code blink led trong file main.c 

Chương trình sử dụng các hàm I/O chuẩn (`fopen`, `fprintf`) để thao tác với các file ảo của hệ thống Sysfs:

```c
cat << 'EOF' > package/blink_led/src/main.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define GPIO_PIN "60"
#define BLINK_COUNT 10

// Hàm phụ trợ để ghi dữ liệu vào file Sysfs
void write_sysfs(const char *path, const char *value) {
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        printf("Lỗi: Không thể mở file %s\n", path);
        exit(1);
    }
    fprintf(fp, "%s", value);
    fclose(fp);
}

int main() {
    printf("========================================\n");
    printf("   CHƯƠNG TRÌNH BLINK LED QUA SYSFS\n");
    printf("========================================\n");

    // 1. Đánh thức Driver (Export)
    printf("[-] Dang kich hoat GPIO %s...\n", GPIO_PIN);
    write_sysfs("/sys/class/gpio/export", GPIO_PIN);
    usleep(100000); // Đợi 100ms để hệ thống kịp tạo thư mục gpio60

    // 2. Cài đặt hướng đầu ra (Direction = out)
    printf("[-] Cai dat huong chan la OUTPUT...\n");
    write_sysfs("/sys/class/gpio/gpio60/direction", "out");

    // 3. Vòng lặp nhấp nháy LED
    printf("[-] Bat dau nhap nhay LED %d lan...\n", BLINK_COUNT);
    for (int i = 0; i < BLINK_COUNT; i++) {
        write_sysfs("/sys/class/gpio/gpio60/value", "1"); // ON
        printf("    LED ON\n");
        sleep(1);
        
        write_sysfs("/sys/class/gpio/gpio60/value", "0"); // OFF
        printf("    LED OFF\n");
        sleep(1);
    }

    // 4. Trả lại tài nguyên (Unexport)
    printf("[-] Hoan thanh. Tra lai tai nguyen GPIO...\n");
    write_sysfs("/sys/class/gpio/unexport", GPIO_PIN);
    printf("========================================\n");

    return 0;
}
EOF
```

**Bước 2: Khai báo package mới cho menuconfig (Config.in)**
```bash
cat << 'EOF' > package/blink_led/Config.in
config BR2_PACKAGE_BLINK_LED
	bool "Blink LED Application"
	help
	  Chuong trinh C dieu khien nhap nhay LED qua GPIO Sysfs.
EOF
```

**Bước 3: Viết quy trình biên dịch tự động (blink_led.mk)**
```makefile
cat << 'EOF' > package/blink_led/blink_led.mk
BLINK_LED_VERSION = 1.0
BLINK_LED_SITE = $(BLINK_LED_PKGDIR)/src
BLINK_LED_SITE_METHOD = local

define BLINK_LED_BUILD_CMDS
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) $(@D)/main.c -o $(@D)/blink_led
endef

define BLINK_LED_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/blink_led $(TARGET_DIR)/usr/bin/blink_led
endef

$(eval $(generic-package))
EOF
```

**Bước 4: Add package vào hệ thống Buildroot**
```bash
sed -i '/menu "Miscellaneous"/a \source "package/blink_led/Config.in"' package/Config.in
```
**Bước 5: Bật APP và chạy Make**

- Mở menu: make menuconfig
- Vào Target packages -> Miscellaneous
- Tìm và đánh dấu [*] vào dòng Blink LED Application.
- Bấm Esc ra ngoài và Save lại.
Gõ lệnh: make

**Bước 6: Flash lại file .img vào thẻ SD**
Đảm bảo đang ở thư mục gốc của Buildroot (~/buildroot-2023.11.1). Tiến hành nạp Image bằng lệnh:
```bash
sudo dd if=output/images/sdcard.img of=/dev/sdc bs=4M status=progress
sync
```

**Bước 7: Khởi chạy trên BBB**
Khi đăng nhập root thành công, gõ lệnh:
```bash
blink_led
```

### 3. Kết quả
<img width="993" height="815" alt="image" src="https://github.com/user-attachments/assets/27badb7f-dd6d-4067-b908-6e04f856ca9d" />

---

## BÀI 3: TỰ KHỞI CHẠY ỨNG DỤNG SAU KHI BOOT (AUTO-START)

### 1. Mục tiêu
Cấu hình hệ thống SysV Init (BusyBox init) để ứng dụng `blink_led` tự động khởi chạy ngầm ngay sau khi hệ điều hành boot thành công mà không cần người dùng đăng nhập hay thao tác lệnh.

### 2. Các bước thực hiện
**Bước 1: Viết Init Script (`S99blink_led`)**
Tạo file S99blink_led nằm ngay trong thư mục package blink_led đã làm ở bài trước. Sử dụng toán tử `&` để đẩy chương trình chạy ngầm (Background), tránh block tiến trình đăng nhập của Kernel.

```bash
cat << 'EOF' > package/blink_led/S99blink_led
#!/bin/sh
# Day la script tu dong chay blink_led khi boot

case "$1" in
  start)
        echo "Starting Blink LED Application..."
        # Chay chuong trinh duoi background (them dau &) de khong lam treo he thong
        /usr/bin/blink_led &
        ;;
  stop)
        echo "Stopping Blink LED Application..."
        killall blink_led
        ;;
  restart)
        $0 stop
        sleep 1
        $0 start
        ;;
  *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
esac

exit 0
EOF
```

**Bước 2: Cập nhật Buildroot Hook**
Thêm chỉ thị `_INSTALL_INIT_SYSV` vào file `blink_led.mk` ở bài trước để Buildroot tự động copy kịch bản trên vào đúng thư mục `/etc/init.d/` của RootFS.

```makefile
cat << 'EOF' > package/blink_led/blink_led.mk
BLINK_LED_VERSION = 1.0
BLINK_LED_SITE = $(BLINK_LED_PKGDIR)/src
BLINK_LED_SITE_METHOD = local

define BLINK_LED_BUILD_CMDS
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) $(@D)/main.c -o $(@D)/blink_led
endef

define BLINK_LED_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/blink_led $(TARGET_DIR)/usr/bin/blink_led
endef

# DAY LA PHAN MOI THEM VAO CHO BAI TAP AUTO-START
define BLINK_LED_INSTALL_INIT_SYSV
	$(INSTALL) -D -m 0755 $(BLINK_LED_PKGDIR)/S99blink_led $(TARGET_DIR)/etc/init.d/S99blink_led
endef

$(eval $(generic-package))
EOF
```

**Bước 3: Dọn dẹp Stamp và Biên dịch lại**
Sử dụng lệnh `make blink_led-dirclean` để xóa file đánh dấu trạng thái hoàn thành cũ của Buildroot, ép hệ thống đọc lại file cấu hình `.mk` mới và tiến hành đóng gói lại Image.

### 3. Kết quả
Sau khi nạp thẻ nhớ và cấp nguồn cho BeagleBone Black (không thực hiện đăng nhập). Khi quá trình boot kết thúc, mạch tự động nhấp nháy LED 10 lần. 
<img width="993" height="815" alt="image" src="https://github.com/user-attachments/assets/f3b740fc-0866-46fb-89a9-4c222f987d9a" />
