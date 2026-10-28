#include <linux/module.h>
#include <asm-generic/errno.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <asm-generic/errno.h>

#define ALL_LEDS_ON 0x7
#define ALL_LEDS_OFF 0
#define NR_GPIO_LEDS  3
#define DEVICE_NAME "leds"
#define CLASS_NAME  "leds_class"

MODULE_DESCRIPTION("ModledsPi_gpiod Kernel Module - FDI-UCM");
MODULE_AUTHOR("Juan Carlos Saez");
MODULE_LICENSE("GPL");

/* Actual GPIOs used for controlling LEDs */
const int led_gpio[NR_GPIO_LEDS] = {25, 27, 4};

/* Array to hold gpio descriptors */
struct gpio_desc* gpio_descriptors[NR_GPIO_LEDS];

static dev_t dev_num;
static struct cdev leds_cdev;
static struct class *leds_class = NULL;
static struct device *leds_device = NULL;

static ssize_t leds_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);

static const struct file_operations fops = {
  .owner = THIS_MODULE,
  .write = leds_write,
};

/* Set led state to that specified by mask */
static inline int set_pi_leds(unsigned int mask) {
  int i;
  for (i = 0; i < NR_GPIO_LEDS; i++)
    gpiod_set_value(gpio_descriptors[i], (mask >> i) & 0x1 );
  return 0;
}

static int __init modleds_init(void)
{
  int i, j;
  int err = 0;
  char gpio_str[10];

  for (i = 0; i < NR_GPIO_LEDS; i++) {
    /* Build string ID */
    sprintf(gpio_str, "led_%d", i);
    /* Request GPIO */
    if ((err = gpio_request(led_gpio[i], gpio_str))) {
      pr_err("Failed GPIO[%d] %d request\n", i, led_gpio[i]);
      goto err_gpios;
    }

    /* Transforming into descriptor */
    if (!(gpio_descriptors[i] = gpio_to_desc(led_gpio[i]))) {
      pr_err("GPIO[%d] %d is not valid\n", i, led_gpio[i]);
      err = -EINVAL;
      goto err_gpios;
    }

    gpiod_direction_output(gpio_descriptors[i], 0);
  }

  /* Reservar (Major/Minor) */
	err = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
	if (err < 0) {
		pr_err("Fallo al reservar char device region\n");
		goto err_chrdev;
  }

	/* cdev */
  cdev_init(&leds_cdev, &fops);
  leds_cdev.owner = THIS_MODULE;
  err = cdev_add(&leds_cdev, dev_num, 1);
  if (err < 0) {
    pr_err("Fallo al añadir cdev\n");
    goto err_cdev;
  }

  /* Crear la clase */
  leds_class = class_create(CLASS_NAME);
  if (IS_ERR(leds_class)) {
    err = PTR_ERR(leds_class);
    pr_err("Fallo al crear class\n");
    goto err_class;
  }

  /* Crear el nodo */
  leds_device = device_create(leds_class, NULL, dev_num, NULL, DEVICE_NAME);
  if (IS_ERR(leds_device)) {
    err = PTR_ERR(leds_device);
    pr_err("Fallo al crear device node\n");
    goto err_device;
  }

  set_pi_leds(ALL_LEDS_ON);
  pr_info("Driver %s cargado correctamente\n", DEVICE_NAME);
  return 0;

  err_device:
    class_destroy(leds_class);
  err_class:
    cdev_del(&leds_cdev);
  err_cdev:
    unregister_chrdev_region(dev_num, 1);
  err_chrdev:
  err_gpios:
    for (j = 0; j < i; j++)
      gpiod_put(gpio_descriptors[j]);
    return err;
}

static void __exit modleds_exit(void) {
  int i = 0;

  device_destroy(leds_class, dev_num);
  class_destroy(leds_class);
  cdev_del(&leds_cdev);
  unregister_chrdev_region(dev_num, 1);

  set_pi_leds(ALL_LEDS_OFF);

  for (i = 0; i < NR_GPIO_LEDS; i++)
    gpiod_put(gpio_descriptors[i]);

  pr_info("Driver %s descargado\n", DEVICE_NAME);
}

static ssize_t leds_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
  char kbuf[16];
  size_t copy_len;
  unsigned int mask;

  // --------------- Verificar que los datos de entrada son validos ----------------
  if (count == 0)
    return -EINVAL;

  copy_len = (count < sizeof(kbuf) - 1) ? count : sizeof(kbuf) - 1;

  if (copy_from_user(kbuf, buf, copy_len))
    return -EFAULT;

  kbuf[copy_len] = '\0';

  if (kbuf[0] < '0' || kbuf[0] > '7')
    return -EINVAL;

  if (copy_len > 1 && kbuf[1] != '\n' && kbuf[1] != '\0')
    return -EINVAL;
  // -------------------------------------------------------------------------------

  mask = kbuf[0] - '0';

  set_pi_leds(mask);

  return count;
}

module_init(modleds_init);
module_exit(modleds_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Modleds");
