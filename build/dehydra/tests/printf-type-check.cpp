#include <cstdio>
#include <cwchar>

extern void foo(const char*, ...) __attribute__((user("format, a, printf, 1, 2")));
extern void bar(const char*, ...) __attribute__((format(printf, 1, 2)));
extern void baz(const char*, ...);
extern void qux(const char*);

int main() {
    const char* s = "%s";
    char buf[256];
    typedef int i32;
    typedef unsigned char u8;

    i32 n = 2;
    printf("%d\n", 123);
    printf("%z\n", 123);
    printf("%d\n", (unsigned long)123);
    printf("%lu\n", (long)123);
    printf("%d%%\n", 123.0);
    printf("%d %+02.7x\n", 123, 456);
    foo("%s", 1);
    foo("%s", n);
    foo("%d", (u8*)"x");
    foo("%s", (unsigned short)3);
    bar("%s", 1);
    baz("%s", 1);
    qux("%s");
    bar("xyz\0%s");
    printf(s, "x");
    printf("%s", "x");
    printf("%d%d", 1, n);
    printf("%s", L"x");
    sprintf(buf, "%s", "x");
    sprintf(buf, "%s", L"x");
    wprintf(L"%s", "x");
    wprintf(L"%s", L"x");
    wprintf(L"%hs", "x");
    wprintf(L"%hs", L"x");
    wprintf(L"%ls", "x");
    wprintf(L"%ls", L"x");
    printf("%.*ls", 1, L"xy");
    printf("%.*ls", L"z", L"xy");
    printf("%.*ls", L"xy");
}
