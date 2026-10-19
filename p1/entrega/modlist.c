#include "linux/gfp_types.h"
#include "linux/stddef.h"
#include "linux/types.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Práctica 1 LIN");
MODULE_AUTHOR("Miguel Platón y Alfonso Barriga");

static struct proc_dir_entry *proc_entry;

struct list_head mylist;
struct list_item {
	int data;
    struct list_head links;
};

struct list_head *lprinted;

static ssize_t list_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
    char kbuf[128];
    char command[64];
    int value;

    if (len > 99) len = 99;
    if (copy_from_user(kbuf, buf, len)) {
        return -EFAULT;
    }
    kbuf[len] = '\0';

    int num_items = sscanf(kbuf, "%s %d", command, &value);
    if (!strcmp(command, "add")) {
        if (num_items != 2) {
            printk(KERN_ERR "número de argumentos inválido\n");
        } else {
            struct list_item *new_item;
            new_item = kmalloc(sizeof(struct list_item), GFP_KERNEL);
            if (!new_item) {
                printk(KERN_ERR "No queda memoria\n");
                return -ENOMEM;
            }

            new_item->data = value;
            list_add(&new_item->links, &mylist);

            printk(KERN_INFO "añadido %d\n", value);
        }
    } else if (!strcmp(command, "remove")) {
        if (num_items != 2) {
            printk(KERN_ERR "número de argumentos inválido\n");
        } else {
            if (mylist.next == NULL) {
                printk(KERN_INFO "lista vacía\n");
                return 0;
            }
            struct list_item *item;
            struct list_item *tmp;
            list_for_each_entry_safe(item, tmp, &mylist, links) {
                if (item->data == value) {
                    list_del(&item->links);
                    kfree(item);
                }
            }
            printk(KERN_INFO "eliminado %d\n", value);
        }
    } else if (!strcmp(command, "cleanup")) {
        struct list_item *item;
        struct list_item *tmp;

        list_for_each_entry_safe(item, tmp, &mylist, links) {
            list_del(&item->links);
            kfree(item);
        }

        printk(KERN_INFO "lista eliminada\n");
    } else {
        printk(KERN_ERR "comando desconocido\n");
    }

    return len;
}

static ssize_t list_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
    printk(KERN_INFO "reading kernel");
    struct list_item *item;
    struct list_item *tmp;

    if (*off == 0) lprinted = mylist.next;
    if (lprinted == &mylist) return 0;

    char *out = kmalloc(len, GFP_KERNEL);
    if (!out) {
        printk(KERN_ERR "No queda memoria\n");
        return -ENOMEM;
    }
    
    int bytes_read = 0;
    struct list_head *curr;

    for (curr = lprinted; curr != &mylist; curr = curr->next) {
        struct list_item *item = list_entry(curr, struct list_item, links);
        int bytes_to_read = snprintf(NULL, 0, "%d\n", item->data);

        if (bytes_read + bytes_to_read > len)
            break;

        snprintf(out+bytes_read, len-bytes_read, "%d\n", item->data);
        bytes_read += bytes_to_read;
    }

    lprinted = curr;

    copy_to_user(buf, out, len);
    kfree(out);
    *off += bytes_read;
    return bytes_read;
}

static const struct proc_ops proc_entry_fops = {
    .proc_read = list_read,
    .proc_write = list_write,
};

static int init_mod(void)
{
    int ret = 0;
    INIT_LIST_HEAD(&mylist);

    proc_entry = proc_create("modlist", 0666, NULL, &proc_entry_fops);
    if (proc_entry == NULL) {
        ret = -ENOMEM;
        printk(KERN_INFO "Can't create /proc entry\n");
    } else {
        printk(KERN_INFO "P1 module loaded\n");
    }

    return ret;
}

static void exit_mod(void)
{
    remove_proc_entry("modlist", NULL);
    printk("P1 module unloaded\n");
}

module_init(init_mod);
module_exit(exit_mod);
