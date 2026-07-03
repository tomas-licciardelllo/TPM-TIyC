#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

/* ===================== PALETA DE COLORES ===================== */
#define COLOR_FONDO        (Color){ 24, 26, 32, 255 }
#define COLOR_PANEL        (Color){ 34, 37, 46, 255 }
#define COLOR_PANEL_CLARO  (Color){ 46, 50, 61, 255 }
#define COLOR_BORDE        (Color){ 70, 75, 90, 255 }
#define COLOR_ACENTO       (Color){ 86, 182, 194, 255 }
#define COLOR_ACENTO_OSCURO (Color){ 52, 110, 118, 255 }
#define COLOR_TEXTO        (Color){ 225, 228, 232, 255 }
#define COLOR_TEXTO_TENUE  (Color){ 150, 155, 165, 255 }
#define COLOR_OK           (Color){ 120, 200, 140, 255 }
#define COLOR_DIFF         (Color){ 235, 80, 80, 255 }

/* ===================== TAMANIOS DE TEXTO ===================== */
/* Centralizados aca para que sea facil agrandarlos a todos juntos. */
#define TAM_TITULO        28
#define TAM_SUBTITULO      18
#define TAM_BOTON          20
#define TAM_DESCRIPCION    19
#define TAM_LOG            17
#define TAM_MODAL_TITULO   20
#define TAM_MODAL_TEXTO    23
#define TAM_HEADER_COMP    21
#define TAM_FILA_COMP      20
#define TAM_PIE            15

/* ===================== TIPOGRAFIA ===================== */
/* raylib no trae "Times New Roman" embebida (es una fuente propietaria de Microsoft),
   asi que probamos cargarla si esta instalada en el sistema, y si no, caemos en una
   fuente serif equivalente que casi siempre esta disponible en Linux/Mac/Windows.
   Si el usuario quiere forzar la tipografia exacta, puede copiar un "times.ttf" junto
   al ejecutable: ese es el primer candidato que se intenta. */
static Font g_fuente;
static bool g_fuentePropia = false;

/* Por defecto LoadFontEx solo carga el set ASCII basico (32-126), asi que cualquier
   tilde, �/�, �, �, etc. queda sin glifo y no se dibuja. Armamos una lista mas amplia
   de codepoints (ASCII + Latin-1 Supplement + Latin Extended-A) para que se vea
   correctamente cualquier caracter de texto en espanol (y la mayoria de otros idiomas
   latinos). */
static int ConstruirCodepoints(int *buffer, int maxBuffer) {
    int n = 0;
    for (int c = 32; c <= 126 && n < maxBuffer; c++) buffer[n++] = c;          /* ASCII visible */
    for (int c = 0x00A1; c <= 0x017F && n < maxBuffer; c++) buffer[n++] = c;   /* tildes, �/�, �, �, etc. */
    if (n < maxBuffer) buffer[n++] = 0x20AC;                                  /* � */
    return n;
}

static void CargarFuente(void) {
    static const char *candidatos[] = {
        "LiberationSerif-Regular.ttf",                                     /* se entrega junto al .exe: SIEMPRE deberia encontrarse */
        "times.ttf",                                                       /* por si el usuario prefiere copiar su propia Times New Roman */
        "Times New Roman.ttf",
        "C:/Windows/Fonts/times.ttf",                                      /* Windows */
        "/Library/Fonts/Times New Roman.ttf",                              /* macOS */
        "/System/Library/Fonts/Supplemental/Times New Roman.ttf",          /* macOS */
        "/usr/share/fonts/truetype/liberation/LiberationSerif-Regular.ttf",/* Linux (metric-compatible con Times) */
        "/usr/share/fonts/truetype/freefont/FreeSerif.ttf",                /* Linux */
        "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf",                /* Linux */
    };
    int cantidad = (int)(sizeof(candidatos) / sizeof(candidatos[0]));

    static int codepoints[400];
    int totalCodepoints = ConstruirCodepoints(codepoints, 400);

    for (int i = 0; i < cantidad; i++) {
        if (FileExists(candidatos[i])) {
            Font f = LoadFontEx(candidatos[i], 64, codepoints, totalCodepoints);
            if (f.texture.id != 0) {
                SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
                g_fuente = f;
                g_fuentePropia = true;
                return;
            }
        }
    }
    /* nada encontrado: usamos la fuente por defecto de raylib (solo ASCII basico) */
    g_fuente = GetFontDefault();
    g_fuentePropia = false;
}

static void Texto(const char *texto, int x, int y, int fontSize, Color color) {
    DrawTextEx(g_fuente, texto, (Vector2){ (float)x, (float)y }, (float)fontSize, 1.0f, color);
}

static int AnchoTexto(const char *texto, int fontSize) {
    return (int)MeasureTextEx(g_fuente, texto, (float)fontSize, 1.0f).x;
}

/* Texto explicativo de la logica de cada opcion del menu (se ve al pasar el mouse) */
static const char *DESCRIPCION_OPCION[11] = {
    /*0*/ "Salir.\nLibera toda la memoria reservada\n(chain, cadenaH, infoRecuperada)\ny cierra el programa.",
    /*1*/ "Cargar archivo.\nVos elegis el nombre del archivo.\nSe lee byte a byte y se convierte\nen un arreglo de bits (8 bits por\nbyte, MSB primero). Ese arreglo\n'chain' es la base para Hamming.",
    /*2*/ "Compactar (Huffman).\nElegis el archivo de ENTRADA y el\nde SALIDA (.huf). 1) Cuenta la\nfrecuencia de cada byte. 2) Arma\nel arbol combinando los 2 nodos\nmas chicos. 3) Asigna codigos:\n0=izquierda, 1=derecha.",
    /*3*/ "Descompactar.\nElegis el archivo de entrada: puede\nser .huf, o .Dex/.Dcx/.Ha1/.Ha2/.Ha3\nsi adentro tienen datos comprimidos\npor Huffman (ej. si protegiste con\nHamming el .huf en vez del .txt).\nReconstruye el arbol y recorre bit a\nbit: 0 izquierda, 1 derecha, hoja=byte.",
    /*4*/ "Ver archivos en pantalla.\nSiempre te pide los dos archivos a\ncomparar (izquierdo y derecho), sin\nimportar si ya usaste Hamming o no.\nSe muestran linea a linea, con las\ndiferencias marcadas en rojo.",
    /*5*/ "Proteger archivo (Hamming).\nElegis el archivo de ENTRADA y el\nnombre BASE de salida (sin extension).\nSe guarda con extension automatica\nsegun el modulo: .Ha1 (8), .Ha2\n(1024) o .ha3 (16384). La cabecera\nqueda con fecha/hora y el tamanio\noriginal exacto (se usa al desproteger).",
    /*6*/ "Introducir errores.\nElegis el archivo PROTEGIDO a abrir,\nel nombre BASE de salida y el modulo.\nPor cada bloque de tamanio 'modulo'\nse decide al azar si se invierte un\nbit (XOR 1), simulando ruido, y se\nguarda con extension .Hex.",
    /*7*/ "Desproteger SIN corregir.\nElegis el archivo PROTEGIDO a abrir;\nse revisa que su PRIMERA LINEA diga\nhoy o antes, a la hora actual o antes\n(si no, se avisa y no se continua).\nLuego nombre de salida (.Dex) y modulo;\nel tamanio original se lee EXACTO de\nla cabecera. Extrae solo bits de DATOS.",
    /*8*/ "Desproteger CORRIGIENDO.\nElegis el archivo PROTEGIDO a abrir;\nse revisa que su PRIMERA LINEA diga\nhoy o antes, a la hora actual o antes\n(si no, se avisa y no se continua).\nLuego nombre de salida (.Dcx) y modulo;\nel tamanio original se lee EXACTO de\nla cabecera. Corrige por paridad/sindrome.",
    /*9*/ "Ver estadistica.\nElegis los 3 archivos a comparar\n(original, compactado, descompactado)\ny se muestra el tamanio en bytes de\ncada uno y el ratio de compresion\ncomo barras.",
    /*10*/ "Introducir DOS errores.\nElegis el nombre BASE de salida\n(se guarda con extension .H2x).\nIgual logica que 'Introducir errores'\npero invierte DOS bits distintos por\nbloque en vez de uno: un error doble\nque Hamming simple no puede corregir.",
};

static const char *NOMBRE_OPCION[11] = {
    "Salir", "Cargar archivo", "Compactar (Huffman)", "Descompactar",
    "Ver archivos en pantalla", "Proteger (Hamming)", "Introducir errores",
    "Desproteger sin corregir", "Desproteger corrigiendo", "Ver estadistica",
    "Introducir dos errores"
};

/* ===================== UTILIDADES INTERNAS ===================== */

static bool Boton(Rectangle r, const char *texto, bool activo, bool resaltado) {
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, r);

    Color fondo = COLOR_PANEL_CLARO;
    Color borde = COLOR_BORDE;
    Color textoColor = activo ? COLOR_TEXTO : COLOR_TEXTO_TENUE;

    if (resaltado || hover) { fondo = COLOR_ACENTO_OSCURO; borde = COLOR_ACENTO; }

    DrawRectangleRounded(r, 0.18f, 6, fondo);
    DrawRectangleRoundedLines(r, 0.18f, 6, borde);

    int fontSize = TAM_BOTON;
    Texto(texto, (int)(r.x + 14), (int)(r.y + (r.height - fontSize) / 2.0f), fontSize, textoColor);

    return hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

/* Dibuja `texto` comparandolo contra `referencia` caracter por caracter.
   Los tramos donde coinciden se dibujan en colorNormal, donde difieren en rojo. */
static void DibujarLineaConDiff(const char *referencia, const char *texto, int x, int y, int fontSize, Color colorNormal) {
    int i = 0;
    int cursorX = x;
    int len = (int)strlen(texto);
    int lenRef = referencia ? (int)strlen(referencia) : 0;

    while (i < len) {
        int j = i;
        bool difiereInicial = (i >= lenRef) || (texto[i] != referencia[i]);
        while (j < len) {
            bool difiere = (j >= lenRef) || (texto[j] != referencia[j]);
            if (difiere != difiereInicial) break;
            j++;
        }
        char tramo[512];
        int largoTramo = j - i;
        if (largoTramo > 510) largoTramo = 510;
        memcpy(tramo, texto + i, largoTramo);
        tramo[largoTramo] = '\0';

        Color c = difiereInicial ? COLOR_DIFF : colorNormal;
        Texto(tramo, cursorX, y, fontSize, c);
        cursorX += AnchoTexto(tramo, fontSize);
        i = j;
    }
}

/* Calcula cuantos caracteres entran en un ancho disponible (en pixeles) con la fuente actual,
   midiendo una muestra representativa de letras (para fuentes proporcionales). */
static int CaracteresPorLinea(int fontSize, int anchoPixelesDisponible) {
    const char *muestra = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    int anchoMuestra = AnchoTexto(muestra, fontSize);
    float anchoPromedio = (float)anchoMuestra / (float)strlen(muestra);
    if (anchoPromedio < 1.0f) anchoPromedio = 1.0f;
    int caracteres = (int)(anchoPixelesDisponible / anchoPromedio);
    if (caracteres < 10) caracteres = 10;
    if (caracteres > MAX_LINEA_LEN - 2) caracteres = MAX_LINEA_LEN - 2;
    return caracteres;
}

/* Un texto en UTF-8 puede tener caracteres de 1 a 4 bytes (tildes, �, simbolos, etc.).
   Si cortamos una linea larga por cantidad fija de BYTES podemos partir uno de esos
   caracteres a la mitad, lo que lo deja invalido y se ve mal (o no se ve). Esta funcion
   busca, a partir de `desde`, el corte mas cercano a `maxBytes` que no caiga en medio
   de un caracter multibyte, y devuelve cuantos bytes hay que copiar. */
static int LimiteUTF8Seguro(const char *texto, int largoTotal, int desde, int maxBytes) {
    int limite = desde + maxBytes;
    if (limite >= largoTotal) return largoTotal - desde;
    /* 0x80 = 10xxxxxx -> byte de continuacion UTF-8: retrocedemos hasta el inicio del caracter */
    while (limite > desde && ((unsigned char)texto[limite] & 0xC0) == 0x80) limite--;
    return limite - desde;
}

/* ===================== CICLO DE VIDA ===================== */

void UI_Inicializar(EstadoApp *estado) {
    memset(estado, 0, sizeof(EstadoApp));
    estado->pantalla = PANTALLA_MENU;
    estado->opcionHover = -1;
    strcpy(estado->archivoOriginal, "ej.txt");
    strcpy(estado->archivoComprimido, "ej.huf");
    strcpy(estado->archivoDescomprimido, "ej.dhu");
    strcpy(estado->nombreArchivo, "ej.txt");
    CargarFuente();
    UI_AgregarLog(estado, "Bienvenido. Empeza por la opcion 1 (Cargar archivo).");
    if (!g_fuentePropia) {
        UI_AgregarLog(estado, "Tip: no se encontro Times New Roman/serif en el sistema; usando fuente por defecto.");
    }
}

void UI_Liberar(EstadoApp *estado) {
    if (estado->chain) free(estado->chain);
    if (estado->cadenaH) free(estado->cadenaH);
    if (estado->cadenaHOriginal) free(estado->cadenaHOriginal);
    if (estado->infoRecuperada) free(estado->infoRecuperada);
    if (g_fuentePropia) UnloadFont(g_fuente);
    memset(estado, 0, sizeof(EstadoApp));
}

void UI_AgregarLog(EstadoApp *estado, const char *formato, ...) {
    char buffer[MAX_LOG_LEN];
    va_list args;
    va_start(args, formato);
    vsnprintf(buffer, sizeof(buffer), formato, args);
    va_end(args);

    if (estado->logCantidad < MAX_LOG_LINEAS) {
        strcpy(estado->log[estado->logCantidad], buffer);
        estado->logCantidad++;
    } else {
        /* historial lleno: desplazamos todo hacia arriba y agregamos abajo */
        for (int i = 1; i < MAX_LOG_LINEAS; i++) strcpy(estado->log[i - 1], estado->log[i]);
        strcpy(estado->log[MAX_LOG_LINEAS - 1], buffer);
    }
}

/* ===================== PANEL DE LOG (scrolleable) ===================== */

void UI_DibujarLog(EstadoApp *estado) {
    int alto = GetScreenHeight();
    int ancho = GetScreenWidth();
    int panelAlto = 180; /* mismo tamanio de siempre */
    Rectangle panel = { 10, (float)(alto - panelAlto - 10), (float)(ancho - 20), (float)panelAlto };

    DrawRectangleRounded(panel, 0.05f, 8, COLOR_PANEL);
    DrawRectangleRoundedLines(panel, 0.05f, 8, COLOR_BORDE);
    Texto("Registro / mensajes", (int)panel.x + 12, (int)panel.y + 8, TAM_SUBTITULO, COLOR_ACENTO);

    int barraAncho = 22;
    int filaAlto = TAM_LOG + 6;
    int areaTop = (int)panel.y + 36;
    int areaAlto = panelAlto - 44;
    int areaAnchoTexto = (int)panel.width - 24 - barraAncho;
    int filasVisibles = areaAlto / filaAlto;
    if (filasVisibles < 1) filasVisibles = 1;

    int maxScroll = estado->logCantidad - filasVisibles;
    if (maxScroll < 0) maxScroll = 0;

    /* ---- controles de scroll: botones arriba/abajo + barra con "ascensor" arrastrable ---- */
    Rectangle barra = { (float)(panel.x + panel.width - barraAncho - 6), (float)areaTop, (float)barraAncho, (float)areaAlto };
    Rectangle btnArriba = { barra.x, barra.y, (float)barraAncho, (float)barraAncho };
    Rectangle btnAbajo  = { barra.x, barra.y + areaAlto - barraAncho, (float)barraAncho, (float)barraAncho };

    if (Boton(btnArriba, "^", maxScroll > 0, false)) estado->scrollLog += 1;  /* ver mensajes mas viejos */
    if (Boton(btnAbajo,  "v", maxScroll > 0, false)) estado->scrollLog -= 1;  /* ver mensajes mas nuevos (volver hacia abajo) */

    /* rueda del mouse sobre el panel, como atajo extra */
    if (CheckCollisionPointRec(GetMousePosition(), panel)) {
        float rueda = GetMouseWheelMove();
        if (rueda != 0) estado->scrollLog += (int)(rueda * 3);
    }

    if (estado->scrollLog < 0) estado->scrollLog = 0;
    if (estado->scrollLog > maxScroll) estado->scrollLog = maxScroll;

    /* "ascensor" (thumb) que se puede arrastrar para moverse rapido por todo el historial */
    if (maxScroll > 0) {
        float trackY = barra.y + barraAncho;
        float trackAlto = (float)areaAlto - 2.0f * barraAncho;
        float thumbAlto = trackAlto * ((float)filasVisibles / (float)estado->logCantidad);
        if (thumbAlto < 16.0f) thumbAlto = 16.0f;
        if (thumbAlto > trackAlto) thumbAlto = trackAlto;

        /* scrollLog=0 (lo mas nuevo) -> ascensor abajo. scrollLog=maxScroll (lo mas viejo) -> ascensor arriba. */
        float progreso = 1.0f - ((float)estado->scrollLog / (float)maxScroll);
        float thumbY = trackY + progreso * (trackAlto - thumbAlto);
        Rectangle thumb = { barra.x + 3, thumbY, (float)barraAncho - 6, thumbAlto };

        DrawRectangleRounded((Rectangle){ barra.x + 3, trackY, (float)barraAncho - 6, trackAlto }, 0.4f, 4, COLOR_PANEL_CLARO);

        static bool arrastrando = false;
        bool mouseSobreThumb = CheckCollisionPointRec(GetMousePosition(), thumb);
        if (mouseSobreThumb && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) arrastrando = true;
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) arrastrando = false;
        if (arrastrando && (trackAlto - thumbAlto) > 0.5f) {
            float mouseY = GetMousePosition().y;
            float rel = (mouseY - trackY - thumbAlto / 2.0f) / (trackAlto - thumbAlto);
            if (rel < 0) rel = 0;
            if (rel > 1) rel = 1;
            estado->scrollLog = (int)((1.0f - rel) * (float)maxScroll + 0.5f);
        }

        DrawRectangleRounded(thumb, 0.4f, 4, (mouseSobreThumb || arrastrando) ? COLOR_ACENTO : COLOR_BORDE);
    }

    /* dibujamos de abajo hacia arriba: el mensaje mas nuevo visible queda al fondo del panel */
    int indiceMasNuevoVisible = estado->logCantidad - 1 - estado->scrollLog;
    int y = areaTop + (filasVisibles - 1) * filaAlto;

    BeginScissorMode((int)panel.x, areaTop - 2, areaAnchoTexto + 12, areaAlto + 4);
    for (int i = 0; i < filasVisibles; i++) {
        int idx = indiceMasNuevoVisible - i;
        if (idx < 0) break;
        Texto(estado->log[idx], (int)panel.x + 12, y, TAM_LOG, COLOR_TEXTO_TENUE);
        y -= filaAlto;
    }
    EndScissorMode();

    if (estado->logCantidad > filasVisibles) {
        char info[80];
        snprintf(info, sizeof(info), "%d/%d mensajes", estado->logCantidad - estado->scrollLog, estado->logCantidad);
        Texto(info, (int)panel.x + 12 + areaAnchoTexto - AnchoTexto(info, TAM_PIE), (int)panel.y + 10, TAM_PIE, COLOR_TEXTO_TENUE);
    }
}

/* ===================== CARTEL DE ADVERTENCIA ===================== */

void UI_MostrarAdvertencia(EstadoApp *estado, const char *mensaje, int opcionReintento) {
    strncpy(estado->mensajeAdvertencia, mensaje, sizeof(estado->mensajeAdvertencia) - 1);
    estado->mensajeAdvertencia[sizeof(estado->mensajeAdvertencia) - 1] = '\0';
    estado->opcionReintento = opcionReintento;
    estado->pantalla = PANTALLA_ADVERTENCIA;
}

int UI_DibujarAdvertencia(EstadoApp *estado) {
    ClearBackground(COLOR_FONDO);
    int ancho = GetScreenWidth(), alto = GetScreenHeight();

    bool conReintento = (estado->opcionReintento > 0);

    Rectangle caja = { (float)(ancho / 2 - 380), (float)(alto / 2 - 180), 760, 360 };
    DrawRectangleRounded(caja, 0.05f, 8, COLOR_PANEL);
    DrawRectangleRoundedLines(caja, 0.05f, 8, COLOR_DIFF);

    const char *titulo = conReintento ? "ATENCION" : "AVISO";
    Texto(titulo, (int)caja.x + 26, (int)caja.y + 22, TAM_HEADER_COMP, COLOR_DIFF);
    Texto(estado->mensajeAdvertencia, (int)caja.x + 26, (int)caja.y + 64, TAM_DESCRIPCION, COLOR_TEXTO);

    Rectangle rBoton = { (float)(caja.x + caja.width / 2 - 130), (float)(caja.y + caja.height - 68), 260, 46 };
    bool ok = Boton(rBoton, conReintento ? "Entendido, reintentar" : "Entendido", true, true);
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) ok = true;
    return ok ? 1 : 0;
}

/* ===================== MENU PRINCIPAL ===================== */

int UI_ActualizarYDibujarMenu(EstadoApp *estado) {
    ClearBackground(COLOR_FONDO);

    int ancho = GetScreenWidth();

    Texto("COMPRESOR HUFFMAN + PROTECCION HAMMING", 20, 16, TAM_TITULO, COLOR_ACENTO);
    char sub[200];
    snprintf(sub, sizeof(sub), "Archivo de trabajo (Hamming): %s   |   Estado: %s",
             estado->nombreArchivo, estado->archivoListo ? "cargado" : "sin cargar");
    Texto(sub, 20, 52, TAM_SUBTITULO, COLOR_TEXTO_TENUE);

    /* ---- columna izquierda: botones del menu ---- */
    int x = 20, y = 90, w = 400, h = 38, espacio = 6;
    int opcionClick = -1;
    estado->opcionHover = -1;

    for (int op = 1; op <= 10; op++) {
        Rectangle r = { (float)x, (float)(y + (op - 1) * (h + espacio)), (float)w, (float)h };
        bool activo = 1;
        if (op == 10 && estado->cadenaH == NULL) activo = 0; /* necesita Hamming ya aplicado en memoria */

        if (CheckCollisionPointRec(GetMousePosition(), r)) estado->opcionHover = op;

        char etiqueta[64];
        snprintf(etiqueta, sizeof(etiqueta), "%d. %s", op, NOMBRE_OPCION[op]);
        if (Boton(r, etiqueta, activo, false) && activo) opcionClick = op;
    }

    /* boton salir, separado y abajo */
    Rectangle rSalir = { (float)x, (float)(y + 10 * (h + espacio) + 10), (float)w, (float)h };
    if (CheckCollisionPointRec(GetMousePosition(), rSalir)) estado->opcionHover = 0;
    if (Boton(rSalir, "0. Salir", true, false)) opcionClick = 0;

    /* ---- panel derecho: descripcion de la logica de la opcion bajo el mouse ---- */
    Rectangle panelInfo = { (float)(x + w + 30), 90, (float)(ancho - (x + w + 30) - 20), 420 };
    DrawRectangleRounded(panelInfo, 0.04f, 8, COLOR_PANEL);
    DrawRectangleRoundedLines(panelInfo, 0.04f, 8, COLOR_BORDE);

    int op = (estado->opcionHover >= 0) ? estado->opcionHover : -1;
    if (op >= 0) {
        char titulo[64];
        snprintf(titulo, sizeof(titulo), "Opcion %d: %s", op, NOMBRE_OPCION[op]);
        Texto(titulo, (int)panelInfo.x + 16, (int)panelInfo.y + 14, TAM_HEADER_COMP, COLOR_ACENTO);
        Texto(DESCRIPCION_OPCION[op], (int)panelInfo.x + 16, (int)panelInfo.y + 50, TAM_DESCRIPCION, COLOR_TEXTO);
    } else {
        Texto("Pasa el mouse sobre una opcion\npara ver que hace internamente.",
                 (int)panelInfo.x + 16, (int)panelInfo.y + 14, TAM_DESCRIPCION, COLOR_TEXTO_TENUE);
    }

    /* estado rapido de las estructuras en memoria */
    int yy = (int)panelInfo.y + panelInfo.height + 16;
    char info1[160], info2[160], info3[160];
    snprintf(info1, sizeof(info1), "chain: %s   bits leidos: %d", estado->chain ? "cargado" : "(vacio)", estado->n > 0 ? estado->n - 1 : 0);
    snprintf(info2, sizeof(info2), "cadenaH (Hamming): %s   modulo: %d   largo: %d   errores: %d",
             estado->cadenaH ? "presente" : "(vacio)", estado->modulo, estado->largoH, estado->errores);
    snprintf(info3, sizeof(info3), "infoRecuperada: %s   largo: %d", estado->infoRecuperada ? "presente" : "(vacio)", estado->largoInfo);
    Texto(info1, (int)panelInfo.x, yy, TAM_PIE, COLOR_TEXTO_TENUE);
    Texto(info2, (int)panelInfo.x, yy + 21, TAM_PIE, COLOR_TEXTO_TENUE);
    Texto(info3, (int)panelInfo.x, yy + 42, TAM_PIE, COLOR_TEXTO_TENUE);

    UI_DibujarLog(estado);

    return opcionClick;
}

/* ===================== MODALES DE ENTRADA ===================== */

void UI_PedirCampo(EstadoApp *estado, int accion, int indice, int total, const char *valorDefault, const char *titulo) {
    estado->pantalla = PANTALLA_PIDIENDO_NOMBRE;
    estado->accionPendiente = accion;
    estado->campoActual = indice;
    estado->totalCampos = total;

    strncpy(estado->tituloCampo, titulo ? titulo : "", sizeof(estado->tituloCampo) - 1);
    estado->tituloCampo[sizeof(estado->tituloCampo) - 1] = '\0';

    strncpy(estado->inputBuffer, valorDefault ? valorDefault : "", MAX_NOMBRE_LEN - 1);
    estado->inputBuffer[MAX_NOMBRE_LEN - 1] = '\0';
    estado->inputLen = (int)strlen(estado->inputBuffer);
}

void UI_PedirModulo(EstadoApp *estado, int accion) {
    estado->pantalla = PANTALLA_PIDIENDO_MODULO;
    estado->accionPendiente = accion;
    if (estado->modulo == 8 || estado->modulo == 1024 || estado->modulo == 16384) {
        snprintf(estado->inputBuffer, MAX_NOMBRE_LEN, "%d", estado->modulo);
    } else {
        estado->inputBuffer[0] = '\0';
    }
    estado->inputLen = (int)strlen(estado->inputBuffer);
}

/* Extensiones de archivos protegidos (cualquier tipo: con o sin error) que tiene sentido
   ofrecer al elegir el archivo a desproteger en las opciones 7 y 8. */
static bool EsExtensionDeArchivoProtegido(const char *nombre) {
    static const char *extensiones[] = { ".ha1", ".ha2", ".ha3", ".hex", ".h2x" };
    size_t len = strlen(nombre);
    if (len < 4) return false;
    char ext[5];
    for (int i = 0; i < 4; i++) ext[i] = (char)tolower((unsigned char)nombre[len - 4 + i]);
    ext[4] = '\0';
    for (int i = 0; i < 5; i++) if (strcmp(ext, extensiones[i]) == 0) return true;
    return false;
}

int UI_ActualizarYDibujarInput(EstadoApp *estado) {
    bool esModulo = (estado->pantalla == PANTALLA_PIDIENDO_MODULO);
    bool soloProtegidos = (estado->accionPendiente == 6 || estado->accionPendiente == 7 || estado->accionPendiente == 8);
    bool mostrarListaArchivos = !esModulo &&
        ((soloProtegidos && estado->campoActual == 0) ||
         (estado->accionPendiente == 4 && estado->campoActual <= 1));

    ClearBackground(COLOR_FONDO);

    int ancho = GetScreenWidth(), alto = GetScreenHeight();
    float anchoLista = 340.0f;
    float gap = 20.0f;
    float anchoTotal = 640.0f + (mostrarListaArchivos ? (gap + anchoLista) : 0.0f);

    Rectangle caja = { (float)ancho / 2.0f - anchoTotal / 2.0f, (float)(alto / 2 - 100), 640, 200 };
    DrawRectangleRounded(caja, 0.06f, 8, COLOR_PANEL);
    DrawRectangleRoundedLines(caja, 0.06f, 8, COLOR_ACENTO);

    char titulo[140];
    if (esModulo) {
        strcpy(titulo, "Ingrese el modulo de Hamming: 8, 1024 o 16384");
    } else if (estado->totalCampos > 1) {
        snprintf(titulo, sizeof(titulo), "(%d/%d) %s", estado->campoActual + 1, estado->totalCampos, estado->tituloCampo);
    } else {
        strcpy(titulo, estado->tituloCampo);
    }
    Texto(titulo, (int)caja.x + 20, (int)caja.y + 18, TAM_MODAL_TITULO, COLOR_TEXTO);

    Rectangle cajaTexto = { caja.x + 20, caja.y + 64, caja.width - 40, 44 };
    DrawRectangleRounded(cajaTexto, 0.2f, 6, COLOR_PANEL_CLARO);
    DrawRectangleRoundedLines(cajaTexto, 0.2f, 6, COLOR_BORDE);
    Texto(estado->inputBuffer, (int)cajaTexto.x + 10, (int)cajaTexto.y + 9, TAM_MODAL_TEXTO, COLOR_TEXTO);

    /* cursor parpadeante */
    if (((int)(GetTime() * 2)) % 2 == 0) {
        int cursorX = (int)cajaTexto.x + 10 + AnchoTexto(estado->inputBuffer, TAM_MODAL_TEXTO);
        Texto("_", cursorX, (int)cajaTexto.y + 9, TAM_MODAL_TEXTO, COLOR_ACENTO);
    }

    Texto("ENTER para confirmar   |   ESC para cancelar", (int)caja.x + 20, (int)caja.y + 130, TAM_PIE, COLOR_TEXTO_TENUE);

    /* ---- tabla al costado con archivos disponibles: solo protegidos al pedir el archivo
       de entrada en las opciones 6, 7 y 8 (con o sin error), o todos los archivos de la
       carpeta al elegir los dos archivos a comparar en la opcion 4. La opcion 10 NO la
       usa (a proposito: el enunciado pide dejarla afuera). Scrolleable con la rueda del
       mouse o arrastrando la barra, para cuando hay mas archivos de los que entran. ---- */
    if (mostrarListaArchivos) {
        Rectangle panelLista = { caja.x + caja.width + gap, caja.y, anchoLista, caja.height };
        DrawRectangleRounded(panelLista, 0.06f, 8, COLOR_PANEL);
        DrawRectangleRoundedLines(panelLista, 0.06f, 8, COLOR_BORDE);
        if (soloProtegidos) {
            Texto("Archivos protegidos disponibles", (int)panelLista.x + 16, (int)panelLista.y + 14, TAM_SUBTITULO, COLOR_ACENTO);
            Texto("(.Ha1 .Ha2 .ha3 .Hex .H2x - click para usar)", (int)panelLista.x + 16, (int)panelLista.y + 36, TAM_PIE, COLOR_TEXTO_TENUE);
        } else {
            Texto("Archivos disponibles en esta carpeta", (int)panelLista.x + 16, (int)panelLista.y + 14, TAM_SUBTITULO, COLOR_ACENTO);
            Texto("(cualquier archivo - click para usar)", (int)panelLista.x + 16, (int)panelLista.y + 36, TAM_PIE, COLOR_TEXTO_TENUE);
        }

        int barraAncho = 10;
        Rectangle areaLista = { panelLista.x + 10, panelLista.y + 60, panelLista.width - 20 - barraAncho, panelLista.height - 70 };

        /* 1) reunimos TODOS los nombres que matchean, sin importar cuantos entren en el panel */
        static char nombresEncontrados[256][MAX_NOMBRE_LEN];
        int totalEncontrados = 0;
        FilePathList archivos = LoadDirectoryFiles(".");
        for (unsigned int i = 0; i < archivos.count && totalEncontrados < 256; i++) {
            const char *nombre = GetFileName(archivos.paths[i]);
            if (!IsPathFile(archivos.paths[i])) continue; /* nos saltamos subcarpetas */
            if (soloProtegidos && !EsExtensionDeArchivoProtegido(nombre)) continue;
            strncpy(nombresEncontrados[totalEncontrados], nombre, MAX_NOMBRE_LEN - 1);
            nombresEncontrados[totalEncontrados][MAX_NOMBRE_LEN - 1] = '\0';
            totalEncontrados++;
        }
        UnloadDirectoryFiles(archivos);

        /* 2) calculamos cuantas filas entran visibles y aplicamos scroll (rueda del mouse) */
        int filaAlto = 26;
        int filasVisibles = (int)areaLista.height / filaAlto;
        if (filasVisibles < 1) filasVisibles = 1;

        static int scrollLista = 0;
        static int accionAnterior = -1, campoAnterior = -1;
        if (estado->accionPendiente != accionAnterior || estado->campoActual != campoAnterior) {
            scrollLista = 0;
            accionAnterior = estado->accionPendiente;
            campoAnterior = estado->campoActual;
        }

        int maxScroll = totalEncontrados - filasVisibles;
        if (maxScroll < 0) maxScroll = 0;

        if (CheckCollisionPointRec(GetMousePosition(), areaLista)) {
            float rueda = GetMouseWheelMove();
            if (rueda != 0) scrollLista -= (int)(rueda * 3);
        }
        if (scrollLista < 0) scrollLista = 0;
        if (scrollLista > maxScroll) scrollLista = maxScroll;

        /* 3) dibujamos solo la porcion visible */
        BeginScissorMode((int)areaLista.x, (int)areaLista.y, (int)areaLista.width, (int)areaLista.height);
        int y = (int)areaLista.y;
        for (int i = scrollLista; i < totalEncontrados && i < scrollLista + filasVisibles; i++) {
            Rectangle fila = { areaLista.x, (float)y, areaLista.width, (float)filaAlto - 2 };
            bool hover = CheckCollisionPointRec(GetMousePosition(), fila);
            if (hover) DrawRectangleRounded(fila, 0.25f, 4, COLOR_PANEL_CLARO);
            Texto(nombresEncontrados[i], (int)fila.x + 8, y + 3, TAM_PIE + 2, hover ? COLOR_ACENTO : COLOR_TEXTO);

            if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                strncpy(estado->inputBuffer, nombresEncontrados[i], MAX_NOMBRE_LEN - 1);
                estado->inputBuffer[MAX_NOMBRE_LEN - 1] = '\0';
                estado->inputLen = (int)strlen(estado->inputBuffer);
            }
            y += filaAlto;
        }
        EndScissorMode();

        /* 4) barra de scroll: indicador + arrastrable, mismo estilo que el panel de log */
        if (maxScroll > 0) {
            Rectangle barra = { areaLista.x + areaLista.width + 4, areaLista.y, (float)barraAncho, areaLista.height };
            DrawRectangleRounded(barra, 0.4f, 4, COLOR_PANEL_CLARO);

            float thumbAlto = areaLista.height * ((float)filasVisibles / (float)totalEncontrados);
            if (thumbAlto < 14.0f) thumbAlto = 14.0f;
            if (thumbAlto > areaLista.height) thumbAlto = areaLista.height;
            float progreso = (float)scrollLista / (float)maxScroll;
            float thumbY = barra.y + progreso * (areaLista.height - thumbAlto);
            Rectangle thumb = { barra.x, thumbY, (float)barraAncho, thumbAlto };

            static bool arrastrandoLista = false;
            bool hoverThumb = CheckCollisionPointRec(GetMousePosition(), thumb);
            if (hoverThumb && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) arrastrandoLista = true;
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) arrastrandoLista = false;
            if (arrastrandoLista && (areaLista.height - thumbAlto) > 0.5f) {
                float rel = (GetMousePosition().y - barra.y - thumbAlto / 2.0f) / (areaLista.height - thumbAlto);
                if (rel < 0) rel = 0;
                if (rel > 1) rel = 1;
                scrollLista = (int)(rel * maxScroll + 0.5f);
            }
            DrawRectangleRounded(thumb, 0.4f, 4, (hoverThumb || arrastrandoLista) ? COLOR_ACENTO : COLOR_BORDE);
        }

        if (totalEncontrados == 0) {
            Texto("(no se encontro ninguno\nen esta carpeta)", (int)areaLista.x + 4, (int)areaLista.y + 4, TAM_PIE, COLOR_TEXTO_TENUE);
        } else if (totalEncontrados > filasVisibles) {
            char info[48];
            snprintf(info, sizeof(info), "%d-%d de %d", scrollLista + 1,
                     (scrollLista + filasVisibles < totalEncontrados) ? scrollLista + filasVisibles : totalEncontrados, totalEncontrados);
            Texto(info, (int)panelLista.x + 16, (int)(panelLista.y + panelLista.height - 22), TAM_PIE, COLOR_TEXTO_TENUE);
        }
    }

    /* ---- captura de teclado ---- */
    int tecla = GetCharPressed();
    while (tecla > 0) {
        bool valido = esModulo ? (tecla >= '0' && tecla <= '9')
                                : (tecla >= 32 && tecla < 127);
        if (valido && estado->inputLen < MAX_NOMBRE_LEN - 1) {
            estado->inputBuffer[estado->inputLen++] = (char)tecla;
            estado->inputBuffer[estado->inputLen] = '\0';
        }
        tecla = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && estado->inputLen > 0) {
        estado->inputLen--;
        estado->inputBuffer[estado->inputLen] = '\0';
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        estado->pantalla = PANTALLA_MENU;
        estado->inputBuffer[0] = '\0';
        estado->inputLen = 0;
        return 0;
    }
    if (IsKeyPressed(KEY_ENTER) && estado->inputLen > 0) {
        return 1;
    }
    return 0;
}

/* ===================== DECODIFICACION DE BITS A TEXTO (para la comparacion Hamming) ===================== */

/* Junta de a 8 los bits (0/1) y los vuelca como caracteres legibles.
   Los bytes que no son ASCII imprimible se muestran como '.' para no romper la lectura. */
static void BitsATexto(const int *bits, int cantidadBits, char *salida, int maxSalida) {
    int idx = 0, i = 0;
    while (i + 8 <= cantidadBits && idx < maxSalida - 1) {
        unsigned char byte = 0;
        for (int b = 0; b < 8; b++) {
            int v = bits[i + b];
            byte = (unsigned char)((byte << 1) | ((v >= 1) ? 1 : 0));
        }
        salida[idx++] = (byte >= 32 && byte < 127) ? (char)byte : '.';
        i += 8;
    }
    salida[idx] = '\0';
}

/* Extrae SOLO los bits de DATOS (las posiciones que no son potencia de 2) de la cadena
   protegida con Hamming, bloque a bloque, ignorando los bits de paridad. No corrige nada:
   muestra el contenido tal cual quedo en cadenaH en este momento. */
static int ExtraerBitsDeDatos(const int *cadenaH, int modulo, int largoTotal, int *bitsOut, int maxBitsOut) {
    int idx = 0;
    for (int k = 1; k < largoTotal && idx < maxBitsOut; k += modulo) {
        if (k + modulo - 1 > largoTotal) break;
        for (int i = 1; i < modulo && idx < maxBitsOut; i++) {
            if ((i & (i - 1)) != 0) bitsOut[idx++] = cadenaH[k + i - 1];
        }
    }
    return idx;
}

/* Parte dos textos largos en lineas de `ancho` caracteres, alineadas por posicion,
   sin cortar caracteres UTF-8 multibyte a la mitad. */
static void PartirEnLineas(EstadoApp *estado, const char *textoIzq, const char *textoDer, int ancho) {
    int largoIzq = (int)strlen(textoIzq);
    int largoDer = (int)strlen(textoDer);
    int largoMax = (largoIzq > largoDer) ? largoIzq : largoDer;
    int totalLineas = (largoMax + ancho - 1) / ancho;
    if (totalLineas < 1) totalLineas = 1;
    if (totalLineas > MAX_LINEAS_COMP) totalLineas = MAX_LINEAS_COMP;

    int posIzq = 0, posDer = 0;
    for (int i = 0; i < totalLineas; i++) {
        int avanceIzq = (posIzq < largoIzq) ? LimiteUTF8Seguro(textoIzq, largoIzq, posIzq, ancho) : 0;
        if (avanceIzq > 0) memcpy(estado->comparacionTexto[i].izquierda, textoIzq + posIzq, avanceIzq);
        estado->comparacionTexto[i].izquierda[avanceIzq] = '\0';
        posIzq += avanceIzq;

        int avanceDer = (posDer < largoDer) ? LimiteUTF8Seguro(textoDer, largoDer, posDer, ancho) : 0;
        if (avanceDer > 0) memcpy(estado->comparacionTexto[i].derecha, textoDer + posDer, avanceDer);
        estado->comparacionTexto[i].derecha[avanceDer] = '\0';
        posDer += avanceDer;
    }
    estado->comparacionTextoCantidad = totalLineas;
}

/* Ancho de columna disponible (en pixeles) para el contenido dentro de cada recuadro
   de la pantalla de comparacion. Usado tanto para envolver el texto al cargarlo como
   para dibujarlo, asi nunca se va del recuadro. */
static int AnchoColumnaComparacionPx(void) {
    int ancho = GetScreenWidth();
    int margen = 20, gap = 16, padding = 14;
    int colAncho = (ancho - margen * 2 - gap) / 2;
    return colAncho - padding * 2;
}

/* ===================== COMPARACION DE TEXTO (HUFFMAN) ===================== */

/* Abre el archivo en BINARIO y vuelca cada bit en un arreglo de enteros (0/1), exactamente
   igual a como se lee 'chain' en el resto del proyecto (CargarArchivo en main.c): por cada
   byte se separan sus 8 bits, MSB primero. Devuelve el arreglo (malloc) y la cantidad total
   de bits leidos en *totalBits. Si el archivo no existe devuelve NULL. */
static int *LeerBitsDeArchivo(const char *ruta, long *totalBits) {
    FILE *f = fopen(ruta, "rb");
    if (!f) { if (totalBits) *totalBits = 0; return NULL; }

    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    rewind(f);

    if (tam <= 0) { fclose(f); if (totalBits) *totalBits = 0; return NULL; }

    int *bits = malloc((size_t)tam * 8 * sizeof(int));
    long n = 0;
    int c;
    while ((c = fgetc(f)) != EOF) {
        for (int t = 7; t >= 0; t--) bits[n++] = ((unsigned char)c >> t) & 1;
    }
    fclose(f);

    if (totalBits) *totalBits = n;
    return bits;
}

/* Recorre el arreglo de bits DE A 8 a la vez y reconstruye cada letra (byte), tal cual
   pediste: "leer de a 8 bits e ir poniendolo en el visualizador como letras". Los bytes de
   control (no imprimibles) se muestran como '.' para que no rompan la visualizacion.
   Devuelve la cantidad de letras reconstruidas. */
static int BitsAOctetos(const int *bits, long totalBits, char *salida, int maxSalida) {
    int idx = 0;
    long i = 0;
    while (i + 8 <= totalBits && idx < maxSalida - 1) {
        unsigned char byte = 0;
        for (int b = 0; b < 8; b++) byte = (unsigned char)((byte << 1) | (bits[i + b] ? 1 : 0));
        salida[idx++] = (byte < 32 || byte == 127) ? '.' : (char)byte;
        i += 8;
    }
    salida[idx] = '\0';
    return idx;
}

void UI_CargarComparacionHuffman(EstadoApp *estado, const char *archivoIzq, const char *archivoDer,
                                  const char *tituloIzq, const char *tituloDer) {
    estado->comparacionTextoCantidad = 0;
    strcpy(estado->tituloIzquierda, tituloIzq);
    strcpy(estado->tituloDerecha, tituloDer);

    /* PASO 1: abrimos los dos archivos en binario y leemos TODOS sus bits a un arreglo. */
    long totalBitsIzq = 0, totalBitsDer = 0;
    int *bitsIzq = LeerBitsDeArchivo(archivoIzq, &totalBitsIzq);
    int *bitsDer = LeerBitsDeArchivo(archivoDer, &totalBitsDer);

    if (!bitsIzq && !bitsDer) {
        UI_AgregarLog(estado, "No se pudieron abrir '%s' ni '%s'.", archivoIzq, archivoDer);
        estado->pantalla = PANTALLA_MENU;
        return;
    }

    /* PASO 2: leemos esos arreglos de a 8 bits para reconstruir las letras de cada archivo. */
    long maxCharsIzq = totalBitsIzq / 8 + 1;
    long maxCharsDer = totalBitsDer / 8 + 1;
    char *textoIzq = malloc((size_t)maxCharsIzq + 1);
    char *textoDer = malloc((size_t)maxCharsDer + 1);

    int lenIzq = bitsIzq ? BitsAOctetos(bitsIzq, totalBitsIzq, textoIzq, (int)maxCharsIzq + 1) : 0;
    int lenDer = bitsDer ? BitsAOctetos(bitsDer, totalBitsDer, textoDer, (int)maxCharsDer + 1) : 0;
    if (!bitsIzq) textoIzq[0] = '\0';
    if (!bitsDer) textoDer[0] = '\0';

    /* PASO 3: cortamos ambos textos con el MISMO ancho fijo, asi los renglones quedan
       prolijos y la posicion j de una fila es siempre la posicion j de la otra columna. */
    int anchoCaracteres = CaracteresPorLinea(TAM_FILA_COMP, AnchoColumnaComparacionPx());
    int maxLen = (lenIzq > lenDer) ? lenIzq : lenDer;
    int totalFilas = (maxLen / anchoCaracteres) + 1; /* al menos 1 fila, incluso si esta vacio */
    if (totalFilas > MAX_LINEAS_COMP) totalFilas = MAX_LINEAS_COMP;

    for (int fila = 0; fila < totalFilas; fila++) {
        int desde = fila * anchoCaracteres;

        int copiarI = lenIzq - desde; if (copiarI < 0) copiarI = 0; if (copiarI > anchoCaracteres) copiarI = anchoCaracteres;
        if (copiarI > 0) memcpy(estado->comparacionTexto[fila].izquierda, textoIzq + desde, copiarI);
        estado->comparacionTexto[fila].izquierda[copiarI] = '\0';

        int copiarD = lenDer - desde; if (copiarD < 0) copiarD = 0; if (copiarD > anchoCaracteres) copiarD = anchoCaracteres;
        if (copiarD > 0) memcpy(estado->comparacionTexto[fila].derecha, textoDer + desde, copiarD);
        estado->comparacionTexto[fila].derecha[copiarD] = '\0';
    }

    /* PASO 4: la comparacion letra por letra (misma posicion j en ambas filas) la hace
       DibujarLineaConDiff al momento de dibujar -- ver UI_DibujarComparacion mas abajo. */

    estado->comparacionTextoCantidad = totalFilas;
    estado->scrollComparacion = 0;
    free(bitsIzq);
    free(bitsDer);
    free(textoIzq);
    free(textoDer);
    estado->pantalla = PANTALLA_COMPARAR_HUFFMAN;
}

void UI_CargarComparacionHamming(EstadoApp *estado) {
    if (!estado->cadenaH || !estado->chain) {
        UI_AgregarLog(estado, "Todavia no se aplico Hamming sobre ningun archivo.");
        return;
    }

    /* texto de la izquierda: el archivo original, tal como se cargo (opcion 1) */
    int cantidadBitsOriginal = estado->n - 1;
    int tamBufIzq = cantidadBitsOriginal / 8 + 2;
    char *textoIzq = malloc(tamBufIzq);
    BitsATexto(&estado->chain[1], cantidadBitsOriginal, textoIzq, tamBufIzq);

    /* texto de la derecha: se extraen los bits de DATOS del estado ACTUAL de cadenaH
       (con errores introducidos y/o ya corregidos) y se decodifican igual que el original */
    int maxBitsDatos = cantidadBitsOriginal + estado->modulo + 8;
    int *bitsDatos = malloc(sizeof(int) * maxBitsDatos);
    int cantidadBitsDatos = ExtraerBitsDeDatos(estado->cadenaH, estado->modulo, estado->largoH, bitsDatos, maxBitsDatos);
    int tamBufDer = cantidadBitsDatos / 8 + 2;
    char *textoDer = malloc(tamBufDer);
    BitsATexto(bitsDatos, cantidadBitsDatos, textoDer, tamBufDer);

    strcpy(estado->tituloIzquierda, "ORIGINAL (archivo cargado)");
    strcpy(estado->tituloDerecha, estado->hayErrores ? "ACTUAL (con errores / corregido)" : "PROTEGIDO (recien aplicado Hamming)");

    int anchoCaracteres = CaracteresPorLinea(TAM_FILA_COMP, AnchoColumnaComparacionPx());
    PartirEnLineas(estado, textoIzq, textoDer, anchoCaracteres);
    estado->scrollComparacion = 0;
    estado->pantalla = PANTALLA_COMPARAR_HAMMING;

    free(textoIzq);
    free(bitsDatos);
    free(textoDer);
}

int UI_DibujarBarraVolver(EstadoApp *estado, const char *titulo) {
    ClearBackground(COLOR_FONDO);
    int ancho = GetScreenWidth();
    DrawRectangle(0, 0, ancho, 60, COLOR_PANEL);
    Texto(titulo, 20, 16, TAM_HEADER_COMP, COLOR_ACENTO);

    Rectangle rVolver = { (float)(ancho - 200), 10, 180, 40 };
    bool volver = Boton(rVolver, "<- Volver (ESC)", true, false);
    if (IsKeyPressed(KEY_ESCAPE)) volver = true;
    if (volver) { estado->pantalla = PANTALLA_MENU; return 1; }
    return 0;
}

void UI_DibujarComparacion(EstadoApp *estado) {
    bool esHamming = (estado->pantalla == PANTALLA_COMPARAR_HAMMING);
    char titulo[160];
    snprintf(titulo, sizeof(titulo), "Comparacion: %s",
              esHamming ? "texto original vs. texto recuperado tras Hamming" : "archivo original / recuperado");
    if (UI_DibujarBarraVolver(estado, titulo)) return;

    int ancho = GetScreenWidth(), alto = GetScreenHeight();
    int topY = 74;
    int margen = 20, gap = 16, padding = 14;
    int colAncho = (ancho - margen * 2 - gap) / 2;

    Rectangle cajaIzq = { (float)margen, (float)(topY + 32), (float)colAncho, (float)(alto - topY - 32 - 60) };
    Rectangle cajaDer = { (float)(margen + colAncho + gap), (float)(topY + 32), (float)colAncho, (float)(alto - topY - 32 - 60) };

    Texto(estado->tituloIzquierda, (int)cajaIzq.x + 4, topY, TAM_HEADER_COMP, COLOR_ACENTO);
    Texto(estado->tituloDerecha, (int)cajaDer.x + 4, topY, TAM_HEADER_COMP, COLOR_ACENTO);

    DrawRectangleRounded(cajaIzq, 0.02f, 6, COLOR_PANEL);
    DrawRectangleRoundedLines(cajaIzq, 0.02f, 6, COLOR_BORDE);
    DrawRectangleRounded(cajaDer, 0.02f, 6, COLOR_PANEL);
    DrawRectangleRoundedLines(cajaDer, 0.02f, 6, COLOR_BORDE);

    int filaAlto = TAM_FILA_COMP + 8;
    int filasVisibles = ((int)cajaIzq.height - padding * 2) / filaAlto;
    if (filasVisibles < 1) filasVisibles = 1;

    /* scroll con la rueda del mouse (funciona sobre cualquiera de los dos recuadros) */
    if (CheckCollisionPointRec(GetMousePosition(), cajaIzq) || CheckCollisionPointRec(GetMousePosition(), cajaDer)) {
        float rueda = GetMouseWheelMove();
        if (rueda != 0) estado->scrollComparacion -= (int)(rueda * 3);
    }
    if (estado->scrollComparacion < 0) estado->scrollComparacion = 0;

    int total = estado->comparacionTextoCantidad;
    if (estado->scrollComparacion > total - 1 && total > 0) estado->scrollComparacion = total - 1;

    /* recortamos el dibujo a cada recuadro: ninguna linea larga puede "salirse" de su columna */
    BeginScissorMode((int)cajaIzq.x, (int)cajaIzq.y, (int)cajaIzq.width, (int)cajaIzq.height);
    for (int fila = 0; fila < filasVisibles; fila++) {
        int idx = estado->scrollComparacion + fila;
        if (idx >= total) break;
        int y = (int)cajaIzq.y + padding + fila * filaAlto;
        Texto(estado->comparacionTexto[idx].izquierda, (int)cajaIzq.x + padding, y, TAM_FILA_COMP, COLOR_TEXTO);
    }
    EndScissorMode();

    BeginScissorMode((int)cajaDer.x, (int)cajaDer.y, (int)cajaDer.width, (int)cajaDer.height);
    for (int fila = 0; fila < filasVisibles; fila++) {
        int idx = estado->scrollComparacion + fila;
        if (idx >= total) break;
        int y = (int)cajaDer.y + padding + fila * filaAlto;
        DibujarLineaConDiff(estado->comparacionTexto[idx].izquierda,
                             estado->comparacionTexto[idx].derecha,
                             (int)cajaDer.x + padding, y, TAM_FILA_COMP, COLOR_TEXTO);
    }
    EndScissorMode();

    char posicion[64];
    snprintf(posicion, sizeof(posicion), "Filas %d-%d de %d", estado->scrollComparacion + 1,
             (estado->scrollComparacion + filasVisibles < total) ? estado->scrollComparacion + filasVisibles : total, total);
    Texto(posicion, margen, alto - 36, TAM_PIE, COLOR_TEXTO_TENUE);

    const char *pie = esHamming
        ? "Rueda del mouse: scroll  |  Rojo = caracter recuperado distinto del original ('.' = byte no imprimible)"
        : "Rueda del mouse: scroll  |  Rojo = caracter distinto entre ambos archivos";
    Texto(pie, margen, alto - 18, TAM_PIE, COLOR_TEXTO_TENUE);
}

/* ===================== ESTADISTICAS ===================== */

void UI_CargarEstadisticas(EstadoApp *estado) {
    FILE *f;
    long tam;

    f = fopen(estado->archivoOriginal, "rb");
    tam = -1;
    if (f) { fseek(f, 0, SEEK_END); tam = ftell(f); fclose(f); }
    estado->tOrig = tam;

    f = fopen(estado->archivoComprimido, "rb");
    tam = -1;
    if (f) { fseek(f, 0, SEEK_END); tam = ftell(f); fclose(f); }
    estado->tComp = tam;

    f = fopen(estado->archivoDescomprimido, "rb");
    tam = -1;
    if (f) { fseek(f, 0, SEEK_END); tam = ftell(f); fclose(f); }
    estado->tDesc = tam;

    estado->pantalla = PANTALLA_ESTADISTICAS;
}

static void BarraEstadistica(int x, int y, int w, int hMax, long valor, long base, const char *etiqueta, Color color) {
    int alto = (base > 0) ? (int)(hMax * ((double)valor / (double)base)) : 0;
    if (alto > hMax) alto = hMax;
    if (alto < 0) alto = 0;

    DrawRectangle(x, y + (hMax - alto), w, alto, color);
    DrawRectangleLines(x, y, w, hMax, COLOR_BORDE);

    char txt[MAX_NOMBRE_LEN + 32];
    snprintf(txt, sizeof(txt), "%s\n%ld bytes", etiqueta, valor);
    Texto(txt, x, y + hMax + 10, TAM_SUBTITULO, COLOR_TEXTO);
}

void UI_DibujarEstadisticas(EstadoApp *estado) {
    if (UI_DibujarBarraVolver(estado, "Estadisticas de compresion")) return;

    int ancho = GetScreenWidth();
    int hMax = 320;
    int y = 110;
    int w = 130;
    int separacion = 230;
    int x = ancho / 2 - separacion - w / 2;

    long base = (estado->tOrig > 0) ? estado->tOrig : 1;

    BarraEstadistica(x, y, w, hMax, estado->tOrig, base, estado->archivoOriginal, COLOR_ACENTO);
    BarraEstadistica(x + separacion, y, w, hMax, estado->tComp, base, estado->archivoComprimido, COLOR_OK);
    BarraEstadistica(x + 2 * separacion, y, w, hMax, estado->tDesc, base, estado->archivoDescomprimido, COLOR_ACENTO_OSCURO);

    if (estado->tOrig > 0 && estado->tComp >= 0) {
        double ratio = 100.0 * estado->tComp / estado->tOrig;
        double ahorro = 100.0 - ratio;
        char resumen[200];
        snprintf(resumen, sizeof(resumen), "Ratio de compresion: %.2f%%      Ahorro de espacio: %.2f%%", ratio, ahorro);
        Texto(resumen, 20, y + hMax + 74, TAM_SUBTITULO, COLOR_TEXTO);
    }
    if (estado->tOrig == estado->tDesc && estado->tOrig > 0) {
        Texto("La descompresion es CORRECTA (tamanios iguales).", 20, y + hMax + 104, TAM_SUBTITULO, COLOR_OK);
    } else if (estado->tDesc >= 0) {
        Texto("ATENCION: los tamanios original y descompactado difieren.", 20, y + hMax + 104, TAM_SUBTITULO, COLOR_DIFF);
    }
}
