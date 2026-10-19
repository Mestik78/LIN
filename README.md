# Prácticas - Arquitectura Interna de Linux y Android (LIN)

Repositorio de trabajo en equipo para el desarrollo de las prácticas de la asignatura.

## Estructura del repositorio

Para cada práctica (`p1`, `p2`, `p3`), la organización es la siguiente:

- `docs/`: Guiones oficiales de la práctica y documentación de apoyo.
- `upstream/`: Código base y ejemplos originales proporcionados por el profesor.
- `ejercicios/`: Ejercicios preparatorios previos al desarrollo principal.
- `entrega/`: Código fuente final de la práctica listo para entrega (cumple la estructura exigida en el guión).

## Flujo de trabajo

### Programación y ramas
#### Tipos de ramas
* **`main`**: Solo versiones terminadas y probadas listas para entregar (etiquetadas con `vX.0`). Nadie commitea directamente aquí.
* **`pX/dev`**: Rama compartida de integración para la práctica activa (`p1/dev`, `p2/dev`, etc.).
* **`feat/...`**: Ramas locales de corta duración para tareas específicas.

```text
main       ──●────────────────────────────────────────────────● (v1.0)
             ╰── p1/dev ──●─────────────●──●───────────────●──╯
                          ╰── feat-1 ───╯  ╰── feat-2 ─────╯
```

#### Ciclo de trabajo para una tarea
1. **Crear rama local desde la práctica activa:**
   ```bash
   git checkout p1/dev
   git pull
   git checkout -b feat/mi-tarea
   ```
2. **Desarrollar y commitear libremente**
3. **Integrar en `p1/dev` como un único commit:**
   ```bash
   git checkout p1/dev
   git merge --squash feat/mi-tarea
   git commit -m "feat(p1): descripción de la funcionalidad implementada"
   git push
   ```
4. **Borrar la rama local de la tarea:**
   ```bash
   git branch -D feat/mi-tarea
   ```

### Cierre y entrega de la práctica
La estructura de cada práctica es fiel a la de la entrega, así que solo hay que comprimir los archivos para entregarlos.
```bash
cd px/entrega
make clean
tar -czvf ../px_entrega.tar.gz *
```
Una vez finalizada y probada la práctica en `pX/dev`, se integra en `main` y se añade la etiqueta:
```bash
git checkout main
git pull origin main
git merge --no-ff p1/dev -m "release: Práctica 1 finalizada"
git tag -a v1.0 -m "Entrega Práctica 1"
git push origin main --tags
```
