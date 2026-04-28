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
