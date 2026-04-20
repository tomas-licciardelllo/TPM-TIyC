#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>


void imprimirB(unsigned char c);
int* Hamming(int cadena[], int modulo, int limite, int *arr_out);
void introducir_errores(int *cadena_h, int modulo, int largo_total);
int* decodificarHamming(int cadena_H[], int modulo, int largo_total, int *largo_info);
void generarArchivoDEX(int *cadena_H, int modulo, int largo_total, char *nombre_salida);



void imprimirB(unsigned char c) {
    int i;
    for (i = 7; i >= 0; i--) {
        printf("%d", (c >> i) & 1);
    }
    printf("\n");
}

int* Hamming(int cadena[], int modulo, int limite, int *arr_out) {
    int *cadena_h = malloc(1 * sizeof(int));
    int n = 1, indice = 1, arr = 1, i = 1, contador = 1;
    int suma = 0, ind_H = 1, x, aux;

    cadena_h[0] = 0;


    while (contador < limite) {
        while (indice <= modulo && contador < limite) {
            arr++;
            cadena_h = realloc(cadena_h, arr * sizeof(int));
            if ((indice & (indice - 1)) == 0) {
                cadena_h[n] = 2; // Placeholder
            } else {
                cadena_h[n] = cadena[i];
                i++;
                contador++;
            }
            indice++;
            n++;
        }
        indice = 1;
    }

    contador = 1;
    ind_H = 1;
    while (contador < arr) {
        if ((contador & (contador - 1)) == 0) {
            suma = 0;

            for (x = 1; x < arr; x++) {
                if ((x & contador) && (x != contador)) {
                    suma ^= cadena_h[x];
                }
            }
            cadena_h[contador] = suma;
        }
        contador++;
    }

    *arr_out = arr;
    return cadena_h;
}

void introducir_errores(int *cadena_h, int modulo, int largo_total) {
    int i, posicion_relativa, posicion_absoluta;
    srand(time(NULL)); //semilla
    for (i = 1; i < largo_total; i += modulo)
        {
        if (rand() % 2 == 0) {
            posicion_relativa = rand() % modulo;
            posicion_absoluta = i + posicion_relativa; //como es un arreglo que tiene todos los bloques juntos, sumamos el inicio del bloque con la posicion elegida para saber que pos hay q cambiar
            if (posicion_absoluta < largo_total) { // este if evita acceder a una parte de la memoria q no exista , por las dudash
                printf("\n--- Error introducido en posicion: %d ---", posicion_absoluta);
                cadena_h[posicion_absoluta] ^= 1;
            }
        }
    }
}

int* decodificarHamming(int cadena_H[], int modulo, int largo_total, int *largo_info) {
    int p = 0, posicion_error = 0, i, j, pos_p, suma_xor, idx_info;
    int *informacion;

    while ((1 << p) < (modulo + 1)) p++; // la paridad breooo

    // Calculo del Síndrome , revisamos los bits de control
    for (i = 0; i < p; i++) {
        pos_p = (1 << i);
        suma_xor = 0;
        for (j = 1; j < largo_total; j++) {
            if (j & pos_p) suma_xor ^= cadena_H[j]; // la suma xor, si hay numero impar de 1, nos da != 0
        }
        if (suma_xor != 0) posicion_error += pos_p; // nos da la posicion de error
    }

    // Correccionn de error
    if (posicion_error > 0 && posicion_error < largo_total) {
        printf("\n¡Error detectado y corregido en la posicion: %d!", posicion_error);
        cadena_H[posicion_error] ^= 1;
    }
    //extraemos la info
    informacion = malloc(largo_total * sizeof(int));
    idx_info = 0;
    for (i = 1; i < largo_total; i++) {
        if ((i & (i - 1)) != 0) {
            informacion[idx_info++] = cadena_H[i];
        }
    }
    *largo_info = idx_info;
    return informacion;
}

void generarArchivoDEX(int *cadena_H, int modulo, int largo_total, char *nombre_salida) {
    int k, pos_en_bloque, bit_count;
    unsigned char caracter = 0;

   FILE *out = fopen(nombre_salida, "wb");
    if (out == NULL) return;

    bit_count = 0;
    for (k = 1; k < largo_total; k++) {
        pos_en_bloque = ((k - 1) % modulo) + 1; //este calculo sirve para calcular la pos relativa del bit dentro de su bloque de hamming actual
        if ((pos_en_bloque & (pos_en_bloque - 1)) != 0) //filtramos bits de info
            {
            if (cadena_H[k] == 1) caracter |= (1 << (7 - bit_count)); //Bitmasking ,Si el bit es un 1, lo "prendemos" en la posición correspondiente.
            bit_count++; // vamos contando para el bloque
            if (bit_count == 8) //tenemos un byte
            {
                fputc(caracter, out); //escribimos el caracter en el archivoooo
                caracter = 0;
                bit_count = 0;
            }
        }
    }
    fclose(out);
    printf("\nArchivo %s generado exitosamente.\n", nombre_salida);
}


int main() {
    int *chain = malloc(1 * sizeof(int));
    int i, n = 1, t, caracter, largo_h, largo_info;
    int *cadena_H, *info_recuperada;
    FILE *arch;

    arch = fopen("ej.txt", "rb");
    if (arch == NULL) {
        printf("Error: No se encontro ej.txt\n");
        return 1;
    }

    printf("Contenido del archivo (bits):\n");
    while ((caracter = fgetc(arch)) != EOF) {
        imprimirB((unsigned char)caracter);
        for (t = 7; t >= 0; t--) {
            chain = realloc(chain, (n + 1) * sizeof(int));
            chain[n] = ((unsigned char)caracter >> t) & 1;
            n++;
        }
    }
    fclose(arch);


    cadena_H = Hamming(chain, 8, n, &largo_h);

    printf("\nTrama Protegida: ");
    for(i = 1; i < largo_h; i++) printf("%d", cadena_H[i]);
    introducir_errores(cadena_H, 8, largo_h);
    generarArchivoDEX(cadena_H, 8, largo_h, "archivo.DEX");
  info_recuperada = decodificarHamming(cadena_H, 8, largo_h, &largo_info);

    printf("\nInformacion Recuperada: ");
    for(i = 0; i < largo_info; i++) printf("%d", info_recuperada[i]);

    free(chain);
    free(cadena_H);
    free(info_recuperada);

    return 0;
}
