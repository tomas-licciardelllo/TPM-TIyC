#include "huffman.h"
//Lecura de frecuencias dinamica
TablaFrecuencias contarFrecuencias(const char *archivo) {
    int i;
    TablaFrecuencias tabla;
    tabla.capacidad = 16; //pongo 16 porque es un numero bueno para archivos cortos
    tabla.cantidad = 0;
    tabla.items = malloc(sizeof(Frecuencia) * tabla.capacidad);

    FILE *f = fopen(archivo, "rb");
    if (!f) {
        printf("Error al abrir archivo\n");
        tabla.items = NULL;
        return tabla;
    }

    int c;
    while ((c = fgetc(f)) != EOF) {
        unsigned char ch = (unsigned char)c;

        // Buscar si ya existe en la tabla
        int encontrado = 0;
        for (i = 0; i < tabla.cantidad; i++) {
            if (tabla.items[i].caracter == ch) {
                tabla.items[i].frecuencia++;
                encontrado = 1;
                break;
            }
        }

        // Si no existe, agregarlo (creciendo el array si hace falta)
        if (!encontrado) {
            if (tabla.cantidad >= tabla.capacidad) {
                tabla.capacidad *= 2;
                tabla.items = realloc(tabla.items, sizeof(Frecuencia) * tabla.capacidad);
            }
            tabla.items[tabla.cantidad].caracter = ch;
            tabla.items[tabla.cantidad].frecuencia = 1;
            tabla.cantidad++;
        }
    }

    fclose(f);
    printf("Caracteres distintos encontrados: %d\n", tabla.cantidad);
    return tabla;
}

void liberarTablaFrecuencias(TablaFrecuencias *t) {
    if (t->items) free(t->items);
    t->items = NULL;
    t->cantidad = 0;
}

// COLA DE PRIORIDAD (lista ordenada vinculada simple)
typedef struct ListaNodo {
    Nodo *nodo;
    struct ListaNodo *sig;
} ListaNodo;
//Esto es de menor prioridad para combinar las frecuencias con menor probabilidad para luego subirlas
void insertarOrdenado(ListaNodo **lista, Nodo *n)
{
    ListaNodo *nuevo = malloc(sizeof(ListaNodo));
    nuevo->nodo = n;
    nuevo->sig = NULL;
  //Caso insertar al principio, con lista vacia o es la frecuencia es menor o igual que la cabecera
    if (*lista == NULL || (*lista)->nodo->frecuencia > n->frecuencia) {
        nuevo->sig = *lista;
        *lista = nuevo;
        return;
    }
    //Caso geeneral, insertar al medio o al final
    ListaNodo *actual = *lista;
    while (actual->sig != NULL && actual->sig->nodo->frecuencia <= n->frecuencia) //Avanzamos mientras no estemos al final y el siguiente nodo tenga frecuencia menor a la del nuevo
        {
        actual = actual->sig;
    }
    nuevo->sig = actual->sig;
    actual->sig = nuevo;
}

Nodo* extraerMin(ListaNodo **lista)
{
    if (*lista == NULL) return NULL;
    ListaNodo *primero = *lista; //guardamos el primero
     Nodo *n = primero->nodo; // lo que contiene
    *lista = primero->sig;
    free(primero);
    return n;
}

// CONSTRUIR EL ARBOL DE HUFFMAN
Nodo* construirArbol(TablaFrecuencias *tabla) {
   int i,j;
    ListaNodo *cola = NULL; //cola


    // 1) Crear un nodo hoja por cada carácter y meterlo en la cola
    for (i = 0; i < tabla->cantidad; i++)
        {
        Nodo *n = malloc(sizeof(Nodo));
        n->caracter = tabla->items[i].caracter;
        n->frecuencia = tabla->items[i].frecuencia;
        n->izq = n->der = NULL;
        insertarOrdenado(&cola, n);
    }

    // Caso especial: archivo con un solo caracter distinto, creamos un padre artificial en el arbol
    if (tabla->cantidad == 1)
        {
        Nodo *unico = extraerMin(&cola);
        Nodo *raiz = malloc(sizeof(Nodo));
        raiz->caracter = 0;
        raiz->frecuencia = unico->frecuencia;
        raiz->izq = unico;
        raiz->der = NULL;
        return raiz;
    }

    // 2) Combinar los dos nodos de menor frecuencia hasta que quede uno solo
    while (cola != NULL && cola->sig != NULL) //Bucle de Huffman, mientras haya al meenos 2 nodos en la cola
        {
        Nodo *a = extraerMin(&cola);
        Nodo *b = extraerMin(&cola);
        Nodo *padre = malloc(sizeof(Nodo));
        padre->caracter = 0; //simbolo que no tiene significado
        padre->frecuencia = a->frecuencia + b->frecuencia;
        padre->izq = a;
        padre->der = b;
        insertarOrdenado(&cola, padre);
    }
    return extraerMin(&cola);
}


// Recorre el arbol desde la raiz. Cada vez que baja a la izquierda agrega un 0 al codigo actual,
 // y cada vez que baja a la derecha agregá un 1.
 // Cuando llega a una hoja, guarda ese codigo como el codigo del caracter que esta en la hoja, el asociado seria

static void generarCodigosRec(Nodo *raiz, char *codigoActual, int profundidad,TablaCodigos *tabla) {
    if (!raiz) return; //nada

    // Es una hoja
    if (!raiz->izq && !raiz->der)
        {
        codigoActual[profundidad] = '\0'; //es un terminador de string en pos profundidad, sirve para que el strcpy sepa donde termina
        tabla->items[tabla->cantidad].caracter = raiz->caracter; //Guardamos el caracter
        strcpy(tabla->items[tabla->cantidad].codigo,profundidad > 0 ? codigoActual : "0");
        tabla->items[tabla->cantidad].longitud = profundidad > 0 ? profundidad : 1;
        tabla->cantidad++;
        return;
    }

    if (raiz->izq) { //Bajamos por izquierda y ponemos un 0 y llamamos la recu
        codigoActual[profundidad] = '0';
        generarCodigosRec(raiz->izq, codigoActual, profundidad + 1, tabla);
    }
    if (raiz->der) {
        codigoActual[profundidad] = '1';
        generarCodigosRec(raiz->der, codigoActual, profundidad + 1, tabla);
    }
}

TablaCodigos generarTablaCodigos(Nodo *raiz, int cantidadHojas) {
    TablaCodigos tabla;
    tabla.items = malloc(sizeof(CodigoHuffman) * cantidadHojas); //Reservamos memoria justa
    tabla.cantidad = 0;

    char buffer[MAX_CODE_LEN]; //un buffer temporal
    generarCodigosRec(raiz, buffer, 0, &tabla); //La recu
    return tabla;
}

// Buscar el codigo de un caracter en la tabla, dado un char te devuelve su codigo en string
static CodigoHuffman* buscarCodigo(TablaCodigos *tabla, unsigned char ch) {
    int i;
    for (i = 0; i < tabla->cantidad; i++) {
        if (tabla->items[i].caracter == ch) return &tabla->items[i];
    }
    return NULL;
}

// COMPRIMIR
void comprimirArchivo(const char *entrada, const char *salida) {
    int j,i;
    TablaFrecuencias frecs = contarFrecuencias(entrada);
    if (frecs.cantidad == 0)
        {
        printf("El archivo esta vacio o no se pudo leer.\n");
        return;
    }
//Preparamos todo para comprimir
    Nodo *raiz = construirArbol(&frecs);
    TablaCodigos codigos = generarTablaCodigos(raiz, frecs.cantidad);

   // Mostrar los códigos generados
    printf("\n--- Tabla de codigos generada ---\n");
    for  (i = 0; i < codigos.cantidad; i++) {
    unsigned char c = codigos.items[i].caracter;

    // Buscar la frecuencia real de este caracter
    unsigned long freq = 0;
        for (j = 0; j < frecs.cantidad; j++) {
            if (frecs.items[j].caracter == c) {
                freq = frecs.items[j].frecuencia;
                break;
        }
    }
   printf("'%c'  freq=%lu  =>  %s\n",
           (c >= 32 && c < 127) ? c : '?', // el signo de pregunta son para los caracteres que tienen acento o no estan el rango de 32 y 127 en ASCII
           freq,
           codigos.items[i].codigo);

   /*

        SE PUEDE USAR ESTE BLOQUE COMO UNA ALTERNATIVA PARA MOSTRAR LOS CARACTERES QUE SON DE MAS DE 8 BITS


        printf("'%c' (0x%02X) freq=%lu  =>  %s\n",
       (c >= 32 && c < 127) ? c : '.',
        c,
        freq,
        codigos.items[i].codigo);
    */
}
    FILE *fin = fopen(entrada, "rb");
    FILE *fout = fopen(salida, "wb");
    if (!fin || !fout) { printf("Error de archivos\n"); return; }

    //CABECERA del huf
    // Cantidad de caracteres distintos
    fwrite(&frecs.cantidad, sizeof(int), 1, fout);
    // Por cada caracter: byte + frecuencia
    for (i = 0; i < frecs.cantidad; i++)
        {
        fwrite(&frecs.items[i].caracter, sizeof(unsigned char), 1, fout);
        fwrite(&frecs.items[i].frecuencia, sizeof(unsigned long), 1, fout);
    }

    // datos comprimidos bit a bit
    unsigned char buffer = 0;
    int bitsEnBuffer = 0;
    int c;
    while ((c = fgetc(fin)) != EOF) { //leemos caracter
        CodigoHuffman *cod = buscarCodigo(&codigos, (unsigned char)c); //buscamos su codigo
        for (i = 0; cod->codigo[i]; i++) { //recoremos cada bit del codigo
            buffer <<= 1; //shift a la izq
            if (cod->codigo[i] == '1') buffer |= 1;
            bitsEnBuffer++;
            if (bitsEnBuffer == 8) {
                fputc(buffer, fout);
                buffer = 0;
                bitsEnBuffer = 0;
            }
        }
    }

    // Vaciar bits restantes
    if (bitsEnBuffer > 0) {
        buffer <<= (8 - bitsEnBuffer);
        fputc(buffer, fout);
    }
    fclose(fin);
    fclose(fout);

    free(codigos.items);
    liberarArbol(raiz);
    liberarTablaFrecuencias(&frecs);

    printf("\nArchivo comprimido: %s\n", salida);
}

// DESCOMPRIMIR
void descomprimirArchivo(const char *entrada, const char *salida) {
    int i;
    FILE *fin = fopen(entrada, "rb");
    FILE *fout = fopen(salida, "wb");
    if (!fin || !fout) { perror("Error de archivos"); return; }

    // 1) Leer cantidad de caracteres distintos
    TablaFrecuencias frecs;
    fread(&frecs.cantidad, sizeof(int), 1, fin);
    frecs.capacidad = frecs.cantidad;
    frecs.items = malloc(sizeof(Frecuencia) * frecs.cantidad);

    // 2) Leer cada par (caracter, frecuencia)
    unsigned long total = 0;
    for (i = 0; i < frecs.cantidad; i++) {
        fread(&frecs.items[i].caracter, sizeof(unsigned char), 1, fin);
        fread(&frecs.items[i].frecuencia, sizeof(unsigned long), 1, fin);
        total += frecs.items[i].frecuencia; // suma totales de la frecuencia, sirve para saber la cant total de caracteres
    }

    // 3) Reconstruir el arbol con las mismas frecuencias
    Nodo *raiz = construirArbol(&frecs);
    Nodo *actual = raiz;

    // 4) Decodificar bit por bit
    int c;
    unsigned long escritos = 0;
    while (escritos < total && (c = fgetc(fin)) != EOF) {
        for (i = 7; i >= 0 && escritos < total; i--) {
            int bit = (c >> i) & 1; //procesamos de izquierda a derecha
            actual = (bit == 0) ? actual->izq : actual->der; //bajamos por el arbol segun el bit

            if (!actual->izq && !actual->der) { //veoms si llegamos a una hoja
                fputc(actual->caracter, fout); //escribimos el caracter
                escritos++;
                actual = raiz; //volvemos a la raiz
            }
        }
    }

    fclose(fin);
    fclose(fout);
    liberarArbol(raiz);
    liberarTablaFrecuencias(&frecs);

    printf("Archivo descomprimido: %s\n", salida);
}

void liberarArbol(Nodo *raiz) {
    if (!raiz) return;
    liberarArbol(raiz->izq);
    liberarArbol(raiz->der);
    free(raiz);
}
