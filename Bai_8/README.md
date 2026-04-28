# BÁO CÁO: HỆ ĐIỀU HÀNH NHÚNG - Sử dụng các công cụ gỡ lỗi và đánh giá hiệu năng cơ bản

---

## BÀI 2.1: Cài đặt thành công gdbserver trên Target.

### 1. Mục tiêu
Cài đặt công cụ `gdbserver` lên hệ điều hành của BBB (Target) và cấu hình `cross-gdb` trên máy tính Ubuntu (Host).

### 2. Các bước thực hiện

**Bước 1: Cấu hình Buildroot hỗ trợ C++ và GDB**
Đi vào thư mục Buildroot và mở menu cấu hình:
```bash
cd ~/buildroot-2023.11.1
make menuconfig
```
Thực hiện cấu hình 2 mục sau:
1. Cấu hình Toolchain (Bật C++ và Host GDB):
- Đi theo đường dẫn: Toolchain  --->
- Đánh dấu [*] Enable C++ support
- Đánh dấu [*] Build cross gdb for the host
2. Đóng gói gdbserver vào Target:
- Đi ra màn hình chính, vào Target packages  --->
- Vào Debugging, profiling and benchmark  --->
- Đánh dấu [*] gdb và chọn thêm [*] gdbserver ở menu con bên dưới.
Lưu cấu hình (Save) và thoát (Exit).


**Bước 2: Biên dịch lại hệ thống**
Do có sự thay đổi cốt lõi ở phần Toolchain (thêm C++), bắt buộc phải dọn dẹp hệ thống cũ và biên dịch lại từ đầu.
```bash
make clean
make
```
(Quá trình mất khoảng 45 phút - 1 tiếng).


**Bước 3: Chép "gdbserver" và Thư viện C++ sang mạch (Qua thẻ nhớ)**
Sau khi biên dịch xong, đưa file thực thi gdbserver và bộ thư viện động của C++ sang bo mạch BBB.
1. Tắt BBB, tháo thẻ MicroSD cắm vào máy tính.
2. Mở Terminal, chép file thực thi gdbserver vào thư mục /usr/bin/ của thẻ nhớ:
```bash
sudo cp output/target/usr/bin/gdbserver /media/chnam24/rootfs/usr/bin/
```
3. Chép thư viện liên kết động của C++ (libstdc++.so*) sang thư mục /usr/lib/ của thẻ nhớ
```bash
sudo cp -a output/target/usr/lib/libstdc++.so* /media/chnam24/rootfs/usr/lib/
```
4. Lưu dữ liệu an toàn và rút thẻ nhớ:
```bash
sync
sudo umount /media/chnam24/rootfs
sudo umount /media/chnam24/boot
```


**Bước 4: Kiểm tra kết quả**
Cắm thẻ nhớ lại vào BBB, cấp nguồn và đăng nhập vào Terminal của BBB qua Serial.
Gõ lệnh kiểm tra phiên bản:
```bash
gdbserver --version
```


### 3. Kết quả nghiệm thu
<img width="768" height="331" alt="image" src="https://github.com/user-attachments/assets/8b14dd3a-5b97-4f1f-a3e8-08bf532a5303" />
Kết quả: Màn hình trả về thông tin phiên bản và kiến trúc chính xác của chip ARM.

---

## Bài tập 2.2: Thực hiện sử dụng gdb để điều khiển luồng cơ bản chương trình từ host -> target.

### 1. Mục tiêu
Thiết lập kết nối Remote Debugging giữa máy tính Ubuntu (Host) và mạch BBB (Target). Sử dụng GDB để can thiệp trực tiếp vào tiến trình đang chạy: đặt điểm dừng, chạy từng bước, xem/thay đổi giá trị biến theo thời gian thực và truy xuất thanh ghi bộ vi xử lý.

### 2. Chuẩn bị
**2.1. Mã nguồn và biên dịch**
Viết một chương trình C cơ bản (`test_gdb.c`) chứa vòng lặp và gọi hàm.
```c
#include <stdio.h>
#include <unistd.h>

void in_lan_dem(int lan) {
    printf("Dem lan %d\n", lan);
}

int main() {
    int bien_dem = 0;
    printf("--- START TEST GDB ---\n");
    
    while(bien_dem < 50) {
        bien_dem = bien_dem + 1;
        in_lan_dem(bien_dem);
        sleep(1); // Dung 1 giay
    }
    
    printf("--- END! ---\n");
    return 0;
}
```

Biên dịch bằng Cross-Compiler, bắt buộc đi kèm cờ gỡ lỗi `-g` để giữ lại symbol (tên biến, số dòng) cho GDB:
```bash
arm-buildroot-linux-gnueabihf-gcc -g test_gdb.c -o test_gdb
```

**2.2. Thiết lập đường truyền mạng (USB Ethernet Interface)**
Kết nối hệ thống máy tính và BBB qua giao diện mạng ảo USB (CDC Ethernet).

Trên Target (BBB): Kích hoạt driver g_ether và gán IP tĩnh:
```bash
modprobe g_ether
ifconfig usb0 192.168.7.2 up
```

Trên Host (Ubuntu): Thiết lập cấu hình IP tĩnh cho card mạng USB tương ứng là 192.168.7.1 và thực hiện ping 192.168.7.2 để xác nhận đường truyền thông suốt.

### 3. Các bước Gỡ lỗi
**Bước 1: Khởi tạo Server gỡ lỗi (Trên Target)**
Chạy gdbserver để mở cổng mạng 1234 và đính kèm vào phần mềm:
```bash
gdbserver 192.168.7.2:1234 ./test_gdb
```

**Bước 2: Khởi tạo Client gỡ lỗi & Kết nối (Trên Host)**
```bash
arm-buildroot-linux-gnueabihf-gdb ./test_gdb
(gdb) target remote 192.168.7.2:1234
```
<img width="1063" height="272" alt="Screenshot from 2026-04-28 22-09-22" src="https://github.com/user-attachments/assets/a8073dcb-73fc-4cce-8317-4e11deef7e3c" />

**Bước 3: Thực thi các lệnh điều khiển luồng (Kết quả test)**
1. Thêm/Xóa Breakpoint:
- b main: Đặt điểm dừng tại đầu hàm main.
- b 13: Đặt điểm dừng tại một dòng code cụ thể (dòng 13).
- info b: Liệt kê các điểm dừng đang hoạt động.
2. Thực thi điều khiển chạy:
- c (Continue): Tiếp tục chạy đến điểm dừng tiếp theo.
- n (Next): Chạy qua dòng lệnh hiện tại (bước qua hàm, không đi sâu vào trong).
- s (Step): Nhảy vào bên trong hàm con để gỡ lỗi chi tiết.
3. In giá trị biến:
- p bien_dem: Xuất giá trị hiện tại của biến. (Quá trình test có ghi nhận và xử lý thành công hiện tượng mất dấu biến (Out of Scope) khi step vào hàm con, sử dụng lệnh up/frame để điều hướng Call Stack).
4. Gán giá trị biến (Runtime Modification):
- set bien_dem = 48: Can thiệp trực tiếp vào bộ nhớ RAM, ép biến đếm thay đổi giá trị để tua nhanh tiến trình kiểm thử vòng lặp while.
5. Xem giá trị thanh ghi:
- info registers: Xuất trạng thái các thanh ghi ARM (r0-r12, sp, lr, pc). Xác nhận được sự ánh xạ của biến cục bộ lên thanh ghi vật lý (VD: r3 = 0x31 tương đương 49).
6. Xem trạng thái Stack trace:
- bt (Backtrace): Truy xuất lịch sử gọi hàm để xác định đường dẫn thực thi hiện tại.
7. Kết thúc phiên:
- q (Quit): Ngắt kết nối GDB an toàn và tự động dọn dẹp tiến trình trên Target.

<img width="975" height="1088" alt="Screenshot from 2026-04-28 11-29-12" src="https://github.com/user-attachments/assets/74358412-b133-4df6-8826-0c047936b348" />
<img width="975" height="1088" alt="Screenshot from 2026-04-28 11-29-18" src="https://github.com/user-attachments/assets/7381c63f-51f3-411c-85d7-7ff50ec71860" />
<img width="975" height="1088" alt="Screenshot from 2026-04-28 11-29-21" src="https://github.com/user-attachments/assets/12738661-6457-480e-af5e-f59036248968" />

---

## Bài tập 2.3: Phân tích về Bộ nhớ

### 1. Mục tiêu
Sử dụng công cụ Valgrind Memcheck để phát hiện các lỗi quản lý bộ nhớ tiềm ẩn mà trình biên dịch thông thường không phát hiện được.

### 2. Các bước thực hiện
**Bước 1: Cài đặt công cụ phân tích trên Host (Ubuntu)**
```bash
sudo apt update 
sudo apt install valgrind -y
```

**Bước 2: Tạo chương trình mẫu có lỗi rò rỉ bộ nhớ**
Tạo file leak.c thực hiện cấp phát 400 bytes nhưng cố tình không thu hồi trước khi chương trình kết thúc.

```c
#include <stdio.h>
#include <stdlib.h>
 

void error_func() {
    // Xin hệ điều hành 400 bytes RAM
    int *data_arr = (int *)malloc(100 * sizeof(int));

    // Giả sử có thao tác gì đó với dữ liệu
    data_arr[0] = 99;

    // Kết thúc hàm nhưng không gọi lệnh trả tiền "free(data_arr);"
    // Biến con trỏ 'data_arr' sẽ biến mất, nhưng 400 bytes RAM kia vẫn bị treo cứng vĩnh viễn!
}

int main() {
    printf("--- START TEST VALGRIND ---\n");
    error_func();
    printf("--- END TEST ---\n");
    return 0;
}
```

**Bước 3: Biên dịch chương trình với ký hiệu gỡ lỗi**
```bash
gcc -g leak.c -o leak
```
Sử dụng cờ -g để Valgrind có thể ánh xạ địa chỉ bộ nhớ bị lỗi về chính xác dòng code trong tệp nguồn C.
**Bước 4: Biên dịch chương trình với ký hiệu gỡ lỗi**
Sử dụng lệnh kiểm tra mức độ chi tiết nhất:
```bash
valgrind --leak-check=full ./leak
```
<img width="933" height="615" alt="valgrind phat hien loi leak ram" src="https://github.com/user-attachments/assets/f882a57b-88c7-4a7d-af89-9baa807683aa" />

**Bước 5: Vá lỗi và Kiểm tra lại**
Bổ sung lệnh "free(data_arr);" vào cuối phạm vi sử dụng của biến.
```c
#include <stdio.h>
#include <stdlib.h>
 

void error_func() {
    // Xin hệ điều hành 400 bytes RAM
    int *data_arr = (int *)malloc(100 * sizeof(int));

    // Giả sử có thao tác gì đó với dữ liệu
    data_arr[0] = 99;

    // Vá lỗi 
    free(data_arr);
}

int main() {
    printf("--- START TEST VALGRIND ---\n");
    error_func();
    printf("--- END TEST ---\n");
    return 0;
}
```

Biên dịch và chạy lại Valgrind.
```bash
gcc -g leak.c -o leak
valgrind --leak-check=full ./leak
```
<img width="932" height="424" alt="Screenshot from 2026-04-28 22-44-52" src="https://github.com/user-attachments/assets/d3dd11af-9614-4196-91ae-df6b8de7390f" />


---
