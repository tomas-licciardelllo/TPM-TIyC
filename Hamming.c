#include "huffman.h"

/*void imprimirB(unsigned char c) {
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
    // FIX: El bloque mide 'modulo' (8). Saltamos de a 8 para no desfasarnos.
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


void introducir_dos_errores(int *cadena_h, int modulo, int largo_total, int *errores) {
    srand(time(NULL));

    // 1. Calculamos la cantidad de bloques completos que hay en la cadena.
    // Restamos 1 porque el arreglo en tu implementación de Hamming empieza en el índice 1.
    int num_bloques = (largo_total - 1) / modulo;

    if (num_bloques == 0) {
        // Protección de seguridad por si el arreglo es más pequeño que un bloque.
        return;
    }

    // 2. Elegimos un bloque al azar.
    int bloque_elegido = rand() % num_bloques;

    // Calculamos en qué índice absoluto empieza ese bloque elegido.
    int inicio_bloque = 1 + (bloque_elegido * modulo);

    // 3. Elegimos la primera posición relativa (del 0 al modulo-1).
    int pos1 = rand() % modulo;
    int pos2;

    // 4. Elegimos la segunda posición relativa, asegurándonos de que no sea igual a la primera.
    // Si fueran iguales, el XOR la voltearía dos veces y dejaría el bit intacto (0 errores).
    do {
        pos2 = rand() % modulo;
    } while (pos1 == pos2);

    // 5. Calculamos las posiciones absolutas en el arreglo general.
    int abs1 = inicio_bloque + pos1;
    int abs2 = inicio_bloque + pos2;

    // 6. Introducimos los errores (invirtiendo el bit con XOR) y sumamos al contador.
    if (abs1 < largo_total && abs2 < largo_total) {
        cadena_h[abs1] ^= 1;
        cadena_h[abs2] ^= 1;
        (*errores) += 2;
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
*/

void introducir_dos_errores(int *cadena_h, int modulo, int largo_total, int *errores) {
    srand(time(NULL));

    // 1. Calculamos la cantidad de bloques completos que hay en la cadena.
    // Restamos 1 porque el arreglo en tu implementación de Hamming empieza en el índice 1.
    int num_bloques = (largo_total - 1) / modulo;

    if (num_bloques == 0) {
        // Protección de seguridad por si el arreglo es más pequeño que un bloque.
        return;
    }

    // 2. Elegimos un bloque al azar.
    int bloque_elegido = rand() % num_bloques;

    // Calculamos en qué índice absoluto empieza ese bloque elegido.
    int inicio_bloque = 1 + (bloque_elegido * modulo);

    // 3. Elegimos la primera posición relativa (del 0 al modulo-1).
    int pos1 = rand() % modulo;
    int pos2;

    // 4. Elegimos la segunda posición relativa, asegurándonos de que no sea igual a la primera.
    // Si fueran iguales, el XOR la voltearía dos veces y dejaría el bit intacto (0 errores).
    do {
        pos2 = rand() % modulo;
    } while (pos1 == pos2);

    // 5. Calculamos las posiciones absolutas en el arreglo general.
    int abs1 = inicio_bloque + pos1;
    int abs2 = inicio_bloque + pos2;

    // 6. Introducimos los errores (invirtiendo el bit con XOR) y sumamos al contador.
    if (abs1 < largo_total && abs2 < largo_total) {
        cadena_h[abs1] ^= 1;
        cadena_h[abs2] ^= 1;
        (*errores) += 2;
    }
}

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



void generarArchivoDEX(int *cadena_H, int modulo, int largo_total, char *nombre_salida, long total_byts) {
    int k, pos_en_bloque, bit_count = 0;
    unsigned char caracter = 0;
    long bytes_escritos = 0; // 1. Nuevo contador para llevar el control

    int tamano_bloque = modulo;
    FILE *out = fopen(nombre_salida, "wb");
    if (!out) return;

    for (k = 1; k < largo_total; k++) {
        // 2. Condición de parada temprana: si ya escribimos lo que queríamos, salimos.
        if (bytes_escritos >= total_byts) {
            break;
        }

        pos_en_bloque = ((k - 1) % tamano_bloque) + 1;

        // Verificamos si la posición actual es de datos (no es potencia de 2)
        if ((pos_en_bloque & (pos_en_bloque - 1)) != 0) {

            if (cadena_H[k] == 1) {
                caracter |= (1 << (7 - bit_count));
            }
            bit_count++;

            // Cuando completamos un byte (8 bits)...
            if (bit_count == 8) {
                fputc(caracter, out);     // Lo guardamos en el archivo
                bytes_escritos++;         // 3. Incrementamos nuestro contador

                caracter = 0;             // Reiniciamos variables para el próximo byte
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
void guardarArchivoH(int* cadena_H, int modulo, int largo_total)
{
    char nombre_salida[15] = "Hamming";

    switch(modulo){
        case 8:
            strcat(nombre_salida,".HA1");
        break;
        case 1024:
            strcat(nombre_salida,".HA2");
        break;
        case 16384:
            strcat(nombre_salida,".HA3");
        break;
    }

    int k, bit_count = 0;
    unsigned char caracter = 0;
    FILE *out = fopen(nombre_salida, "wb");

    if (!out) return;

    for (k = 1; k < largo_total; k++) {
        if (cadena_H[k] == 1) {
            caracter |= (1 << (7 - bit_count));
        }
        bit_count++;

        if (bit_count == 8) {
            fputc(caracter, out);
            caracter = 0;
            bit_count = 0;
        }
    }

    if (bit_count > 0) {
        fputc(caracter, out);
    }

    fclose(out);
}
char* CargarArchivo(const char *ruta, int *out_size) {
    FILE *f = fopen(ruta, "rb"); // "rb" es vital para archivos alterados
    if (!f) {
        if (out_size) *out_size = 0;
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(f);
        if (out_size) *out_size = 0;
        return NULL;
    }

    fread(buffer, 1, size, f);
    buffer[size] = '\0'; // Solo un nulo de seguridad al puro final
    fclose(f);

    if (out_size) *out_size = (int)size;
    return buffer;
}
