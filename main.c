#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "string.h"
void imprimirB(unsigned char c);
int* Hamming(int cadena[], int modulo, int limite, int *arr_out);
void introducir_errores(int *cadena_h, int modulo, int largo_total, int *errores);
int* decodificarHamming(int cadena_H[], int modulo, int largo_total, int *largo_info,int cantidad_de_caracteres_originales);
void generarArchivoDEX(int *cadena_H, int modulo, int largo_total, char *nombre_salida);
void guardarInfoRecuperada(int *info_recuperada, int largo_info, char *nombre_archivo);

void imprimirB(unsigned char c) {
    int i;
    for (i = 7; i >= 0; i--) printf("%d", (c >> i) & 1);
    printf("\n");
}

int* Hamming(int cadena[], int modulo, int limite, int* largo)
{
    int *cadena_h = malloc(1*sizeof(int));
    cadena_h[0]=0;
    int n=1;
    int indice = 1;
    int arr = 1;
    int i = 1;
    int contador = 1;
    int suma=0;
    int suma_total = 0;
    while(contador<limite)
    {
        //el contador marca por cual posicion dentro del arreglo original estamos para no salirnos de el mismo
        while(indice<=modulo)
        {
            //el indice marca en que parte estamos escribiendo del modulo para no tener que sobrepisar un modulo
            if((indice & (indice - 1))== 0)
            {
                //en este bloque queremos saber si es potencia de 2 la posicion en la estamos para poner un 2, es un señuelo para decir que es una posicion de paridad
                arr++;
                cadena_h = realloc(cadena_h, arr * sizeof(int));
                cadena_h[n] = 2;
            }
            else
            {
                //aqui se pone el bit que debe ir de la cadena original en la nueva cadena donde estan lo bits de contenido y de paridad
                arr++;
                cadena_h = realloc(cadena_h, arr * sizeof(int));
                if(contador >= limite){
                    cadena_h[n] = 0;
                }
                else
                {
                    cadena_h[n] = cadena[i];
                }
                i++;
                contador++;
            }
            indice++;
            n++;
        }
        indice=1;
    }
    int x = 1;
    int aux;
    contador = 1;
    int ind_H=1;
    int ind_p = 1;
    while(contador < arr)
    {
        //aca evitamos irnos del arreglo que ya posee los bits de datos y los de paridad sin tener el contenido adecuado en los bits de paridad
        while(indice < modulo)
        {
            //aca iteramos hasta completar el rellleno de todos los bits de paridad de un modulo
            if((indice & (indice - 1))== 0)
            {
                //evaluamos si el bit que estamos viendo es un bit de paridad, en el caso de serlo itearamos sobre el arreglo y buscamos los bits
                //que deben ser sumados para comprobar la paridad del bit
                n = indice;
                aux  = ind_H;
                while(n<modulo)
                {
                    //inicamos la iteracion
                    if(((indice & n) == indice) && ((n & (n-1))!= 0))
                    {
                        //comprobamos si es un bit que NO es potencia de 2 y es un bit adecuado para la suma de ese bit de paridad
                        suma += cadena_h[ind_H];
                    }
                    ind_H++;
                    n++;
                }
                ind_H = aux;
                if(suma % 2 == 0)
                {
                    //si la suma de la paridad es par no se le suma nada
                    cadena_h[ind_H] = 0;
                }
                else
                {
                    //si la suma de la paridad es impar se le suma uno para lograr la paridad necesaria
                    cadena_h[ind_H] = 1;
                }
                ind_p = ind_p * 2;
                suma = 0;
            }
            indice++;
            contador++;
            ind_H++;
        }
        ind_p = 1;
        x = (ind_H - modulo) + 1;
        while(x<ind_H)
        {
            suma_total+=cadena_h[x];
            x++;
        }
         if(suma_total % 2 == 0)
        {
            cadena_h[ind_H] = 0;
        }
        else
        {
            cadena_h[ind_H] = 1;
        }
        suma_total=0;
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
int* decodificarHamming(int cadena_H[], int modulo, int largo_total, int *largo_info, int cantidad_de_caracteres_originales) {
    int i, k, idx_info = 0;
    int tamano_bloque = modulo;

    // 1. Calculamos cuántos bits representan esos caracteres (1 char = 8 bits)
    int bits_objetivo = cantidad_de_caracteres_originales * 8;

    // 2. Optimizamos el malloc para el tamaño exacto solicitado
    int *informacion = malloc(bits_objetivo * sizeof(int));
    if (informacion == NULL) return NULL; // Control de error de memoria

    for (k = 1; k < largo_total; k += tamano_bloque) {
        // Si ya alcanzamos el total de bits, salimos del bucle de bloques
        if (idx_info >= bits_objetivo) break;

        int posicion_error_relativa = 0;
        int paridad_global_actual = 0;

        if (k + modulo - 1 > largo_total) break;

        // --- Lógica de detección de errores (se mantiene igual) ---
        for (i = 1; i <= modulo; i++) {
            int bit = (cadena_H[k + i - 1] >= 1) ? (cadena_H[k + i - 1] % 2) : 0;
            paridad_global_actual ^= bit;
            if (i < modulo && bit == 1) {
                posicion_error_relativa ^= i;
            }
        }

        // --- Lógica de corrección (se mantiene igual) ---
        if (posicion_error_relativa == 0 && paridad_global_actual == 0) {
            // OK
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

        // --- Extracción de bits de información con límite ---
        for (i = 1; i < modulo; i++) {
            // Verificamos si es una posición de datos (no potencia de 2)
            if ((i & (i - 1)) != 0) {
                if ((k + i - 1) < largo_total) {
                    int bit_final = (cadena_H[k + i - 1] >= 1) ? (cadena_H[k + i - 1] % 2) : 0;
                    informacion[idx_info++] = bit_final;
                }
            }
            // 3. Salida temprana si completamos los bits del último carácter
            if (idx_info >= bits_objetivo) break;
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
    int contador_caracteres=0; // contamos la cantidad de caracteres para que despues cuando leamos el archivo encontremos la cantidad justa de letras.
    int caracteres_leidos=0;//contamos la cantidad de caracteres del texto para verificar que leimos la cantidad necesaria
    int i, n = 1, t, caracter, largo_h, largo_info, errores = 0;
    int *cadena_H, *info_recuperada;
    int opcion = 0;
    char nombre[100];
    int mod;
    int max_bits;
    int *chain;
    FILE *arch;
    int archivo_listo = 0;

    char sen;

    do {
        system("cls");
        // Mostramos las 5 opciones solicitadas
        printf("\n--- MENU DE OPCIONES ---\n");
        printf("1. Seleccionar un txt \n");
        printf("2. Ingresar el modulo para el proceso de Hamming\n");
        printf("3. Introducir errores en el txt\n");
        printf("4. Desproteger archivo sin corregir\n");
        printf("5. Desproteger archivo corrigiendo\n");
        printf("0. Salir\n");
        printf("Seleccione una opcion: ");

        // Capturamos la seleccion del usuario
        scanf("%d", &opcion);

        // Procesamos la opcion seleccionada
        switch (opcion) {
            case 1:
                printf("\n>> Has seleccionado la Opcion 1.\n");
                printf("Ingresa el nombre del archivo.\n");
                scanf("%s", &nombre);
                strcat(nombre,".txt");
                arch = fopen(nombre, "rb");
                if (!arch) {
                    printf("Error: No se encontro %s\n",nombre);
                    return 1;
                }

                /* OPTIMIZACION: Leemos el tamaño del archivo para pedir memoria una sola vez */
                fseek(arch, 0, SEEK_END);
                long file_size = ftell(arch);
                rewind(arch);

                max_bits = (file_size * 8) + 10;
                chain = malloc(max_bits * sizeof(int));

                printf("Procesando archivo de %ld bytes...\n", file_size);
                while ((caracter = fgetc(arch)) != EOF) {
                    for (t = 7; t >= 0; t--) {
                        chain[n++] = ((unsigned char)caracter >> t) & 1;
                    }
                }
                fclose(arch);
                archivo_listo = 1;
                fgetchar();
                printf("Presione cualquier letra para continuar\n");
                scanf("%c",&sen);
                break;
            case 2:
                if (!archivo_listo) {
                    printf("\n[!] Error: Primero debes cargar un archivo (Opcion 1).\n");
                    break;
                }
                printf("\n>> Has seleccionado la Opcion 2.\n");
                printf("Ingrese el modulo con el cual quiere aplicar el proceso de Hamming [8],[1024] o [16384]\n");
                scanf("%d", &mod);
                while((mod != 8) && (mod != 1024) && (mod != 16384))
                {
                    printf("ERROR, NO ES UN MODULO VALIDO.\n");
                    scanf("%d", &mod);

                }
                cadena_H = Hamming(chain, mod, n, &largo_h);
                fgetchar();
                printf("Presione cualquier letra para continuar\n");
                scanf("%c",&sen);
                break;
            case 3:
                if (!archivo_listo) {
                    printf("\n[!] Error: Primero debes cargar un archivo (Opcion 1).\n");
                    break;
                }
                printf("\n>> Has seleccionado la Opcion 3.\n");
                introducir_errores(cadena_H, mod, largo_h, &errores);
                printf("Se introdujeron %d errores.\n", errores);
                fgetchar();
                printf("Presione cualquier letra para continuar\n");
                scanf("%c",&sen);
                break;
            case 4:
                if (!archivo_listo) {
                    printf("\n[!] Error: Primero debes cargar un archivo (Opcion 1).\n");
                    break;
                }
                printf("\n>> Has seleccionado la Opcion 4.\n");
                generarArchivoDEX(cadena_H, mod, largo_h, "archivo_con_error.DEx");
                printf("Desprotegido correctamente y almacenado con el nombre archivo_con_error.DEx\n");
                fgetchar();
                printf("Presione cualquier letra para continuar\n");
                scanf("%c",&sen);
                break;
            case 5:
                if (!archivo_listo) {
                    printf("\n[!] Error: Primero debes cargar un archivo (Opcion 1).\n");
                    break;
                }
                printf("\n>> Has seleccionado la Opcion 5.\n");
                 info_recuperada = decodificarHamming(cadena_H, mod, largo_h, &largo_info,file_size);
                guardarInfoRecuperada(info_recuperada, largo_info, "recuperado.DCx");
                fgetchar();
                printf("Presione cualquier letra para continuar\n");
                scanf("%c",&sen);
                break;
            case 0:
                printf("\nSaliendo del programa...\n");
                free(info_recuperada);
                free(chain);
                free(cadena_H);
                break;
            default:
                printf("\n[!] Opcion no valida. Intente de nuevo.\n");
        }

    } while (opcion != 0);
/*
    arch = fopen("ej.txt", "rb");
    if (!arch) {
        printf("Error: No se encontro ej.txt\n");
        return 1;
    }

    // OPTIMIZACION: Leemos el tamaño del archivo para pedir memoria una sola vez
    fseek(arch, 0, SEEK_END);
    long file_size = ftell(arch);
    rewind(arch);

    //int max_bits = (file_size * 8) + 10;
    //int *chain = malloc(max_bits * sizeof(int));

    printf("Procesando archivo de %ld bytes...\n", file_size);
    while ((caracter = fgetc(arch)) != EOF) {
        for (t = 7; t >= 0; t--) {
            chain[n++] = ((unsigned char)caracter >> t) & 1;
        }
    }
    fclose(arch);

    cadena_H = Hamming(chain, mod, n, &largo_h);
    printf("Trama generada exitosamente. Total bits: %d\n", largo_h - 1);

    introducir_errores(cadena_H, mod, largo_h, &errores);
    printf("Se introdujeron %d errores.\n", errores);

    generarArchivoDEX(cadena_H, mod, largo_h, "archivo_con_error.txt");

    //guardarInfoRecuperada(cadena_H, largo_info, "recuperado.txt");
    info_recuperada = decodificarHamming(cadena_H, mod, largo_h, &largo_info,file_size);
    guardarInfoRecuperada(info_recuperada, largo_info, "recuperado.DCx");

    free(info_recuperada);
    free(chain);
    free(cadena_H);
    */

    return 0;
}
