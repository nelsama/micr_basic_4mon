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
;; guardan/restauran el sp de la ROM ($000E) y pushean TODOS los
;; argumentos al stack de la ROM antes de llamar a la funcion.
;;
;; Las funciones ROM son C standard (NO fastcall) - todos los args
;; van en el software stack, orden right-to-left.
;;
;; ZP usage:
;;   $F4-$F5 = puntero (little-endian)
;;   $F6-$F7 = valor/tamano (little-endian)
;;
;; $F0-$F3 NO se usan (reservados para mfs_read_ext por la ROM API)
;;
;; ===========================================================================

.export _mfs_create_wrap
.export _mfs_write_wrap
.export _mfs_delete_wrap
.export _mfs_open_wrap


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
    ; Poner nombre en AX (fastcall - no necesita stack)
    lda $F4
    ldx $F5
    ; Llamar a mfs_open en ROM
    jsr $BF06
    ; Guardar valor de retorno
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
.segment "CODE"

_mfs_create_wrap:
    ; Guardar sp de la ROM ($000E) en HW stack
    lda $0E
    pha
    lda $0F
    pha

    ; Push arg2 (size) al stack ROM
    sec
    lda $0E
    sbc #2
    sta $0E
    lda $0F
    sbc #0
    sta $0F
    ldy #0
    lda $F6          ; size low byte
    sta ($0E),y
    iny
    lda $F7          ; size high byte
    sta ($0E),y

    ; Push arg1 (name) al stack ROM
    sec
    lda $0E
    sbc #2
    sta $0E
    lda $0F
    sbc #0
    sta $0F
    ldy #0
    lda $F4          ; name low byte
    sta ($0E),y
    iny
    lda $F5          ; name high byte
    sta ($0E),y

    ; Llamar a mfs_create en ROM
    jsr $BF3C

    ; Guardar valor de retorno (A)
    sta $F6

    ; Restaurar sp de la ROM
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

    ; Push arg1 (buf): la funcion espera buf en stack
    sec
    lda $0E
    sbc #2
    sta $0E
    lda $0F
    sbc #0
    sta $0F
    ldy #0
    lda $F4          ; buf low byte
    sta ($0E),y
    iny
    lda $F5          ; buf high byte
    sta ($0E),y

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
