/* here I pull the width and height, using hte name of the aspect ratio */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aspcolconst.h"

#define CHKNARGS(x, a); \
    if((x-1) != (a)) { \
        printf("Usage error: program requires %i arguments.\n", (a)); \
        exit(EXIT_FAILURE); \
    }

typedef struct
{
    int h,v;
    char fn[24];
} pos;

void givemeasp(char *aspname, int *cwid, int *chei) /* give me aspect */
{
    int i=0;

    /*  the following will grab the first ocurrence of the name */
    while((strcmp(aspff[i++].nasp, aspname)) != 0 )
        if(i==SZASP) { /*  take care when we overshoot */
            printf("Error. You typed the name of the aspect ratio wrong\n"); 
            exit(EXIT_FAILURE);
        }

    *cwid = aspff[i-1].awd;
    *chei = aspff[i-1].aht;
}

pos **givemepos(int srcw, int srch, int cw, int ch, int *hts, int *vts)
{

    int i, j;
    int vtimes = *vts;
    int htimes = *hts;

    int hm=srcw/cw; /* _h_orizontal multiples */
    int hrem=srcw%cw; /* h remainders */
    int vm=srch/ch;
    int vrem=srch%ch;
    htimes= (hrem!=0)? hm+1: hm;
    vtimes= (vrem!=0)? vm+1: vm;

    pos **parr=malloc(vtimes*sizeof(pos*));
    for(i=0;i<vtimes;++i) 
        parr[i]=calloc(htimes, sizeof(pos));

    for(j=1;j<htimes-1;j++)
        parr[0][j].h = parr[0][j-1].h + cw;
    parr[0][htimes-1].h = srcw-cw;

    for(j=1;j<vtimes-1;j++)
        parr[j][0].v = parr[j-1][0].v + ch;
    parr[vtimes-1][0].v = srch-ch;

    for(i=1;i<vtimes-1;++i) {
        parr[i][htimes-1].h = srcw-cw;
        parr[i][htimes-1].v = parr[i-1][htimes-1].v + ch;
        for(j=1;j<htimes-1;j++) {
            parr[i][j].h = parr[i][j-1].h + cw;
            parr[i][j].v = parr[i-1][j].v + ch;
        }
    }
    for(j=1;j<htimes-1;j++) {
        parr[vtimes-1][j].h = parr[vtimes-1][j-1].h + cw;
        parr[vtimes-1][j].v = srch-ch;
    }
    parr[vtimes-1][htimes-1].h = srcw-cw;
    parr[vtimes-1][htimes-1].v = srch-ch;

    for(j=0;j<vtimes;++j)
        for(i=0;i<htimes;i++) 
            sprintf(parr[j][i].fn, "row%04i_col%04i.jpg", j+1, i+1);
            // sprintf(parr[j][i].fn, "cro_%06i_%06i.jpg", parr[j][i].h, parr[j][i].v);

    for(j=0;j<vtimes;++j) {
        for(i=0;i<htimes;i++) 
            printf("%s ", parr[j][i].fn);
        printf("\n"); 
    }
    *vts=vtimes;
    *hts=htimes;
    return parr;
}

void freeposarr(pos **parr, int vtimes)
{
    int i;
    for(i=0;i<vtimes;++i) 
        free(parr[i]);
    free(parr);
}

int main(int argc, char *argv[])
{
    int i, j;
    CHKNARGS(argc, 3);
    char *asp=argv[1];
    int srcw= atoi(argv[2]);
    int srch= atoi(argv[3]);

    int cw, ch, vtimes, htimes;

    givemeasp(asp, &cw, &ch);

    pos **parr = givemepos(srcw, srch, cw, ch, &vtimes, &htimes);
    for(j=0;j<vtimes;++j) {
        for(i=0;i<htimes;i++) 
            printf("%s ", parr[j][i].fn);
        printf("\n"); 
    }

    freeposarr(parr, vtimes);

    return 0;
}
