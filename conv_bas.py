import sys

def convert_bas(input_txt, output_bin):
    # Leer como UTF-8 para soportar tildes/ñ en comentarios sin crashear
    with open(input_txt, 'r', encoding='utf-8') as f:
        text = f.read().upper()

    # Filtrar SOLO caracteres ASCII válidos para BASIC (32-126 + saltos)
    # Esto elimina automáticamente tildes, ñ y símbolos no compatibles
    cleaned = []
    for char in text:
        code = ord(char)
        if code in (10, 13, 9):          # \n, \r, \t
            cleaned.append(char)
        elif 32 <= code <= 126:          # ASCII imprimible
            cleaned.append(char)
        # Todo lo demás (á, é, í, ó, ú, ñ, etc.) se ignora silenciosamente

    text = ''.join(cleaned)

    # Convertir saltos de línea a separadores internos (0x00)
    text = text.replace('\r\n', '\x00').replace('\n', '\x00').replace('\r', '\x00').replace('\t', ' ')

    # Eliminar líneas vacías accidentales
    while '\x00\x00' in text:
        text = text.replace('\x00\x00', '\x00')

    # Asegurar terminación nula obligatoria
    if not text.endswith('\x00'):
        text += '\x00'

    # Escribir binario limpio
    with open(output_bin, 'wb') as f:
        f.write(text.encode('ascii'))
    
    print(f"✅ Convertido: {len(text)} bytes → {output_bin}")
    print("ℹ️  Caracteres no ASCII (tildes, ñ) fueron ignorados automáticamente.")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Uso: python conv_bas.py programa.txt programa.bin")
    else:
        convert_bas(sys.argv[1], sys.argv[2])