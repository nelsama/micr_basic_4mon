# Manual de Usuario: 6502 Tiny BASIC
**Versión:** 8.7  
**Plataforma:** FPGA Tang Nano 9K / Nexys (Arquitectura 6502)

Intérprete de BASIC minimalista para procesador 6502 implementado en FPGA.
Control directo de hardware (LEDs, memoria mapeada), gestión de archivos en SD
y lógica aritmética con variables enteras de 16 bits.

---

## 1. Especificaciones Técnicas

| Característica | Valor |
|:---|---:|
| **Memoria de programa** | 4500 bytes (~4.5 KB) |
| **Variables** | 26 globales (A-Z), entero 16 bits con signo |
| **Aritmética** | `+`, `-`, `*`, `/`, `%`, paréntesis |
| **Comparación** | `=`, `==`, `>`, `<` |
| **FOR anidados** | Hasta 8 niveles |
| **GOSUB anidados** | Hasta 8 niveles |
| **Hardware** | LEDs ($C001), memoria del sistema |
| **Almacenamiento** | SD Card via MicroFS (ROM API) |

---

## 2. Comandos de Sistema

| Comando | Descripción |
| :--- | :--- |
| `LIST` | Muestra todas las líneas del programa. |
| `RUN` | Ejecuta el programa desde la primera línea. |
| `NEW` | Borra el programa actual de la memoria. |
| `FREE` | Muestra los bytes libres para código BASIC. |
| `LOAD "ARCHIVO"` | Carga un programa `.bin` desde la tarjeta SD. |
| `SAVE "ARCHIVO"` | Guarda el programa actual en la tarjeta SD. |
| `QUIT` | Sale al monitor/ROM ($8000). |

---

## 3. Statements de Programación

### `PRINT`
Muestra texto o valores en la terminal.
* **Sintaxis:** `PRINT "TEXTO"` o `PRINT expresión`
* **Múltiples argumentos:** separar con `;` (sin salto) o `,` (espacio)
* **Ejemplo:** `10 PRINT "A = "; A , "B = "; B`

### `LET` (Opcional)
Asigna valor a variable. La palabra `LET` es opcional.
* **Sintaxis:** `LET X = 10` o `X = 10`

### `INPUT`
Solicita un número al usuario por teclado.
* **Sintaxis:** `INPUT Variable`
* **Ejemplo:** `20 INPUT B`

### `GOTO`
Salto incondicional a una línea específica.
* **Sintaxis:** `GOTO número_de_línea`

### `GOSUB` / `RETURN`
Salta a una subrutina y retorna. Hasta 8 niveles de anidamiento.
* **Sintaxis:** `GOSUB línea` / `RETURN`
* **Ejemplo:** `100 GOSUB 500: PRINT "OK"`

### `IF ... THEN`
Ejecución condicional.
* **Sintaxis:** `IF expr operador expr THEN comando`
* **Ejemplo:** `30 IF A > 10 THEN GOTO 100`

### `FOR` / `NEXT`
Bucle con variable de control. Hasta 8 anidados.
* **Sintaxis:** `FOR var = inicio TO fin` / `NEXT var`
* **Ejemplo:** `40 FOR I = 1 TO 10: PRINT I: NEXT I`

### `WAIT`
Pausa la ejecución en milisegundos.
* **Sintaxis:** `WAIT milisegundos`
* **Ejemplo:** `50 WAIT 500`

### `STOP`
Detiene la ejecución del programa.
* **Sintaxis:** `STOP`

### `REM`
Comentario. Todo lo que sigue en la línea se ignora.
* **Sintaxis:** `REM cualquier texto`

---

## 4. Comandos de Hardware y Memoria

### `LEDS`
Controla los LEDs de la placa.
* **Sintaxis:** `LEDS valor` (0-255).
* **Ejemplo:** `LEDS 255` (enciende todos), `LEDS 0` (apaga todos).

### `POKE`
Escribe un byte en una dirección de memoria.
* **Sintaxis:** `POKE dirección, valor`
* **Ejemplo:** `POKE 49153, 1` (LED 0 vía $C001).

### `PEEK()`
Lee un byte de memoria. Paréntesis obligatorios.
* **Sintaxis:** `variable = PEEK(dirección)`
* **Ejemplo:** `60 A = PEEK(49153)`

### `GET`
Lee un carácter del teclado sin esperar Enter. Si no hay tecla, retorna 0.
* **Sintaxis:** `GET variable`

---

## 5. Edición de Programas

* **Agregar línea:** `10 PRINT "HOLA"`
* **Reemplazar línea:** escribir el mismo número con nuevo contenido
* **Eliminar línea:** escribir solo el número: `10`
* **Programa vacío:** usar `NEW`

---

## 6. Almacenamiento en SD

El intérprete puede cargar y guardar programas en una tarjeta SD
mediante el sistema de archivos MicroFS (provisto por la ROM del monitor).

### Cargar un programa
```
LOAD "MI_PROGRAMA.BIN"
```
Carga el archivo binario desde la SD a la memoria del intérprete.

### Guardar un programa
```
SAVE "MI_PROGRAMA.BIN"
```
Guarda el programa actual en la SD en formato binario interno.

> **Nota:** Los archivos se guardan en formato raw (separador nulo entre líneas),
> que es el mismo formato interno del intérprete y el que espera LOAD.

---

## 7. Ejemplos

### Contador de LEDs
```basic
10 A = 0
20 LEDS A
30 PRINT "VALOR: "; A
40 A = A + 1
50 IF A = 256 THEN A = 0
60 WAIT 250
70 GOTO 20
```

### Sumatoria con FOR
```basic
10 S = 0
20 FOR I = 1 TO 10
30 S = S + I
40 PRINT I; ":"; S
50 NEXT I
```

---

## 8. Compilación

```
make        — Compilar el intérprete
make clean  — Limpiar archivos generados
make info   — Ver tamaño del binario
```

Requiere **CC65** (`cl65`, `ca65`, `ld65`) y **Python 3** (para conversión de programas BASIC).

### Cargar en el monitor 6502
```
LOAD BASIC.BIN 0800
R 0800
```
