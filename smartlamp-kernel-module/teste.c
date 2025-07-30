#include <stdio.h>
#include <stdlib.h>

char *usb_out_buffer;

void teste(char *cmd, int param){
    sprintf(usb_out_buffer, "%s %d", cmd, param);
}

int main() {
    usb_out_buffer = malloc(100);

    teste("Mateus", 1);
    printf("%s\n", usb_out_buffer);
    return 0;
}