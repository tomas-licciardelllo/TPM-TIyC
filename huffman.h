#ifndef HUFFMAN_H
#define HUFFMAN_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define MAX_CODE_LEN 256

// Nodo del arbol de Huffman
typedef struct Nodo {
    unsigned char caracter;
    unsigned long frecuencia;
    struct Nodo *izq;
    struct Nodo *der;
} Nodo;

// Lista dinámica de frecuencias (solo caracteres que aparecen
typedef struct {
    unsigned char caracter;
    unsigned long frecuencia;
} Frecuencia;

typedef struct {
    Frecuencia *items;     // array dinamico
    int cantidad;          // cuantos caracteres distintos hay
    int capacidad;         // capacidad actual del array
} TablaFrecuencias;

// Tabla de codigos
typedef struct {
    unsigned char caracter;
    char codigo[MAX_CODE_LEN];
    int longitud;
} CodigoHuffman;

typedef struct {
    CodigoHuffman *items;
    int cantidad;
} TablaCodigos;


TablaFrecuencias contarFrecuencias(const char *archivo);
Nodo* construirArbol(TablaFrecuencias *tabla);
TablaCodigos generarTablaCodigos(Nodo *raiz, int cantidadHojas);
void comprimirArchivo(const char *entrada, const char *salida);
void descomprimirArchivo(const char *entrada, const char *salida);
void liberarArbol(Nodo *raiz);
void liberarTablaFrecuencias(TablaFrecuencias *t);

void verArchivos(const char *original, const char *recuperado);
void verEstadistica(const char *original, const char *comprimido, const char *descomprimido);



void imprimirB(unsigned char c);
int* Hamming(int cadena[], int modulo, int limite, int *arr_out);
void introducir_errores(int *cadena_h, int modulo, int largo_total, int *errores);
int* decodificarHamming(int cadena_H[], int modulo, int largo_total, int *largo_info,int cantidad_de_caracteres_originales);
void generarArchivoDEX(int *cadena_H, int modulo, int largo_total, char *nombre_salida,long total_byts);
void guardarInfoRecuperada(int *info_recuperada, int largo_info, char *nombre_archivo);
void introducir_dos_errores(int *cadena_h, int modulo, int largo_total, int *errores);

#endif
