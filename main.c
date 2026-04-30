#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "raylib.h"
#include"string.h"
typedef enum{
PANTALLA_MENU,
PANTALLA_UNO,
PANTALLA_DOS,
PANTALLA_TRES,
PANTALLA_CUATRO,
PANTALLA_CINCO,
PANTALLA_SEIS
}Pantalla;


void imprimirB(unsigned char c);
int* Hamming(int cadena[], int modulo, int limite, int *arr_out);
void introducir_errores(int *cadena_h, int modulo, int largo_total, int *errores);
int* decodificarHamming(int cadena_H[], int modulo, int largo_total, int *largo_info,int cantidad_de_caracteres_originales);
void generarArchivoDEX(int *cadena_H, int modulo, int largo_total, char *nombre_salida,long total_byts);
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



int DibujarBoton(int x, int y, int ancho, int alto, const char *texto) {
    Rectangle rect = { x, y, ancho, alto };
    Vector2 mouse = GetMousePosition();

    int hover = CheckCollisionPointRec(mouse, rect);

    if (hover) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    // Cambia color si el mouse está encima
    Color colorFondo  = hover ? LIGHTGRAY : RAYWHITE;
    Color colorBorde  = hover ? DARKGRAY  : GRAY;

    DrawRectangleRec(rect, colorFondo);
    DrawRectangleLinesEx(rect, 2, colorBorde);

    // Centrar texto dentro del botón
    int tamTexto = 20;
    int anchoTexto = MeasureText(texto, tamTexto);
    int textoX = x + (ancho - anchoTexto) / 2;
    int textoY = y + (alto - tamTexto) / 2;
    DrawText(texto, textoX, textoY, tamTexto, DARKGRAY);

    // Devuelve 1 si se hizo clic encima
    return hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void DibujarColumnaTexto(const char *contenido,const char *comprobacion, int tamano_archivo, const char *titulo,
                          int x, int ancho, int alto, int scroll,
                          int yInicio, Font fuente, int pasada) {

    int tamLetra   = 30;
    int lineaAlto  = 28;
    int margen     = 10;
    int anchoMax   = ancho - margen * 2;
    FILE *arch = fopen(comprobacion, "rb"); // "rb" es más seguro para contar bytes
    char *chain = NULL; // Lo declaramos aquí para poder liberarlo al final

    if (arch != NULL) {
        int n = 0;

        // 1. Reservamos espacio para la cantidad REAL de caracteres, +1 para el nulo final
        chain = malloc(tamano_archivo + 1);

        // 2. Variable int para evitar errores con EOF
        int caracter_leido;

        // 3. Leemos asegurándonos de no pasarnos de tamano_archivo
        while ((caracter_leido = fgetc(arch)) != EOF && n < tamano_archivo) {
            chain[n] = (char)caracter_leido;
            n++;
        }

        fclose(arch); // ¡AHORA SÍ FUNCIONARÁ SIN ROMPERSE!
    }
    // Fondo y borde
    DrawRectangle(x, yInicio, ancho, alto, (Color){245, 245, 245, 255});
    DrawRectangleLinesEx((Rectangle){x, yInicio, ancho, alto}, 1, LIGHTGRAY);

    // Título
    DrawRectangle(x, yInicio, ancho, 28, DARKGRAY);
    int anchoTit = MeasureText(titulo, 16);
    DrawText(titulo, x + (ancho - anchoTit) / 2, yInicio + 6, 16, WHITE);

    if (contenido == NULL) {
        DrawText("(archivo no cargado)", x + margen, yInicio + 40, 30, GRAY);
        return;
    }


    #define MAX_LINEAS 4096
    char lineas[MAX_LINEAS][256];
    int totalLineas = 0;

    char lineaBuf[1024];
    int idx = 0;

    // --- REEMPLAZO DEL WHILE POR UN FOR BASADO EN EL TAMAÑO EXACTO ---
    for (int i = 0; i < tamano_archivo && totalLineas < MAX_LINEAS; i++) {
        char c = contenido[i];

        // Reemplazo visual temporal para que Raylib/C no corten el texto al dibujar.
        // El arreglo original 'contenido' NO se modifica.
        if (c == '\0') {
            c = '?'; // O puedes poner '?' si prefieres ver dónde están los errores
        }

        if (c == '\n' || idx >= 1023) {
            lineaBuf[idx] = '\0';

            const char *lp = lineaBuf;
            char trozo[256];
            int tIdx = 0;

            while (*lp != '\0' && totalLineas < MAX_LINEAS) {
                trozo[tIdx++] = *lp;
                trozo[tIdx]   = '\0';

                char siguiente[256];
                snprintf(siguiente, sizeof(siguiente), "%s%c", trozo, *(lp+1));

                // Usando MeasureTextEx con la corrección anterior
                if (MeasureTextEx(fuente, siguiente, 30.0f, 1.0f).x > anchoMax || *(lp+1) == '\0') {
                    strncpy(lineas[totalLineas], trozo, 255);
                    lineas[totalLineas][255] = '\0';
                    totalLineas++;
                    tIdx = 0;
                }
                lp++;
            }
            if (tIdx == 0 && *lineaBuf == '\0' && totalLineas < MAX_LINEAS) {
                lineas[totalLineas][0] = '\0';
                totalLineas++;
            }
            idx = 0;
        } else {
            lineaBuf[idx++] = c; // Guardamos el caracter procesado
        }
    }
    // Última línea si no termina en \n
    if (idx > 0 && totalLineas < MAX_LINEAS) {
        lineaBuf[idx] = '\0';
        const char *lp = lineaBuf;
        char trozo[256];
        int tIdx = 0;
        while (*lp != '\0' && totalLineas < MAX_LINEAS) {
            trozo[tIdx++] = *lp;
            trozo[tIdx]   = '\0';
            char siguiente[256];
            snprintf(siguiente, sizeof(siguiente), "%s%c", trozo, *(lp+1));
            if (MeasureText(siguiente, tamLetra) > anchoMax || *(lp+1) == '\0') {
                strncpy(lineas[totalLineas], trozo, 255);
                lineas[totalLineas][255] = '\0';
                totalLineas++;
                tIdx = 0;
            }
            lp++;
        }
    }
    // --- Dibujado aplicando la máscara de Scroll ---
    BeginScissorMode(x, yInicio + 28, ancho, alto - 28);
    int yTexto  = yInicio + 34;
    int limiteY = yInicio + alto;

    // 1. Sincronizamos el índice 'n' adelantándolo según cuánto Scroll hayamos hecho.
    int n = 0;
    for (int k = 0; k < scroll; k++) {
        for (int j = 0; lineas[k][j] != '\0'; j++) {
            // Saltamos los '\n' o '\r' originales porque no se guardaron en 'lineas'
            while (n < tamano_archivo && (chain[n] == '\n' || chain[n] == '\r')) {
                n++;
            }
            n++; // Consumimos la letra que quedó oculta por el scroll
        }
    }

    // 2. Dibujamos las líneas visibles
    for (int i = scroll; i < totalLineas; i++) {
        int y = yTexto + (i - scroll) * lineaAlto;
        if (y >= limiteY) break;

        // Empezamos en el margen izquierdo para cada nuevo renglón
        float cursorX = x + margen;

        // Iteramos letra por letra en el renglón actual
        for (int j = 0; lineas[i][j] != '\0'; j++) {
            char letraActual = lineas[i][j];
            char letraStr[2] = { letraActual, '\0' };

            // Volvemos a ignorar saltos de línea para mantener la sincronía perfecta
            while (n < tamano_archivo && (chain[n] == '\n' || chain[n] == '\r')) {
                n++;
            }

            // Por defecto, la letra es gris oscura
            Color colorLetra = DARKGRAY;
            float tamañoLetraActual = (float)tamLetra;

            // --- LA SOLUCIÓN DEL COLOR ESTÁ AQUÍ ---
            // Solo pintamos de rojo si estamos en la columna derecha (pasada == 2)
            // Y si la letra actual no coincide con la del archivo original
            if (pasada == 2 && n < tamano_archivo && letraActual != chain[n]) {
                colorLetra = RED;
            }

            // Dibujamos solo esa letra
            DrawTextEx(fuente, letraStr, (Vector2){cursorX, y}, tamañoLetraActual, 1.0f, colorLetra);

            // Calculamos cuánto midió esta letra específica para mover el cursor
            Vector2 medidaLetra = MeasureTextEx(fuente, letraStr, tamañoLetraActual, 1.0f);
            cursorX += medidaLetra.x + 1.0f;

            if (n < tamano_archivo) {
                n++;
            }
        }
    }
    EndScissorMode();

    // Liberar la memoria del archivo de referencia
    if (chain != NULL) {
        free(chain);
    }
}


int main() {

    // Variables para el visor de dos archivos
    int size1=0;
    int size2=0;
    char *contenido1 = NULL;
    char *contenido2 = NULL;
    int scroll1 = 0;
    int scroll2 = 0;
    int visorCargado = 0;
    char archivoVisor1[72] = "\0";
    char archivoVisor2[72] = "\0";



    int i, n = 1, t, caracter, largo_h, largo_info, errores = 0;
    int *cadena_H, *info_recuperada;
    FILE *arch;

    int paso_txt=0;//para marcar si se ingreso un txt o no
    int paso=0;//paso de la opcion para introducir errores
    int paso_P4=0;
    int paso_p5=0;
    int seleccion= 0;//usado para ver q se ejecuto hamming

    long file_size;// para saber el tamaño del archivo
    int mod;// sirve para saber el modulo que venimos trabajando despues de haber seleccionado el hamming

    const int ANCHO = 800;
    const int ALTO  = 600;
    InitWindow(ANCHO, ALTO, "Navegacion entre pantallas");
    printf("Carpeta actual: %s\n", GetWorkingDirectory());
    SetTargetFPS(60);
    Font miFuente = LoadFont("Oswald-Regular.ttf");

    if (miFuente.texture.id == 0) {
        printf("ERROR: no se pudo cargar la fuente\n");
    } else {
        printf("Fuente cargada OK\n");
    }

    char nombreArchivo[64] = "\0";
    int cantChars = 0;
    int campoActivo = 0;
    int archivoValido = 0;      // 1 si el archivo existe
    char mensajeError[64] = "\0";
    Pantalla pantallaActual = PANTALLA_MENU;
    int hammingSeleccionado = 0;  // 0 = ninguno, 8, 1024 o 16384
    // Anchos de cada botón
    int anchoBtn1 = 200;  // Ingresar TXT
    int anchoBtn2 = 240;  // Proteger el archivo
    int anchoBtn3 = 230;  // Introducir errores
    int anchoBtn4 = 450;  // Desproteger sin corregir
    int anchoBtn5 = 400;  // Desproteger corregido
    int anchoBtn6 = 200;  // Salir
    int anchoBtn7 = 280;  // ver archivos
    int altoBtn   = 50;
    int espaciado = 12;


    while (!WindowShouldClose()) {

        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (pantallaActual == PANTALLA_MENU) {

            // Título centrado
            int anchoTitulo = MeasureText("MENU PRINCIPAL", 30);
            DrawText("MENU PRINCIPAL", (ANCHO - anchoTitulo) / 2, 40, 30, DARKGRAY);

            // Calcular Y inicial para que el grupo de botones quede centrado
            int totalBotones = 6;
            int altoTotal = totalBotones * altoBtn + (totalBotones - 1) * espaciado;
            int yInicial  = (ALTO - altoTotal) / 2;

            // Cada botón centrado individualmente con (ANCHO - anchoBtn) / 2
            if (DibujarBoton((ANCHO - anchoBtn1) / 2, yInicial,                              anchoBtn1, altoBtn, "Ingresar TXT")) {
                pantallaActual = PANTALLA_UNO;
            }
            if (DibujarBoton((ANCHO - anchoBtn3) / 2, yInicial + (altoBtn + espaciado),  anchoBtn3, altoBtn, "Introducir errores")) {
                pantallaActual = PANTALLA_TRES;
            }
            if (DibujarBoton((ANCHO - anchoBtn4) / 2, yInicial + (altoBtn + espaciado) * 2,  anchoBtn4, altoBtn, "Desproteger archivo sin corregir errores")) {
                pantallaActual = PANTALLA_CUATRO;
            }
            if (DibujarBoton((ANCHO - anchoBtn5) / 2, yInicial + (altoBtn + espaciado) * 3,  anchoBtn5, altoBtn, "Desproteger archivo corregido")) {
                pantallaActual = PANTALLA_CINCO;
            }
            if (DibujarBoton((ANCHO - anchoBtn7) / 2, yInicial + (altoBtn + espaciado) * 4,anchoBtn7, altoBtn, "Ver archivos")) {
                pantallaActual = PANTALLA_SEIS;
            }
            if (DibujarBoton((ANCHO - anchoBtn6) / 2, yInicial + (altoBtn + espaciado) * 5,  anchoBtn6, altoBtn, "Salir")) {
                break;
            }
        }
        // -----------------------------------------------

        else if (pantallaActual == PANTALLA_UNO) {

            /*if(paso_txt)
            {
                printf("ENTRO A LIBERAR LA CADENA\n");
                free(cadena_H);
                cadena_H = NULL;
            }*/
            // Título
            int anchoTitulo = MeasureText("INGRESAR ARCHIVO TXT", 28);
            DrawText("INGRESAR ARCHIVO TXT", (ANCHO - anchoTitulo) / 2, 60, 28, DARKGRAY);

            // Instrucción
            int anchoInstr = MeasureText("Escribe el nombre del archivo (sin .txt):", 18);
            DrawText("Escribe el nombre del archivo (sin .txt):",
                     (ANCHO - anchoInstr) / 2, 100, 18, GRAY);
            int anchoConf = MeasureText("Luego de ingresar el nombre presiona ENTER para confirmar:",18);
            DrawText("Luego de ingresar el nombre presiona ENTER para confirmar",
                     (ANCHO - anchoConf) / 2, 200, 18, GRAY);

            // Campo de texto centrado
            Rectangle campo = { (ANCHO - 400) / 2, 140, 400, 45 };

            // Activar/desactivar campo al hacer clic
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mouse = GetMousePosition();
                campoActivo = CheckCollisionPointRec(mouse, campo);
            }

            // Leer input si el campo está activo
            if (campoActivo) {
                int tecla = GetCharPressed();
                while (tecla > 0) {
                    if (cantChars < 63) {
                        nombreArchivo[cantChars] = (char)tecla;
                        cantChars++;
                        nombreArchivo[cantChars] = '\0';
                        archivoValido = 0;      // resetear validación al escribir
                        mensajeError[0] = '\0';
                    }
                    tecla = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && cantChars > 0) {
                    cantChars--;
                    nombreArchivo[cantChars] = '\0';
                    archivoValido = 0;
                    mensajeError[0] = '\0';
                }
                // Confirmar con Enter
                if (IsKeyPressed(KEY_ENTER) && cantChars > 0) {
                    // Armar la ruta con .txt y verificar si existe
                    char ruta[72];
                    snprintf(ruta, sizeof(ruta), "%s.txt", nombreArchivo);
                    FILE *prueba = fopen(ruta, "r");
                    if (prueba) {
                        fclose(prueba);
                        archivoValido = 1;
                        snprintf(mensajeError, sizeof(mensajeError), "");
                    } else {
                        archivoValido = 0;
                        snprintf(mensajeError, sizeof(mensajeError),
                                 "Archivo no encontrado: %s.txt", nombreArchivo);
                    }
                }
            }

            // Dibujar campo
            Color colorBorde = campoActivo ? BLUE : GRAY;
            DrawRectangleRec(campo, LIGHTGRAY);
            DrawRectangleLinesEx(campo, 2, colorBorde);
            DrawText(nombreArchivo, campo.x + 10, campo.y + 12, 20, DARKGRAY);

            // Cursor parpadeante
            if (campoActivo && (GetTime() * 2) - (int)(GetTime() * 2) < 0.5) {
                int anchoTexto = MeasureText(nombreArchivo, 20);
                DrawText("|", campo.x + 10 + anchoTexto, campo.y + 12, 20, DARKGRAY);
            }

            // Hint ".txt" en gris a la derecha del campo
            int anchoNombre = MeasureText(nombreArchivo, 20);
            if (cantChars == 0) {
                DrawText(".txt", campo.x + 10, campo.y + 12, 20, LIGHTGRAY);
            }

            // Mensaje de error o éxito
            if (mensajeError[0] != '\0') {
                int anchoError = MeasureText(mensajeError, 16);
                DrawText(mensajeError, (ANCHO - anchoError) / 2, 275, 16, RED);
            }
            if (archivoValido) {
                char msgOk[80];
                snprintf(msgOk, sizeof(msgOk), "Archivo encontrado: %s.txt", nombreArchivo);
                int anchoOk = MeasureText(msgOk, 16);
                DrawText(msgOk, (ANCHO - anchoOk) / 2, 225, 16, GREEN);
            }
            // Botón confirmar (solo activo si el archivo es válido)
          /*  int anchoBtnConf = 200;
            Color colorBtn = archivoValido ? DARKGREEN : GRAY;
            Rectangle btnConfirmar = { (ANCHO - anchoBtnConf) / 2, 300, anchoBtnConf, 45 };
            Vector2 mouse = GetMousePosition();
            int hoverConf = CheckCollisionPointRec(mouse, btnConfirmar) && archivoValido;
            DrawRectangleRec(btnConfirmar, hoverConf ? GREEN : colorBtn);
            DrawRectangleLinesEx(btnConfirmar, 2, DARKGRAY);
            int anchoConfTxt = MeasureText("Confirmar", 20);
            DrawText("Confirmar", btnConfirmar.x + (anchoBtnConf - anchoConfTxt) / 2,
                     btnConfirmar.y + 12, 20, WHITE);
            if (hoverConf && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                hammingSeleccionado = 0;  // resetear selección al confirmar
            }*/

            // Botones Hamming (solo visibles si el archivo es válido)
            if (archivoValido) {

                int anchoH = 160;
                int altoH  = 45;
                int espacioH = 20;
                int totalAncho = anchoH * 3 + espacioH * 2;
                int xBase = (ANCHO - totalAncho) / 2;
                int yH    = 270;

                // Etiquetas y valores de cada botón
                const char *etiquetas[3] = { "Hamming 8", "Hamming 1024", "Hamming 16384" };
                int valores[3]           = { 8, 1024, 16384 };

                DrawText("Selecciona el tipo de Hamming:",
                         (ANCHO - MeasureText("Selecciona el tipo de Hamming:", 16)) / 2,
                         250, 16, GRAY);

                for (int i = 0; i < 3; i++) {
                    int xBtn = xBase + i * (anchoH + espacioH);
                    Rectangle btn = { xBtn, yH, anchoH, altoH };
                    Vector2 m = GetMousePosition();
                    int hover  = CheckCollisionPointRec(m, btn);
                    int activo = (hammingSeleccionado == valores[i]);

                    // Color: azul si está seleccionado, gris hover si no
                    Color fondo  = activo ? BLUE : (hover ? LIGHTGRAY : RAYWHITE);
                    Color borde  = activo ? DARKBLUE : GRAY;
                    Color txtCol = activo ? WHITE : DARKGRAY;

                    DrawRectangleRec(btn, fondo);
                    DrawRectangleLinesEx(btn, 2, borde);

                    int anchoTxt = MeasureText(etiquetas[i], 18);
                    DrawText(etiquetas[i],
                             xBtn + (anchoH - anchoTxt) / 2,
                             yH + (altoH - 18) / 2,
                             18, txtCol);

                    if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        hammingSeleccionado = valores[i];
                    }
                }

                // Mostrar selección actual
                if (hammingSeleccionado != 0) {
                    char msgSel[48];
                    snprintf(msgSel, sizeof(msgSel), "Seleccionado: Hamming %d", hammingSeleccionado);
                    int anchoSel = MeasureText(msgSel, 16);
                    DrawText(msgSel, (ANCHO - anchoSel) / 2, 380, 16, DARKBLUE);

                }

                if (hammingSeleccionado != 0) {
                    int anchoProcesar = 200;
                    Rectangle btnProcesar = { (ANCHO - anchoProcesar) / 2, 330, anchoProcesar, 45 };
                    Vector2 mp = GetMousePosition();
                    int hoverP = CheckCollisionPointRec(mp, btnProcesar);
                    DrawRectangleRec(btnProcesar, hoverP ? DARKBLUE : BLUE);
                    DrawRectangleLinesEx(btnProcesar, 2, DARKBLUE);
                    int anchoPTxt = MeasureText("Procesar", 20);
                    DrawText("Procesar", btnProcesar.x + (anchoProcesar - anchoPTxt) / 2,btnProcesar.y + 12, 20, WHITE);


                    if (hoverP && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    {
                        char ruta[72];
                        snprintf(ruta, sizeof(ruta), "%s.txt", nombreArchivo);
                        printf("ARCHIVO NOMBRE %s", ruta);

                        arch = fopen(ruta, "rb");

                        // OPTIMIZACION: Leemos el tamaño del archivo para pedir memoria una sola vez
                        fseek(arch, 0, SEEK_END);
                        file_size = ftell(arch);
                        rewind(arch);

                        printf("%d\n", file_size);

                        int max_bits = (file_size * 8) + 10;
                        int *chain = malloc(max_bits * sizeof(int));

                        n=1;

                        printf("Procesando archivo de %ld bytes...\n", file_size);
                        while ((caracter = fgetc(arch)) != EOF) {

                            for (t = 7; t >= 0; t--) {
                                chain[n++] = ((unsigned char)caracter >> t) & 1;
                            }
                        }
                        fclose(arch);
                        if(paso_txt==1)
                        {
                            free(cadena_H);

                        }
                        cadena_H = Hamming(chain,hammingSeleccionado,n,&largo_h);

                        free(chain);
                        mod = hammingSeleccionado;
                        seleccion= 1;
                        guardarArchivoH(cadena_H,mod,largo_h);
                        printf("pasa por aca\n");
                        paso_txt=1;

                        errores =0;
                        paso = 0;         // Obliga a la Pantalla 3 a introducir errores
                        paso_P4 = 0;      // Obliga a la Pantalla 4 a guardar el DEX
                        paso_p5 = 0;      // Obliga a la Pantalla 5 a decodificar
                        visorCargado = 0; // Obliga a la Pantalla 6 a recargar los TXT visuales
                }
                if(seleccion==1)
                {
                    int anchoTerminadoHam = MeasureText("El archivo fue codificado correctamente",30);
                        DrawText("El archivo fue codificado correctamente",(ANCHO - anchoTerminadoHam) / 2, 440, 30, GREEN);
                }
             }
            }

            int salir = DibujarBoton((ANCHO - 200) / 2, 535, 200, 45, "Volver al menu");
            // Botón volver
            if (salir){
                nombreArchivo[0]  = '\0';
                cantChars         = 0;
                campoActivo       = 0;
                archivoValido     = 0;
                mensajeError[0]   = '\0';
                hammingSeleccionado = 0;
                pantallaActual    = PANTALLA_MENU;
                paso_txt=1;
                seleccion=0;


            }

        }
        else if(pantallaActual == PANTALLA_TRES)
        {
            // Ejecutar UNA sola vez
            if(paso == 0)
            {
                printf("Entro a donde quiero\n");
                introducir_errores(cadena_H, mod, largo_h, &errores);
                printf("Errores %d",errores);
                paso = 1;  // marcar como hecho inmediatamente
            }

            // Dibujar SIEMPRE (no dentro del if)
            DrawText("PANTALLA 3", 320, 80, 30, GREEN);
            DrawText("Se introdujeron los errores con exito", 240, 200, 20, DARKGRAY);

            char texto[60];
            snprintf(texto, sizeof(texto), "Se introdujeron %d errores en la cadena", errores);
            DrawText(texto, 215, 240, 20, DARKGRAY);

            // Botón volver (solo una vez, no duplicado)
            if (DibujarBoton((ANCHO - 200) / 2, 535, 200, 45, "Volver al menu"))
            {
                paso = 0;  // resetear para la próxima vez que entres
                pantallaActual = PANTALLA_MENU;
            }
        }
        else if(pantallaActual == PANTALLA_CUATRO)
        {
            DrawText("PANTALLA 4", 320, 80, 30, GREEN);
            DrawText("Se guardo el archivo .DEx con exito", 240, 200, 20, DARKGRAY);
            if(paso_P4 == 0)
            {
               generarArchivoDEX(cadena_H, mod, largo_h, "archivo_con_error.txt",file_size);
                paso_P4 = 1;  // marcar como hecho inmediatamente
            }

             if (DibujarBoton((ANCHO - 200) / 2, 535, 200, 45, "Volver al menu"))
            {
                paso = 0;  // resetear para la próxima vez que entres
                pantallaActual = PANTALLA_MENU;
            }

        }else if(pantallaActual == PANTALLA_CINCO)
        {
            DrawText("PANTALLA 4", 320, 80, 30, GREEN);
            DrawText("Se guardo el archivo .DCx con exito", 240, 200, 20, DARKGRAY);
            if(paso_p5 == 0)
            {
               info_recuperada = decodificarHamming(cadena_H, mod, largo_h, &largo_info,file_size);
                guardarInfoRecuperada(info_recuperada, largo_info, "recuperado.DCx");
                paso_p5 = 1;  // marcar como hecho inmediatamente
            }

             if (DibujarBoton((ANCHO - 200) / 2, 535, 200, 45, "Volver al menu"))
            {
                paso = 0;  // resetear para la próxima vez que entres
                pantallaActual = PANTALLA_MENU;


            }

        }
        else if (pantallaActual == PANTALLA_SEIS) {

            // Cargar archivos la primera vez que se entra
            if (!visorCargado) {
                // Liberar si había algo antes
                if (contenido1 != NULL) { free(contenido1); contenido1 = NULL; }
                if (contenido2 != NULL) { free(contenido2); contenido2 = NULL; }


                // Cargar los dos archivos — ajusta los nombres según necesites
                snprintf(archivoVisor1, sizeof(archivoVisor1), "ej.txt", nombreArchivo);
                snprintf(archivoVisor2, sizeof(archivoVisor2), "archivo_con_error.txt");

                contenido1 = CargarArchivo(archivoVisor1,&size1);
                contenido2 = CargarArchivo(archivoVisor2,&size2);
                scroll1 = 0;
                scroll2 = 0;
                visorCargado = 1;
            }

            // Título general
            int anchoTit = MeasureText("VISOR DE ARCHIVOS", 24);
            DrawText("VISOR DE ARCHIVOS", (ANCHO - anchoTit) / 2, 10, 24, DARKGRAY);

            int yVisor    = 44;
            int altoVisor = 480;
            int anchoCol  = (ANCHO - 30) / 2;

            // Detectar en qué columna está el mouse para el scroll
            Vector2 mouse = GetMousePosition();
            float rueda = GetMouseWheelMove();

            // Columna izquierda: x entre 10 y 10+anchoCol
            if (mouse.x >= 10 && mouse.x <= 10 + anchoCol &&
                mouse.y >= yVisor && mouse.y <= yVisor + altoVisor) {
                scroll1 -= (int)rueda * 2;
                if (scroll1 < 0) scroll1 = 0;
            }
            // Columna derecha
            if (mouse.x >= 20 + anchoCol && mouse.x <= ANCHO - 10 &&
                mouse.y >= yVisor && mouse.y <= yVisor + altoVisor) {
                scroll2 -= (int)rueda * 2;
                if (scroll2 < 0) scroll2 = 0;
            }

            // Dibujar columnas
            DibujarColumnaTexto(contenido1, "ej.txt", size1, archivoVisor1,
                    10, anchoCol, altoVisor, scroll1, yVisor, miFuente,1);

            DibujarColumnaTexto(contenido2, "ej.txt",size2, archivoVisor2,
                    20 + anchoCol, anchoCol, altoVisor, scroll2, yVisor, miFuente,2);

            // Instrucción de scroll
            DrawText("Rueda del mouse sobre cada columna para hacer scroll",
                     (ANCHO - MeasureText("Rueda del mouse sobre cada columna para hacer scroll", 14)) / 2,
                     530, 14, GRAY);

            // Botón volver
            if (DibujarBoton((ANCHO - 200) / 2, 558, 200, 38, "Volver al menu")) {
                visorCargado = 0;
                pantallaActual = PANTALLA_MENU;
            }
        }

        EndDrawing();
    }
   UnloadFont(miFuente);
    CloseWindow();


    return 0;
}



