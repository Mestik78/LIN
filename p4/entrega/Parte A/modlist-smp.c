#include "linux/gfp_types.h"
#include "linux/stddef.h"
#include "linux/types.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Practica 4 Parte A - modlist SMP-safe");
MODULE_AUTHOR("Miguel Platón y Alfonso Barriga");

static struct proc_dir_entry *proc_entry;

static LIST_HEAD(mylist);
static DEFINE_SPINLOCK(sp);

struct list_item {
	int data;
	struct list_head links;
};

static int list_open(struct inode *inode, struct file *file)
{
	if (!try_module_get(THIS_MODULE))
		return -ENODEV;
	return 0;
}

static int list_release(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
	return 0;
}

static ssize_t list_write(struct file *filp, const char __user *buf, size_t len,
			  loff_t *off)
{
	char kbuf[128];
	char command[64];
	int value;

	if (len > 127)
		len = 127;

	if (copy_from_user(kbuf, buf, len))
		return -EFAULT;

	kbuf[len] = '\0';

	int num_items = sscanf(kbuf, "%63s %d", command, &value);

	if (!strcmp(command, "add")) {
		struct list_item *new_item;

		if (num_items != 2) {
			printk(KERN_ERR "número de argumentos inválido\n");
			return -EINVAL;
		}

		new_item = kmalloc(sizeof(struct list_item), GFP_KERNEL);
		if (!new_item) {
			printk(KERN_ERR "No queda memoria\n");
			return -ENOMEM;
		}
		new_item->data = value;

		spin_lock(&sp);
		list_add(&new_item->links, &mylist);
		spin_unlock(&sp);

		printk(KERN_INFO "añadido %d\n", value);

	} else if (!strcmp(command, "remove")) {
		struct list_item *item, *tmp;
		LIST_HEAD(to_free);

		if (num_items != 2) {
			printk(KERN_ERR "número de argumentos inválido\n");
			return -EINVAL;
		}

		/* Mover los nodos lista local*/
		spin_lock(&sp);
		list_for_each_entry_safe(item, tmp, &mylist, links) {
			if (item->data == value) {
				list_move(&item->links, &to_free);
			}
		}
		spin_unlock(&sp);

		list_for_each_entry_safe(item, tmp, &to_free, links) {
			list_del(&item->links);
			kfree(item);
		}

		printk(KERN_INFO "eliminado %d\n", value);

	} else if (!strcmp(command, "cleanup")) {
		struct list_item *item;
		struct list_item *tmp;
		LIST_HEAD(to_free);

		/* Vaciar mylist */
		spin_lock(&sp);
		list_splice_init(&mylist, &to_free);
		spin_unlock(&sp);

		list_for_each_entry_safe(item, tmp, &to_free, links) {
			list_del(&item->links);
			kfree(item);
		}

		printk(KERN_INFO "lista eliminada\n");

	} else {
		printk(KERN_ERR "comando desconocido\n");
		return -EINVAL;
	}

	return len;
}

static ssize_t list_read(struct file *filp, char __user *buf, size_t len,
			 loff_t *off)
{
	int bytes_read = 0;
	struct list_item *item;

	if (*off > 0)
		return 0;

	char *out = kmalloc(len, GFP_KERNEL);
	if (!out) {
		printk(KERN_ERR "No queda memoria\n");
		return -ENOMEM;
	}

	spin_lock(&sp);
	list_for_each_entry(item, &mylist, links) {
		int bytes_to_read = snprintf(NULL, 0, "%d\n", item->data);

		if (bytes_read + bytes_to_read >= len)
			break;

		snprintf(out + bytes_read, len - bytes_read, "%d\n",
			 item->data);
		bytes_read += bytes_to_read;
	}
	spin_unlock(&sp);

	if (copy_to_user(buf, out, bytes_read)) {
		kfree(out);
		return -EFAULT;
	}

	kfree(out);
	*off += bytes_read;
	return bytes_read;
}

static const struct proc_ops proc_entry_fops = {
	.proc_open = list_open,
	.proc_release = list_release,
	.proc_read = list_read,
	.proc_write = list_write,
};

static int __init init_mod(void)
{
	proc_entry = proc_create("modlist-smp", 0666, NULL, &proc_entry_fops);
	if (proc_entry == NULL) {
		printk(KERN_ERR "Can't create /proc entry\n");
		return -ENOMEM;
	}

	printk(KERN_INFO "P2 Parte A module loaded\n");
	return 0;
}

static void __exit exit_mod(void)
{
	struct list_item *item, *tmp;

	remove_proc_entry("modlist-smp", NULL);

	list_for_each_entry_safe(item, tmp, &mylist, links) {
		list_del(&item->links);
		kfree(item);
	}

	printk(KERN_INFO "P4 Parte A module unloaded\n");
}

module_init(init_mod);
module_exit(exit_mod);
