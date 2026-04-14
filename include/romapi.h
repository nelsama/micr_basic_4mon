/**
 * ROMAPI.H - Header para acceder a funciones de la ROM
 * 
 * Este header permite a programas standalone llamar funciones
 * que están en la ROM del monitor sin incluir las librerías.
 * 
 * La ROM expone una Jump Table en $BF00 con las funciones comunes.
 * 
 * IMPORTANTE: Este archivo NO debe usarse en el monitor,
 * solo en programas que se cargan en RAM.
 */

#ifndef ROMAPI_H
#define ROMAPI_H

#include <stdint.h>

/* ===========================================================================
 * DIRECCIONES DE LA JUMP TABLE EN ROM ($BF00)
 * ===========================================================================
 * Cada entrada usa 3 bytes (JMP $xxxx)
 */

#define ROMAPI_BASE         0xBF00

/* SD Card */
#define ROMAPI_SD_INIT      0xBF00

/* MicroFS */
#define ROMAPI_MFS_MOUNT    0xBF03
#define ROMAPI_MFS_OPEN     0xBF06
#define ROMAPI_MFS_READ     0xBF09
#define ROMAPI_MFS_CLOSE    0xBF0C
#define ROMAPI_MFS_GET_SIZE 0xBF0F
#define ROMAPI_MFS_LIST     0xBF12

/* UART */
#define ROMAPI_UART_INIT    0xBF15
#define ROMAPI_UART_PUTC    0xBF18
#define ROMAPI_UART_GETC    0xBF1B
#define ROMAPI_UART_PUTS    0xBF1E
#define ROMAPI_UART_RX_READY 0xBF21
#define ROMAPI_UART_TX_READY 0xBF24

/* Funciones especiales para programas externos */
#define ROMAPI_MFS_READ_EXT 0xBF27  /* mfs_read con params en ZP $F0-$F3 */
#define ROMAPI_XMODEM_RECV  0xBF2A  /* xmodem_receive(dest_addr) */

/* Timer - Solo funciones esenciales */
#define ROMAPI_GET_MICROS   0xBF2D
#define ROMAPI_DELAY_US     0xBF30
#define ROMAPI_DELAY_MS     0xBF33

/* Magic y versión */
#define ROMAPI_MAGIC_ADDR   0xBF50
#define ROMAPI_MAGIC        "ROMAPI"

/* ===========================================================================
 * MACROS PARA LLAMAR FUNCIONES DE ROM (desde C)
 * ===========================================================================
 * Uso: 
 *   result = rom_sd_init();
 *   result = rom_mfs_mount();
 *   rom_uart_puts("Hola");
 */

/* SD Card */
#define rom_sd_init()       (((uint8_t (*)(void))ROMAPI_SD_INIT)())

/* MicroFS */
#define rom_mfs_mount()     (((uint8_t (*)(void))ROMAPI_MFS_MOUNT)())
#define rom_mfs_open(name)  (((uint8_t (*)(const char*))ROMAPI_MFS_OPEN)(name))
#define rom_mfs_read(buf,len) (((uint16_t (*)(void*, uint16_t))ROMAPI_MFS_READ)(buf, len))
#define rom_mfs_close()     (((void (*)(void))ROMAPI_MFS_CLOSE)())
#define rom_mfs_get_size()  (((uint16_t (*)(void))ROMAPI_MFS_GET_SIZE)())

/* Definir struct para mfs_list si se necesita */
typedef struct {
    char     name[12];
    uint16_t size;
    uint8_t  index;
} rom_mfs_fileinfo_t;

#define rom_mfs_list(idx,info) (((uint8_t (*)(uint8_t, rom_mfs_fileinfo_t*))ROMAPI_MFS_LIST)(idx, info))

/* UART */
#define rom_uart_init()     (((void (*)(void))ROMAPI_UART_INIT)())
#define rom_uart_putc(c)    (((void (*)(char))ROMAPI_UART_PUTC)(c))
#define rom_uart_getc()     (((char (*)(void))ROMAPI_UART_GETC)())
#define rom_uart_puts(s)    (((void (*)(const char*))ROMAPI_UART_PUTS)(s))
#define rom_uart_rx_ready() (((uint8_t (*)(void))ROMAPI_UART_RX_READY)())
#define rom_uart_tx_ready() (((uint8_t (*)(void))ROMAPI_UART_TX_READY)())

/* XMODEM */
#define rom_xmodem_receive(addr) (((int (*)(unsigned int))ROMAPI_XMODEM_RECV)(addr))

/* Timer - Solo funciones esenciales */
#define rom_get_micros()    (((uint32_t (*)(void))ROMAPI_GET_MICROS)())
#define rom_delay_us(us)    (((void (*)(uint16_t))ROMAPI_DELAY_US)(us))
#define rom_delay_ms(ms)    (((void (*)(uint16_t))ROMAPI_DELAY_MS)(ms))

/* ===========================================================================
 * EJEMPLOS DE USO
 * ===========================================================================
 * 
 * Delays:
 *   rom_delay_ms(100);        // Delay de 100ms
 *   rom_delay_us(500);        // Delay de 500 microsegundos
 * 
 * Medición de tiempo:
 *   uint32_t start = rom_get_micros();
 *   // ... código ...
 *   uint32_t elapsed = rom_get_micros() - start;
 * 
 * UART:
 *   rom_uart_puts("Hola mundo!\r\n");
 *   char c = rom_uart_getc();
 * 
 * SD Card + MicroFS:
 *   rom_sd_init();
 *   rom_mfs_mount();
 *   rom_mfs_open("FILE.TXT");
 *   rom_mfs_read(buffer, 256);
 *   rom_mfs_close();
 */

/* ===========================================================================
 * NOTA SOBRE mfs_read_ext ($BF27)
 * ===========================================================================
 * Para programas externos que tienen su propio software stack de CC65,
 * usar mfs_read_ext en lugar de mfs_read. Esta función recibe parámetros
 * en posiciones fijas de Zero Page en lugar del stack:
 * 
 *   $F0-$F1 = puntero al buffer destino
 *   $F2-$F3 = cantidad de bytes a leer
 *   Retorna: A/X = bytes leídos (uint16_t)
 * 
 * Esto evita conflictos cuando el programa y la ROM usan diferentes
 * posiciones de ZP para el software stack pointer (sp).
 * 
 * Ver examples/sidplayer para un ejemplo de uso con wrapper ASM.
 * ===========================================================================
 */

/* ===========================================================================
 * CÓDIGOS DE ERROR (compatibles con las librerías originales)
 * =========================================================================== */

/* SD Card */
#define SD_OK               0x00
#define SD_ERROR_TIMEOUT    0x01
#define SD_ERROR_CMD        0x02
#define SD_ERROR_INIT       0x03
#define SD_ERROR_READ       0x04
#define SD_ERROR_WRITE      0x05

/* MicroFS */
#define MFS_OK              0
#define MFS_ERR_DISK        1
#define MFS_ERR_NOFS        2
#define MFS_ERR_NOTFOUND    3
#define MFS_ERR_FULL        4
#define MFS_ERR_EXISTS      5

/* XMODEM */
#define XMODEM_ERROR_TIMEOUT   -1
#define XMODEM_ERROR_CANCELLED -2
#define XMODEM_ERROR_SYNC      -3
#define XMODEM_ERROR_CHECKSUM  -4

#endif /* ROMAPI_H */
