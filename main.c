#include <stdio.h>
#include <stdlib.h>

void imprimirB(char c)
{
    int i;
    for(i = 7; i >= 0;  i --)
    {
        printf("%d", (c >> i) & 1);
    }
}



int Hamming(int *cadena, int modulo)
{
    int cantidad;
    switch (modulo)
    {
        case 8:
            cantidad = 4;
            p = 4;
        break;
        case 1024:
            info = 1013;
            p = 10;
        break;
        case 16384:
            cantidad = 16369;
            p = 14;
        break;
    }


    int d = 1;
    int acumulado=0;
    int p_aux = p-(p-1);
    while(p!= 0)
    {
        for(i = p; i <= cantidad; i++)
        {
            if((i & (i-1))== 0)
            {
                printf("nada\n");
            }
            else
            {
                acumulado = acumulado + (p_aux & i);
            }
        }
        p_aux++;
        p--;
    }
    return 0;
}


int main()
{
    FILE *arch;
    arch = fopen("ej.txt","rb");
    if(arch == 'NULL')
    {
        printf("Ocurrio un error");
        return 0;
    }

    fseek(arch, 0, SEEK_END);
    long largo = ftell(arch);
    fseek(arch,0, SEEK_SET);
    char *buffer = malloc(largo);
    if(buffer)
    {
        fread(buffer, 1,largo,arch);
    }

    printf("%s\n", buffer);
    /*int caracter;
    while((caracter = fgetc(arch))!= EOF)
    {
        printf("%d", caracter);
        imprimirB((char)caracter);
    }*/
    fclose(arch);


    int salir = 0;

    while(salir == 0){
        printf("Ingrese una opcion\n");
        printf("[1] Leer archivo txt\n");
            salir =1;
    }
    return 0;
}
