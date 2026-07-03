#ifndef UI_H_INCLUDED
#define UI_H_INCLUDED

#include "raylib.h"

/* ===================== CONSTANTES ===================== */

#define MAX_LOG_LINEAS    300     /* historial guardado; el panel muestra una ventana scrolleable de esto */
#define MAX_LOG_LEN       180
#define MAX_NOMBRE_LEN    128
#define MAX_LINEAS_COMP   4000
#define MAX_LINEA_LEN     220
#define MAX_CAMPOS_INPUT  3       /* maximo de archivos que se piden en secuencia para una sola opcion */

/* ===================== PANTALLAS ===================== */
/* La UI es una maquina de estados simple: en cada momento hay UNA pantalla activa. */
typedef enum {
    PANTALLA_MENU = 0,          /* menu principal con las 10 opciones                 */
    PANTALLA_PIDIENDO_NOMBRE,   /* modal pidiendo nombre(s) de archivo, campo a campo */
    PANTALLA_PIDIENDO_MODULO,   /* modal pidiendo el modulo de Hamming (8/1024/16384) */
    PANTALLA_COMPARAR_HUFFMAN,  /* comparacion de dos archivos de texto, linea x linea*/
    PANTALLA_COMPARAR_HAMMING,  /* comparacion: texto original vs. texto recuperado   */
    PANTALLA_ESTADISTICAS,      /* graficos de barras con tamanios de archivo         */
    PANTALLA_ADVERTENCIA        /* cartel de "archivo no encontrado", con reintento   */
} Pantalla;

/* Par de lineas (una de cada archivo/columna) usado en la pantalla de comparacion.
   Izquierda y derecha se cortan con el MISMO ancho fijo, asi que la posicion j de una
   fila siempre corresponde a la posicion j de la otra: se pueden comparar directamente
   caracter por caracter sin necesitar nada mas. */
typedef struct {
    char izquierda[MAX_LINEA_LEN];
    char derecha[MAX_LINEA_LEN];
} ParLinea;

/* ===================== ESTADO GLOBAL DE LA APP ===================== */
typedef struct {
    Pantalla pantalla;
    int      opcionHover;        /* opcion de menu (0-9) sobre la que esta el mouse, -1 si ninguna */

    /* ---- modal de entrada de texto / numero (soporta pedir varios campos en secuencia) ---- */
    char inputBuffer[MAX_NOMBRE_LEN];
    int  inputLen;
    int  accionPendiente;        /* opcion de menu (1-9) que disparo el pedido de input */
    char campoTemp[MAX_CAMPOS_INPUT][MAX_NOMBRE_LEN]; /* campos ya confirmados de la secuencia actual */
    int  campoActual;            /* indice (0-based) del campo que se esta pidiendo ahora */
    int  totalCampos;            /* cuantos campos en total pide la accion actual */
    char tituloCampo[96];        /* texto de la pregunta para el campo actual */

    /* ---- log de mensajes (reemplaza los printf de consola), con scroll ---- */
    char log[MAX_LOG_LINEAS][MAX_LOG_LEN];
    int  logCantidad;
    int  scrollLog;               /* 0 = viendo los mensajes mas nuevos (abajo del todo) */

    /* ---- cartel de advertencia (archivo no encontrado, etc.) ---- */
    char mensajeAdvertencia[700];
    int  opcionReintento;          /* opcion de menu a re-iniciar al cerrar el cartel; 0 = volver al menu */

    /* ---- datos del archivo cargado / cadena de bits cruda ---- */
    char nombreArchivo[MAX_NOMBRE_LEN];
    int  archivoListo;
    long tamanioArchivo;
    int *chain;                  /* bits del archivo original (1 bit por celda) */
    int  n;                      /* cantidad de bits ocupados + 1 (idem main.c original) */

    /* ---- nombres de trabajo (ahora editables desde cada opcion, no fijos) ---- */
    char archivoOriginal[MAX_NOMBRE_LEN];
    char archivoComprimido[MAX_NOMBRE_LEN];
    char archivoDescomprimido[MAX_NOMBRE_LEN];

    /* ---- Hamming ---- */
    int  modulo;
    int *cadenaH;                /* estado ACTUAL (puede tener errores o ya corregida) */
    int *cadenaHOriginal;        /* snapshot tomado justo despues de Hamming(), antes de introducir errores */
    int  largoH;
    int  hayErrores;             /* 1 si ya se introdujeron errores sobre cadenaH */
    char archivoProtegidoActual[MAX_NOMBRE_LEN]; /* ultimo .Ha1/.Ha2/.ha3/.Hex/.H2x guardado, sugerido como default al desproteger */

    int *infoRecuperada;
    int  largoInfo;
    int  errores;

    /* ---- comparacion de archivos (texto, linea por linea, ya envuelto a un ancho que entra en el recuadro) ---- */
    ParLinea comparacionTexto[MAX_LINEAS_COMP];
    int      comparacionTextoCantidad;
    int      scrollComparacion;
    char     tituloIzquierda[64];
    char     tituloDerecha[64];

    /* ---- estadisticas ---- */
    long tOrig, tComp, tDesc;
} EstadoApp;

/* ===================== CICLO DE VIDA ===================== */
void UI_Inicializar(EstadoApp *estado);
void UI_Liberar(EstadoApp *estado);

/* Agrega una linea al panel de log (estilo printf, con formato) */
void UI_AgregarLog(EstadoApp *estado, const char *formato, ...);

/* ===================== MENU PRINCIPAL ===================== */
/* Dibuja y procesa el menu. Devuelve la opcion (0-9) clickeada este frame, o -1 si ninguna. */
int UI_ActualizarYDibujarMenu(EstadoApp *estado);

/* ===================== MODALES DE ENTRADA ===================== */
/* Pide el campo numero `indice` (0-based) de un total de `total` campos para la accion `accion`,
   precargado con `valorDefault` y mostrando `titulo` como pregunta. */
void UI_PedirCampo(EstadoApp *estado, int accion, int indice, int total, const char *valorDefault, const char *titulo);
void UI_PedirModulo(EstadoApp *estado, int accion);

/* Dibuja el modal activo y procesa teclado. Devuelve 1 el frame en que el usuario confirma con ENTER. */
int UI_ActualizarYDibujarInput(EstadoApp *estado);

/* ===================== COMPARACION DE ARCHIVOS ===================== */
void UI_CargarComparacionHuffman(EstadoApp *estado, const char *archivoIzq, const char *archivoDer,
                                  const char *tituloIzq, const char *tituloDer);
void UI_CargarComparacionHamming(EstadoApp *estado);
void UI_DibujarComparacion(EstadoApp *estado);

/* ===================== ESTADISTICAS ===================== */
void UI_CargarEstadisticas(EstadoApp *estado);
void UI_DibujarEstadisticas(EstadoApp *estado);

/* ===================== PANEL DE LOG (comun a todas las pantallas, scrolleable con la rueda o la barra) ===================== */
void UI_DibujarLog(EstadoApp *estado);

/* ===================== CARTEL DE ADVERTENCIA ===================== */
/* Muestra `mensaje` en un cartel modal. Si `opcionReintento` > 0, al cerrarlo se vuelve
   a pedir los archivos de esa opcion (con los ultimos valores escritos como default). */
void UI_MostrarAdvertencia(EstadoApp *estado, const char *mensaje, int opcionReintento);
/* Dibuja el cartel. Devuelve 1 el frame en que el usuario lo cierra. */
int UI_DibujarAdvertencia(EstadoApp *estado);

/* Barra superior con boton "Volver al menu" (ESC), usada en sub-pantallas */
int UI_DibujarBarraVolver(EstadoApp *estado, const char *titulo);

#endif // UI_H_INCLUDED
