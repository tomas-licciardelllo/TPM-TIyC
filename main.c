#include <stdio.h>
#include <stdlib.h>
#include <math.h>
int imprimirB(unsigned char c)
{
    int i;
    for(i = 7; i >= 0;  i --)
    {
        printf("%d", (c >> i) & 1);
    }
}


int* Hamming(int cadena[], int modulo, int limite)
{
    int i,suma;
    int n;
    int m = 0;
    while(limite!= m)
    {
        printf("Entro");
        for(i=1; i<=modulo; i ++)
        {
            if((i & (i-1)) == 0)
            {
                for(n=i; n<=modulo; n++)
                {
                    if((n & (n-1) != 0) && ((n & i)>>(int)log2(i)))
                   {
                        suma += cadena[n];
                        m++;
                   }
                }
                m++;
                if(suma % 2 == 0)
                {
                     cadena[i] = suma;
                }
                else
                {
                    cadena[i] = suma + 1;
                }
            }
        }

    }
    return cadena;
}



/*int Hamming(int *cadena, int modulo)
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
}*/


int main()
{
    int *chain = malloc(2 *sizeof(int));
    chain[0] = 0;
    int i,n,t;
    i = 1;
    n = 1;
    FILE *arch;
    arch = fopen("ej.txt","rb");
    if(arch == 'NULL')
    {
        printf("Ocurrio un error");
        return 0;
    }

    int caracter;
    while((caracter = fgetc(arch))!= EOF)
    {
        printf("%c", caracter);
        printf("no salgp\n");
        imprimirB((unsigned char)caracter);
        for(t= 7; t>=0; t--)
        {
            n+=1;
            chain = realloc(chain, n * sizeof(int));
            chain[i]= (((unsigned char)caracter >> t) & 1);
            i++;
        }

    }
    fclose(arch);

    int salir = 0;

    i =0;
    printf("MOSTRAME ESTO %d\n",n);
    while(i<n)
    {
      printf("%d", chain[i]);
      i++;
    }
    printf("\n");
    //Hamming(chain,8,n);
    while(salir == 0){


        printf("%d \n",(int)log2(32));

        printf("Ingrese una opcion\n");
        printf("[1] Leer archivo txt\n");
            salir =1;
    }
    return 0;
}
