#include "stream.h"

bool stream_init(stream_t *stream, word capacity) {
    RETURN_VAL_IF_FAIL(stream, false);

    stream_clear(stream);

    *stream = (stream_t){
        .capacity = capacity,
    };

    if (capacity > 0) {
        stream->pos = stream->data = (byte *)calloc(capacity, 1);
        if (stream->data == nullptr)
            return false;
    }

    return true;
}

word stream_get_position(stream_t *stream) {
    RETURN_VAL_IF_FAIL(stream, 0);

    return (stream->pos - stream->data);
}

void stream_set_position(stream_t *stream, word position) {
    RETURN_IF_FAIL(stream);
    RETURN_IF_FAIL(position < stream->capacity);

    stream->pos = stream->data + position;
}

word stream_available(stream_t *stream) {
    RETURN_VAL_IF_FAIL(stream, 0);

    return (word)(stream->capacity - (stream->pos - stream->data));
}

word stream_write(stream_t *stream, const byte *data, word size) {
    RETURN_VAL_IF_FAIL(stream, 0);
    RETURN_VAL_IF_FAIL(data, 0);
    RETURN_VAL_IF_FAIL(size > 0, 0);

    word available = stream_available(stream);
    if (available < size) {
        word position = stream_get_position(stream);
        word new_size = (size - available) + stream->capacity;
        stream->data  = (byte *)realloc(stream->data, new_size);
        if (stream->data == nullptr) {
            LOG_FATAL_NOT_ENOUGH_MEMORY();
            return 0;
        }

        stream->capacity = new_size;
        stream_set_position(stream, position);
    }

    memcpy(stream->pos, data, size);
    stream->pos += size;

    return (int)size;
}

word stream_write_string(stream_t *stream, const char *fmt, ...) {
    RETURN_VAL_IF_FAIL(stream, 0);
    RETURN_VAL_IF_FAIL(fmt, 0);

    va_list args;
    va_start(args, fmt);
    word result = 0;
    char str_aux[20];

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == 'd' || *fmt == 'i') {
                snprintf(str_aux, sizeof(str_aux), "%d", va_arg(args, int));
                result += stream_write(stream, (const byte *)str_aux, strlen(str_aux));
            } else if (*fmt == 'f') {
                snprintf(str_aux, sizeof(str_aux), "%.2f", (float)va_arg(args, double));
                result += stream_write(stream, (const byte *)str_aux, strlen(str_aux));
            } else if (*fmt == 'c') {
                char c = (char)va_arg(args, int);
                result += stream_write(stream, (const byte *)&c, 1);
            } else if (*fmt == 's') {
                const char *str = va_arg(args, char *);
                result += stream_write(stream, (const byte *)str, strlen(str));
            }
        } else {
            result += stream_write(stream, (const byte *)fmt, 1);
        }
        fmt++;
    }

    va_end(args);

    return result;
}

word stream_read(stream_t *stream, byte *data, word size) {
    RETURN_VAL_IF_FAIL(stream, 0);
    RETURN_VAL_IF_FAIL(data, 0);
    RETURN_VAL_IF_FAIL(size > 0, 0);

    size = MIN(size, stream_available(stream));
    memcpy(data, stream->pos, size);
    stream->pos += size;

    return size;
}

void stream_reset(stream_t *stream) {
    RETURN_IF_FAIL(stream);

    memset(stream->data, 0, stream->capacity);
    stream->pos = stream->data;
}

void stream_clear(stream_t *stream) {
    RETURN_IF_FAIL(stream);

    free(stream->data);
    *stream = (stream_t){ 0 };
}

word stream_get_size_and_reset_position(stream_t *stream) {
    RETURN_VAL_IF_FAIL(stream, 0);

    word size = stream_get_position(stream);
    stream_set_position(stream, 0);

    return size;
}