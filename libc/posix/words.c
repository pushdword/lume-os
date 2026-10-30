/* SPDX-License-Identifier: BSD-3-Clause */
#include <wordexp.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void wordfree(wordexp_t *words)
{
    if (!words || !words->we_wordv)
        return;
    for (size_t i = 0; i < words->we_wordc; ++i)
        free(words->we_wordv[words->we_offs + i]);
    free(words->we_wordv);
    words->we_wordv = NULL;
    words->we_wordc = 0;
}

int wordexp(const char *text, wordexp_t *words, int flags)
{
    if (flags & ~(WRDE_NOCMD | WRDE_DOOFFS))
        return WRDE_NOSYS;
    if (strpbrk(text, "$`*?[]{}~"))
        return WRDE_NOSYS;
    wordexp_t result = {.we_offs = flags & WRDE_DOOFFS ? words->we_offs : 0};
    if (result.we_offs > 1024)
        return WRDE_NOSPACE;
    result.we_wordv = calloc(result.we_offs + 2, sizeof(char *));
    if (!result.we_wordv)
        return WRDE_NOSPACE;
    while (*text)
    {
        while (isspace((unsigned char)*text))
            ++text;
        if (!*text)
            break;
        char *word = malloc(strlen(text) + 1);
        if (!word)
        {
            wordfree(&result);
            return WRDE_NOSPACE;
        }
        size_t used = 0;
        char quote = 0;
        while (*text && (quote || !isspace((unsigned char)*text)))
        {
            char c = *text++;
            if (!quote && strchr("|&;<>()\n", c))
            {
                free(word);
                wordfree(&result);
                return WRDE_BADCHAR;
            }
            if ((c == '\'' || c == '"') && (!quote || c == quote))
                quote = quote ? 0 : c;
            else if (c == '\\' && quote != '\'')
            {
                if (!*text)
                    quote = '\\';
                else
                    word[used++] = *text++;
            }
            else
                word[used++] = c;
        }
        if (quote)
        {
            free(word);
            wordfree(&result);
            return WRDE_SYNTAX;
        }
        word[used] = 0;
        char **vector =
            realloc(result.we_wordv, (result.we_offs + result.we_wordc + 2) * sizeof(char *));
        if (!vector)
        {
            free(word);
            wordfree(&result);
            return WRDE_NOSPACE;
        }
        result.we_wordv = vector;
        vector[result.we_offs + result.we_wordc++] = word;
        vector[result.we_offs + result.we_wordc] = NULL;
    }
    *words = result;
    return 0;
}
