# Plantilla para Proyectos en C - Monitor 6502

Esta es una **plantilla base** para crear programas en C para el **Monitor 6502** en la placa Tang Nano 9K. Proporciona la estructura básica, configuración de compilación y ejemplos para empezar rápidamente con el desarrollo en C para el procesador 6502.

## Características

- ✅ Programa en C usando **cc65** (compilador C para 6502)
- ✅ Usa **ROM API** para UART, delays y funciones del sistema
- ✅ Configuración optimizada para programas pequeños
- ✅ Código de inicio (startup.s) para inicializar runtime C
- ✅ Makefile completo para compilación automática
- ✅ Header de ROM API para acceso a funciones del monitor
- ✅ Estructura organizada para proyectos escalables

## Estructura del Proyecto

```
c-template/
├── src/
│   ├── main.c          # Código fuente principal (punto de entrada)
│   └── startup.s       # Código de inicio del runtime C
├── config/
│   └── programa.cfg    # Configuración del linker para CC65
├── include/
│   └── romapi.h        # Header para funciones ROM del monitor
├── build/              # Archivos objeto (generados automáticamente)
├── output/             # Binario final (generado automáticamente)
├── makefile            # Script de compilación
└── README.md           # Esta documentación
```

## Hardware Requerido

- **Tang Nano 9K** con Monitor 6502 v2.2.0+
- **SD Card** para transferir programas (opcional, se puede usar XMODEM)

## Software Requerido

- **CC65** instalado en `D:\cc65` (ajustar ruta en makefile si es necesario)
- **Monitor 6502 v2.2.0+** con ROM API en `$BF00`
- **SD Card** para transferir el programa (o usar XMODEM)

## Configuración Rápida

### 1. Clonar/Copiar la Plantilla
```bash
# Copiar esta carpeta completa como base para tu nuevo proyecto
cp -r c-template mi-proyecto
cd mi-proyecto
```

### 2. Configurar Nombre del Proyecto
Editar el archivo `makefile` y cambiar:
```makefile
# Nombre del programa
PROGRAM_NAME = mi-proyecto
```

### 3. Escribir tu Código
Editar `src/main.c` con tu programa:
```c
#include <stdint.h>
#include "romapi.h"

int main(void) {
    // Tu código aquí
    rom_uart_puts("Hola desde mi proyecto!\r\n");
    return 0;
}
```

### 4. Compilar
```bash
make
```

### 5. Cargar y Ejecutar
En el monitor 6502:
```
LOAD MI-PROYECTO_C 0800
R 0800
```

## Compilación

```bash
# Compilar el programa
make

# Limpiar archivos generados
make clean

# Ver tamaño del binario
make info

# Ver mapa de memoria
make map

# Mostrar ayuda
make help
```

## Uso

### Vía SD Card
1. Compilar con `make`
2. Copiar `output/mi-proyecto.bin` a la SD Card como `MI-PROYECTO_C`
3. En el monitor:
   ```
   SD                      ; Inicializar SD
   LOAD MI-PROYECTO_C      ; Cargar programa (default: $0800)
   R                       ; Ejecutar
   ```

### Vía XMODEM
```
XRECV                   ; Recibir via XMODEM (default: $0800)
R                       ; Ejecutar
```

## Ejemplos de Código

### Uso Básico de ROM API
```c
#include <stdint.h>
#include "romapi.h"

void uart_print(const char *s) {
    while (*s) rom_uart_putc(*s++);
}

int main(void) {
    uart_print("\r\n===================================\r\n");
    uart_print("  Mi Proyecto - Monitor 6502       \r\n");
    uart_print("===================================\r\n");
    
    // Delay de 1 segundo
    rom_delay_ms(1000);
    
    // Enviar más mensajes
    rom_uart_puts("Programa ejecutandose...\r\n");
    
    return 0;
}
```

### Control de LEDs (si tu hardware los tiene)
```c
#define LEDS (*(volatile uint8_t *)0xC001)   // LEDs (lógica negativa)

void blink_leds(void) {
    for (int i = 0; i < 5; i++) {
        LEDS = 0x00;            // Encender todos
        rom_delay_ms(200);
        LEDS = 0xFF;            // Apagar todos
        rom_delay_ms(200);
    }
}
```

### Lectura de Botones (ejemplo)
```c
#define BUTTONS (*(volatile uint8_t *)0xC002)   // Botones

void check_buttons(void) {
    uint8_t buttons = BUTTONS;
    if (buttons != 0xFF) {
        rom_uart_puts("Boton presionado!\r\n");
    }
}
```

## Mapa de Memoria

| Rango | Uso |
|-------|-----|
| `$0002-$001F` | Zero Page del Monitor (NO USAR) |
| `$0020-$007F` | Zero Page disponible para programas |
| `$0100-$01FF` | Stack del 6502 (compartido) |
| `$0200-$07FF` | BSS del Monitor (NO USAR) |
| `$0800-$3DFF` | RAM para programas (código, datos, BSS) |
| `$3E00-$3FFF` | Stack de CC65 (512 bytes) |
| `$C000-$C0FF` | Puertos de I/O |
| `$BF00-$BF2F` | ROM API (Jump Table) |

## ROM API Disponible

### Funciones Principales
```c
// UART
void rom_uart_putc(char c);         // Enviar carácter por UART
char rom_uart_getc(void);           // Leer carácter de UART
void rom_uart_puts(const char *s);  // Enviar string por UART
uint8_t rom_uart_rx_ready(void);    // Verificar si hay datos disponibles

// Delays
void rom_delay_ms(uint16_t ms);     // Delay en milisegundos
void rom_delay_us(uint16_t us);     // Delay en microsegundos
uint32_t rom_get_micros(void);      // Obtener tiempo en microsegundos

// SD Card y Sistema de Archivos
uint8_t rom_sd_init(void);          // Inicializar SD Card
uint8_t rom_mfs_mount(void);        // Montar sistema de archivos
uint8_t rom_mfs_open(const char *name); // Abrir archivo
uint16_t rom_mfs_read(void *buf, uint16_t len); // Leer archivo

// XMODEM
int rom_xmodem_receive(unsigned int addr); // Recibir archivo via XMODEM
```

Ver `include/romapi.h` para la lista completa de funciones.

## Agregar Librerías Externas

### 1. Agregar Directorio de Librería
En el `makefile`, agregar la ruta:
```makefile
# Librerías (agregar más aquí)
MI_LIB_DIR = ../../libs/mi-libreria

INCLUDES = -I$(MI_LIB_DIR)/include
```

### 2. Agregar Archivo Objeto
```makefile
# Archivos objeto adicionales
MI_LIB_OBJ = $(BUILD_DIR)/mi-libreria.o

OBJECTS = $(ASM_OBJECTS) $(C_OBJECTS) $(MI_LIB_OBJ)

# Regla de compilación para la librería
$(MI_LIB_OBJ): $(MI_LIB_DIR)/src/mi-libreria.c
	$(CC) -c $(CFLAGS) -o $@ $<
```

### 3. Incluir Header en tu Código
```c
#include "mi-libreria.h"
```

## Personalización Avanzada

### Cambiar Dirección de Carga
Editar `config/programa.cfg`:
```cfg
# Cambiar dirección de inicio
RAM: start = $1000, size = $2E00, type = rw, file = %O, define = yes;
```

### Aumentar Tamaño del Stack
```cfg
# Aumentar stack a 1KB
STACK: start = $3C00, size = $0400, type = rw, define = yes;
```

### Agregar Segmentos Personalizados
```cfg
# Agregar segmento para datos grandes
BIGDATA: load = RAM, type = rw, optional = yes;
```

## Resolución de Problemas

### Error: "CC65_HOME no encontrado"
Ajustar la ruta en el makefile:
```makefile
CC65_HOME = D:\tu-ruta\cc65
```

### Error: "Programa demasiado grande"
- Optimizar código con `-O` (ya está habilitado)
- Reducir uso de variables globales
- Usar `const` para datos de solo lectura
- Considerar usar ensamblador para código crítico

### Programa no se ejecuta
Verificar:
1. Dirección de carga correcta (`LOAD MI-PROYECTO_C 0800`)
2. Monitor 6502 v2.2.0+ instalado
3. ROM API disponible (verificar con `MEM $BF00`)

### LEDs no funcionan
Recordar que usan **lógica negativa**:
- `0x00` = todos encendidos
- `0xFF` = todos apagados
- `~0x55` = patrón alternado (01010101)

## Mejores Prácticas

### 1. Uso de Zero Page
- Usar solo `$20-$7F` para variables
- Nunca usar `$02-$1F` (reservado por monitor)
- Declarar variables frecuentes en ZP para mayor velocidad

### 2. Optimización de Código
- Usar tipos pequeños (`uint8_t`, `int8_t`)
- Evitar divisiones y multiplicaciones cuando sea posible
- Usar `const` para datos que no cambian
- Considerar inline assembly para código crítico

### 3. Manejo de Memoria
- Stack limitado (512 bytes por defecto)
- No usar recursión profunda
- Reutilizar buffers cuando sea posible
- Verificar tamaño con `make info`

### 4. Depuración
- Usar `rom_uart_puts()` para mensajes de depuración
- Verificar mapa de memoria con `make map`
- Probar en pequeños incrementos

## Ejemplos Completos Incluidos

### Hello World Básico
```c
#include <stdint.h>
#include "romapi.h"

int main(void) {
    rom_uart_puts("\r\nHello World from 6502!\r\n");
    return 0;
}
```

### Blink LEDs
```c
#include <stdint.h>
#include "romapi.h"

#define LEDS (*(volatile uint8_t *)0xC001)

int main(void) {
    rom_uart_puts("LED Blink Demo\r\n");
    
    while (1) {
        LEDS = 0x00;            // LEDs ON
        rom_delay_ms(500);
        LEDS = 0xFF;            // LEDs OFF
        rom_delay_ms(500);
        rom_uart_putc('.');     // Feedback
    }
    
    return 0;
}
```

### Lectura Serial Interactiva
```c
#include <stdint.h>
#include "romapi.h"

int main(void) {
    rom_uart_puts("Echo Server - Type characters\r\n");
    rom_uart_puts("Press ESC to exit\r\n\r\n");
    
    while (1) {
        if (rom_uart_rx_ready()) {
            char c = rom_uart_getc();
            
            if (c == 0x1B) {  // ESC
                rom_uart_puts("\r\nExiting...\r\n");
                break;
            }
            
            rom_uart_putc(c);  // Echo
        }
    }
    
    return 0;
}
```

## Próximos Pasos

1. **Explorar ROM API**: Ver todas las funciones disponibles en `include/romapi.h`
2. **Agregar Periféricos**: Conectar displays, sensores, etc.
3. **Optimizar**: Reducir tamaño y aumentar velocidad
4. **Crear Librerías**: Reutilizar código entre proyectos
5. **Integrar Ensamblador**: Para código crítico de tiempo real

## Recursos Adicionales

- [Documentación CC65](https://cc65.github.io/doc/)
- [Monitor 6502 Repository](https://github.com/...)
- [Ejemplos Avanzados](../../examples/)
- [Foro de la Comunidad](https://...)

## Licencia

Este proyecto está licenciado bajo la **GNU General Public License v3.0**.

---

**Nota**: Esta plantilla está diseñada para ser un punto de partida. Modifícala según las necesidades de tu proyecto específico. Para proyectos complejos, considera dividir el código en múltiples archivos y crear tu propio sistema de build.