#ifndef IDL_STREAM_H
#define IDL_STREAM_H

#include "commons.h"

typedef struct {
    word capacity;
    byte *pos;
    byte *data;
} stream_t;

bool stream_init(stream_t *stream, word capacity);
word stream_get_position(stream_t *stream);
void stream_set_position(stream_t *stream, word position);
word stream_available(stream_t *stream);
word stream_write(stream_t *stream, const byte *data, word size);
word stream_write_string(stream_t *stream, const char *fmt, ...);
word stream_read(stream_t *stream, byte *data, word size);
void stream_reset(stream_t *stream);
void stream_clear(stream_t *stream);

word stream_get_size_and_reset_position(stream_t *stream);

#endif // IDL_STREAM_H