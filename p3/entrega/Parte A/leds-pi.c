#include <linux/module.h>
#include <asm-generic/errno.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>  /* for copy_to_user */
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,1)
#define __cconst__ const
#else
#define __cconst__ 
#endif

#define NR_GPIO_LEDS  3
#define KERNEL_BUFFER_SIZE 128
#define DEVICE_NAME "led"
#define CLASS_NAME "led"

MODULE_DESCRIPTION("ModledsPi_gpiod Kernel Module - FDI-UCM");
MODULE_AUTHOR("Juan Carlos Saez");
MODULE_AUTHOR("Miguel Platón");
MODULE_AUTHOR("Alfonso Barriga");
MODULE_LICENSE("GPL");

static dev_t start;
static struct cdev* chardev = NULL;
static struct class* class = NULL;
static struct device* device = NULL;

/* Actual GPIOs used for controlling LEDs */
const int led_gpio[NR_GPIO_LEDS] = {25, 27, 4};

/* Array to hold gpio descriptors */
struct gpio_desc* gpio_descriptors[NR_GPIO_LEDS];

/* Set led state to that specified by mask */
static inline int set_pi_leds(unsigned int mask) {
  int i;
  for (i = 0; i < NR_GPIO_LEDS; i++)
    gpiod_set_value(gpio_descriptors[i], (mask >> i) & 0x1 );
  return 0;
}


static ssize_t led_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
    char kbuf[KERNEL_BUFFER_SIZE];
    unsigned int mask;

    if (len >= KERNEL_BUFFER_SIZE)
        return -EINVAL;

    if (copy_from_user(kbuf, buf, len))
        return -EFAULT;

    kbuf[len] = '\0';

    if (kstrtouint(kbuf, 0, &mask))
        return -EINVAL;

    if (mask > 0x7)
        return -EINVAL;

    set_pi_leds(mask);

    return len;
}

static struct file_operations fops = {
  .read = led_read,
  .write = led_write,
};

static char *cool_devnode(__cconst__ struct device *dev, umode_t *mode)
{
    if (!mode)
        return NULL;
    if (MAJOR(dev->devt) == MAJOR(start))
        *mode = 0666;
    return NULL;
}

static int __init modleds_init(void)
{
    int major;      /* Major number assigned to our device driver */
    int minor;      /* Minor number assigned to the associated character device */
    int ret;
    int i, j;
    int err = 0;
    char gpio_str[10];


    // -- driver setup

    /* Get available (major,minor) range */
    if ((ret = alloc_chrdev_region (&start, 0, 1, DEVICE_NAME))) {
        printk(KERN_INFO "Can't allocate chrdev_region()");
        return ret;
    }

    /* Create associated cdev */
    if ((chardev = cdev_alloc()) == NULL) {
        printk(KERN_INFO "cdev_alloc() failed ");
        ret = -ENOMEM;
        goto error_alloc;
    }

    cdev_init(chardev, &fops);

    if ((ret = cdev_add(chardev, start, 1))) {
        printk(KERN_INFO "cdev_add() failed ");
        goto error_add;
    }

    /* Create custom class */
    class = class_create(THIS_MODULE, CLASS_NAME);

    if (IS_ERR(class)) {
        pr_err("class_create() failed \n");
        ret = PTR_ERR(class);
        goto error_class;
    }

    /* Establish function that will take care of setting up permissions for device file */
    class->devnode = cool_devnode;

    /* Creating device */
    device = device_create(class, NULL, start, NULL, DEVICE_NAME);

    if (IS_ERR(device)) {
        pr_err("Device_create failed\n");
        ret = PTR_ERR(device);
        goto error_device;
    }

    major = MAJOR(start);
    minor = MINOR(start);

    printk(KERN_INFO "I was assigned major number %d. To talk to\n", major);
    printk(KERN_INFO "the driver try to cat and echo to /dev/%s.\n", DEVICE_NAME);
    printk(KERN_INFO "Remove the module when done.\n");



    // -- gpio setup

    for (i = 0; i < NR_GPIO_LEDS; i++) {
        /* Build string ID */
        sprintf(gpio_str, "led_%d", i);
        /* Request GPIO */
        if ((err = gpio_request(led_gpio[i], gpio_str))) {
        pr_err("Failed GPIO[%d] %d request\n", i, led_gpio[i]);
        goto error_gpio;
        }

        /* Transforming into descriptor */
        if (!(gpio_descriptors[i] = gpio_to_desc(led_gpio[i]))) {
        pr_err("GPIO[%d] %d is not valid\n", i, led_gpio[i]);
        err = -EINVAL;
        goto error_gpio;
        }

        gpiod_direction_output(gpio_descriptors[i], 0);
    }

    return 0;

    // ---- errors ----

error_gpio:
  for (j = 0; j < i; j++)
    gpiod_put(gpio_descriptors[j]);
error_device:
    class_destroy(class);
error_class:
    /* Destroy chardev */
    if (chardev) {
        cdev_del(chardev);
        chardev = NULL;
    }
error_add:
    /* Destroy partially initialized chardev */
    if (chardev)
        kobject_put(&chardev->kobj);
error_alloc:
    unregister_chrdev_region(start, 1);

    return ret;}

static void __exit modleds_exit(void) {
    int i;
    for (i = 0; i < NR_GPIO_LEDS; i++)
        gpiod_put(gpio_descriptors[i]);

    /* Destroy the device and the class */
    if (device)
        device_destroy(class, device->devt);

    if (class)
        class_destroy(class);

    /* Destroy chardev */
    if (chardev)
        cdev_del(chardev);

    /*
     * Release major minor pair
     */
    unregister_chrdev_region(start, 1);

}

module_init(modleds_init);
module_exit(modleds_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Modleds");
