#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "romapi.h"

/* Ajuste a 7200 para solucionar el desbordamiento de 91 bytes */
#define PROG_SIZE   7200  
#define VAR_COUNT   26

/* --- Registros Hardware Nexys --- */
#define PORT_SALIDA_LED      (*(volatile uint8_t*)0xC001)
#define CONF_PORT_SALIDA_LED (*(volatile uint8_t*)0xC003)

static int  variables[VAR_COUNT];
static char program[PROG_SIZE];
static char *txtpos;
static char *current_line_ptr; 
static int  is_running_prog = 0; 
static int  running = 1;
static int  temp_wait;

/* --- Helpers E/S --- */
void out_char(char c) { if (c == '\n') rom_uart_putc('\r'); rom_uart_putc(c); }
void out_str(const char *s) { while (*s) out_char(*s++); }
void out_num(int n) {
    char b[7]; int i = 0;
    if (n == 0) { out_char('0'); return; }
    if (n < 0) { out_char('-'); n = -n; }
    while (n > 0) { b[i++] = (n % 10) + '0'; n /= 10; }
    while (i > 0) out_char(b[--i]);
}
char in_char(void) { while (!rom_uart_rx_ready()); return rom_uart_getc(); }
void do_wait(int ms) {
    temp_wait = ms;
    __asm__ ("ldy %v+1", temp_wait); 
    __asm__ ("lda %v", temp_wait);   
    __asm__ ("jsr $BF33"); 
}

/* --- Parser Aritmético Recursivo --- */
static void ignore_spaces(void) { while (*txtpos == ' ') txtpos++; }
static int expression(void);

static int factor(void) {
    int val = 0; int sign = 1;
    ignore_spaces();
    if (*txtpos == '-') { sign = -1; txtpos++; ignore_spaces(); }
    
    if (strncmp(txtpos, "PEEK", 4) == 0) {
        txtpos += 4; ignore_spaces();
        if (*txtpos != '(') {
            out_str("\r\nSYNTAX ERR: PEEK EXPECTS (\r\n");
            is_running_prog = 0; return 0;
        }
        txtpos++; val = expression(); ignore_spaces();
        if (*txtpos == ')') {
            txtpos++;
        } else {
            out_str("\r\nSYNTAX ERR: PEEK EXPECTS )\r\n");
            is_running_prog = 0; return 0;
        }
        return (int)(*(volatile uint8_t*)(uint16_t)val);
    }

    if (*txtpos == '(') {
        txtpos++; val = expression(); ignore_spaces();
        if (*txtpos == ')') txtpos++;
        return val * sign;
    }
    if (isdigit(*txtpos)) { 
        while (isdigit(*txtpos)) val = val * 10 + (*txtpos++ - '0'); 
    } else if (*txtpos >= 'A' && *txtpos <= 'Z') { 
        val = variables[*txtpos++ - 'A']; 
    }
    return val * sign;
}

static int term(void) {
    int t1 = factor(); ignore_spaces();
    while (*txtpos == '*' || *txtpos == '/' || *txtpos == '%') {
        char op = *txtpos++; int t2 = factor();
        if (op == '*') t1 *= t2;
        else if (op == '/') { if (t2 != 0) t1 /= t2; else out_str("DIV0 ERR\r\n"); }
        else if (op == '%') { if (t2 != 0) t1 %= t2; }
        ignore_spaces();
    }
    return t1;
}

static int expression(void) {
    int t1 = term(); ignore_spaces();
    while (*txtpos == '+' || *txtpos == '-') {
        char op = *txtpos++; int t2 = term();
        if (op == '+') t1 += t2; else t1 -= t2;
        ignore_spaces();
    }
    return t1;
}

/* --- Motor de Comandos --- */
void execute_statement(void) {
    int var_idx, res, v1, target, addr;
    char *p;
    ignore_spaces();
    if (*txtpos == '\0' || *txtpos == '\n' || *txtpos == '\r') return;

    if (strncmp(txtpos, "FREE", 4) == 0) {
        p = program; v1 = 0;
        while (*p) { v1 += (strlen(p) + 1); p += (strlen(p) + 1); }
        out_num(PROG_SIZE - v1); out_str(" BYTES FREE\r\n");
    }
    else if (strncmp(txtpos, "NEW", 3) == 0) { memset(program, 0, PROG_SIZE); out_str("OK\r\n"); }
    else if (strncmp(txtpos, "QUIT", 4) == 0) { out_str("BYE\r\n"); running = 0; __asm__ ("jmp $B000"); }
    else if (strncmp(txtpos, "PRINT", 5) == 0) {
        txtpos += 5; ignore_spaces();
        if (*txtpos == '"') {
            txtpos++; while (*txtpos && *txtpos != '"') out_char(*txtpos++);
            if (*txtpos == '"') txtpos++;
        } else out_num(expression());
        out_str("\r\n");
    }
    else if (strncmp(txtpos, "INPUT", 5) == 0) {
        char buf[16]; int idx = 0;
        txtpos += 5; ignore_spaces();
        if (*txtpos >= 'A' && *txtpos <= 'Z') {
            var_idx = *txtpos++ - 'A'; out_str("? ");
            while (1) {
                char c = in_char();
                if (c == '\r') { out_str("\r\n"); break; }
                if (c == 0x08 && idx > 0) { idx--; out_str("\b \b"); }
                else if ((isdigit(c) || (c == '-' && idx == 0)) && idx < 15) { buf[idx++] = c; out_char(c); }
            }
            buf[idx] = '\0'; variables[var_idx] = atoi(buf);
        }
    }
    else if (strncmp(txtpos, "LEDS", 4) == 0) { txtpos += 4; ignore_spaces(); PORT_SALIDA_LED = (uint8_t)expression(); }
    else if (strncmp(txtpos, "POKE", 4) == 0) {
        txtpos += 4; ignore_spaces();
        addr = (uint16_t)expression(); 
        ignore_spaces(); if (*txtpos == ',') txtpos++;
        v1 = expression();
        *(volatile uint8_t*)(uint16_t)addr = (uint8_t)v1;
    }
    else if (strncmp(txtpos, "WAIT", 4) == 0) { txtpos += 4; ignore_spaces(); do_wait(expression()); }
    else if (strncmp(txtpos, "GET", 3) == 0) {
        txtpos += 3; ignore_spaces();
        if (*txtpos >= 'A' && *txtpos <= 'Z') {
            var_idx = *txtpos++ - 'A';
            variables[var_idx] = rom_uart_rx_ready() ? (int)rom_uart_getc() : 0;
        }
    }
    else if (strncmp(txtpos, "IF", 2) == 0) {
        txtpos += 2; v1 = expression(); ignore_spaces();
        res = 0;
        if (txtpos[0] == '=' && txtpos[1] == '=') { txtpos += 2; res = (v1 == expression()); }
        else if (*txtpos == '=') { txtpos++; res = (v1 == expression()); }
        else if (*txtpos == '>') { txtpos++; res = (v1 > expression()); }
        else if (*txtpos == '<') { txtpos++; res = (v1 < expression()); }
        ignore_spaces();
        if (strncmp(txtpos, "THEN", 4) == 0) { 
            txtpos += 4; ignore_spaces();
            if (res) { execute_statement(); return; }
            else { while (*txtpos && *txtpos != '\r' && *txtpos != '\n') txtpos++; }
        }
    }
    else if (strncmp(txtpos, "GOTO", 4) == 0) {
        txtpos += 4; ignore_spaces();
        target = expression(); p = program;
        while (*p) {
            char *ps = p; while (*ps == ' ') ps++;
            if (isdigit(*ps) && atoi(ps) == target) { current_line_ptr = p; is_running_prog = 2; return; }
            p += strlen(p) + 1;
        }
        out_str("LINE NOT FOUND\r\n"); is_running_prog = 0;
    }
    else if (strncmp(txtpos, "LET", 3) == 0 || (*txtpos >= 'A' && *txtpos <= 'Z')) {
        if (strncmp(txtpos, "LET", 3) == 0) txtpos += 3;
        ignore_spaces();
        if (*txtpos >= 'A' && *txtpos <= 'Z') {
            var_idx = *txtpos++ - 'A'; ignore_spaces();
            if (*txtpos == '=') { txtpos++; variables[var_idx] = expression(); }
        }
    } 
    else { 
        out_str("UNKNOWN COMMAND: ");
        p = txtpos; for(v1=0; v1<10 && *p >= ' '; v1++) out_char(*p++); 
        out_str("\r\n"); is_running_prog = 0; 
    }
}

/* --- Gestión de Programa --- */
void add_line(int num, char *full_line) {
    char *p = program; char *insert_pos = NULL; char *abs_end, *next, *prog_end, *check;
    int line_len = (int)strlen(full_line) + 1;
    p = program;
    while (*p) {
        if (atoi(p) == num) {
            next = p + strlen(p) + 1; abs_end = program;
            while (*abs_end) abs_end += strlen(abs_end) + 1;
            abs_end++; memmove(p, next, (size_t)(abs_end - next));
            p = program; continue;
        }
        p += strlen(p) + 1;
    }
    check = full_line; while (isdigit(*check)) check++; while (*check == ' ') check++;
    if (*check == '\0') return;
    p = program;
    while (*p) { if (atoi(p) > num) { insert_pos = p; break; } p += strlen(p) + 1; }
    if (insert_pos == NULL) insert_pos = p;
    prog_end = program; while (*prog_end) prog_end += strlen(prog_end) + 1;
    if ((prog_end - program) + line_len >= PROG_SIZE) { out_str("MEM FULL\r\n"); return; }
    memmove(insert_pos + line_len, insert_pos, (size_t)(prog_end - insert_pos + 1));
    memcpy(insert_pos, full_line, (size_t)line_len);
}

int main(void) {
    char input_line[64]; uint8_t i;
    CONF_PORT_SALIDA_LED = 0xC0; PORT_SALIDA_LED = 0x00;
    memset(program, 0, PROG_SIZE);
    for(i=0; i<VAR_COUNT; i++) variables[i] = 0;
    running = 1;
    out_str("\r\n6502 TINY BASIC V3.5.6\r\nREADY\r\n");
    while (running) {
        out_str("> "); i = 0;
        while (1) {
            char c = in_char();
            if (c == '\r') { input_line[i] = '\0'; out_str("\r\n"); break; }
            if (c == 0x08 && i > 0) { i--; out_str("\b \b"); }
            else if (i < 63) { input_line[i++] = (char)toupper((int)c); out_char(input_line[i-1]); }
        }
        if (i == 0) continue;
        txtpos = input_line;
        if (isdigit(*txtpos)) add_line(atoi(txtpos), input_line);
        else if (strcmp(input_line, "RUN") == 0) {
            current_line_ptr = program; 
            while (*current_line_ptr && running) {
                is_running_prog = 1; txtpos = current_line_ptr;
                while (isdigit(*txtpos)) txtpos++;
                execute_statement();
                if (is_running_prog == 2) continue;
                if (is_running_prog == 0) break;
                current_line_ptr += strlen(current_line_ptr) + 1;
            }
            is_running_prog = 0;
        } else if (strcmp(input_line, "LIST") == 0) {
            char *scan = program;
            while (*scan) { out_str(scan); out_str("\r\n"); scan += strlen(scan) + 1; }
        } else execute_statement();
    }
    return 0;
}