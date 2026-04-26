#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void imprimirB(unsigned char c);
int* Hamming(int cadena[], int modulo, int limite, int *arr_out);
void introducir_errores(int *cadena_h, int modulo, int largo_total, int *errores);
int* decodificarHamming(int cadena_H[], int modulo, int largo_total, int *largo_info);
void generarArchivoDEX(int *cadena_H, int modulo, int largo_total, char *nombre_salida);
void guardarInfoRecuperada(int *info_recuperada, int largo_info, char *nombre_archivo);

void imprimirB(unsigned char c) {
    int i;
    for (i = 7; i >= 0; i--) printf("%d", (c >> i) & 1);
    printf("\n");
}

int* Hamming(int cadena[], int modulo, int limite, int* largo) {
    /* OPTIMIZACION: 4 bits de datos generan 8 de Hamming. Calculamos el maximo y pedimos memoria 1 sola vez */
    int max_size = ((limite / 4) + 2) * 8;
    int *cadena_h = malloc(max_size * sizeof(int));

    cadena_h[0] = 0;
    int n = 1, indice = 1, arr = 1, i = 1, contador = 1;
    int suma = 0, suma_total = 0;

    while(contador < limite) {
        while(indice <= modulo) {
            if((indice & (indice - 1)) == 0) {
                arr++;
                cadena_h[n] = 2;
            } else {
                arr++;
                if(contador >= limite) {
                    cadena_h[n] = 0;
                } else {
                    cadena_h[n] = cadena[i];
                }
                i++;
                contador++;
            }
            indice++;
            n++;
        }
        indice = 1;
    }

    int aux, ind_H = 1, ind_p = 1, x = 1;
    contador = 1;

    while(contador < arr) {
        while(indice < modulo) {
            if((indice & (indice - 1)) == 0) {
                n = indice;
                aux = ind_H;
                while(n < modulo) {
                    if(((indice & n) == indice) && ((n & (n - 1)) != 0)) {
                        suma += cadena_h[ind_H];
                    }
                    ind_H++;
                    n++;
                }
                ind_H = aux;
                cadena_h[ind_H] = (suma % 2 == 0) ? 0 : 1;
                ind_p = ind_p * 2;
                suma = 0;
            }
            indice++;
            contador++;
            ind_H++;
        }

        ind_p = 1;
        x = (ind_H - modulo) + 1;
        while(x < ind_H) {
            suma_total += cadena_h[x];
            x++;
        }
        cadena_h[ind_H] = (suma_total % 2 == 0) ? 0 : 1;
        suma_total = 0;
        indice = 1;
        ind_H++;
        contador++;
    }
    *largo = arr;
    return cadena_h;
}

void introducir_errores(int *cadena_h, int modulo, int largo_total, int *errores) {
    int i, posicion_relativa, posicion_absoluta;
    /* FIX: El bloque mide 'modulo' (8). Saltamos de a 8 para no desfasarnos. */
    int tamano_bloque_real = modulo;
    srand(time(NULL));

    for (i = 1; i < largo_total; i += tamano_bloque_real) {
        if (rand() % 2 == 0) {
            posicion_relativa = rand() % tamano_bloque_real;
            posicion_absoluta = i + posicion_relativa;
            if (posicion_absoluta < largo_total) {
                cadena_h[posicion_absoluta] ^= 1;
                (*errores)++;
            }
        }
    }
}

int* decodificarHamming(int cadena_H[], int modulo, int largo_total, int *largo_info) {
    int i, k, idx_info = 0;
    int tamano_bloque = modulo;
    int *informacion = malloc(largo_total * sizeof(int));

    for (k = 1; k < largo_total; k += tamano_bloque) {
        int posicion_error_relativa = 0;
        int paridad_global_actual = 0;

        if (k + modulo - 1 > largo_total) break;

        for (i = 1; i <= modulo; i++) {
            int bit = (cadena_H[k + i - 1] >= 1) ? (cadena_H[k + i - 1] % 2) : 0;
            paridad_global_actual ^= bit;
            if (i < modulo && bit == 1) {
                posicion_error_relativa ^= i;
            }
        }

        /* Silenciamos los printf masivos para que no colapse la consola con textos largos */
        if (posicion_error_relativa == 0 && paridad_global_actual == 0) {
            // Caso 1: OK. No imprimimos nada para ganar velocidad.
        }
        else if (posicion_error_relativa != 0 && paridad_global_actual != 0) {
            int pos_abs = k + posicion_error_relativa - 1;
            if (pos_abs < largo_total) cadena_H[pos_abs] ^= 1;
        }
        else if (posicion_error_relativa == 0 && paridad_global_actual != 0) {
            cadena_H[k + modulo - 1] ^= 1;
        }
        else {
            printf("\n--- Caso 4: Error doble detectado en bloque %d. Incorregible. ---", k);
        }

        for (i = 1; i < modulo; i++) {
            if ((i & (i - 1)) != 0) {
                if ((k + i - 1) < largo_total) {
                    int bit_final = (cadena_H[k + i - 1] >= 1) ? (cadena_H[k + i - 1] % 2) : 0;
                    informacion[idx_info++] = bit_final;
                }
            }
        }
    }

    *largo_info = idx_info;
    return informacion;
}

void generarArchivoDEX(int *cadena_H, int modulo, int largo_total, char *nombre_salida) {
    int k, pos_en_bloque, bit_count = 0;
    unsigned char caracter = 0;
    int tamano_bloque = modulo;
    FILE *out = fopen(nombre_salida, "wb");
    if (!out) return;

    for (k = 1; k < largo_total; k++) {
        pos_en_bloque = ((k - 1) % tamano_bloque) + 1;
        if ((pos_en_bloque & (pos_en_bloque - 1)) != 0) {
            if (cadena_H[k] == 1) caracter |= (1 << (7 - bit_count));
            bit_count++;
            if (bit_count == 8) {
                fputc(caracter, out);
                caracter = 0;
                bit_count = 0;
            }
        }
    }
    fclose(out);
}

void guardarInfoRecuperada(int *info_recuperada, int largo_info, char *nombre_archivo) {
    int i, bit_count = 0;
    unsigned char caracter = 0;
    FILE *arch = fopen(nombre_archivo, "wb");
    if (!arch) return;

    for (i = 0; i < largo_info; i++) {
        if (info_recuperada[i] == 1) caracter |= (1 << (7 - bit_count));
        bit_count++;
        if (bit_count == 8) {
            fputc(caracter, arch);
            caracter = 0;
            bit_count = 0;
        }
    }
    fclose(arch);
    printf("\nArchivo '%s' recuperado exitosamente.\n", nombre_archivo);
}

int main() {
    int i, n = 1, t, caracter, largo_h, largo_info, errores = 0;
    int *cadena_H, *info_recuperada;
    FILE *arch;

    arch = fopen("ej.txt", "rb");
    if (!arch) {
        printf("Error: No se encontro ej.txt\n");
        return 1;
    }

    /* OPTIMIZACION: Leemos el tamaño del archivo para pedir memoria una sola vez */
    fseek(arch, 0, SEEK_END);
    long file_size = ftell(arch);
    rewind(arch);

    int max_bits = (file_size * 8) + 10;
    int *chain = malloc(max_bits * sizeof(int));

    printf("Procesando archivo de %ld bytes...\n", file_size);
    while ((caracter = fgetc(arch)) != EOF) {
        for (t = 7; t >= 0; t--) {
            chain[n++] = ((unsigned char)caracter >> t) & 1;
        }
    }
    fclose(arch);

    cadena_H = Hamming(chain, 8, n, &largo_h);
    printf("Trama generada exitosamente. Total bits: %d\n", largo_h - 1);

    introducir_errores(cadena_H, 8, largo_h, &errores);
    printf("Se introdujeron %d errores.\n", errores);

    generarArchivoDEX(cadena_H, 8, largo_h, "archivo_con_error.txt");

    info_recuperada = decodificarHamming(cadena_H, 8, largo_h, &largo_info);
    guardarInfoRecuperada(info_recuperada, largo_info, "recuperado.txt");

    free(info_recuperada);
    free(chain);
    free(cadena_H);

    return 0;
}
