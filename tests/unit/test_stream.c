#include "idl_test.h"

static void test_write_grows(void) {
    stream_t stream = {0};
    CHECK(stream_init(&stream, 4));

    CHECK_INT(stream_write(&stream, (const byte *)"0123456789", 10), 10);
    CHECK_INT(stream_get_position(&stream), 10);
    CHECK(stream.capacity >= 10);
    CHECK(memcmp(stream.data, "0123456789", 10) == 0);

    stream_clear(&stream);
    CHECK(stream.data == nullptr);
    CHECK_INT(stream.capacity, 0);
}

static void test_read_and_position(void) {
    stream_t stream = {0};
    stream_init(&stream, 16);
    stream_write(&stream, (const byte *)"abcdef", 6);

    CHECK_INT(stream_get_size_and_reset_position(&stream), 6);
    CHECK_INT(stream_get_position(&stream), 0);

    byte buf[8] = {0};
    CHECK_INT(stream_read(&stream, buf, 4), 4);
    CHECK(memcmp(buf, "abcd", 4) == 0);
    CHECK_INT(stream_get_position(&stream), 4);
    CHECK_INT(stream_available(&stream), 12);

    stream_set_position(&stream, 1);
    CHECK_INT(stream_read(&stream, buf, 2), 2);
    CHECK(memcmp(buf, "bc", 2) == 0);

    stream_clear(&stream);
}

static void test_write_string(void) {
    stream_t stream = {0};
    stream_init(&stream, 8);

    stream_write_string(&stream, "n=%d s=%s c=%c f=%f", 7, "x", 'y', 1.5);
    stream_write(&stream, (const byte *)"", 1);
    CHECK_STR((const char *)stream.data, "n=7 s=x c=y f=1.50");

    stream_clear(&stream);
}

static void test_write_empty(void) {
    stream_t stream = {0};
    stream_init(&stream, 4);

    // Writing nothing is not an error: an empty %s is common.
    CHECK_INT(stream_write(&stream, (const byte *)"", 0), 0);
    CHECK_INT(stream_get_position(&stream), 0);
    stream_write_string(&stream, "[%s]", "");
    stream_write(&stream, (const byte *)"", 1);
    CHECK_STR((const char *)stream.data, "[]");
    stream_clear(&stream);
}

static void test_reset(void) {
    stream_t stream = {0};
    stream_init(&stream, 8);
    stream_write(&stream, (const byte *)"abc", 3);

    stream_reset(&stream);
    CHECK_INT(stream_get_position(&stream), 0);
    CHECK_INT(stream.data[0], 0);

    stream_clear(&stream);
}

int main(void) {
    RUN_TEST(test_write_grows);
    RUN_TEST(test_read_and_position);
    RUN_TEST(test_write_string);
    RUN_TEST(test_write_empty);
    RUN_TEST(test_reset);
    return idl_test_report();
}
