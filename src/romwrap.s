;; ===========================================================================
;; romwrap.s - Wrappers para ROM API que manejan conflicto de ZP sp
;; ===========================================================================
;;
;; PROBLEMA: El programa BASIC usa sp en ZP $0026, pero la ROM
;; (monitor) usa sp en ZP $000E. Al llamar funciones ROM que reciben
;; argumentos por stack software, la ROM lee desde $000E y encuentra
;; basura (los args estan en el stack del programa apuntado por $0026).
;;
;; SOLUCION: Estos wrappers reciben argumentos via ZP fija ($F4-$F7),
;; guardan/restauran el sp de la ROM ($000E) y pushean los argumentos
;; al stack de la ROM antes de llamar a la funcion.
;;
;; CONVENCIONES DE LLAMADA (cada funcion usa una distinta):
;;   mfs_create(name, size): push size al stack, name en AX
;;   mfs_write(buf, len):    push buf al stack, len en AX
;;   mfs_open(name):         fastcall (name en AX, no usa stack)
;;   mfs_delete(name):       fastcall (name en AX, no usa stack)
;;
;; La funcion hace pushax en su prologo, salvando AX al tope del stack.
;; Por eso solo se pushea UN argumento al stack; el otro va en AX.
;;
;; Optimizacion: Subrutina comun push_rom_sp para evitar codigo repetido.
;;
;; ZP usage:
;;   $F0-$F3 = usados por mfs_read_ext (ROM API)
;;   $F4-$F5 = WRAP_PTR (puntero, little-endian)
;;   $F6-$F7 = WRAP_VAL (valor/tamano, little-endian)
;;
;; ===========================================================================

.export _mfs_create_wrap
.export _mfs_write_wrap
.export _mfs_delete_wrap
.export _mfs_open_wrap

.segment "CODE"

; ===========================================================================
; push_rom_sp - Subrutina para pushear AX al stack de la ROM ($000E)
; ===========================================================================
; Entrada: AX = valor de 2 bytes a pushear (little-endian, A=lo, X=hi)
; Efecto:  Decrementa $000E en 2 y almacena AX ahi
; ===========================================================================
push_rom_sp:
    pha                    ; guardar lo byte en HW stack temporal
    sec
    lda $0E
    sbc #2
    sta $0E
    lda $0F
    sbc #0
    sta $0F
    ldy #0
    pla                    ; recuperar lo byte
    sta ($0E),y
    iny
    txa                    ; hi byte desde X
    sta ($0E),y
    rts

; ===========================================================================
; mfs_open_wrap - Abre un archivo en MicroFS
; ===========================================================================
; Entrada: $F4-$F5 = puntero a nombre de archivo
; Salida:  A = 0 (exito) o codigo de error
;
; Llamada: JSR $BF06 (_mfs_open)
;   _mfs_open(const char *name) - fastcall: nombre en AX
; ===========================================================================
_mfs_open_wrap:
    ; Poner nombre en AX (fastcall - no necesita stack ni save $0E)
    lda $F4
    ldx $F5
    jsr $BF06
    sta $F6
    rts

; ===========================================================================
; mfs_create_wrap - Crea un archivo en MicroFS
; ===========================================================================
; Entrada: $F4-$F5 = puntero a nombre de archivo
;          $F6-$F7 = tamano del archivo
; Salida:  A = 0 (exito) o codigo de error
;
; Llamada: JSR $BF3C (_mfs_create)
;   _mfs_create(const char *name, unsigned int size) - C standard calling conv.
;   Stack: [size_lo, size_hi, name_lo, name_hi] (right-to-left push order)
; ===========================================================================
_mfs_create_wrap:
    ; Guardar sp de la ROM ($000E) en HW stack
    lda $0E
    pha
    lda $0F
    pha

    ; Push arg2 (size) al stack ROM usando subrutina comun
    lda $F6
    ldx $F7
    jsr push_rom_sp

    ; Push arg1 (name) al stack ROM usando subrutina comun
    lda $F4
    ldx $F5
    jsr push_rom_sp

    ; Llamar a mfs_create en ROM
    jsr $BF3C

    ; Guardar valor de retorno (A) y restaurar sp ROM
    sta $F6
    pla
    sta $0F
    pla
    sta $0E

    ; Retornar A
    lda $F6
    rts

; ===========================================================================
; mfs_write_wrap - Escribe datos a un archivo en MicroFS
; ===========================================================================
; Entrada: $F4-$F5 = puntero al buffer de datos
;          $F6-$F7 = cantidad de bytes a escribir
; Salida:  A/X = bytes escritos (uint16_t)
;
; Llamada: JSR $BF3F (_mfs_write)
;   CONVENCION (desde monitor): push arg1 (buf), AX = arg2 (len)
;   La funcion hara pushax para salvar AX (len) al tope del stack
; ===========================================================================
_mfs_write_wrap:
    ; Guardar sp de la ROM ($000E)
    lda $0E
    pha
    lda $0F
    pha

    ; Push arg1 (buf) usando subrutina comun
    lda $F4
    ldx $F5
    jsr push_rom_sp

    ; AX = arg2 (len): la funcion espera len en AX
    lda $F6
    ldx $F7

    ; Llamar a mfs_write en ROM
    jsr $BF3F

    ; Guardar valor de retorno (A/X = bytes escritos)
    sta $F6
    stx $F7

    ; Restaurar sp de la ROM
    pla
    sta $0F
    pla
    sta $0E

    ; Retornar A/X
    lda $F6
    ldx $F7
    rts

; ===========================================================================
; mfs_delete_wrap - Elimina un archivo en MicroFS
; ===========================================================================
; Entrada: $F4-$F5 = puntero a nombre de archivo
; Salida:  A = 0 (exito) o codigo de error
;
; Llamada: JSR $BF42 (_mfs_delete)
;   FASTCALL: nombre en AX, no necesita stack
; ===========================================================================
_mfs_delete_wrap:
    lda $F4
    ldx $F5
    jsr $BF42
    sta $F6
    rts
