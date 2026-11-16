# Ejercicio 3
## 1. ¿Qué sucede si, mientras un proceso está bloqueado en clipboard_read(), tecleamos CTRL + C en la misma terminal (enviando la señal SIGINT al proceso que ejecuta cat)? ¿Podría conseguirse el mismo comportamiento si se hubiera usado wait_event() en lugar de wait_event_interruptible() en la implementación? Justifica la respuesta.
Al recibir la señal wait_event_interruptible permite que se retome la ejecución y además devuelve un valor distinto a cero, por lo que se ejecuta el código dentro del if, avisando de que se ha interrumpido la lectura y finalizándola con un error.
Si se usase wait_event las señales se quedarían pendientes de ejecución y no se retomarían hasta volver al espacio de usuario.
## 2. ¿Garantiza la implementación exclusión mutua en el acceso a la variable clipboard? En caso de que no sea así propón una solución que garantice que la implementación sea SMP-safe.
No lo garantiza ya que no hay protección ante lectura o escritura sobre la variable clipboard. Podríamos usar un rw_semaphore rodeando los accesos a la variable tanto en el write como en el read.
