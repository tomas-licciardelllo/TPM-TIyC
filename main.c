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
printf("\n");
}




int* Hamming(int cadena[], int modulo, int limite)
{
    int *cadena_h = malloc(1*sizeof(int));
    cadena_h[0]=0;
    int n=1;
    int indice = 1;
    int arr = 1;
    int i = 1;
    int contador = 1;
    while(contador<limite)
    {
        while(indice<=modulo)
        {
            if((indice & (indice - 1))== 0)
            {
                arr++;
                cadena_h = realloc(cadena_h, arr * sizeof(int));
                cadena_h[n] = 2;
            }
            else
            {
                arr++;
                cadena_h = realloc(cadena_h, arr * sizeof(int));
                cadena_h[n] = cadena[i];
                i++;
                contador++;
            }
            indice++;
            n++;
        }
        indice=1;
    }
    return cadena_h;
}

/*int* Hamming(int cadena[], int modulo, int limite)
{
    int *cadena_H = malloc(1 *sizeof(int));
    int arr = 1;
    cadena_H[0] = 0;
    int i,suma=0;
    int n=1;
    int m = 1;
    int indice = 1;
    int cantidad=0;
    switch(modulo)
    {
        case 8:
            cantidad = 4;
        break;
        case 1024:
            cantidad = 1013;
        break;
        case 16384:
            cantidad = 16369;
        break;
    }
    int check = 0;
    int p=0;
    while(limite!= m)
    {
        printf("Entro\n");
        for(i=1; i<=modulo; i ++)
        {
            if((i & (i-1)) == 0)
            {
                p=i;
                /*for(n=i; n<=cantidad; n++)
                {
                    check = (check << (int)log2(i)) | 1;
                    if((n & (n-1) != 0) && (p & i))
                   {
                        suma += cadena[n];
                   }
                   p++;
                }
                n=indice;
                while(n<=cantidad)
                {
                    if((p&i) && ((p & (p-1)) != 0))
                    {
                        suma +=cadena[n];
                        n++;
                    }
                    else
                    {
                        if((p&i)==0 && ((p & (p-1)) != 0))
                        {
                            n++;
                        }
                    }
                    p++;
                }
                if(suma % 2 == 0)
                {
                     indice++;
                     cadena_H = realloc(cadena_H,arr *sizeof(int));
                     cadena_H[i] = suma;

                }
                else
                {
                    indice++;
                     cadena_H = realloc(cadena_H,arr *sizeof(int));
                     cadena_H[i] = suma + 1;
                }
                suma = 0;
            }
            else
            {
                indice++;
                cadena_H = realloc(cadena_H,arr *sizeof(int));
                cadena_H[i] = cadena[i];
                m++;
            }
        }
    }
    return cadena_H;
}*/



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

   /* i =0;
    printf("MOSTRAME ESTO %d\n",n);
    while(i<n)
    {
      printf("%d", chain[i]);
      i++;
    }
    printf("\n");
    */
    int *cadena_H = Hamming(chain,8,n);
    i =0;
    while(i<17)
    {
        printf("%d", cadena_H[i]);
        i++;
    }
    printf("\n AVER Q ONDA");
    while(salir == 0){


        //printf("%d \n",(int)log2(32));

        printf("Ingrese una opcion\n");
        printf("[1] Leer archivo txt\n");
            salir =1;
    }
    return 0;
}
