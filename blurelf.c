#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <elf.h>
#include <string.h>

#define SQRT_2PI 2.5066282746310002

int usage(const char *prog);
double gaussian(double x, double sigma);
int apply_gaussian(int i, const uint8_t *data, size_t size, double sigma);

int main(int argc, char **argv) {
    struct stat st              = {0};
    const char  *in_path        = NULL;
    const char  *out_path       = NULL;
    const char  *section        = ".text";
    FILE        *in             = NULL;
    FILE        *out            = NULL;
    Elf64_Ehdr  *ehdr           = NULL;
    Elf64_Shdr  *shdr           = NULL;
    uint8_t     *data           = NULL; 
    uint8_t     *blured_section = NULL;
    double      deviation       = 1.8;
    
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'o') {
                if (argc < i) {
                    return usage(argv[0]);
                }
                out_path = argv[++i];
                continue;
        }       
        if (argv[i][0] == '-' && argv[i][1] == 'S') {
                if (argc < i) {
                    return usage(argv[0]);
                }
                section = argv[++i];
                continue;
        }
        if (argv[i][0] == '-' && argv[i][1] == 's') {
                if (argc < i) {
                    return usage(argv[0]);
                }
                sscanf(argv[++i], "%lf", &deviation);
                continue;
        }
        in_path = argv[argc - 1];
    }

    if (out_path == NULL) {
        out_path = "a.out";
    }

    if (in_path == NULL) {
        return usage(argv[0]);
    }

    in = fopen(in_path, "rb");
    stat(in_path, &st);

    data = malloc(st.st_size);
    if (!data) {
        perror("malloc");
        return 1;
    }

    if (!fread(data, 1, st.st_size, in)) {
        perror("fread");
        return 1;
    }
    
    ehdr = (Elf64_Ehdr *)data;
    shdr = (Elf64_Shdr *)(data + ehdr->e_shoff);
    const char *shrstab = (char *)(data + shdr[ehdr->e_shstrndx].sh_offset);

    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (i == ehdr->e_shstrndx) {
            continue;
        }
        if (strcmp(shrstab + shdr[i].sh_name, section)) {
            continue;
        }

        blured_section = malloc(shdr[i].sh_size);
        for (int j = 0; j < (int)shdr[i].sh_size; j++) {
            blured_section[j] = apply_gaussian(j, data + shdr[i].sh_offset, shdr[i].sh_size, deviation);
        }

        memcpy(data + shdr[i].sh_offset, blured_section, shdr[i].sh_size);
        free(blured_section);
    }

    out = fopen(out_path, "wb");
    if (!out) {
        perror("fopen");
        return 1;
    }

    if (!fwrite(data, 1, st.st_size, out)) {
        perror("fwrite");
        return 1;
    }

    chmod(out_path, st.st_mode | S_IXUSR | S_IXGRP | S_IXOTH);
}

int apply_gaussian(int i, const uint8_t *data, size_t size, double sigma) {
    double result = 0;
    for (int j = 0; j < 10; j++) {
        int m1 = (i+j)%(int)size;
        int m2 = (int)(i-j) < 0 ? (int)size + (i-j) : (i-j);

        double g = gaussian(j, sigma);
        result += ((double)data[m1]) * g;
        result += ((double)data[m2]) * g;
    }

    return result > 255.0 ? 255 : (uint8_t)result;
}

double gaussian(double x, double sigma) {
    return exp(-x * x / (2.0 * sigma * sigma)) / (sigma * SQRT_2PI);
}

int usage(const char *prog) {
    fprintf(stderr, "Usage: %s [-a] [-o <output>] [-s <standard deviation>] <input>\n", prog);
    return 1;
}