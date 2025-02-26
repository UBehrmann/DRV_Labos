#include <stdio.h>

int main() {
    unsigned int value = 0xBEEFCAFE;
    unsigned char *byte_ptr = (unsigned char *)&value;

    printf("Value in hex: 0x%X\n", value);
    printf("Bytes in memory:\n");

    for (int i = 0; i < sizeof(value); i++) {
        printf("Byte %d: 0x%X\n", i, byte_ptr[i]);
    }

    return 0;
}
