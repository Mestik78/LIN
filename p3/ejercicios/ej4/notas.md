# Ejercicio 4
## En ambos módulos del kernel están presentes las funciones clipboard_write() y clipboard_read() para gestionar las acciones de lectura y escritura sobre el fichero especial que representa el “clipboard” en cada caso. ¿Qué diferencias encuentras entre la implementación de estas funciones presentes tanto en Clipboard como en Clipboard-dev?
Las funciones son idénticas, ya que se están implementando para un archivo en ambos casos, así que el contexto en el que se ejecutan es equivalente.
