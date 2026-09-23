#include "strutils.h"

/* Copy each non-token argument into its own allocated space. */
static int strutils_split_copy(const char *string, char token, char **array, word count);

char *strutils_format(const char *fmt, ...) {
    RETURN_VAL_IF_FAIL(fmt, nullptr);

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);
    if (len < 0)
        return nullptr;

    char *result = (char *)malloc(len + 1);
    if (result) {
        va_start(args, fmt);
        vsnprintf(result, len + 1, fmt, args);
        va_end(args);
    }
    return result;
}

char *strutils_strndup(const char *str, int size) {
    if (str == nullptr || size <= 0)
        return nullptr;

    char *result = (char *)malloc(size + 1);
    if (result) {
        int n;
        for (n = 0; ((n < size) && (str[n] != 0)); n++)
            result[n] = str[n];
        result[n] = 0;
    }

    return result;
}

bool strutils_lshift(char *dst, const char *src, int size) {
    if (dst == nullptr || src == nullptr || size <= 0)
        return false;

    int i = 0;
    while (i++ != size && (*dst++ = *src++))
        ;

    return true;
}

int strutils_cmp(const char *a, word size_a, const char *b, word size_b) {
    RETURN_VAL_IF_FAIL(a, -1);
    RETURN_VAL_IF_FAIL(b, 1);

    register const unsigned char *s1 = (const unsigned char *)a;
    register const unsigned char *s2 = (const unsigned char *)b;

    int result = 0;
    uint32 min = MIN(size_a, size_b);

    while (min-- > 0) {
        if (*s1++ != *s2++) {
            result = s1[-1] < s2[-1] ? -1 : 1;
            break;
        }
    }

    return (result == 0 && size_a != size_b) ? (size_a > size_b ? 1 : -1) : result;
}

bool strutils_trim(char *str) {
    char *p1, *p2;
    int i;

    if (str == NULL)
        return false;

    p2 = str;
    while (isspace(*p2))
        if (*p2++ == 0)
            return false;

    p1 = strdup(p2);
    if (p1 == NULL)
        return false;

    for (i = strlen(p1); i > 0; i--)
        if (!isspace(p1[i - 1]))
            break;

    p1[i] = 0;

    word max_len = strlen(p1) + 1;

    memcpy(str, p1, max_len);

    free(p1);

    return true;
}

bool strutils_one_space(char *str) {
    char *p1, *p2;
    unsigned int i, len;
    int just_one;

    if (str == NULL)
        return false;

    len = strlen(str);
    p1  = (char *)malloc(len + 1);
    if (p1 == NULL)
        return false;

    just_one = 0;
    p2       = p1;

    for (i = 0; i < len; i++) {
        if (isspace(str[i])) {
            if (just_one)
                continue;

            just_one = 1;
            *p2++    = ' ';
        } else {
            just_one = 0;
            *p2++    = str[i];
        }
    }
    *p2 = 0;
    strcpy(str, p1);
    free(p1);

    return true;
}

bool strutils_no_space(char *str) {
    char *p1, *p2;
    unsigned int i, len;

    if (str == NULL)
        return false;

    len = strlen(str);
    p1  = (char *)malloc(len + 1);
    if (p1 == NULL)
        return false;

    p2 = p1;

    for (i = 0; i < len; i++) {
        if (!isblank(str[i])) {
            *p2++ = str[i];
        }
    }
    *p2 = 0;
    strcpy(str, p1);
    free(p1);

    return true;
}

bool strutils_str_tostr(const char *str, char **value) {
    if (str == NULL || value == NULL)
        return false;

    *value = strdup(str);
    return true;
}

bool strutils_str_toint(const char *str, int *value) {
    if (str == NULL || value == NULL)
        return false;

    *value = atoi(str);
    return true;
}

bool strutils_str_tofloat(const char *str, float *value) {
    if (str == NULL || value == NULL)
        return false;

    *value = atof(str);
    return true;
}

bool strutils_str_tobool(const char *str, bool *value) {
    if (str == NULL || value == NULL)
        return false;

    *value = (strcmp(str, "true") == 0) ? true : false;
    return true;
}

int strutils_count_matches(char *s, char c) {
    char *actual, *last;
    int count = 0;

    actual = strchr(s, c);
    last   = strrchr(s, c);

    if (actual == NULL)
        return count;

    count++;

    while (actual != last && actual != NULL) {
        actual = strchr(actual + 1, c);
        count++;
    }
    return count;
}

/* Count the number of arguments. */
static int strutils_split_count(const char *string, char token) {
    const char *p = NULL;
    int count     = 0;

    if (string == NULL || token == 0)
        return -1;

    p = string;
    while (*p) {
        // skip
        while (*p && *p == token)
            p++;

        // want
        if (*p && *p != token) {
            count++;
            // want
            while (*p && *p != token)
                p++;
        }
    }
    return count;
}

bool strutils_split(const char *string, char token, char ***array, word *count) {
    int c;
    char **a;

    if (string == NULL || token == 0 || array == NULL || count == NULL)
        return false;

    c = strutils_split_count(string, token);
    if (c == 0)
        return false;

    a = malloc(sizeof(char *) * c);
    if (a == NULL)
        return false;

    if (strutils_split_copy(string, token, a, c) == -1) {
        free(a);
        return false;
    }

    *count = c;
    *array = a;

    return true;
}

static int strutils_split_copy(const char *string, char token, char **array, word count) {
    int i = 0;
    const char *p;

    if (string == NULL || token == 0 || array == NULL || count == 0)
        return -1;

    p = string;
    while (*p) {
        // skip
        while (*p && *p == token)
            p++;

        // want
        if (*p && *p != token) {
            const char *end = p;
            char *copy;

            // want
            while (*end && *end != token)
                end++;

            copy = array[i] = malloc(end - p + 1);

            if (!array[i])
                return -1;

            // want
            while (*p && *p != token)
                *copy++ = *p++;

            *copy = 0;
            i++;
        }
    }

    if (i != count)
        return -1;

    return 0;
}

bool strutils_str_to_list(const char *buffer, word size, char separator, list_t *list) {
    RETURN_VAL_IF_FAIL(buffer, false);
    RETURN_VAL_IF_FAIL(size > 0, false);
    RETURN_VAL_IF_FAIL(list, false);

    char *buf = NULL;
    char *end, *begin;
    bool result = false;

    buf = strutils_strndup(buffer, size);
    if (buf == NULL) {
        goto parse_end;
    }

    begin = buf;
    while ((end = strchr(begin, separator))) {
        *end = 0;

        strutils_trim(begin);
        word len = strlen(begin);
        if (len > 0) {
            result = list_add(list, strutils_strndup(begin, len));
            if (!result)
                goto parse_end;
        }

        begin = ++end;
    }

    strutils_trim(begin);
    if (strlen(begin) > 0) {
        result = list_add(list, strutils_strndup(begin, strlen(begin)));
        if (!result)
            goto parse_end;
    }

parse_end:
    free(buf);
    return result;
}
