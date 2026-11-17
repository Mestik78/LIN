#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Practica 4 Parte B - productor consumidor");
MODULE_AUTHOR("Miguel Platón y Alfonso Barriga");

#define MAX_CHARS_KBUF 32

static unsigned int max_items = 4;
module_param(max_items, uint, 0444);
MODULE_PARM_DESC(max_items, "Capacidad del buffer en enteros: 4 u 8");

static struct kfifo cbuf;
static struct semaphore buffer_sem;
static struct semaphore slots;
static struct semaphore items;

static int prodcons_open(struct inode *inode, struct file *file)
{
	if (!try_module_get(THIS_MODULE))
		return -ENODEV;
	return 0;
}

static int prodcons_release(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
	return 0;
}

static ssize_t prodcons_write(struct file *file, const char __user *buf,
			      size_t len, loff_t *off)
{
	char kbuf[MAX_CHARS_KBUF + 1];
	int value = 0;

	if (len == 0)
		return 0;
	if (len > MAX_CHARS_KBUF)
		return -EINVAL;

	if (copy_from_user(kbuf, buf, len))
		return -EFAULT;
	kbuf[len] = '\0';

	if (sscanf(kbuf, "%d", &value) != 1)
		return -EINVAL;


	if (down_interruptible(&slots))
		return -EINTR;
	if (down_interruptible(&buffer_sem)) {
		up(&slots);
		return -EINTR;
	}

	kfifo_in(&cbuf, &value, sizeof(int));
	up(&buffer_sem);
	up(&items);

	return len;
}

static ssize_t prodcons_read(struct file *file, char __user *buf, size_t len,
			     loff_t *off)
{
	char kbuf[MAX_CHARS_KBUF + 1];
	int nr_bytes = 0;
	int value;

	if (len == 0 || *off > 0)
		return 0;
	if (down_interruptible(&items))
		return -EINTR;
	if (down_interruptible(&buffer_sem)) {
		up(&items);
		return -EINTR;
	}

	kfifo_out(&cbuf, &value, sizeof(int));
	up(&buffer_sem);
	up(&slots);

	nr_bytes = sprintf(kbuf, "%i\n", value);
	if (len < nr_bytes)
		return -ENOSPC;
	if (copy_to_user(buf, kbuf, nr_bytes))
		return -EFAULT;

	*off += nr_bytes;
	return nr_bytes;
}

static const struct file_operations prodcons_fops = {
	.open = prodcons_open,
	.release = prodcons_release,
	.read = prodcons_read,
	.write = prodcons_write,
};

static struct miscdevice prodcons_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "prodcons",
	.fops = &prodcons_fops,
	.mode = 0666,
};

static int __init prodcons_init(void)
{
	int ret;

	if (max_items != 4 && max_items != 8)
		return -EINVAL;

	ret = kfifo_alloc(&cbuf, max_items * sizeof(int), GFP_KERNEL);
	if (ret)
		return ret;

	sema_init(&buffer_sem, 1);
	sema_init(&slots, max_items);
	sema_init(&items, 0);

	ret = misc_register(&prodcons_device);
	if (ret) {
		kfifo_free(&cbuf);
		return ret;
	}

	pr_info("ProdCons cargado: capacidad de %u enteros\n", max_items);
	return 0;
}

static void __exit prodcons_exit(void)
{
	misc_deregister(&prodcons_device);
	kfifo_free(&cbuf);
	pr_info("ProdCons descargado\n");
}

module_init(prodcons_init);
module_exit(prodcons_exit);
