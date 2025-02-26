#include <stdio.h>

struct a {
    int b;
    char c;
};

struct {
    int b;
    char c;
} a;

int main() {
    struct a data;
    data.b = 42;
    data.c = 'A';

    a.b = 24;
    a.c = 'B';

    printf("Data1 - Number: %d, Character: %c\n", data.b, data.c);
    printf("Data2 - Number: %d, Character: %c\n", a.b, a.c);

    return 0;
}
