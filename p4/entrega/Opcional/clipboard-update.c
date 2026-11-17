#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 1)
#define __cconst__ const
#else
#define __cconst__
#endif

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Clipboard-update Kernel Module - FDI-UCM");
MODULE_AUTHOR("Juan Carlos Saez");

#define BUFFER_LENGTH PAGE_SIZE
/* Dev name as it appears in /proc/devices   */
#define DEVICE_NAME "clipboard_update"
#define CLASS_NAME "clip"

/*
 * Global variables are declared as static, so are global within the file.
 */

static dev_t start;
static struct cdev *chardev = NULL;
static struct class *class = NULL;
static struct device *device = NULL;
static char *clipboard; // Space for the "clipboard"

static struct semaphore clipboard_sem;
static struct semaphore update_sem;
static DEFINE_SPINLOCK(readers_lock);
static unsigned int waiting_readers = 0;
static bool waking_readers = false;

static int clipboard_open(struct inode *inode, struct file *file)
{
	if (!try_module_get(THIS_MODULE))
		return -ENODEV;
	return 0;
}

static int clipboard_release(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
	return 0;
}

static ssize_t clipboard_write(struct file *filp, const char __user *buf,
			       size_t len, loff_t *off)
{
	int available_space = BUFFER_LENGTH - 1;

	/* The application can write in this entry just once !! */
	if (*off > 0)
		return 0;
	if (len == 0)
		return 0;

	if (len > available_space) {
		printk(KERN_INFO "clipboard: not enough space!!\n");
		return -ENOSPC;
	}

	if (down_interruptible(&clipboard_sem))
		return -EINTR;

	/* Transfer data from user to kernel space */
	if (copy_from_user(&clipboard[0], buf, len)) {
		up(&clipboard_sem);
		return -EFAULT;
	}

	clipboard[len] = '\0'; /* Add the `\0' */
	*off += len; /* Update the file position indicator */

	spin_lock(&readers_lock);
	if (waiting_readers) {
		waking_readers = true;
		up(&update_sem);
	} else {
		up(&clipboard_sem);
	}
	spin_unlock(&readers_lock);

	return len;
}

static ssize_t clipboard_read(struct file *filp, char __user *buf, size_t len,
			      loff_t *off)
{
	int nr_bytes;
	ssize_t ret;

	/* Tell the application that there is nothing left to read */
	if (*off > 0)
		return 0;
	if (len == 0)
		return 0;

	/* No se puede registrar si se está tratando una actualización */
	if (down_interruptible(&clipboard_sem))
		return -EINTR;
	spin_lock(&readers_lock);
	waiting_readers++;
	spin_unlock(&readers_lock);
	up(&clipboard_sem);

	/* Wait until next clipboard update */
	if (down_interruptible(&update_sem)) {
		spin_lock(&readers_lock);
		waiting_readers--;
		if (waking_readers && !waiting_readers) {
			if (!down_trylock(&update_sem)) {
				waking_readers = false;
				up(&clipboard_sem);
			}
		}
		spin_unlock(&readers_lock);
		pr_info("Blocking operation interrupted due to signal\n");
		return -EINTR;
	}

	nr_bytes = strlen(clipboard);
	if (len < nr_bytes) {
		ret = -ENOSPC;
		goto out;
	}

	/* Transfer data from the kernel to userspace */
	if (copy_to_user(buf, clipboard, nr_bytes)) {
		ret = -EFAULT;
		goto out;
	}

	*off += nr_bytes;
	ret = nr_bytes;
out:
	spin_lock(&readers_lock);
	waiting_readers--;
	if (waiting_readers) {
		up(&update_sem);
	} else {
		waking_readers = false;
		up(&clipboard_sem);
	}
	spin_unlock(&readers_lock);
	return ret;
}

static struct file_operations fops = {
	.open = clipboard_open,
	.release = clipboard_release,
	.read = clipboard_read,
	.write = clipboard_write,
};

static char *custom_devnode(__cconst__ struct device *dev, umode_t *mode)
{
	if (!mode)
		return NULL;
	if (MAJOR(dev->devt) == MAJOR(start))
		*mode = 0666;
	return NULL;
}

int init_clipboard_module(void)
{
	int major; /* Major number assigned to our device driver */
	int minor; /* Minor number assigned to the associated character device */
	int ret;

	clipboard = (char *)vmalloc(BUFFER_LENGTH);
	if (!clipboard) {
		printk(KERN_INFO "Can't allocate clipboard memory");
		return -ENOMEM;
	}
	memset(clipboard, 0, BUFFER_LENGTH);

	sema_init(&clipboard_sem, 1);
	sema_init(&update_sem, 0);

	/* Get available (major,minor) range */
	if ((ret = alloc_chrdev_region(&start, 0, 1, DEVICE_NAME))) {
		printk(KERN_INFO "Can't allocate chrdev_region()");
		goto error_alloc_region;
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
	class->devnode = custom_devnode;

	/*Creating device*/
	device = device_create(class, NULL, start, NULL, DEVICE_NAME);
	if (IS_ERR(device)) {
		pr_err("Device_create failed\n");
		ret = PTR_ERR(device);
		goto error_device;
	}

	major = MAJOR(start);
	minor = MINOR(start);
	printk(KERN_INFO "I was assigned major number %d. To talk to\n", major);
	printk(KERN_INFO "the driver try to cat and echo to /dev/%s.\n",
	       DEVICE_NAME);
	printk(KERN_INFO "Remove the module when done.\n");
	printk(KERN_INFO "Clipboard-update: Module loaded.\n");
	return 0;

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
error_alloc_region:
	vfree(clipboard);
	return ret;
}

void exit_clipboard_module(void)
{
	if (device)
		device_unregister(device);
	if (class)
		class_destroy(class);

	/* Destroy chardev */
	if (chardev)
		cdev_del(chardev);

	/*
	 * Release major minor pair
	 */
	unregister_chrdev_region(start, 1);
	vfree(clipboard);
	printk(KERN_INFO "Clipboard-update: Module unloaded.\n");
}

module_init(init_clipboard_module);
module_exit(exit_clipboard_module);
