#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>


void imprimirB(unsigned char c);
int* Hamming(int cadena[], int modulo, int limite, int *arr_out);
void introducir_errores(int *cadena_h, int modulo, int largo_total,int *errores);
int* decodificarHamming(int cadena_H[], int modulo, int largo_total, int *largo_info);
void generarArchivoDEX(int *cadena_H, int modulo, int largo_total, char *nombre_salida);



void imprimirB(unsigned char c) {
    int i;
    for (i = 7; i >= 0; i--) {
        printf("%d", (c >> i) & 1);
    }
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
    int cucu = 1;
    while(cucu < 17)
    {
        printf("%d",cadena_h[cucu]);

        cucu++;
    }
    printf("\n");
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
void introducir_errores(int *cadena_h, int modulo, int largo_total,int *errores) {
    int i, posicion_relativa, posicion_absoluta;
    int tamano_bloque_real = modulo + 1;
    srand(time(NULL)); //semilla
    for (i = 1; i < largo_total; i += tamano_bloque_real)
        {
        if (rand() % 2 == 0) {
            posicion_relativa = rand() % tamano_bloque_real;
            posicion_absoluta = i + posicion_relativa; //como es un arreglo que tiene todos los bloques juntos, sumamos el inicio del bloque con la posicion elegida para saber que pos hay q cambiar
            if (posicion_absoluta < largo_total) {// este if evita acceder a una parte de la memoria q no exista , por las dudash
                printf("\n--- Error introducido en posicion: %d ---", posicion_absoluta);
                cadena_h[posicion_absoluta] ^= 1;
                    *errores += 1;
                    }
        }
    }
}
int* decodificarHamming(int cadena_H[], int modulo, int largo_total, int *largo_info) {
    int i, k, idx_info;
    int tamano_bloque = modulo;
    int *informacion = malloc(largo_total * sizeof(int));
    idx_info = 0;

    for (k = 1; k < largo_total; k += tamano_bloque) {
        int posicion_error_relativa = 0;
        int paridad_global_actual = 0;

        /* Evitamos procesar bloques incompletos o basura de memoria */
        if (k + modulo - 1 > largo_total) break;


       //Sindrome
        for (i = 1; i <= modulo; i++) {
           //Esto es por los bits 2
            int bit = (cadena_H[k + i - 1] >= 1) ? (cadena_H[k + i - 1] % 2) : 0;
            paridad_global_actual ^= bit;
            if (i < modulo && bit == 1) {
                posicion_error_relativa ^= i;
            }
        }

        printf("\nBloque k=%d:" ,k);

        //Los 4 casos

        if (posicion_error_relativa == 0 && paridad_global_actual == 0) {
            printf("\n--- Caso 1: Bloque sin errores ---");
        }
        else if (posicion_error_relativa != 0 && paridad_global_actual != 0) {
            int pos_abs = k + posicion_error_relativa - 1;
            if (pos_abs < largo_total) {
                printf("\n--- Caso 2: Error simple corregido en pos absoluta: %d ---", pos_abs);
                cadena_H[pos_abs] ^= 1;
            }
        }
        else if (posicion_error_relativa == 0 && paridad_global_actual != 0) {
            printf("\n--- Caso 3: Error en Paridad corregido (pos %d) ---", k + modulo - 1);
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
    if (out == NULL) return;

    for (k = 1; k < largo_total; k++) {
        pos_en_bloque = ((k - 1) % tamano_bloque) + 1;
        if ((pos_en_bloque & (pos_en_bloque - 1)) != 0) //filtro todas las potencias de dos
            {

         //extraigo el bit, ignorando los placeholder que son 2
            if (cadena_H[k] == 1) {
                caracter |= (1 << (7 - bit_count));
            }
            bit_count++;

            /* Cuando juntamos 8 bits de datos, escribimos el byte */
            if (bit_count == 8) {
                fputc(caracter, out);
                caracter = 0;
                bit_count = 0;
            }
        }
    }
    fclose(out);
    printf("\nArchivo %s generado exitosamente.\n", nombre_salida);
}
void guardarInfoRecuperada(int *info_recuperada, int largo_info, char *nombre_archivo) {
    int i, bit_count;
    unsigned char caracter;
    FILE *arch = fopen(nombre_archivo, "wb");
    if (arch == NULL)
    {
        printf("Error al crear el archivo\n");
        return;
    }
    bit_count = 0;
    caracter = 0;
    for (i = 0; i < largo_info; i++)
    {
            if (info_recuperada[i] == 1)
                {
                caracter |= (1 << (7 - bit_count));
                }
        bit_count++;
                if (bit_count == 8)
                {
                fputc(caracter, arch);
                caracter = 0;
                bit_count = 0;
                }
    }
    fclose(arch);
    printf("\nArchivo '%s' generado exitosamente\n", nombre_archivo);
}
int main() {
    int *chain = malloc(1 * sizeof(int));
    int i, n = 1, t, caracter, largo_h, largo_info;
    int errores = 0;
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
    for(i = 1; i < largo_h; i++) {
        printf("%d", cadena_H[i]);
    }
    printf("\nLargo total de trama: %d bits", largo_h - 1);
    introducir_errores(cadena_H, 8, largo_h,&errores);
    printf("\nCadena luego de introducir errores\n");
    for(i = 1; i < largo_h; i++) {
        printf("%d", cadena_H[i]);
    }
    printf("\n Errores %d\n",errores);

    generarArchivoDEX(cadena_H, 8, largo_h, "archivo_con_error.DEX");
    info_recuperada = decodificarHamming(cadena_H, 8, largo_h, &largo_info);
    printf("\n---Informacion Recuperada: ----\n");
        for(i = 0; i < largo_info; i++)
            {
            printf("%d", info_recuperada[i]);
        }
        printf("\n");
        guardarInfoRecuperada(info_recuperada, largo_info, "recuperado.txt");
        free(info_recuperada);
    free(chain);
    free(cadena_H);

    return 0;
}
