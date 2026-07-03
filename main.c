#include "raylib.h"
#include "huffman.h"
#include "ui.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/* ===================== ACCIONES DE CADA OPCION DEL MENU ===================== */
/* Toda la logica de "que hacer cuando se elige la opcion N" vive aca, separada
   de la parte grafica (ui.c/ui.h) y de los algoritmos (huffman.c/Hamming.c/...). */

static void CargarArchivo(EstadoApp *estado) {
    FILE *arch = fopen(estado->nombreArchivo, "rb");
    if (!arch) {
        UI_AgregarLog(estado, "Error: no se encontro '%s'", estado->nombreArchivo);
        return;
    }

    fseek(arch, 0, SEEK_END);
    long file_size = ftell(arch);
    rewind(arch);

    if (estado->chain) { free(estado->chain); estado->chain = NULL; }
    int max_bits = (int)(file_size * 8) + 10;
    estado->chain = malloc(max_bits * sizeof(int));

    int n = 1, caracter, t;
    while ((caracter = fgetc(arch)) != EOF) {
        for (t = 7; t >= 0; t--) estado->chain[n++] = ((unsigned char)caracter >> t) & 1;
    }
    fclose(arch);

    estado->n = n;
    estado->tamanioArchivo = file_size;
    estado->archivoListo = 1;

    /* si ya habia una proteccion Hamming aplicada, queda invalida para el archivo nuevo */
    if (estado->cadenaH) { free(estado->cadenaH); estado->cadenaH = NULL; }
    if (estado->cadenaHOriginal) { free(estado->cadenaHOriginal); estado->cadenaHOriginal = NULL; }
    estado->largoH = 0;
    estado->errores = 0;
    estado->hayErrores = 0;

    UI_AgregarLog(estado, "Archivo '%s' cargado (%ld bytes, %d bits).", estado->nombreArchivo, file_size, n - 1);
}

static void AplicarHamming(EstadoApp *estado, int modulo) {
    if (estado->cadenaH) { free(estado->cadenaH); estado->cadenaH = NULL; }
    if (estado->cadenaHOriginal) { free(estado->cadenaHOriginal); estado->cadenaHOriginal = NULL; }

    int largo = 0;
    int *resultado = Hamming(estado->chain, modulo, estado->n, &largo);

    estado->cadenaH = resultado;
    estado->largoH = largo;
    estado->modulo = modulo;
    estado->errores = 0;
    estado->hayErrores = 0;

    /* snapshot: copia del estado recien protegido, ANTES de introducir errores (queda reservado
       por si se necesita mas adelante; la vista de comparacion actual no lo usa directamente). */
    estado->cadenaHOriginal = malloc(largo * sizeof(int));
    memcpy(estado->cadenaHOriginal, resultado, largo * sizeof(int));
}

/* Empaqueta de a 8 los bits (0/1) de cadenaH en bytes y los guarda en disco.
   Antes de los bits escribe una PRIMERA LINEA de texto con la fecha y hora
   (DD/MM/AAAA HH:MM), para poder verificarla despues al desproteger (opciones 7 y 8).
   Si `fechaHoraPersonalizada` es NULL o vacia, se usa la fecha/hora actual del sistema
   (asi siguen funcionando igual las opciones 6 y 10). Si se pasa un valor, se usa tal
   cual (la opcion 5 permite que el usuario la escriba a mano).
   Los bits arrancan en el indice 1 porque cadenaH[0] es solo un relleno (ver Hamming.c). */
/* Empaqueta de a 8 los bits (0/1) de cadenaH en bytes y los guarda en disco.
   Escribe DOS lineas de cabecera antes de los bits:
     linea 1: fecha y hora (DD/MM/AAAA HH:MM) -- para verificarla al desproteger (opciones 7/8).
     linea 2: "BYTES:<tamanio original exacto>" -- para saber EXACTAMENTE cuantos bytes
              de datos reales hay, sin tener que adivinarlo a partir del tamanio del
              archivo protegido (que con modulos grandes y archivos chicos queda casi
              todo relleno de ceros, y adivinar por tamanio metia ese relleno como si
              fuera contenido real).
   Si `fechaHoraPersonalizada` es NULL o vacia, se usa la fecha/hora actual del sistema.
   Los bits arrancan en el indice 1 porque cadenaH[0] es solo un relleno (ver Hamming.c). */
static void GuardarCadenaProtegida(const int *bits, int largo, const char *nombreArchivo,
                                    const char *fechaHoraPersonalizada, long tamanioOriginalBytes) {
    FILE *f = fopen(nombreArchivo, "wb");
    if (!f) return;

    if (fechaHoraPersonalizada && fechaHoraPersonalizada[0]) {
        fprintf(f, "%s\n", fechaHoraPersonalizada);
    } else {
        time_t ahora = time(NULL);
        struct tm *local = localtime(&ahora);
        fprintf(f, "%02d/%02d/%04d %02d:%02d\n",
                local->tm_mday, local->tm_mon + 1, local->tm_year + 1900, local->tm_hour, local->tm_min);
    }
    fprintf(f, "BYTES:%ld\n", tamanioOriginalBytes);

    unsigned char byte = 0;
    int bitCount = 0;
    for (int i = 1; i < largo; i++) {
        int v = (bits[i] >= 1) ? 1 : 0;
        byte = (unsigned char)((byte << 1) | v);
        bitCount++;
        if (bitCount == 8) { fputc(byte, f); byte = 0; bitCount = 0; }
    }
    if (bitCount > 0) { byte = (unsigned char)(byte << (8 - bitCount)); fputc(byte, f); }
    fclose(f);
}

/* Quita la extension (si tiene) de `nombre` y lo deja en `salida`, para proponer
   un nombre base por defecto cuando se pide el archivo de salida de una opcion. */
static void QuitarExtension(const char *nombre, char *salida, size_t tam) {
    strncpy(salida, nombre, tam - 1);
    salida[tam - 1] = '\0';
    char *punto = strrchr(salida, '.');
    if (punto) *punto = '\0';
}

/* Extension automatica segun el modulo de Hamming usado para proteger. */
static const char *ExtensionHamming(int modulo) {
    if (modulo == 8) return ".Ha1";
    if (modulo == 1024) return ".Ha2";
    return ".ha3"; /* 16384 */
}

static int TieneExtensionDescompactable(const char *ruta) {
    static const char *extensiones[] = { ".huf", ".dex", ".dcx", ".ha1", ".ha2", ".ha3" };
    size_t len = strlen(ruta);
    if (len < 4) return 0;
    char ext[5];
    for (int i = 0; i < 4; i++) ext[i] = (char)tolower((unsigned char)ruta[len - 4 + i]);
    ext[4] = '\0';
    for (int i = 0; i < 6; i++) if (strcmp(ext, extensiones[i]) == 0) return 1;
    return 0;
}

/* descomprimirArchivo (huffman.c) lee los primeros 4 bytes de CUALQUIER archivo como si
   fueran "cantidad de caracteres distintos" de la cabecera, sin validar nada. Si el archivo
   no tiene esa forma, ese numero sale disparatado (negativo, gigante, etc.) y el malloc
   posterior con ese tamano hace que el programa se caiga. Esta funcion valida ANTES de
   llamar a descomprimirArchivo, sin tocar su codigo: 1) que la extension sea una de las
   que pueden contener datos comprimidos por Huffman (.huf directamente, o .Dex/.Dcx que
   son lo que queda despues de desproteger un .huf que se protegio con Hamming, o incluso
   .Ha1/.Ha2/.Ha3 por si alguien los prueba directo), y 2) que la cabecera tenga una forma
   plausible (entre 1 y 256 caracteres distintos, y que el archivo realmente pese lo
   necesario para esa cabecera) -- esto ultimo es lo que sigue frenando, por ejemplo, un
   .Ha1 que todavia esta protegido con Hamming (su cabecera es la fecha/hora en texto, no
   una cabecera de Huffman real, asi que no pasa este chequeo). */
static int EsArchivoHufValido(const char *ruta, char *motivo, size_t tamMotivo) {
    if (!TieneExtensionDescompactable(ruta)) {
        snprintf(motivo, tamMotivo, "la extension debe ser .huf, .Dex, .Dcx, .Ha1, .Ha2 o .Ha3.");
        return 0;
    }

    FILE *f = fopen(ruta, "rb");
    if (!f) {
        snprintf(motivo, tamMotivo, "no se pudo abrir el archivo.");
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long tamArchivo = ftell(f);
    rewind(f);

    int cantidad = 0;
    size_t leidos = fread(&cantidad, sizeof(int), 1, f);
    fclose(f);

    if (leidos != 1) {
        snprintf(motivo, tamMotivo, "el archivo es demasiado chico para tener una cabecera .huf.");
        return 0;
    }

    /* como mucho puede haber 256 caracteres (bytes) distintos en cualquier archivo */
    if (cantidad <= 0 || cantidad > 256) {
        snprintf(motivo, tamMotivo,
                 "la cabecera dice tener %d caracteres distintos; un .huf real tiene entre 1 y 256.",
                 cantidad);
        return 0;
    }

    long tamMinimoEsperado = (long)sizeof(int) + (long)cantidad * (long)(sizeof(unsigned char) + sizeof(unsigned long));
    if (tamArchivo < tamMinimoEsperado) {
        snprintf(motivo, tamMotivo,
                 "el archivo pesa menos de lo que su propia cabecera necesita "
                 "(dice %d caracteres en la tabla, pero el archivo no llega a ese tamanio).",
                 cantidad);
        return 0;
    }

    return 1;
}

/* Recorre cadena_H bloque por bloque, SIN MODIFICAR NADA, solo para contar cuantos bloques
   tienen un error DOBLE (dos bits invertidos en el mismo modulo). Es exactamente el mismo
   calculo de sindrome/paridad que ya hace decodificarHamming por dentro (su "Caso 4":
   sindrome distinto de 0 pero paridad global par) -- esto no cambia ni reimplementa la
   correccion, solo la detecta desde afuera para poder avisarla en la interfaz, ya que el
   printf que tiene decodificarHamming no se ve en una app sin consola. */
static int ContarBloquesConErrorDoble(const int *cadena_H, int modulo, int largo_total) {
    int bloquesIncorregibles = 0;
    for (int k = 1; k < largo_total; k += modulo) {
        if (k + modulo - 1 > largo_total) break;

        int posicion_error_relativa = 0;
        int paridad_global_actual = 0;
        for (int i = 1; i <= modulo; i++) {
            int bit = (cadena_H[k + i - 1] >= 1) ? (cadena_H[k + i - 1] % 2) : 0;
            paridad_global_actual ^= bit;
            if (i < modulo && bit == 1) posicion_error_relativa ^= i;
        }

        if (posicion_error_relativa != 0 && paridad_global_actual == 0) {
            bloquesIncorregibles++;
        }
    }
    return bloquesIncorregibles;
}

/* Lee la primera linea de un archivo (texto, sin el salto de linea). Devuelve 1 si pudo,
   0 si el archivo no existe o esta vacio. */
static int LeerPrimeraLinea(const char *ruta, char *salida, size_t tamSalida) {
    FILE *f = fopen(ruta, "rb");
    if (!f) return 0;
    bool hayLinea = (fgets(salida, (int)tamSalida, f) != NULL);
    fclose(f);
    if (!hayLinea) return 0;
    salida[strcspn(salida, "\r\n")] = '\0';
    return 1;
}

/* Lee las DOS lineas de cabecera de un archivo protegido (fecha/hora y "BYTES:<tamanio>"),
   sin cargar los bits. Devuelve 1 si pudo leer ambas. Util cuando no hace falta el arreglo
   de bits completo, solo propagar esta info (por ejemplo, la opcion 10 que sigue operando
   sobre el cadenaH ya en memoria, pero necesita saber que fecha/tamanio tenia el archivo
   del que viene ese estado). */
static int LeerCabeceraProtegida(const char *ruta, char *fechaHora, size_t tamFechaHora, long *tamanioOriginal) {
    FILE *f = fopen(ruta, "rb");
    if (!f) return 0;

    if (!fgets(fechaHora, (int)tamFechaHora, f)) { fclose(f); return 0; }
    fechaHora[strcspn(fechaHora, "\r\n")] = '\0';

    char lineaBytes[64];
    if (!fgets(lineaBytes, sizeof(lineaBytes), f)) { fclose(f); return 0; }
    fclose(f);
    lineaBytes[strcspn(lineaBytes, "\r\n")] = '\0';

    long valor = 0;
    if (sscanf(lineaBytes, "BYTES:%ld", &valor) != 1) return 0;
    *tamanioOriginal = valor;
    return 1;
}

/* Verifica que `linea` tenga el formato DD/MM/AAAA HH:MM y que corresponda al dia de HOY,
   a la hora ANTERIOR a la actual (hora actual menos 1). Devuelve 1 si coincide, 0 si no
   -- y en ese caso deja en `motivo` una descripcion legible de por que fallo. */
static int VerificarFechaHoraLinea(const char *linea, char *motivo, size_t tamMotivo) {
    int dia, mes, anio, hora, minuto;
    if (sscanf(linea, "%d/%d/%d %d:%d", &dia, &mes, &anio, &hora, &minuto) != 5) {
        snprintf(motivo, tamMotivo,
                 "la primera linea ('%s') no tiene el formato esperado DD/MM/AAAA HH:MM.", linea);
        return 0;
    }

    time_t ahora = time(NULL);
    struct tm *local = localtime(&ahora);
    int diaHoy   = local->tm_mday;
    int mesHoy   = local->tm_mon + 1;
    int anioHoy  = local->tm_year + 1900;
    int horaActual = local->tm_hour;

    /* comparamos las fechas completas (no solo el numero de dia) para que un "5 del mes
       que viene" no se confunda con un "5 anterior a hoy" */
    long fechaArchivo = (long)anio * 10000 + mes * 100 + dia;
    long fechaHoy      = (long)anioHoy * 10000 + mesHoy * 100 + diaHoy;

    if (fechaArchivo > fechaHoy) {
        snprintf(motivo, tamMotivo,
                 "la fecha del archivo (%02d/%02d/%04d) es POSTERIOR a la de hoy (%02d/%02d/%04d): "
                 "debe ser el mismo dia o uno anterior.",
                 dia, mes, anio, diaHoy, mesHoy, anioHoy);
        return 0;
    }
    if (fechaArchivo == fechaHoy && hora > horaActual) {
        snprintf(motivo, tamMotivo,
                 "la hora del archivo (%02d hs) es POSTERIOR a la actual (%02d hs): "
                 "debe ser la misma hora o una anterior (siendo el mismo dia de hoy).",
                 hora, horaActual);
        return 0;
    }
    return 1;
}

/* Combina las dos anteriores: lee la primera linea de `ruta` y verifica su fecha/hora.
   Usado SOLO al desproteger (opciones 7 y 8) -- el resto de las opciones no lo chequean. */
static int VerificarMarcaDeTiempoArchivo(const char *ruta, char *motivo, size_t tamMotivo) {
    char primeraLinea[256];
    if (!LeerPrimeraLinea(ruta, primeraLinea, sizeof(primeraLinea))) {
        snprintf(motivo, tamMotivo, "el archivo esta vacio o no se pudo leer su primera linea.");
        return 0;
    }
    return VerificarFechaHoraLinea(primeraLinea, motivo, tamMotivo);
}

/* Carga un archivo PROTEGIDO (.Ha1/.Ha2/.ha3/.Hex/.H2x -- todos son el mismo formato:
   fecha/hora, despues "BYTES:<tamanio original exacto>", despues los bits empaquetados
   de a 8) desde disco a un arreglo, con la MISMA convencion que usa el resto del programa
   para cadenaH: la posicion 0 es un relleno (vale 0) y los bits reales arrancan en la
   posicion 1. Devuelve el arreglo (malloc) y deja en *largoOut la cantidad total de
   posiciones (relleno + bits). En *fechaHoraOut y *tamanioOriginalOut (si no son NULL)
   deja la fecha/hora y el tamanio EXACTO en bytes que tenia el archivo antes de proteger
   -- asi nunca hay que adivinarlo a partir del tamanio del archivo protegido, que con
   modulos grandes y archivos chicos queda casi todo relleno de ceros (ese relleno se
   filtraba antes como si fuera contenido real). Si el archivo no existe devuelve NULL. */
static int *CargarCadenaProtegidaDesdeArchivo(const char *ruta, int *largoOut,
                                               char *fechaHoraOut, size_t tamFechaHoraOut,
                                               long *tamanioOriginalOut) {
    FILE *f = fopen(ruta, "rb");
    if (!f) { *largoOut = 0; return NULL; }

    /* linea 1: fecha/hora */
    char lineaFecha[256];
    if (!fgets(lineaFecha, sizeof(lineaFecha), f)) { fclose(f); *largoOut = 0; return NULL; }
    lineaFecha[strcspn(lineaFecha, "\r\n")] = '\0';
    if (fechaHoraOut && tamFechaHoraOut > 0) {
        strncpy(fechaHoraOut, lineaFecha, tamFechaHoraOut - 1);
        fechaHoraOut[tamFechaHoraOut - 1] = '\0';
    }

    /* linea 2: "BYTES:<tamanio original exacto>" */
    char lineaBytes[64];
    long tamOriginal = 0;
    if (fgets(lineaBytes, sizeof(lineaBytes), f)) {
        lineaBytes[strcspn(lineaBytes, "\r\n")] = '\0';
        sscanf(lineaBytes, "BYTES:%ld", &tamOriginal);
    }
    if (tamanioOriginalOut) *tamanioOriginalOut = tamOriginal;

    long posDatos = ftell(f);
    fseek(f, 0, SEEK_END);
    long tamBytes = ftell(f) - posDatos;
    fseek(f, posDatos, SEEK_SET);

    if (tamBytes <= 0) { fclose(f); *largoOut = 0; return NULL; }

    int totalBits = (int)(tamBytes * 8);
    int *cadena = malloc((size_t)(totalBits + 1) * sizeof(int));
    cadena[0] = 0;

    int idx = 1, c;
    while ((c = fgetc(f)) != EOF) {
        for (int t = 7; t >= 0; t--) cadena[idx++] = ((unsigned char)c >> t) & 1;
    }
    fclose(f);

    *largoOut = idx; /* idx == totalBits + 1 */
    return cadena;
}

/* ===================== SELECCION DE ARCHIVOS POR OPCION ===================== */
/* Cada opcion que necesita uno o mas nombres de archivo arranca aca, pidiendolos
   de a uno (con un valor por defecto editable) antes de ejecutar la accion real. */

static void IniciarSeleccionDeArchivos(EstadoApp *estado, int opcion) {
    switch (opcion) {
        case 1:
            UI_PedirCampo(estado, 1, 0, 1,
                          estado->campoTemp[0][0] ? estado->campoTemp[0] : estado->nombreArchivo,
                          "Archivo a CARGAR:");
            break;
        case 2:
            UI_PedirCampo(estado, 2, 0, 2,
                          estado->campoTemp[0][0] ? estado->campoTemp[0] : estado->nombreArchivo,
                          "Archivo de ENTRADA a compactar:");
            break;
        case 3:
            UI_PedirCampo(estado, 3, 0, 2,
                          estado->campoTemp[0][0] ? estado->campoTemp[0] : estado->archivoComprimido,
                          "Archivo COMPRIMIDO de entrada (.huf/.Dex/.Dcx/.Ha1/.Ha2/.Ha3):");
            break;
        case 4:
            UI_PedirCampo(estado, 4, 0, 2,
                          estado->campoTemp[0][0] ? estado->campoTemp[0] : estado->nombreArchivo,
                          "Archivo IZQUIERDO a comparar:");
            break;
        case 5:
            UI_PedirCampo(estado, 5, 0, 3,
                          estado->campoTemp[0][0] ? estado->campoTemp[0] : estado->nombreArchivo,
                          "Archivo a PROTEGER (Hamming):");
            break;
        case 6:
            UI_PedirCampo(estado, 6, 0, 2,
                          estado->campoTemp[0][0] ? estado->campoTemp[0] : estado->archivoProtegidoActual,
                          "Archivo PROTEGIDO al que introducirle errores:");
            break;
        case 7:
            UI_PedirCampo(estado, 7, 0, 2,
                          estado->campoTemp[0][0] ? estado->campoTemp[0] : estado->archivoProtegidoActual,
                          "Archivo PROTEGIDO a desproteger (.Ha1/.Ha2/.ha3/.Hex/.H2x):");
            break;
        case 8:
            UI_PedirCampo(estado, 8, 0, 2,
                          estado->campoTemp[0][0] ? estado->campoTemp[0] : estado->archivoProtegidoActual,
                          "Archivo PROTEGIDO a desproteger (.Ha1/.Ha2/.ha3/.Hex/.H2x):");
            break;
        case 9:
            UI_PedirCampo(estado, 9, 0, 3, estado->archivoOriginal, "Archivo ORIGINAL:");
            break;
        case 10: {
            char base[MAX_NOMBRE_LEN];
            QuitarExtension(estado->nombreArchivo, base, sizeof(base));
            strncat(base, "_dos_errores", sizeof(base) - strlen(base) - 1);
            UI_PedirCampo(estado, 10, 0, 1, base, "Nombre BASE de salida (se le agrega .H2x):");
            break;
        }
        default:
            break;
    }
}

/* Se llama cada vez que el usuario confirma un campo del modal. Si la accion necesita
   mas campos, pide el siguiente; si no, ejecuta la logica real con todos los campos juntos. */
static void ContinuarSeleccionDeArchivos(EstadoApp *estado) {
    int opcion = estado->accionPendiente;
    int indice = estado->campoActual;

    strncpy(estado->campoTemp[indice], estado->inputBuffer, MAX_NOMBRE_LEN - 1);
    estado->campoTemp[indice][MAX_NOMBRE_LEN - 1] = '\0';

    int siguiente = indice + 1;
    if (siguiente < estado->totalCampos) {
        const char *titulo = "";
        char defaultBuf[MAX_NOMBRE_LEN] = "";
        const char *valorDefault = defaultBuf;
        switch (opcion) {
            case 2: titulo = "Archivo de SALIDA compactado (.huf):"; strcpy(defaultBuf, estado->archivoComprimido); break;
            case 3: titulo = "Archivo de SALIDA descomprimido:";    strcpy(defaultBuf, estado->archivoDescomprimido); break;
            case 4: titulo = "Archivo DERECHO a comparar:";
                    strcpy(defaultBuf, estado->campoTemp[1][0] ? estado->campoTemp[1] : estado->archivoDescomprimido);
                    break;
            case 5:
                if (siguiente == 1) {
                    titulo = "Nombre BASE de salida (la extension se agrega segun el modulo):";
                    QuitarExtension(estado->campoTemp[0], defaultBuf, sizeof(defaultBuf));
                } else {
                    time_t ahora = time(NULL);
                    struct tm *local = localtime(&ahora);
                    titulo = "Fecha y hora a grabar en el archivo (DD/MM/AAAA HH:MM):";
                    snprintf(defaultBuf, sizeof(defaultBuf), "%02d/%02d/%04d %02d:%02d",
                              local->tm_mday, local->tm_mon + 1, local->tm_year + 1900,
                              local->tm_hour, local->tm_min);
                }
                break;
            case 6: titulo = "Nombre BASE de salida (se le agrega .Hex):";
                    QuitarExtension(estado->campoTemp[0], defaultBuf, sizeof(defaultBuf));
                    strncat(defaultBuf, "_con_errores", sizeof(defaultBuf) - strlen(defaultBuf) - 1);
                    break;
            case 7: titulo = "Nombre BASE de salida (se le agrega .Dex):";
                    QuitarExtension(estado->campoTemp[0], defaultBuf, sizeof(defaultBuf));
                    strncat(defaultBuf, "_sin_corregir", sizeof(defaultBuf) - strlen(defaultBuf) - 1);
                    break;
            case 8: titulo = "Nombre BASE de salida (se le agrega .Dcx):";
                    QuitarExtension(estado->campoTemp[0], defaultBuf, sizeof(defaultBuf));
                    strncat(defaultBuf, "_corregido", sizeof(defaultBuf) - strlen(defaultBuf) - 1);
                    break;
            case 9:
                if (siguiente == 1) { titulo = "Archivo COMPACTADO:";    strcpy(defaultBuf, estado->archivoComprimido); }
                else                { titulo = "Archivo DESCOMPACTADO:"; strcpy(defaultBuf, estado->archivoDescomprimido); }
                break;
            default: break;
        }
        UI_PedirCampo(estado, opcion, siguiente, estado->totalCampos, valorDefault, titulo);
        return;
    }

    /* ya tenemos todos los campos que pedia esta opcion: validamos archivos de ENTRADA y ejecutamos */
    switch (opcion) {
        case 1: {
            if (!FileExists(estado->campoTemp[0])) {
                char msg[256];
                snprintf(msg, sizeof(msg), "No se encontro el archivo '%s'.\nEscribi el nombre correcto (con su extension) e intenta de nuevo.", estado->campoTemp[0]);
                UI_MostrarAdvertencia(estado, msg, 1);
                break;
            }
            strncpy(estado->nombreArchivo, estado->campoTemp[0], MAX_NOMBRE_LEN - 1);
            estado->nombreArchivo[MAX_NOMBRE_LEN - 1] = '\0';
            strcpy(estado->archivoOriginal, estado->nombreArchivo);
            CargarArchivo(estado);
            estado->pantalla = PANTALLA_MENU;
            break;
        }

        case 2: {
            if (!FileExists(estado->campoTemp[0])) {
                char msg[256];
                snprintf(msg, sizeof(msg), "No se encontro el archivo '%s'.\nEscribi el nombre correcto (con su extension) e intenta de nuevo.", estado->campoTemp[0]);
                UI_MostrarAdvertencia(estado, msg, 2);
                break;
            }
            strcpy(estado->archivoOriginal, estado->campoTemp[0]);
            strcpy(estado->archivoComprimido, estado->campoTemp[1]);
            comprimirArchivo(estado->campoTemp[0], estado->campoTemp[1]);
            UI_AgregarLog(estado, "Archivo '%s' compactado -> '%s'", estado->campoTemp[0], estado->campoTemp[1]);
            estado->pantalla = PANTALLA_MENU;
            break;
        }

        case 3: {
            if (!FileExists(estado->campoTemp[0])) {
                char msg[256];
                snprintf(msg, sizeof(msg), "No se encontro el archivo '%s'.\nEscribi el nombre correcto (con su extension) e intenta de nuevo.", estado->campoTemp[0]);
                UI_MostrarAdvertencia(estado, msg, 3);
                break;
            }
            char motivo[300];
            if (!EsArchivoHufValido(estado->campoTemp[0], motivo, sizeof(motivo))) {
                char msg[600];
                snprintf(msg, sizeof(msg),
                         "El archivo '%s' no se puede descompactar:\n%s\n\nExtensiones aceptadas: .huf, .Dex, .Dcx, .Ha1, .Ha2, .Ha3.",
                         estado->campoTemp[0], motivo);
                UI_MostrarAdvertencia(estado, msg, 3);
                break;
            }
            strcpy(estado->archivoComprimido, estado->campoTemp[0]);
            strcpy(estado->archivoDescomprimido, estado->campoTemp[1]);
            descomprimirArchivo(estado->campoTemp[0], estado->campoTemp[1]);
            UI_AgregarLog(estado, "Archivo '%s' descompactado -> '%s'", estado->campoTemp[0], estado->campoTemp[1]);
            estado->pantalla = PANTALLA_MENU;
            break;
        }

        case 4: {
            bool existeIzq = FileExists(estado->campoTemp[0]);
            bool existeDer = FileExists(estado->campoTemp[1]);
            if (!existeIzq || !existeDer) {
                char msg[400];
                if (!existeIzq && !existeDer)
                    snprintf(msg, sizeof(msg), "No se encontraron '%s' ni '%s'.\nEscribi los nombres correctos (con su extension) e intenta de nuevo.",
                             estado->campoTemp[0], estado->campoTemp[1]);
                else if (!existeIzq)
                    snprintf(msg, sizeof(msg), "No se encontro el archivo IZQUIERDO '%s'.\nEscribi el nombre correcto (con su extension) e intenta de nuevo.", estado->campoTemp[0]);
                else
                    snprintf(msg, sizeof(msg), "No se encontro el archivo DERECHO '%s'.\nEscribi el nombre correcto (con su extension) e intenta de nuevo.", estado->campoTemp[1]);
                UI_MostrarAdvertencia(estado, msg, 4);
                break;
            }
            /* UI_CargarComparacionHuffman ya deja la pantalla lista (o vuelve al menu si fallo) */
            UI_CargarComparacionHuffman(estado, estado->campoTemp[0], estado->campoTemp[1], "ORIGINAL", "COMPARADO");
            break;
        }

        case 5: {
            if (!FileExists(estado->campoTemp[0])) {
                char msg[256];
                snprintf(msg, sizeof(msg), "No se encontro el archivo '%s'.\nEscribi el nombre correcto (con su extension) e intenta de nuevo.", estado->campoTemp[0]);
                UI_MostrarAdvertencia(estado, msg, 5);
                break;
            }
            int dia, mes, anio, hora, minuto;
            if (sscanf(estado->campoTemp[2], "%d/%d/%d %d:%d", &dia, &mes, &anio, &hora, &minuto) != 5) {
                UI_AgregarLog(estado, "Formato de fecha/hora invalido ('%s'). Usa DD/MM/AAAA HH:MM.", estado->campoTemp[2]);
                UI_PedirCampo(estado, 5, 2, 3, estado->campoTemp[2], "Fecha y hora a grabar en el archivo (DD/MM/AAAA HH:MM):");
                break;
            }
            /* archivo, nombre base y fecha/hora ya confirmados (campoTemp[0]/[1]/[2]):
               falta el modulo, que se pide en una pantalla separada y se resuelve en main(). */
            UI_PedirModulo(estado, 5);
            break;
        }

        case 6: {
            if (!FileExists(estado->campoTemp[0])) {
                char msg[256];
                snprintf(msg, sizeof(msg), "No se encontro el archivo '%s'.\nEscribi el nombre correcto (con su extension) e intenta de nuevo.", estado->campoTemp[0]);
                UI_MostrarAdvertencia(estado, msg, 6);
                break;
            }
            /* archivo y nombre de salida ya confirmados (campoTemp[0]/[1]): falta el modulo,
               que se pide en una pantalla separada y se resuelve en main(). */
            UI_PedirModulo(estado, 6);
            break;
        }

        case 7: {
            if (!FileExists(estado->campoTemp[0])) {
                char msg[256];
                snprintf(msg, sizeof(msg), "No se encontro el archivo '%s'.\nEscribi el nombre correcto (con su extension) e intenta de nuevo.", estado->campoTemp[0]);
                UI_MostrarAdvertencia(estado, msg, 7);
                break;
            }
            char motivo[300];
            if (!VerificarMarcaDeTiempoArchivo(estado->campoTemp[0], motivo, sizeof(motivo))) {
                char msg[600];
                snprintf(msg, sizeof(msg),
                         "El archivo '%s' no paso la verificacion de fecha/hora\n(debe decir, en su primera linea: hoy, a la hora anterior a la actual):\n\n%s",
                         estado->campoTemp[0], motivo);
                UI_MostrarAdvertencia(estado, msg, 7);
                break;
            }
            /* archivo y nombre de salida ya confirmados (campoTemp[0]/[1]): falta el modulo,
               que se pide en una pantalla separada y se resuelve en main(). El tamanio original
               ya no se pide: se calcula solo a partir del propio archivo protegido cargado. */
            UI_PedirModulo(estado, 7);
            break;
        }

        case 8: {
            if (!FileExists(estado->campoTemp[0])) {
                char msg[256];
                snprintf(msg, sizeof(msg), "No se encontro el archivo '%s'.\nEscribi el nombre correcto (con su extension) e intenta de nuevo.", estado->campoTemp[0]);
                UI_MostrarAdvertencia(estado, msg, 8);
                break;
            }
            char motivo[300];
            if (!VerificarMarcaDeTiempoArchivo(estado->campoTemp[0], motivo, sizeof(motivo))) {
                char msg[600];
                snprintf(msg, sizeof(msg),
                         "El archivo '%s' no paso la verificacion de fecha/hora\n(debe decir, en su primera linea: hoy, a la hora anterior a la actual):\n\n%s",
                         estado->campoTemp[0], motivo);
                UI_MostrarAdvertencia(estado, msg, 8);
                break;
            }
            UI_PedirModulo(estado, 8);
            break;
        }

        case 9:
            strcpy(estado->archivoOriginal, estado->campoTemp[0]);
            strcpy(estado->archivoComprimido, estado->campoTemp[1]);
            strcpy(estado->archivoDescomprimido, estado->campoTemp[2]);
            /* UI_CargarEstadisticas ya deja la pantalla en PANTALLA_ESTADISTICAS */
            UI_CargarEstadisticas(estado);
            break;

        case 10: {
            char nombreFinal[MAX_NOMBRE_LEN + 8];
            snprintf(nombreFinal, sizeof(nombreFinal), "%s.H2x", estado->campoTemp[0]);
            introducir_dos_errores(estado->cadenaH, estado->modulo, estado->largoH, &estado->errores);
            estado->hayErrores = 1;

            /* mantenemos la MISMA fecha/hora y el MISMO tamanio original exacto que ya tenia
               el archivo protegido del que viene el estado actual en memoria, en vez de
               volver a estampar la hora actual o perder el tamanio real */
            char fechaOriginal[256];
            long tamOriginalPropagado = 0;
            bool hayCabecera = estado->archivoProtegidoActual[0] &&
                                LeerCabeceraProtegida(estado->archivoProtegidoActual, fechaOriginal, sizeof(fechaOriginal), &tamOriginalPropagado);
            const char *fechaAUsar = hayCabecera ? fechaOriginal : NULL;
            GuardarCadenaProtegida(estado->cadenaH, estado->largoH, nombreFinal, fechaAUsar, tamOriginalPropagado);
            strncpy(estado->archivoProtegidoActual, nombreFinal, MAX_NOMBRE_LEN - 1);
            estado->archivoProtegidoActual[MAX_NOMBRE_LEN - 1] = '\0';
            UI_AgregarLog(estado, "Dos errores introducidos por bloque (total %d) -> guardado en '%s'", estado->errores, nombreFinal);
            estado->pantalla = PANTALLA_MENU;
            break;
        }

        default:
            estado->pantalla = PANTALLA_MENU;
            break;
    }
}

static void ProcesarOpcion(EstadoApp *estado, int opcion, bool *salir) {
    switch (opcion) {
        case 1: case 2: case 3: case 5: case 6: case 7: case 8: case 9: case 10:
            IniciarSeleccionDeArchivos(estado, opcion);
            break;

        case 4:
            /* Siempre se eligen los dos archivos a comparar, sin importar el estado de Hamming. */
            IniciarSeleccionDeArchivos(estado, 4);
            break;

        case 0:
            *salir = true;
            break;
    }
}

/* ===================== PUNTO DE ENTRADA ===================== */

int main(void) {
    const int anchoVentana = 1280;
    const int altoVentana  = 800;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(anchoVentana, altoVentana, "Compresor Huffman + Proteccion Hamming");
    SetExitKey(KEY_NULL); /* ESC ya NO cierra la ventana: cada pantalla lo usa para volver atras */
    SetTargetFPS(60);

    /* "static" es clave aca: EstadoApp pesa varios MB (por comparacionTexto[4000]),
       y como variable local normal (en el stack) desborda el stack por defecto de
       Windows (tipicamente 1 MB), causando un crash STATUS_STACK_OVERFLOW (0xC00000FD)
       al arrancar. Como variable estatica vive en el segmento de datos, no en el stack. */
    static EstadoApp estado;
    UI_Inicializar(&estado);

    bool salir = false;

    while (!WindowShouldClose() && !salir) {
        BeginDrawing();

        switch (estado.pantalla) {
            case PANTALLA_MENU: {
                int opcion = UI_ActualizarYDibujarMenu(&estado);
                if (opcion >= 0) ProcesarOpcion(&estado, opcion, &salir);
                break;
            }

            case PANTALLA_PIDIENDO_NOMBRE: {
                if (UI_ActualizarYDibujarInput(&estado)) {
                    ContinuarSeleccionDeArchivos(&estado);
                }
                break;
            }

            case PANTALLA_PIDIENDO_MODULO: {
                if (UI_ActualizarYDibujarInput(&estado)) {
                    int modulo = atoi(estado.inputBuffer);
                    if (modulo != 8 && modulo != 1024 && modulo != 16384) {
                        char msg[200];
                        snprintf(msg, sizeof(msg),
                                 "El modulo ingresado (%d) no es valido.\nLos unicos modulos de Hamming aceptados son: 8, 1024 o 16384.",
                                 modulo);
                        UI_MostrarAdvertencia(&estado, msg, estado.accionPendiente);
                    } else if (estado.accionPendiente == 5) {
                        /* opcion 5: campoTemp[0] = archivo a proteger, campoTemp[1] = nombre base de salida */
                        strncpy(estado.nombreArchivo, estado.campoTemp[0], MAX_NOMBRE_LEN - 1);
                        estado.nombreArchivo[MAX_NOMBRE_LEN - 1] = '\0';
                        strcpy(estado.archivoOriginal, estado.nombreArchivo);
                        CargarArchivo(&estado);

                        if (estado.archivoListo) {
                            AplicarHamming(&estado, modulo);
                            char nombreFinal[MAX_NOMBRE_LEN + 8];
                            snprintf(nombreFinal, sizeof(nombreFinal), "%s%s", estado.campoTemp[1], ExtensionHamming(modulo));
                            GuardarCadenaProtegida(estado.cadenaH, estado.largoH, nombreFinal, estado.campoTemp[2], estado.tamanioArchivo);
                            strncpy(estado.archivoProtegidoActual, nombreFinal, MAX_NOMBRE_LEN - 1);
                            estado.archivoProtegidoActual[MAX_NOMBRE_LEN - 1] = '\0';
                            UI_AgregarLog(&estado, "Archivo '%s' protegido con Hamming (modulo %d) -> guardado en '%s'",
                                          estado.campoTemp[0], modulo, nombreFinal);
                        }
                        estado.pantalla = PANTALLA_MENU;
                    } else if (estado.accionPendiente == 6) {
                        /* opcion 6: campoTemp[0] = archivo protegido de entrada, campoTemp[1] = nombre de salida.
                           Cargamos ese archivo del disco (no necesariamente el que estaba en memoria),
                           le introducimos errores, y el resultado pasa a ser el estado actual en memoria. */
                        estado.modulo = modulo;
                        int largoCadena = 0;
                        char fechaOriginal[256] = "";
                        long tamOriginalLeido = 0;
                        int *cadenaCargada = CargarCadenaProtegidaDesdeArchivo(estado.campoTemp[0], &largoCadena,
                                                                                fechaOriginal, sizeof(fechaOriginal), &tamOriginalLeido);
                        if (!cadenaCargada) {
                            UI_AgregarLog(&estado, "Error: no se pudo leer '%s'.", estado.campoTemp[0]);
                        } else {
                            if (estado.cadenaH) { free(estado.cadenaH); estado.cadenaH = NULL; }
                            if (estado.cadenaHOriginal) { free(estado.cadenaHOriginal); estado.cadenaHOriginal = NULL; }

                            estado.cadenaH = cadenaCargada;
                            estado.largoH = largoCadena;
                            estado.cadenaHOriginal = malloc((size_t)largoCadena * sizeof(int));
                            memcpy(estado.cadenaHOriginal, cadenaCargada, (size_t)largoCadena * sizeof(int));

                            estado.errores = 0;
                            introducir_errores(estado.cadenaH, estado.modulo, estado.largoH, &estado.errores);
                            estado.hayErrores = 1;

                            char nombreFinal[MAX_NOMBRE_LEN + 8];
                            snprintf(nombreFinal, sizeof(nombreFinal), "%s.Hex", estado.campoTemp[1]);

                            /* mantenemos la MISMA fecha/hora y el MISMO tamanio original exacto
                               que ya tenia el archivo protegido de entrada */
                            const char *fechaAUsar = fechaOriginal[0] ? fechaOriginal : NULL;
                            GuardarCadenaProtegida(estado.cadenaH, estado.largoH, nombreFinal, fechaAUsar, tamOriginalLeido);
                            strncpy(estado.archivoProtegidoActual, nombreFinal, MAX_NOMBRE_LEN - 1);
                            estado.archivoProtegidoActual[MAX_NOMBRE_LEN - 1] = '\0';

                            UI_AgregarLog(&estado, "Errores introducidos en '%s' (modulo %d, total %d) -> guardado en '%s'",
                                          estado.campoTemp[0], modulo, estado.errores, nombreFinal);
                        }
                        estado.pantalla = PANTALLA_MENU;
                    } else {
                        /* opciones 7/8: campoTemp[0] = archivo protegido, campoTemp[1] = nombre de salida.
                           Ya tenemos el modulo; cargamos el archivo elegido. El tamanio original EXACTO
                           se lee de su propia cabecera ("BYTES:..."), asi que nunca hay que adivinarlo
                           -- adivinarlo a partir del tamanio del archivo protegido fallaba con modulos
                           grandes (16384) en archivos chicos, porque casi todo el bloque es relleno de
                           ceros y ese relleno se filtraba como si fuera contenido real. */
                        estado.modulo = modulo;
                        int largoCadena = 0;
                        long tamOriginal = 0;
                        int *cadenaCargada = CargarCadenaProtegidaDesdeArchivo(estado.campoTemp[0], &largoCadena,
                                                                                NULL, 0, &tamOriginal);
                        if (!cadenaCargada) {
                            UI_AgregarLog(&estado, "Error: no se pudo leer '%s'.", estado.campoTemp[0]);
                            estado.pantalla = PANTALLA_MENU;
                        } else if (tamOriginal <= 0) {
                            UI_AgregarLog(&estado, "El archivo '%s' no tiene un tamanio original valido en su cabecera "
                                          "(puede ser un archivo protegido con una version anterior del programa).",
                                          estado.campoTemp[0]);
                            free(cadenaCargada);
                            estado.pantalla = PANTALLA_MENU;
                        } else {
                            char nombreFinal[MAX_NOMBRE_LEN + 8];

                            if (estado.accionPendiente == 7) {
                                snprintf(nombreFinal, sizeof(nombreFinal), "%s.Dex", estado.campoTemp[1]);
                                generarArchivoDEX(cadenaCargada, estado.modulo, largoCadena, nombreFinal, tamOriginal);
                                UI_AgregarLog(&estado, "Desprotegido SIN corregir '%s' (modulo %d) -> %s",
                                              estado.campoTemp[0], estado.modulo, nombreFinal);
                                estado.pantalla = PANTALLA_MENU;
                            } else {
                                int bloquesIncorregibles = ContarBloquesConErrorDoble(cadenaCargada, estado.modulo, largoCadena);

                                snprintf(nombreFinal, sizeof(nombreFinal), "%s.Dcx", estado.campoTemp[1]);
                                if (estado.infoRecuperada) { free(estado.infoRecuperada); estado.infoRecuperada = NULL; }
                                estado.infoRecuperada = decodificarHamming(cadenaCargada, estado.modulo, largoCadena,
                                                                            &estado.largoInfo, (int)tamOriginal);
                                guardarInfoRecuperada(estado.infoRecuperada, estado.largoInfo, nombreFinal);

                                if (bloquesIncorregibles > 0) {
                                    char msg[700];
                                    snprintf(msg, sizeof(msg),
                                             "El archivo '%s' tiene %d bloque(s) con DOS errores en el mismo modulo.\n"
                                             "Con un solo bit de paridad extra, Hamming no puede corregir un error doble:\n"
                                             "esos bloques NO se pudieron corregir.\n\n"
                                             "El archivo '%s' se guardo de todas formas, pero el contenido\n"
                                             "de esos bloques puede estar incorrecto.",
                                             estado.campoTemp[0], bloquesIncorregibles, nombreFinal);
                                    UI_MostrarAdvertencia(&estado, msg, 0);
                                } else {
                                    UI_AgregarLog(&estado, "Desprotegido CORRIGIENDO '%s' (modulo %d) -> %s",
                                                  estado.campoTemp[0], estado.modulo, nombreFinal);
                                    estado.pantalla = PANTALLA_MENU;
                                }
                            }
                            free(cadenaCargada);
                        }
                    }
                }
                break;
            }

            case PANTALLA_COMPARAR_HUFFMAN:
            case PANTALLA_COMPARAR_HAMMING:
                UI_DibujarComparacion(&estado);
                break;

            case PANTALLA_ESTADISTICAS:
                UI_DibujarEstadisticas(&estado);
                break;

            case PANTALLA_ADVERTENCIA: {
                if (UI_DibujarAdvertencia(&estado)) {
                    int opcionReintento = estado.opcionReintento;
                    if (opcionReintento > 0) IniciarSeleccionDeArchivos(&estado, opcionReintento);
                    else estado.pantalla = PANTALLA_MENU;
                }
                break;
            }
        }

        EndDrawing();
    }

    UI_Liberar(&estado);
    CloseWindow();
    return 0;
}
