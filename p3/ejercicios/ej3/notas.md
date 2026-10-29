# Ejercicio 3
## 1. ¿Qué ventaja crees que tiene reemplazar las citadas variables globales por la estructura privada?
Permite guardar variables por dispositivo. Así pueden trabajar independientemente con variables privadas entre ellos.
## 2. ¿Que llamada de la API de Linux permite asociar en esta implementación la estructura privada al al struct device que crea el driver?
```c
    ddata->device = device_create(class, NULL, start, ddata, "%s%d", DEVICE_NAME, 0);
```
## 3. ¿Cómo es posible recuperar dicha estructura privada en las operaciones open(), read() y release()? Pista: La estructura struct file* que se pasa como parámetro a estas operaciones representa el fichero abierto sobre el que se invocan las operaciones desde espacio de usuario, y su campo private_data se utiliza en el driver para almacenar un puntero.
En `device_open` se hace
```c
ddata = dev_get_drvdata(device);
file->private_data = ddata;
```
así desde `device_read` y `device_release` se puede acceder a la estructura por medio de
```c
file->private_data = ddata;
```
