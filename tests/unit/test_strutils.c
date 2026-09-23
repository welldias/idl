#include "idl_test.h"

static void test_strndup(void) {
    char *s = strutils_strndup("hello", 3);
    CHECK_STR(s, "hel");
    free(s);

    s = strutils_strndup("hi", 10); // para no fim da string
    CHECK_STR(s, "hi");
    free(s);

    CHECK(strutils_strndup("x", 0) == nullptr);
    CHECK(strutils_strndup(nullptr, 3) == nullptr);
}

static void test_format(void) {
    char *s = strutils_format("%s-%d-%c", "a", 42, 'z');
    CHECK_STR(s, "a-42-z");
    free(s);

    char big[3000];
    memset(big, 'x', sizeof(big) - 1);
    big[sizeof(big) - 1] = '\0';
    s = strutils_format("[%s]", big);
    CHECK_INT(strlen(s), sizeof(big) + 1);
    CHECK(s[0] == '[' && s[strlen(s) - 1] == ']');
    free(s);
}

static void test_cmp(void) {
    CHECK_INT(strutils_cmp("abc", 3, "abc", 3), 0);
    CHECK_INT(strutils_cmp("ab", 2, "abc", 3), -1);
    CHECK_INT(strutils_cmp("abc", 3, "ab", 2), 1);
    CHECK_INT(strutils_cmp("b", 1, "a", 1), 1);
    CHECK_INT(strutils_cmp("a", 1, "b", 1), -1);
}

static void test_trim(void) {
    char s1[] = "  a b \t\n";
    CHECK(strutils_trim(s1));
    CHECK_STR(s1, "a b");

    char s2[] = "   ";
    CHECK(strutils_trim(s2));
    CHECK_STR(s2, "");

    char s3[] = "";
    CHECK(strutils_trim(s3));
    CHECK_STR(s3, "");

    CHECK(!strutils_trim(nullptr));
}

static void test_spaces(void) {
    char s1[] = "a   b\t\tc";
    CHECK(strutils_one_space(s1));
    CHECK_STR(s1, "a b c");

    char s2[] = " a b\tc ";
    CHECK(strutils_no_space(s2));
    CHECK_STR(s2, "abc");
}

static void test_lshift(void) {
    char buf[16] = "xxxxxxxx";
    CHECK(strutils_lshift(buf, "abc", sizeof(buf)));
    CHECK_STR(buf, "abc");
    CHECK(!strutils_lshift(buf, "abc", 0));
    CHECK(!strutils_lshift(nullptr, "abc", 4));
}

static void test_conversions(void) {
    int i = 0;
    CHECK(strutils_str_toint("42", &i));
    CHECK_INT(i, 42);

    float f = 0;
    CHECK(strutils_str_tofloat("1.5", &f));
    CHECK(f > 1.49f && f < 1.51f);

    bool b = false;
    CHECK(strutils_str_tobool("true", &b));
    CHECK(b);
    CHECK(strutils_str_tobool("false", &b));
    CHECK(!b);
    CHECK(strutils_str_tobool("sim", &b));
    CHECK(!b);

    char *s = nullptr;
    CHECK(strutils_str_tostr("texto", &s));
    CHECK_STR(s, "texto");
    free(s);

    CHECK(!strutils_str_toint(nullptr, &i));
}

static void test_count_matches(void) {
    char s[] = "a,b,c";
    CHECK_INT(strutils_count_matches(s, ','), 2);
    CHECK_INT(strutils_count_matches(s, ';'), 0);
}

static void test_split(void) {
    char **parts = nullptr;
    word count = 0;
    CHECK(strutils_split("a,,b,c", ',', &parts, &count));
    CHECK_INT(count, 3);
    if (count == 3) {
        CHECK_STR(parts[0], "a");
        CHECK_STR(parts[1], "b");
        CHECK_STR(parts[2], "c");
    }
    for (word i = 0; i < count; i++)
        free(parts[i]);
    free(parts);

    CHECK(!strutils_split("", ',', &parts, &count));
}

/* Regressão: o último item apontava para memória já liberada. */
static void test_str_to_list(void) {
    const char *path = "/usr/bin: /bin ::/opt/last";
    list_t list = {0};
    list_init(&list, free);

    CHECK(strutils_str_to_list(path, strlen(path), ':', &list));
    CHECK_INT(list_count(&list), 3);
    CHECK(idl_test_list_has(&list, "/usr/bin"));
    CHECK(idl_test_list_has(&list, "/bin"));
    CHECK(idl_test_list_has(&list, "/opt/last"));
    list_clear(&list);

    CHECK(strutils_str_to_list("sozinho\n", 8, ':', &list));
    CHECK_INT(list_count(&list), 1);
    CHECK_STR((const char *)list.head->value, "sozinho");
    list_clear(&list);

    // Usado para dividir a saída do pkg-config.
    const char *flags = "-I/usr/include/x  -DFOO\n";
    CHECK(strutils_str_to_list(flags, strlen(flags), ' ', &list));
    CHECK_INT(list_count(&list), 2);
    CHECK(idl_test_list_has(&list, "-DFOO"));
    list_clear(&list);
}

int main(void) {
    RUN_TEST(test_strndup);
    RUN_TEST(test_format);
    RUN_TEST(test_cmp);
    RUN_TEST(test_trim);
    RUN_TEST(test_spaces);
    RUN_TEST(test_lshift);
    RUN_TEST(test_conversions);
    RUN_TEST(test_count_matches);
    RUN_TEST(test_split);
    RUN_TEST(test_str_to_list);
    return idl_test_report();
}
