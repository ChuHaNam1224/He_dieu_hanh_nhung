#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h> // De su dung: copy_to_user, copy_from_user 
#include <linux/io.h>      // De su dung: ioremap, writel, readl 
#include <linux/device.h>  // De su dung: class_create, device_create 

#define DRIVER_NAME "my_gpio_driver"
#define CLASS_NAME  "my_gpio_class"

/* Cấu hình thông số phần cứng BBB cho GPIO 60 (Tương đương GPIO1_28) */
#define GPIO1_BASE        0x4804C000    // Địa chỉ cơ sở của cụm GPIO1
#define GPIO_SIZE         4096          // Kích thước vùng nhớ (4KB)
#define GPIO_OE           0x134         // Thanh ghi OE (Output Enable)
#define GPIO_SETDATAOUT   0x194         // Thanh ghi trạng thái mức cao
#define GPIO_CLEARDATAOUT 0x190         // Thanh ghi trạng thái mức thấp
#define GPIO_DATAOUT      0x13C         // Thanh ghi đọc trạng thái của GPIO
#define PIN_28            (1 << 28)     

/* Khai báo biến toàn cục */
dev_t dev_num;                  /* Chứa Major/Minor tự động */
static struct cdev my_cdev;     /* Cấu trúc Character Device */
static struct class *my_class;  /* Lớp thiết bị */
static struct device *my_device;/* Thiết bị */
void __iomem *gpio_base_addr;   /* Con trỏ chứa địa chỉ ảo sau khi ioremap */

/* =========================================================================
 * --- YÊU CẦU 1: Các hàm Open/Release/Read/Write cơ bản ---
 * ========================================================================= */
static int my_open(struct inode *inode, struct file *file) {c
    pr_info("Driver: Mo file thiet bi thanh cong!\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file) {
    pr_info("Driver: Dong file thiet bi!\n");
    return 0;
}

/* Read - Đọc dữ liệu từ thanh ghi phần cứng và gửi lên User Space */
static ssize_t my_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset) {
    char val_str[2];
    uint32_t reg_val;
    
    // Đọc thanh ghi DATAOUT bằng readl
    reg_val = readl(gpio_base_addr + GPIO_DATAOUT);
    
    if (reg_val & PIN_28) {
        val_str[0] = '1'; // LED đang sáng
    } else {
        val_str[0] = '0'; // LED đang tắt
    }
    val_str[1] = '\n';

    // Copy dữ liệu từ Kernel lên User Space
    if (copy_to_user(user_buffer, val_str, 2)) {
        pr_err("Driver: Loi copy_to_user!\n");
        return -EFAULT;
    }
    pr_info("Driver: Trang thai LED = %c\n", val_str[0]);
    return 2; /* Trả về số byte đã đọc */
}

/* Write - Đọc dữ liệu từ User Space và ghi vào thanh ghi phần cứng */
static ssize_t my_write(struct file *file, const char __user *user_buffer, size_t size, loff_t *offset) {
    char input_val;

    // Lấy 1 byte dữ liệu từ User Space
    if (copy_from_user(&input_val, user_buffer, 1)) {
        pr_err("Driver: Loi copy_from_user!\n");
        return -EFAULT;
    }

    // Tương tác ngoại vi: Ghi vào thanh ghi bằng writel
    if (input_val == '1') {
        writel(PIN_28, gpio_base_addr + GPIO_SETDATAOUT);
        pr_info("Driver: Bat LED (ON)\n");
    } else if (input_val == '0') {
        writel(PIN_28, gpio_base_addr + GPIO_CLEARDATAOUT);
        pr_info("Driver: Tat LED (OFF)\n");
    } else {
        pr_warn("Driver: Du lieu khong hop le! \n");
    }

    return size;
}

/* Đăng ký thông qua static struct file_operations */
static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .release = my_release,
    .read    = my_read,
    .write   = my_write,
};

/* Init (Khởi tạo) */
static int __init led_driver_init(void) {
    uint32_t reg_oe;

    pr_info("Driver: Bat dau khoi tao...\n");

    /* Tự động cấp phát Major/Minor */
    if (alloc_chrdev_region(&dev_num, 0, 1, DRIVER_NAME) < 0) {
        pr_err("Driver: Khong the cap phat Major/Minor!\n");
        return -1;
    }
    pr_info("Driver: Cap phat thanh cong Major = %d, Minor = %d\n", MAJOR(dev_num), MINOR(dev_num));

    /* Liên kết cdev với fops */
    cdev_init(&my_cdev, &fops);
    if (cdev_add(&my_cdev, dev_num, 1) < 0) {
        pr_err("Driver: Loi cdev_add!\n");
        unregister_chrdev_region(dev_num, 1);
        return -1;
    }

    /* Tự động tạo file Device trong thư mục /dev/ */
    // Tạo Class
    my_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(my_class)) {
        pr_err("Driver: Loi tao Class!\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(my_class);
    }

    // Tạo Device file trong /dev/ (/dev/my_gpio_driver)
    my_device = device_create(my_class, NULL, dev_num, NULL, DRIVER_NAME);
    if (IS_ERR(my_device)) {
        pr_err("Driver: Loi tao Device file!\n");
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(my_device);
    }
    pr_info("Driver: Da tu dong tao file /dev/%s\n", DRIVER_NAME);

    /* Sử dụng ioremap để truy cập bộ nhớ vật lý */
    gpio_base_addr = ioremap(GPIO1_BASE, GPIO_SIZE);
    if (!gpio_base_addr) {
        pr_err("Driver: Loi ioremap!\n");
        // Dọn dẹp nếu lỗi
        device_destroy(my_class, dev_num);
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return -ENOMEM;
    }

    /* Cấu hình chân LED ở chế độ Output (Reference Manual)
     * Thanh ghi OE: Bit = 0 là Output, Bit = 1 là Input */
    reg_oe = readl(gpio_base_addr + GPIO_OE);
    reg_oe &= ~PIN_28; // Xóa bit 28 về 0
    writel(reg_oe, gpio_base_addr + GPIO_OE);

    pr_info("Driver: Khoi tao hoan tat. San sang giao tiep!\n");
    return 0;
}

/* Exit (Dọn dẹp) */
static void __exit led_driver_exit(void) {
    /* Tắt LED trước khi gỡ Driver */
    writel(PIN_28, gpio_base_addr + GPIO_CLEARDATAOUT);

    /* Gỡ bỏ ioremap */
    iounmap(gpio_base_addr);

    /* Gỡ bỏ Device và Class */
    device_destroy(my_class, dev_num);
    class_destroy(my_class);

    /* Gỡ bỏ cdev và Major/Minor */
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);

    pr_info("Driver: Da go bo khoi he thong an toan!\n");
}

module_init(led_driver_init);
module_exit(led_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chu Ha Nam");
MODULE_DESCRIPTION("Driver giao tiep GPIO 60 bang ioremap");
