# Prácticas - Arquitectura Interna de Linux y Android (LIN)

Repositorio de trabajo en equipo para el desarrollo de las prácticas de la asignatura.

## Estructura del repositorio

Para cada práctica (`p1`, `p2`, `p3`), la organización es la siguiente:

- `docs/`: Guiones oficiales de la práctica y documentación de apoyo.
- `upstream/`: Código base y ejemplos originales proporcionados por el profesor.
- `ejercicios/`: Ejercicios preparatorios previos al desarrollo principal.
- `entrega/`: Código fuente final de la práctica listo para entrega (cumple la estructura exigida en el guión).

## Flujo de trabajo

* **Compilación**: Ejecutar `make` en el directorio de trabajo habitual.
* **Generar entrega final (Campus Virtual)**:
  ```bash
  cd p1/entrega
  make clean
  tar -czvf ../p1_entrega.tar.gz *
  ```
