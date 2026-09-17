# Laboratorio 2: desempeño y caché

## Requisitos y compilación

Se necesita `g++` con soporte C++17. Para compilar:

```bash
make
```

## Ejecución

```bash
./lab2 loops 40000
./lab2 matrix 512 32 3
./lab2 single bloques_ijk 256 32
./lab2 single bloques_ikj 256 32
make experiment
```

El comando `matrix` recibe `N`, tamaño de bloque y repeticiones opcionales; compara la versión clásica, la bloqueada original `bloques_ijk` y la nueva `bloques_ikj`. Emite CSV y usa la mediana para amortiguar variaciones del sistema. `make experiment` deja los datos en `results/`.

## Cachegrind y KCachegrind

Instale Valgrind y KCachegrind. El perfilado se ejecuta manualmente para elegir el algoritmo, tamaño de matriz y bloque. Por ejemplo, para reproducir la comparación del informe ($n=512$, bloque $32$):

```bash
mkdir -p results
valgrind --tool=cachegrind --cachegrind-out-file=results/clasico_512.cg ./lab2 single clasico 512 32
valgrind --tool=cachegrind --cachegrind-out-file=results/bloques_ikj_512.cg ./lab2 single bloques_ikj 512 32
```

Para ver los contadores en la terminal:

```bash
cg_annotate results/clasico_512.cg
cg_annotate results/bloques_ikj_512.cg
```

Para abrir ambos perfiles y compararlos visualmente:

```bash
kcachegrind results/clasico_512.cg results/bloques_ikj_512.cg
```

Los campos principales son instrucciones (`Ir`), lecturas/escrituras de datos (`Dr`, `Dw`), fallos L1 de datos (`D1mr`, `D1mw`) y fallos de último nivel (`DLmr`, `DLmw`). Para perfilar IJK, cambie `bloques_ikj` por `bloques_ijk` y use otro nombre de archivo de salida.
