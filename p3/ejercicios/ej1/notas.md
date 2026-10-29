# Ejercicio 1
## 1. ¿Cuál es la ruta (path) del fichero de dispositivo que se crea automáticamente al cargar el módulo?
Automáticamente no se crea ningún fichero de dispositivo. Únicamente se registra el rango Major/Minor y se registra el cdev.
Aun así la salida del programa propone crear el fichero `/dev/chardev` por medio de `mknod`.

## 2. ¿En qué fichero especial del sistema puede consultarse el major number asignado al driver correspondiente?
/proc/devices

## 3. ¿Qué minor number tiene asociado cada uno de los ficheros especiales creados al cargar los módulos del kernel? ¿Qué llamada del driver se encarga de asignar ese minor number?
0, ya que se reserva 1 minor empezando en 0 en la línea
```c
    if ((ret = alloc_chrdev_region (&start, 0, 1, DEVICE_NAME))) {
```

## 4. ¿Qué sucede al escribir en el fichero especial de dispositivo (p.ej., echo hello > /dev/chardev)? ¿Por que se produce este comportamiento?
Se escribe en los logs del kernel `Sorry, this operation isn't supported`, según el `printk` de `device_write`. Además el usuario ve un mensaje de error de operación no permitida `-EPERM`.
