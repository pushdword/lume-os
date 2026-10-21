#include <stdio.h>
#include <string.h>

static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int encode(FILE *input, FILE *output)
{
    unsigned column = 0;
    unsigned char bytes[3];
    size_t count;
    while ((count = fread(bytes, 1, sizeof(bytes), input)))
    {
        unsigned value = (unsigned)bytes[0] << 16;
        if (count > 1)
            value |= (unsigned)bytes[1] << 8;
        if (count > 2)
            value |= bytes[2];
        char encoded[4] = {alphabet[value >> 18], alphabet[(value >> 12) & 63],
                           count > 1 ? alphabet[(value >> 6) & 63] : '=',
                           count > 2 ? alphabet[value & 63] : '='};
        if (fwrite(encoded, 1, sizeof(encoded), output) != sizeof(encoded))
            return 1;
        column += 4;
        if (column == 76)
        {
            if (fputc('\n', output) == EOF)
                return 1;
            column = 0;
        }
    }
    return ferror(input) || (column && fputc('\n', output) == EOF);
}

static int decode(FILE *input, FILE *output)
{
    unsigned values[4], used = 0, finished = 0;
    int byte;
    while ((byte = fgetc(input)) != EOF)
    {
        if (byte == ' ' || byte == '\t' || byte == '\r' || byte == '\n')
            continue;
        if (finished)
            return 1;
        const char *symbol = byte ? strchr(alphabet, byte) : NULL;
        if (!symbol && byte != '=')
            return 1;
        values[used++] = symbol ? (unsigned)(symbol - alphabet) : 64;
        if (used != 4)
            continue;
        if (values[0] == 64 || values[1] == 64 ||
            (values[2] == 64 && (values[3] != 64 || (values[1] & 15))) ||
            (values[3] == 64 && values[2] != 64 && (values[2] & 3)))
            return 1;
        unsigned value = values[0] << 18 | values[1] << 12 |
                         (values[2] & 63) << 6 | (values[3] & 63);
        unsigned char bytes[3] = {(unsigned char)(value >> 16), (unsigned char)(value >> 8),
                                  (unsigned char)value};
        unsigned count = values[2] == 64 ? 1 : values[3] == 64 ? 2 : 3;
        if (fwrite(bytes, 1, count, output) != count)
            return 1;
        finished = count != 3;
        used = 0;
    }
    return ferror(input) || used != 0;
}

int app_main(unsigned argc, char **argv)
{
    unsigned decoding = argc > 1 && !strcmp(argv[1], "-d");
    unsigned first = decoding ? 2 : 1;
    if (argc < first + 1 || argc > first + 2)
    {
        fputs("usage: base64 [-d] INPUT [OUTPUT]\n", stderr);
        return 2;
    }
    FILE *input = fopen(argv[first], "rb");
    if (!input)
    {
        perror("base64");
        return 1;
    }
    FILE *output = argc == first + 2 ? fopen(argv[first + 1], "wb") : stdout;
    if (!output)
    {
        fclose(input);
        perror("base64");
        return 1;
    }
    int result = decoding ? decode(input, output) : encode(input, output);
    if (fclose(input))
        result = 1;
    if ((output == stdout ? fflush(output) : fclose(output)))
        result = 1;
    if (result)
        fputs("base64: invalid input or I/O error\n", stderr);
    return result;
}
