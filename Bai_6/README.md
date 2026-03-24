
# BÁO CÁO: HỆ ĐIỀU HÀNH NHÚNG - ỨNG DỤNG TỔNG HỢP

---

## BÀI 1: GIAO TIẾP VỚI DEVICE DRIVER BẰNG LỆNH COMMAND LINE

### 1. Mục tiêu
Tương tác trực tiếp với Device Driver của GPIO trên BeagleBone Black thông qua các lệnh `cat` và `echo` trong thư mục Sysfs (`/sys/class/gpio/`) để điều khiển ngoại vi (Blink LED).

### 2. Các bước thực hiện
Sử dụng chân **P9_12** (tương ứng với **GPIO 60** trong Linux) để kết nối với cực dương của LED. Truy cập vào mạch qua cổng Serial với quyền `root` và thực hiện lần lượt các lệnh:

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
```

**Giải phóng tài nguyên (Unexport):**
Sau khi hoàn tất, trả lại quyền quản lý chân GPIO cho Kernel để tránh xung đột hệ thống.
```bash
echo 60 > /sys/class/gpio/unexport
```

### 3. Kết quả nghiệm thu
*(KÉO THẢ ẢNH CHỤP MÀN HÌNH TERMINAL THỰC HIỆN CÁC LỆNH TRÊN VÀO ĐÂY)*
<img width="786" height="353" alt="image" src="https://github.com/user-attachments/assets/dbe7d185-9fdf-4d5d-a518-bccd218ea5e4" />

---

## BÀI 2: VIẾT CHƯƠNG TRÌNH GIAO TIẾP VÀ ĐÓNG GÓI VÀO BUILDROOT

### 1. Mục tiêu
Sử dụng C/C++ để tự động hóa quá trình tương tác với Device Driver. Đóng gói chương trình thành một Package của Buildroot để tự động biên dịch chéo và tích hợp sẵn vào file `sdcard.img`.

### 2. Mã nguồn chương trình (main.c)
Chương trình sử dụng các hàm I/O chuẩn (`fopen`, `fprintf`) để thao tác với các file ảo của hệ thống Sysfs:

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void write_sysfs(const char *path, const char *value) {
    FILE *fp = fopen(path, "w");
    if (fp != NULL) {
        fprintf(fp, "%s", value);
        fclose(fp);
    }
}

int main() {
    printf("[-] Kich hoat GPIO 60...\n");
    write_sysfs("/sys/class/gpio/export", "60");
    usleep(100000); 

    printf("[-] Cai dat Direction = OUT...\n");
    write_sysfs("/sys/class/gpio/gpio60/direction", "out");

    printf("[-] Bat dau nhap nhay LED 10 lan...\n");
    for (int i = 0; i < 10; i++) {
        write_sysfs("/sys/class/gpio/gpio60/value", "1");
        printf("    LED ON\n");
        sleep(1);
        
        write_sysfs("/sys/class/gpio/gpio60/value", "0");
        printf("    LED OFF\n");
        sleep(1);
    }

    printf("[-] Tra lai tai nguyen GPIO...\n");
    write_sysfs("/sys/class/gpio/unexport", "60");
    return 0;
}
```

### 3. Cấu hình Buildroot
Tạo Package `blink_led` trong Buildroot với file `blink_led.mk` chỉ định rõ vị trí lưu file thực thi tại `/usr/bin/` của hệ điều hành đích:

```makefile
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
```

### 4. Kết quả nghiệm thu
Sau khi `make` và nạp Image mới xuống bo mạch, ứng dụng được gọi thành công từ mọi vị trí trên hệ thống dưới quyền root.

*(KÉO THẢ ẢNH CHỤP MÀN HÌNH GỌI LỆNH `blink_led` IN RA CÁC DÒNG LED ON/OFF VÀO ĐÂY)*

---

## BÀI 3: TỰ KHỞI CHẠY ỨNG DỤNG SAU KHI BOOT (AUTO-START)

### 1. Mục tiêu
Cấu hình hệ thống SysV Init (BusyBox init) để ứng dụng `blink_led` tự động khởi chạy ngầm ngay sau khi hệ điều hành boot thành công mà không cần người dùng đăng nhập hay thao tác lệnh.

### 2. Các bước thực hiện
**Bước 1: Viết Init Script (`S99blink_led`)**
Tạo file kịch bản bắt đầu bằng chữ `S` (Start) và mức độ ưu tiên `99` (chạy cuối cùng). Sử dụng toán tử `&` để đẩy chương trình chạy ngầm (Background), tránh block tiến trình đăng nhập của Kernel.

```bash
#!/bin/sh
case "$1" in
  start)
        echo "Starting Blink LED Application..."
        /usr/bin/blink_led &
        ;;
  stop)
        killall blink_led
        ;;
  *)
        echo "Usage: $0 {start|stop}"
        exit 1
esac
exit 0
```

**Bước 2: Cập nhật Buildroot Hook**
Thêm chỉ thị `_INSTALL_INIT_SYSV` vào file `blink_led.mk` để Buildroot tự động copy kịch bản trên vào đúng thư mục `/etc/init.d/` của RootFS.

```makefile
define BLINK_LED_INSTALL_INIT_SYSV
	$(INSTALL) -D -m 0755 $(BLINK_LED_PKGDIR)/S99blink_led $(TARGET_DIR)/etc/init.d/S99blink_led
endef
```

**Bước 3: Dọn dẹp Stamp và Biên dịch lại**
Sử dụng lệnh `make blink_led-dirclean` để xóa file đánh dấu trạng thái hoàn thành cũ của Buildroot, ép hệ thống đọc lại file cấu hình `.mk` mới và tiến hành đóng gói lại Image.

### 3. Kết quả nghiệm thu
Sau khi nạp thẻ nhớ và cấp nguồn cho BeagleBone Black (không thực hiện đăng nhập qua UART). Khoảng 15 giây sau, khi quá trình boot kết thúc, mạch tự động nhấp nháy LED 10 lần. 

*(KÉO THẢ ẢNH CHỤP MÀN HÌNH BOOT LOG CÓ DÒNG "Starting Blink LED Application..." HOẶC VIDEO BO MẠCH TỰ SÁNG ĐÈN VÀO ĐÂY)*
