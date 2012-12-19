#include "konkret.h"

size_t KStrlcpy(char* dest, const char* src, size_t size)
{
    size_t i;

    for (i = 0; src[i] && i + 1 < size; i++)
        dest[i] = src[i];

    if (size > 0)
        dest[i] = '\0';

    while (src[i])
        i++;

    return i;
}

size_t KStrlcat(char* dest, const char* src, size_t size)
{
    size_t i;
    size_t j;

    for (i = 0; i < size && dest[i]; i++)
        ;

    if (i == size)
    {
        int j = 0;
        while (src[j])
            j++;

        return size + j;
    }

    for (j = 0; src[j] && i + 1 < size; i++, j++)
        dest[i] = src[j];

    dest[i] = '\0';

    while (src[j])
    {
        j++;
        i++;
    }

    return i;
}
