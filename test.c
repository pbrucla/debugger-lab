#include <stdio.h>
#include <unistd.h>

unsigned long long int x = 0x1234567812345678ULL;

void h() {
    puts("h called");
}

void g() {
    char x[16];
    puts("g called");
    h();
    puts("g finished");
}

void f() {
    puts("f called");
    g();
    puts("f finished");
}

int main(void) {
    f();
    puts("done");
    return 0;
}
