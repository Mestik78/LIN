#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

#ifndef __NR_ledctl
#define __NR_ledctl 548
#endif

long ledctl(unsigned int leds)
{
    return syscall(__NR_ledctl, leds);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <mascara_leds>\n", argv[0]);
        return 1;
    }

    char *endptr;
    unsigned long val = strtoul(argv[1], &endptr, 0);
    if (*endptr != '\0') {
        fprintf(stderr, "Error: argumento inválido '%s'\n", argv[1]);
        return 1;
    }

    unsigned int leds = (unsigned int)val;
    long ret = ledctl(leds);

    if (ret < 0) {
        perror("Error en ledctl");
        return 1;
    }

    printf("syscall ledctl(0x%X) retornó: %ld\n", leds, ret);
    return 0;
}
