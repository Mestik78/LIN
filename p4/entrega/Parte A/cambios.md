# Documentación de Cambios — Parte A (`modlist` SMP-Safe)

Modificaciones realizadas sobre `modlist` para garantizar que sea `SMP-safe` e impedir la descarga del módulo mientras esté en uso.

---

## 1. Nuevas cabeceras del kernel

```c
#include <linux/spinlock.h>
#include <linux/uaccess.h>
```

- **`<linux/spinlock.h>`**: Para declarar, inicializar y operar con el cerrojo (`DEFINE_SPINLOCK`, `spin_lock()`, `spin_unlock()`).
- **`<linux/uaccess.h>`**: Para las funciones de transferencia entre espacio de usuario y kernel (`copy_from_user()`, `copy_to_user()`).

---

## 2. Declaración estática de la lista y del Spinlock

```c
static LIST_HEAD(mylist);
static DEFINE_SPINLOCK(sp);
```

- **`DEFINE_SPINLOCK(sp)`**: Declara e inicializa estáticamente el *spinlock* que protege el acceso concurrente a `mylist`.
- **`static LIST_HEAD(mylist)`**: Inicializa la lista en tiempo de compilación, sustituyendo `INIT_LIST_HEAD(&mylist)` en `init_mod()`.

---

## 3. Protección contra la descarga del módulo en uso

```c
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
```

Registro en `struct proc_ops`:

```c
static const struct proc_ops proc_entry_fops = {
    .proc_open = list_open,
    .proc_release = list_release,
    .proc_read = list_read,
    .proc_write = list_write,
};
```

- **Motivo del cambio**: Incrementa el contador de referencias al abrir `/proc/modlist` (`try_module_get`) y lo decrementa al cerrarlo (`module_put`), impidiendo que `rmmod` descargue el módulo mientras esté en uso.

---

## 4. Sincronización en `list_write()` sin funciones bloqueantes

Regla de los *spinlocks*: **no invocar funciones que puedan dormir** mientras se retiene el cerrojo.

### 4.1. Operación `add`

```c
new_item = kmalloc(sizeof(struct list_item), GFP_KERNEL);
if (!new_item) {
    printk(KERN_ERR "modlist: No queda memoria\n");
    return -ENOMEM;
}
new_item->data =
