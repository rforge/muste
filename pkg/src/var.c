/* _var.c 28.4.1986/SM (13.7.1994) (29.9.1996)
*/

#include <Rmath.h>
#include "survo.h"
#include "survoext.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include "survolib.h"
/* #include "survodat.h" */

#define NDATA 12
#define MAXARG 10
#define EARG '\376'

SURVO_DATA d;
SURVO_DATA sd[NDATA];

int nvar,var[EP4];
char lauseke[LLENGTH];
long jnro;
int lag;
int vm_act; /* ennen uusien muuttujien perustamista */
int ndata;
/* int sdata;  */
char sdat_list[LLENGTH], *sdat[NDATA];

char *str_opnd[MAXARG+4]; /* 11.10.2003 */

int muunnos;
double  *xx;
long  *oxx;
long nxx;
long nx2; /* # of non-missing values 30.12.2002 */
double mean,stddev;
double sum;            /* 11.10.1996 */

int str_muunnos=0;
int first_new_var=0; /* 18.3.92 */

extern int spn;
extern double *arvo;
extern char **spb2;
extern char comline[];
extern int l_virhe;

/* specifications in the edit field */
extern char *splist;
extern char **spa, **spb, **spshad;
extern int spn;
int spn_order;

extern char *spl;
extern int global;

extern double *earg;

double level;  /* ainakin muunto2 käyttää */

/* varstr-funktiot */
int str_var,str_lag,str_var_start,str_var_len;
char str_vasen[LLENGTH];
int code_ind=0;
unsigned char code[256];

int varhaku(char *sana)
        {
        int i;
        char *p;
        char tyyppi;
        char vartype[LLENGTH];
        int varlen;

        tyyppi='4';
        p=strchr(sana,':');
        if (p!=NULL)
            {
            *p=EOS;
            tyyppi=*(p+1);
            if (tyyppi==EOS || strchr("1248S",tyyppi)==NULL)
                {
                if (tyyppi==EOS) tyyppi=' ';
                sprintf(sbuf,"\nUnknown type %c for %s!",tyyppi,sana); sur_print(sbuf);
                sur_print("\nPermitted types are 1,2,4,8 and S.");
                WAIT; return(-1);
                }
            if (tyyppi=='S')
                {
                varlen=atoi(p+2);
                if (varlen<=0)
                    {
                    sur_print("\nEnter the type of a string variable as Sn ");
                    sur_print("\nwhere n is an integer >0.                 ");
                    WAIT; return(-1);
                    }
                }
            }
        i=varfind2(&d,sana,0);
        if (i<0)
            {
            if (d.type!=2)
                {
                sprintf(sbuf,"\nVariable %s not found!",sana);
                sur_print(sbuf); WAIT; return(-1);
                }
            else
                {
                strncpy(vartype,space,(unsigned int)(d.d2.extra-4));
                *vartype=tyyppi;
                vartype[1]='A'; vartype[2]='-';
                if (tyyppi!='S') varlen=(int)(tyyppi-'0');
                i=fi_var_save(&d.d2,d.d2.m,vartype,varlen,sana);
                if (i<0) return(-1);
                d.m=d.d2.m;
                i=d.m-1;
                if (first_new_var==0) first_new_var=i; /* 18.3.92 */
                }
            }
        return(i);
        }

void poista_var()
        {
        int i,k,h;

        k=0;
        for (i=0; i<d.m_act; ++i)
            {
            if (!nvar) { if (d.v[i]==var[0]) { ++k; continue; } }
            else
                {
                for (h=0; h<nvar; ++h)
                    {
                    if (d.v[i]==var[h]) { ++k; break; }
                    }
                if (h<nvar) continue;
                }
            d.v[i-k]=d.v[i];
            }
        d.m_act-=k;
        }


int spread3_var(char *x, int j)
        {
        int i,k,pos;
        char *p;
        char xs[LLENGTH];

        pos=1;
        while (pos<ed1)
            {
            p=strchr(x+pos,'=');
            if (p==NULL) break;
            if (*(p+1)==EOS) break;
            if (*(p+1)=='=') { pos+=2; continue; }

/* Aktivoidun kohdan ohittaminen VAR-listan osalta */
            if (j==r1+r-1 && pos==1)   /* 1.3.1992 */
                {
                p=strstr(x+1," / ");
                if (p==NULL) break;
                p=strchr(p+1,'=');
                if (p==NULL) break;
                }

            if (spn>=specmax) return(-spn);
            pos=p-x; i=pos-1;
            while (i>0 && x[i]!=' ') --i;
            if (spl-splist+pos-i+1>speclist) return(-spn);
            strncpy(spl,x+i+1,(unsigned int)(pos-i-1));
            spa[spn]=spl; spl+=pos-i; *(spl-1)=EOS;
            i=pos+1;
            while (i<ed1 && x[i]!=' ') ++i;
            if (spl-splist+i-pos+1>speclist) return(-spn);
            strncpy(spl,x+pos+1,(unsigned int)(i-pos-1));
            spb[spn++]=spl; spl+=i-pos; *(spl-1)=EOS;

            if (*(spl-2)=='&') { k=jatkorivit(j+1);
                                 if (k<0) return(-spn);
                               }
            spshad[spn-1]=NULL;
            if (zs[j]!=0)
                {
                edread(xs,zs[j]);
                if (spl-splist+i-pos+1>speclist) return(-spn);
                strncpy(spl,xs+pos+1,(unsigned int)(i-pos-1));
                spshad[spn-1]=spl; spl+=i-pos; *(spl-1)=EOS;
                }

            ++pos;
            }
        return(spn);
        }


int spread2_var(int lin, int *raja1)
        {
        char raja[12];
        int j,i;
        char x[LLENGTH];

        strcpy(raja,"*..........");
        for (j=lin-1; j>0; --j)
            {
            edread(x,j);
            i=sur_instr(x,raja);
            if (i>=0) break;
            }
        *raja1=j;
        for (j=*raja1+1; j<=ed2; ++j)
            {
            edread(x,j);
            if (global==1)
                {
                i=sur_instr(x,"*GLOBAL*");
                if (i>0) global=0;
                }
            i=sur_instr(x,raja);
            if (i>=0) break;
            if (j==r1+r-1) continue; /* aktivoidun rivin ohittaminen */
            spn=spread3_var(x,j); if (spn<0) return(spn);
            }


/*  printf("\n"); for (i=0; i<spn; ++i) printf("\n%s=%s varjo=%s",
                                         spa[i],spb[i],spshad[i]); getch();
*/
        return (spn);
        }

int varnimet()
        {
        extern SURVO_DATA d;
        int i,k;
        char nimi[LLENGTH];

        for (i=0; i<d.m_act; ++i)
            {
            strcpy(nimi,d.varname[d.v[i]]);
            k=strlen(nimi); while (nimi[k-1]==' ') nimi[--k]=EOS;

            if (spn>=specmax) return(-spn);
            if (spl-splist+k+1>speclist) return(-spn);

            strncpy(spl,nimi,(unsigned int)k);
            spa[spn]=spl; spb[spn]=NULL;
            spl+=k+1; *(spl-1)=EOS;
            ++spn;
            }
        return(spn);
        }

void spec_error()
        {
        sprintf(sbuf,"\nToo many active variables + specifications (specmax=%d)",
                        specmax); sur_print(sbuf);
        sprintf(sbuf,"\nor too much text in specifications (speclist=%d)",speclist);
        sur_print(sbuf); WAIT;
        }

int sp_init_var(int lin, int m) /* m = aktiivisten muuttujien lkm */
        {
/*
        int i,tila;
        char *p;
        long l;
*/
        int spn1;
        int raja1;
        char x[LLENGTH];


/*      l=(long)speclist+9L*(long)m;
        if (l>65535L) l=65535L;
        speclist=l;
        specmax=specmax+m;
        if (specmax>8192) specmax=8192;
*/
        sp_check();
        speclist+=9*(m+3)+4;   /* 3 sp2_init() */
        specmax+=m+3+2;

        splist=malloc((unsigned int)speclist);
        if (splist==NULL) { not_enough_mem_for_spec(); return(-1); }
        spa=(char **)malloc(specmax*sizeof(char *));
        if (spa==NULL) { not_enough_mem_for_spec(); return(-1); }
        spb=(char **)malloc(specmax*sizeof(char *));
        if (spb==NULL) { not_enough_mem_for_spec(); return(-1); }
        spb2=(char **)malloc(specmax*sizeof(char *));
        if (spb2==NULL) { not_enough_mem_for_spec(); return(-1); }
        spshad=(char **)malloc(specmax*sizeof(char *));
        if (spshad==NULL) { not_enough_mem_for_spec(); return(-1); }
        arvo=(double *)malloc(specmax*sizeof(double));
        if (arvo==NULL) { not_enough_mem_for_spec(); return(-1); }

        spn=0; spl=splist; global=0;

        spn=varnimet(); if (spn<0) { spxxxx(); return(spn); }

        edread(x,lin); spn=spread3_var(x,lin);
        if (spn<0) { spxxxx(); return(spn); }
        spn=spread2_var(lin,&raja1);
        if (spn<0) { spxxxx(); return(spn); }
        if (raja1==0) { spxxxx(); return(spn); }
        spn1=spn;
        global=1;
        spn=spread2_var(1,&raja1);
        if (spn<0) { spxxxx(); return(spn); }
        if (global==1) spn=spn1;
        spxxxx(); return(spn);
        }

int sp_write(char *nimi, double y)
        {
        int k=strlen(nimi);

        strncpy(spl,nimi,(unsigned int)k);
        spa[spn]=spl; spb[spn]=NULL;
        spl+=k+1; *(spl-1)=EOS;
        arvo[spn]=y;
        ++spn;
        return(1);
        }

int sp2_init()
        {
        int i;
        extern SURVO_DATA d;

        i=sp_write("MISSING",MISSING8); if (i<0) return(-1);
        i=sp_write("ORDER",MISSING8); if (i<0) return(-1);
        spn_order=spn-1;
        i=sp_write("N",(double)d.n); if (i<0) return(-1);
        return(1);
        }

int lue_arvot(long j)
        {
        extern SURVO_DATA d;
        int i,k;

        for (i=0; i<d.m_act; ++i)
            {
            k=data_load(&d,j,d.v[i],&arvo[i]);
            if (k<0) return(-1);
            }
        arvo[spn_order]=(double)j;  /* ORDER */
        return(1);
        }

void poista_uudet_muuttujat()  /* 18.3.92 */
        {
        if (d.type==2 && first_new_var)
            {
            fi_puts(&d.d2,&first_new_var,2,20L);
            }
        }

void var_error(char *s)
        {
        sprintf(sbuf,"\nError in %s",s); sur_print(sbuf);
/*        WAIT; */
        poista_uudet_muuttujat();
        }

/* Declaration */
int laske_var();

void korvaa_var(char *s,char *x,char *y)
        {
        char *p,*q;
        char z[LLENGTH];
        int len=strlen(x);

        *z=EOS;
        p=s;
        while ((q=strstr(p,x))!=NULL)
            {
            strncat(z,p,(unsigned int)(q-p));
            strcat(z,y);
            p=q+len;
            }
        strcat(z,p);
        strcpy(s,z);
        }


int f_edit_var(char *s,double *x,int n,double *py)
        {
        extern int n_earg;
        int i,k,len;
        char lauseke[LLENGTH];
        char xx[LLENGTH], *osa[MAXARG];
        char sana[7];     /*  EARG 1 2 3 4 EARG EOS */
        double y;



        len=strlen(s); s[len++]='(';
        i=0;
/**************************************************
for (k=0; k<spn; ++k)
    {
    printf("%d | %s | %s |    \n",k,spa[k],spb[k]);
    }
getch();
***************************************************/

        while (i<spn &&
               ( spa[i]==NULL || *spa[i]==EOS ||
           spa[i][strlen(spa[i])-1]!=':' || strncmp(s,spa[i],(unsigned int)len)!=0) )
                 ++i;

        if (i==spn) { s[len-1]=EOS; return(-1); }

/*      if (!earg_varattu) { k=varaa_earg(); if (k<0) return(-1); } */
        strcpy(lauseke,spb[i]);
        strcpy(xx,spa[i]);
        i=split(xx+len,osa,MAXARG);
        if (i!=n)
           {
           sprintf(sbuf,"\nArgument error in function %s",s); sur_print(sbuf);
           l_virhe=1; WAIT; return(-1);
           }
        osa[n-1][strlen(osa[n-1])-2]=EOS;  /* ): poistetaan */
/*
    for (i=0; i<n; ++i) printf("osa %d: %s\n",i+1,osa[i]); getch();
*/
        for (i=0; i<n; ++i)
            {
            k=aseta_earg(x[i],sana); if (k<0) return(-1);
            korvaa_var(lauseke,osa[i],sana);
            }
        laske_var(lauseke,&y);
        *py=y;
        n_earg-=n;
        return(1);
        }



#define NMAT 20
static double *mat_var[NMAT];
static char *rlab_var[NMAT],*clab_var[NMAT];
static int lr_var[NMAT],lc_var[NMAT];
static int m_var[NMAT],n_var[NMAT];
static int type_var[NMAT];
static char expr_var[NMAT][LNAME];
int nmat_var=0;
static char mat_name_var[NMAT][9];

void mat_function_var(char *f,char **s,int nn,double *yy)
        {
        int i,j,k;
        double xx[2];
/*        char *lab; */

/* printf("f=%s nn=%d %s %s\n",f,nn,s[0],s[1]); getch(); */

        for (k=0; k<nmat_var; ++k)
            {
            if (strcmp(f,mat_name_var[k])==0) break;
            }
        if (nmat_var==0 || k==nmat_var)
            {

            if (nmat_var==NMAT)
                {
                sprintf(sbuf,"Too many matrices (more than %d)!",NMAT);
                sur_print(sbuf); WAIT; l_virhe=1; nmat_var=0; return;
                }

/* mat_load(f,&mat[k],&m[k],&n[k],&rlab[k],&clab[k],&lr[k],&lc[k]); */

    matrix_load(f,&mat_var[k],&m_var[k],&n_var[k],&rlab_var[k],&clab_var[k],&lr_var[k],&lc_var[k],
                        &type_var[k],expr_var[k]);

            strcpy(mat_name_var[k],f);
            ++nmat_var;

            }
        if (nn==1 && m_var[k]==1) { nn=2; s[1]=s[0]; s[0]="1"; }
        i=lab_find(s[0],rlab_var[k],m_var[k],lr_var[k]);
        if (i>0) xx[0]=i;
        else
            {
            laske_var(s[0],&xx[0]);
            sprintf(sbuf,"%g",xx[0]);    /* 9.9.1999 */
            i=lab_find(sbuf,rlab_var[k],m_var[k],lr_var[k]);
            if (i>0) xx[0]=i;
            }
        if (nn>1)
            {
            i=lab_find(s[1],clab_var[k],n_var[k],lc_var[k]);
            if (i>0) xx[1]=i;
            else
                {
                laske_var(s[1],&xx[1]);
                sprintf(sbuf,"%g",xx[1]);    /* 9.9.1999 */
                i=lab_find(sbuf,clab_var[k],n_var[k],lc_var[k]);
                if (i>0) xx[1]=i;
                }
            }

        i=xx[0]; if (nn>1) j=xx[1];
        if (i<1 || i>m_var[k] || (nn>1 && (j<1 || j>n_var[k])) )
            {
            sur_print("\nError in matrix index!"); WAIT;
            l_virhe=1;
            return;
            }
        if (nn==1)
            *yy=mat_var[k][i-1];
        else
            {
            *yy=mat_var[k][i-1+m_var[k]*(j-1)];
            }
        }


double mfunktio_var(char *s, double *x, int n)
        {
        int i,k;
        double y;
        char S[32];
/* ****************************
       printf("mfunktio: %s\n",s);
     for (i=0; i<n; ++i) printf("%g ",x[i]); printf("\n"); getch();
***************************** */
        strncpy(S,s,31); S[31]=EOS; strupr(S);

        if (strcmp(S,"MAX")==0)
            {
            y=x[0];
            for (i=1; i<n; ++i) y=(x[i]>y)? (x[i]):(y);
            return(y);
            }
        if (strcmp(S,"MIN")==0)
            {
            y=x[0];
            for (i=1; i<n; ++i) y=(x[i]<y)? (x[i]):(y);
            return(y);
            }

       if (strcmp(S,"MAXN")==0)  /* 6.4.2003 */
           {
           y=x[0]; k=0;
           for (i=1; i<n; ++i) if (x[i]>y) { y=x[i]; k=i; }
           return((double)(k+1));
           }
       if (strcmp(S,"MINN")==0)
           {
           y=x[0]; k=0;
           for (i=1; i<n; ++i) if (x[i]<y) { y=x[i]; k=i; }
           return((double)(k+1));
           }

        if (strcmp(s,"C")==0)
            {
            double u,v;
            int iu,iv;

            if (n!=2) { arg_virhe(s); }
            iv=v=x[0]; iu=u=x[1];
            if ((double)iu!=u) return(0.0);
            if ((double)iv!=v) return(0.0);
            if (u>v/2) u=v-u;
            if (u<0 || v<0) return(0.0);
            if (u==0) return(1.0);
            y=1.0;
            for (; u>0; --u, --v) y*=(v/u);
            return(y);
            }

        if (strcmp(S,"MOD")==0) /* 29.8.1998 */
            return((double)((unsigned long)x[0]%(unsigned long)x[1]));

        if (strcmp(S,"ROUND")==0) /* 6.5.2003 */
            {
            y=pow(10.0,x[1]);
            return(sur_round(x[0]*y)/y);
            }

        if (*s=='M' && strncmp(s,"MAT_",4)==0)
            {
            mat_function_var(s+4,str_opnd,n,&y);
/* mat_element=0; // ++++  */
            return(y);
            }

/************************
        if (*s=='M' && strncmp(s,"MAT_",4)==0)
            {
            mat_function_var(s+4,x,n,&y);
            return(y);
            }
**********************/
        i=f_edit_var(s,x,n,&y); if (i>0) return(y);
/*      i=f_tiedosto(s,x,n,&y); if (i>0) return(y); */

/*  Väliaikaisesti pois käytöstä
        i=family_f(s,x,n,&y); if (i>0) return(y);
*/
        l_virhe=1;
        return(MISSING8);
        }


double funktio_var(char *s,double x)
        {
/*
        extern double probit();
        extern double uniform();
        extern double sur_rand0();
        extern double round();
        extern double lfact();
*/
        int i;
        double y;
        char S[32];

/* printf("\nf1=%s x=%g|",s,x); getch(); // ++++  */
        if (*s==EOS) return(x);
        if (*s=='M' && strncmp(s,"MAT_",4)==0) /* siirretty alkuun 17.2.2004 */
            {
            mat_function_var(s+4,str_opnd,1,&y);
            return(y);
            }

        if (x==MISSING8) return(x);
        strncpy(S,s,31); S[31]=EOS; strupr(S);
/* Randit väliaikaisesti pois käytöstä
        if (strcmp(S,"RAND")==0) return(sur_rand0(x,1));
        else if (strcmp(S,"URAND")==0) return(sur_rand0(x,2));
        else if (strcmp(S,"SRAND")==0) return(sur_rand0(x,3));
        else if (strcmp(S,"MRAND")==0) return(sur_rand0(x,4));
        else */

        if (strncmp(S,"SQR",3)==0)  /* 29.9.1996 x<0.0 etc. */
            { if (x<0.0) { l_virhe=1; return(0.0); } else return(sqrt(x)); }
        else if (strcmp(S,"LOG")==0)
            { if (x<=0.0) { l_virhe=1; return(0.0); } else return(log(x)); }
        else if (strcmp(S,"EXP")==0) return(exp(x));
        else if (strcmp(S,"SIN")==0) return(sin(x));
        else if (strcmp(S,"COS")==0) return(cos(x));
        else if (strcmp(S,"TAN")==0) return(tan(x));
        else if (strcmp(S,"ARCTAN")==0) return(atan(x));
        else if (strcmp(S,"ARCSIN")==0) return(asin(x));   /* 18.6.92 */
        else if (strcmp(S,"ARCCOS")==0) return(acos(x));
        else if (strcmp(S,"ABS")==0) return(fabs(x));
        else if (strcmp(S,"INT")==0) return(floor(x));
        else if (strcmp(S,"ROUND")==0) return(sur_round(x));
        else if (strcmp(S,"SGN")==0)
            {
            if (x>0.0) return(1.0);
            if (x<0.0) return(-1.0);
            return (0.0);
            }
        else if (strcmp(S,"IND")==0)
            { if (x>0.0) return(1.0); else return(0.0); }
        else if (strcmp(S,"PROBIT")==0) return(probit(x));
        else if (strcmp(S,"RND")==0) return(uniform(x));

        else if (strcmp(S,"LFACT")==0) return(lfact(x)); /* 7.9.2007 */

/******************************
        if (*s=='M' && strncmp(s,"MAT_",4)==0)
            {
            mat_function_var(s+4,&x,1,&y);
            return(y);
            }
***********************************/
        i=f_edit_var(s,&x,1,&y); if (i>0) return(y);
/*      i=f_tiedosto(s,&x,1,&y); if (i>0) return(y); */

/* Väliaikaisesti pois käytöstä
        i=family_f(s,&x,1,&y); if (i>0) return(y);
*/
        l_virhe=1;
        return(MISSING8);
        }


int lag_arvo(char *muuttuja,double *y)
        {
        int i;
        long j;
        extern SURVO_DATA d;

        j=jnro+(long)lag;
        if (j<1L || j>d.n) { *y=MISSING8; return(1); }
        i=varfind(&d,muuttuja); if (i<0) return(-1);
        data_load(&d,j,i,y);
        return(1);
        }

int sup_arvo(char *muuttuja,double *y)
        {
        int i,k;
        long j;
        int sdata;
        char *p;

        p=strchr(muuttuja,':');
        if (p==NULL) { sprintf(sbuf,"Error in %s",muuttuja); sur_print(sbuf); WAIT; exit(0); }
        *p=EOS; ++p;
        sdata=atoi(muuttuja+1);
        if (sdata<1 || sdata>ndata)
            {
            sprintf(sbuf,"\nIndata D%d: not defined!",sdata); sur_print(sbuf); WAIT; exit(0);
            }
        k=sdata-1;
        j=jnro+(long)lag;
        lag=0; /* 6.10.1989 */
        if (j<1L || j>sd[k].n) { *y=MISSING8; return(1); }
        i=varfind2(&sd[k],p,0);
        if (i<0)
            {
            sprintf(sbuf,"\nField %s not found in data %s!",p,sdat[k]);
            sur_print(sbuf); WAIT; exit(0);
            }
        data_load(&sd[k],j,i,y);
        return(1);
        }

/* declarations for laske2_var */
int laske_var();

int laske2_var(char *muuttuja,double *y)
        {
        int i,k;
        char *pvar;

        if (*muuttuja==EARG)
            {
            *y=earg[atoi(muuttuja+1)];
            return(1);
            }
        if (*muuttuja=='D' && (muuttuja[2]==':' || muuttuja[3]==':') )
            { i=sup_arvo(muuttuja,y); return(i); }
        if (lag)
            {
            i=lag_arvo(muuttuja,y); lag=0; if (i<0) l_virhe=1;
            return(i);
            }
        i=spfind(muuttuja);
        if (i<0)
            {
            i=varfind2(&d,muuttuja,0);  /* itse outputmuuttuja */
            if (i<0)
                {
                sprintf(sbuf,"\nValue of %s not found!",muuttuja);
                sur_print(sbuf); WAIT;
                poista_uudet_muuttujat();  /* 18.3.92 */
                exit(1);
                }
            data_load(&d,jnro,i,y);
            return(1);
            }

        if (spb[i]==NULL) { *y=arvo[i]; return(1); }
        if (nvar) { pvar=spa[i]; spa[i]=NULL; }
        k=laske_var(spb[i],y);
        if (nvar) spa[i]=pvar;
        arvo[i]=*y;
        spb[i]=NULL;
        return(1);
        }

double luku_var(char *sana,int len)
        {
        char *p;
        double tulos=1.0;
        int i;

        sana[len]=EOS;
        p=sana; if (*p=='-') ++p;
        if (strchr("1234567890.",*p)==NULL)
            {
            i=laske2_var(p,&tulos); if (i<0) return((double)1.0);
            if (*sana=='-') return(-tulos);
            return(tulos);
            }
        return(atof(sana));
        }

/* tarvittavat declarationit laske_var:ille */
int pos_funktio();
int varif_var();
int arifor_var();

int laske_var(char *lauseke, double *y)
        {
/*
        double luku();
        double oper();
        double funktio();
        double mfunktio();
*/
        char x[LLENGTH];
        char *p,*q;
        char sana[32];
        int len;
        double opnd[MAXARG+4]; char op[MAXARG+4]; int v[MAXARG+4];
        int t,n;
        int narg; /* Usean muuttujan funktion argumenttien lkm     */
        int i;
        double dlag;

int mat_element;
int n_mat_par;

/*// printf("\nlaske %s",lauseke); getch();  */
        *sana=EOS;  /* 17.2.2004 ????  */


        if (*lauseke=='i')
            {
            if (strncmp(lauseke,"if(",3)==0)
                return(varif_var(lauseke,y));
            }
        if (*lauseke=='f')
            {
            if (strncmp(lauseke,"for(",3)==0)
                return(arifor_var(lauseke,y));
            }

        strcpy(x,lauseke);
        len=0;
        p=x;
        t=0;
        lag=0;

        while (*p)
            {
            if (l_virhe) return(-1);
            switch (*p)
                {
              case '+':
                if (len==0) { ++p; break; }
                if (len>0) opnd[t]=luku_var(sana,len); len=0;
                op[t]='+'; v[t++]=1;
                supista(&t,opnd,op,v);
                ++p;
                break;

              case '-':
                if (len==0) { sana[len++]=*p; ++p; break; }
                if (len>0) opnd[t]=luku_var(sana,len); len=0;
                op[t]='-'; v[t++]=1;
                supista(&t,opnd,op,v);
                ++p;
                break;

              case '*':
                if (len==0) { syntax_error(lauseke); return(-1); }
                if (len>0) opnd[t]=luku_var(sana,len); len=0;
                op[t]='*'; v[t++]=2;
                supista(&t,opnd,op,v);
                ++p;
                break;

              case '/':
                if (len==0) { syntax_error(lauseke); return(-1); }
                if (len>0) opnd[t]=luku_var(sana,len); len=0;
                op[t]='/'; v[t++]=2;
                supista(&t,opnd,op,v);
                ++p;
                break;

              case '^':
                if (len==0) { syntax_error(lauseke); return(-1); }
                if (len>0) opnd[t]=luku_var(sana,len); len=0;
                op[t]='^'; v[t++]=3;
                supista(&t,opnd,op,v);
                ++p;
                break;

              case '[':
                q=strchr(p,']');
                if (len==0 || q==NULL) { syntax_error(lauseke); return(-1); }
                *q=EOS; /* lag=atoi(p+1); */
         /*     sdata2=sdata;    */
                laske_var(p+1,&dlag); lag=(int)dlag;
         /*     sdata=sdata2;    */
                p=q+1;
                break;

              case '(':
/* printf("\nsana=%s|",sana); getch(); // ++++  */

                mat_element=0;
        if (strncmp(sana,"MAT_",4)==0) { mat_element=1; n_mat_par=0; }




                if (*sana=='p')                      /* 9.9.89 */
                    {
                    if (strncmp(sana,"pos",3)==0)
                        {
                        p+=pos_funktio(p,&opnd[t]);
                        if (l_virhe) return(-1);
                        len=-1;
                        break;
                        }
                    }
/* printf("\np=%s|",p); getch(); // ++++  */
                q=p+1;
            if (*q==')') { sprintf(sbuf,"\nArguments missing in %s",lauseke);
                               sur_print(sbuf); l_virhe=1; return(-1); }
                n=1;
                narg=1;
                while (n)
                    {
                    ++p;
                    if (*p=='(') { ++n; continue; }
                    if (*p==')') { --n; continue; }
                if (*p==EOS) { sprintf(sbuf,"\n) is missing in %s",lauseke);
                                   sur_print(sbuf); l_virhe=1; return(-1); }
                    if (*p==',' && n==1)
                        {
                        *p=EOS;

                        if (mat_element) str_opnd[n_mat_par++]=q;
                        else laske_var(q,&opnd[t]);
                        ++t;
                        if (t>MAXARG+3)
                         { sprintf(sbuf,"\nToo many arguments in %s",lauseke);
                              sur_print(sbuf); l_virhe=1; return(-1); }
                        ++narg;
                        q=p+1;
                        }

                    }
                if(strchr("+-*/^)\0",*(p+1))==NULL) { syntax_error(lauseke);
                                                      return(-1); }
                *p=EOS; ++p;

                if (mat_element) str_opnd[n_mat_par++]=q;
                else              { i=laske_var(q,&opnd[t]);
                                    if (i<0 || l_virhe) return(-1);
                                  }


/*              i=laske_var(q,&opnd[t]);
//              if (i<0 || l_virhe) return(-1);
*/

                if (len==0) { len=-1; break; }
                sana[len]=EOS;

                if (narg>1)
                    {
/*
             printf("\nArgumentit: ");
             for (i=t-narg+1; i<=t; ++i) printf(" %g",opnd[i]); getch();
*/
                    t=t-narg+1;
                    if (*sana=='-')
                        opnd[t]=-mfunktio_var(sana+1,opnd+t,narg);
                    else
                        opnd[t]=mfunktio_var(sana,opnd+t,narg);
                    if (l_virhe) return(-1);
                    len=-1;
                    break;
                    }

                /* Yhden muuttujan funktiot */
                if (*sana=='-')
                    opnd[t]=-funktio_var(sana+1,opnd[t]);
                else
                    opnd[t]=funktio_var(sana,opnd[t]);
                if (l_virhe) return(-1);
                len=-1;
                break;

              case ')':
                sprintf(sbuf,"\n( missing in %s",lauseke);
                sur_print(sbuf); l_virhe=1; return(-1);

/*            case ':':
                sdata=atoi(sana+1);
                if (!sdata) { sana[len++]=*p; ++p; break; }
                len=0; ++p;
                break;
*/
              case 'e': case 'E':
                if (*sana==EOS) { sana[len++]=*p; ++p; continue; }
                                                         /* 28.2.2004 */
                if (strchr("+-.0123456789",sana[0])!=NULL)
                    {
                    sana[len++]=*p; ++p;
                    if (*p!='+' && *p!='-') break;
                    }
              /* default seurattava suoraan case 'e':n jälkeen */
              default:
                /* tarkistukset puuttuvat */
                sana[len++]=*p;
                ++p;
                }
            }

        if (len<0) { v[t++]=0; }
        else
                   if (len>0) { opnd[t]=luku_var(sana,len); v[t++]=0; }

        supista(&t,opnd,op,v);
        *y=opnd[0];
/*   printf("\n%s=%g",lauseke,*y);  // ++++  */
        return(1);
        }

/* Declarations */
int strvert(char *a,char rel,char *b,char *c,char *dd,double *y);

int varif_var(char *lauseke,double *y)
        {
        char *a,*b,*c,*d;
        char rel;
        char *p;
        int sulut;
        char x[LLENGTH];
        double y1;
        int tosi;

/*      printf("\nvarif: %s",lauseke); getch();     */
        /* if(<a><rel><b>)then(<c>)else(<d>)
           <a>,<b>,<c>,<d> lausekkeita
           <rel>: =,>,<,<>,>=,<=
                  = > < E  S  P
        */

        strcpy(x,lauseke);
        a=x+3;  /* if( ohitetaan */
        p=a; sulut=0;
        while (*p)
            {
            switch(*p)
                {
              case '=':
                rel=*p; *p=EOS; break;
              case '<':

                if (*(p+1)=='=') { rel='P'; *p=EOS; ++p; *p=EOS; break; }
                if (*(p+1)=='>') { rel='E'; *p=EOS; ++p; *p=EOS; break; }
                rel=*p; *p=EOS; break;
              case '>':
                if (*(p+1)=='=') { rel='S'; *p=EOS; ++p; *p=EOS; break; }
                rel=*p; *p=EOS; break;
              case ')':
                --sulut; ++p;
                if (sulut<0)
                    {
                    sprintf(sbuf,"\nrelation symbol =<> missing! in %s",x);
                    sur_print(sbuf); WAIT; l_virhe=1; return(-1);
                    }
                break;
              case '(':
                ++sulut; ++p;
                break;
              default:
                ++p;
                }
            }

/*  printf("\na=%s rel=%c",a,rel);  */
        b=p+1;
        p=b;
        while (1)
            {
            p=strchr(p,')');
            if (p==NULL) { if_syntax_error(lauseke); return(-1); }
            if (strncmp(p,")then(",6)==0) { *p=EOS; break; }
            ++p;
            }
/*  printf(" b=%s",b);   getch();  */
        c=p+6;
        p=c; sulut=0;
        while (*p)
            {
            if (*p=='(') { ++sulut; ++p; continue; }
            if (*p==')')
                {
                if (!sulut) break;
                --sulut;
                }
            ++p;
            }
        if (*p==EOS) { if_syntax_error(lauseke); return(-1); }
        *p=EOS;
        if (strncmp(p+1,"else(",5)!=0) { if_syntax_error(lauseke); return(-1); }
        d=p+6;
        p=d; sulut=0;
        while (*p)
            {
            if (*p=='(') { ++sulut; ++p; continue; }
            if (*p==')')
                {
                if (!sulut) break;
                --sulut;
                }
            ++p;
            }
        if (*p==EOS) { if_syntax_error(lauseke); return(-1); }
        *p=EOS;
/* printf(" c=%s d=%s",c,d);
getch();
*/
        if (strncmp(a,"str(",4)==0)             /* 13.3.1991 */
            {
            tosi=strvert(a,rel,b,c,d,y);
            if (tosi<0) return(-1);
            }

        else
            {
            laske_var(a,y);
            laske_var(b,&y1);
            tosi=0;
            switch (rel)
                {
              case '=': if (*y==y1) tosi=1; break;
              case '<': if (*y<y1) tosi=1; break;
              case '>': if (*y>y1) tosi=1; break;
              case 'E': if (*y!=y1) tosi=1; break;
              case 'P': if (*y<=y1) tosi=1; break;
              case 'S': if (*y>=y1) tosi=1; break;
                }
            }

        if (tosi) laske_var(c,y);
        else      laske_var(d,y);
        return(1);
        }


int arifor_var(char *lauseke,double *y)
        {
/*        int i,  */
        int g;
        char *sana[4],*laus[4];
        char x[LLENGTH];
        long ialku,iloppu,il,imax;
        double d;
        char *p;
        char sterm[LLENGTH];
        double term,sum;
        int iterm,iind,tind;
        char esana[7];
        int max;

        extern int n_earg;

        strcpy(x,lauseke);
        g=parsplit(x,sana,laus,4);
        if (g<0) return(-1);
        if (g<3) { if_syntax_error(lauseke); return(-1); }
/*
   for (i=0; i<g; ++i) printf("\nfor: %d %s %s",i,sana[i],laus[i]); getch();
*/
        p=strchr(laus[0],'=');
        if (p==NULL) { if_syntax_error(lauseke); return(-1); }
        *p=EOS;   /* laus[0]='i'  */
        laske_var(p+1,&d); ialku=d;
        laske_var(laus[1],&d); iloppu=d;

        iterm=0;
        if (strcmp(sana[2],"term")==0)
            {
            iterm=1;
            p=strchr(laus[2],'=');
            if (p==NULL) term=0.0;
            else { *p=EOS; laske_var(p+1,&term); }  /* laus[2]='term' */
            }
        strcpy(sterm,laus[2+iterm]);
        iind=aseta_earg((double)ialku,esana);
        if (iind<0) return(-1);
        korvaa_var(sterm,laus[0],esana);
        if (iterm)
            {
            tind=aseta_earg(term,esana);
            if (iind<0) return(-1);
            korvaa_var(sterm,laus[2],esana);
            }

        max=0; p=sana[2+iterm];
        if (strncmp(p,"max",3)==0 || strncmp(p,"min",3)==0)
            {
            if (p[2]=='n') { max=3; sum=1e300; if (p[3]=='i') max=4; }
            else           { max=1; sum=-1e300; if (p[3]=='i') max=2; }
            for (il=ialku; il<=iloppu; ++il)
                {
                earg[iind]=(double)il;
              if (iterm) { earg[tind]=term; if (il>ialku) laske_var(sterm,&term); }
                else laske_var(sterm,&term);
                if (max<3 && term>sum) { sum=term; imax=il; }
                if (max>2 && term<sum) { sum=term; imax=il; }  /* imax=imin */
                }
            }
        else if (strcmp(p,"sum")==0)
            {
            sum=0.0;
            for (il=ialku; il<=iloppu; ++il)
                {
                earg[iind]=(double)il;
                if (iterm) { earg[tind]=term;
                               if (il>ialku) laske_var(sterm,&term); }
                else laske_var(sterm,&term);
                sum+=term;
                }
            }
        else if (strcmp(p,"product")==0)
            {
            sum=1.0;
            for (il=ialku; il<=iloppu; ++il)
                {
                earg[iind]=(double)il;
                if (iterm) { earg[tind]=term;
                               if (il>ialku) laske_var(sterm,&term); }
                else laske_var(sterm,&term);
                sum*=term;
                }
            }
        else if (strcmp(p,"term")==0)
            {
            for (il=ialku; il<=iloppu; ++il)
                {
                earg[iind]=(double)il;
              if (iterm) { earg[tind]=term; if (il>ialku) laske_var(sterm,&term); }
                else laske_var(sterm,&term);
                }
            sum=term;
            }
        else { if_syntax_error(lauseke); return(-1); }

        if (max==2) *y=(double)imax; else *y=sum;
        n_earg-=1+iterm;
        return(1);
        }

void not_string(char *s)
        {
        sprintf(sbuf,"\n%s is not a string variable!",s);
        sur_print(sbuf); l_virhe=1; WAIT;
        }

int sulku_split(char *x, char **osa, int n, int *pk)
        {
        char *p;
        int sulut;
        int h;

        p=x+1;
        osa[0]=p;
        h=1; *pk=1;
        sulut=0;
        while (1)
            {
            ++p; ++*pk;
            if (*p=='(') { ++sulut; continue; }
            if (*p==')')
                {
                --sulut; if (sulut<0) { *p=EOS; return(h); }
                }
            if (*p==',')
                {
                if (!sulut)
                    {
                    *p=EOS;
                    osa[h]=p+1;
                    ++h;
                    if (h>n) return(n);
                    }
                }
            }
        }


int pos_funktio(char *s,double *y)      /* pos(var,char) tai pos(var,start_pos,char) */
        {
/*        char *p, */
        char *q;
        int var;
        char arvo[LLENGTH];
        double a;
        int start_pos;
        char x[LLENGTH],*osa[3];
        char x2[LLENGTH];
        int i,k,h;
/*        char ch; */

        strcpy(x,s);
        i=sulku_split(x,osa,3,&k);
        var=varfind(&d,osa[0]);
        if (var<0) return(-1);
        if (d.vartype[var][0]!='S')  { not_string(osa[0]); return(-1); }
        data_alpha_load(&d,jnro,var,arvo);
        h=d.varlen[var];
        arvo[h]=' '; arvo[h+1]=EOS;
        start_pos=1;
        if (i==3)
            {
            strcpy(x2,osa[1]);
            laske_var(x2,&a);
            start_pos=a;
            }

        strcpy(x2,osa[i-1]);
        if (sur_strcmpi(x2,"sp")==0 || sur_strcmpi(x2,"space")==0) { *x2=' '; x2[1]=EOS; }
        if (sur_strcmpi(x2,"comma")==0) { *x2=','; x2[1]=EOS; }

        q=strstr(arvo+start_pos-1,x2);
        if (q==NULL) *y=0.0;
        else *y=(double)(q-arvo+1);

        return(k+1);
        }


int tutki_str_lauseke(char *x,int *pvar,int *plag,int *pstart,int *plen,int *pk)
        {
        int i; /* ,k; */
        char *osa[3];
        char x2[LLENGTH];
        double a;
        char *p,*q;

        i=sulku_split(x,osa,3,pk);
        *plag=0;
        p=strchr(osa[0],'[');
        if (p!=NULL)
            {
            *p=EOS; q=strchr(p+1,']');
            if (q==NULL)
                {
                sur_print("\n] missing!");
                WAIT; return(-1);
                }
            *q=EOS; *plag=atoi(p+1);
            }
        *pvar=varfind(&d,osa[0]);
        if (*pvar<0) return(-1);
        if (d.vartype[*pvar][0]!='S')  { not_string(osa[0]); return(-1); }
        *pstart=1;
        if (i>1)
            {
            strcpy(x2,osa[1]);
            laske_var(x2,&a);
            *pstart=(int)a;
            }
        *plen=d.varlen[*pvar]-*pstart+1;
        if (i==3)
            {
            strcpy(x2,osa[2]);
            laske_var(x2,&a);
            *plen=(int)a;
            if (*plen<0)
                {
                sur_print("\nNegative length in str()!");
                WAIT; return(-1);
                }
            }
        return(1);
        }


FILE *codes;

int load_codes(char *codefile,unsigned char *code)
        {
        int i;
        char x[LLENGTH];

        strcpy(x,codefile);
        if (strchr(x,':')==NULL && *x!='.')
            { strcpy(x,survo_path); strcpy(x,"SYS\\"); strcat(x,codefile); }

        codes=fopen(x,"rb");
        if (codes==NULL)
            {
            sprintf(sbuf,"\nCode conversion file %s not found!",x); sur_print(sbuf);
            WAIT; return(-1);
            }
        for (i=0; i<256; ++i) code[i]=(unsigned char)getc(codes);
        fclose(codes);
        return(1);
        }

void conv_var(unsigned char *sana)
        {
        int i;

        for (i=0; i<strlen((char *)sana); ++i) sana[i]=code[sana[i]];
        }

int str_arvo(char *a,char *s)
        {
        char x[LLENGTH];
        int i,k;
        char *p; /*,*q; */

        if (*a=='"')
            {
            strcpy(s,a+1);
            p=strchr(s,'"');
            if (p!=NULL) *p=EOS;
            }
        else
            {
            if (sur_strnicmp(a,"str(",4)!=0) /* 4.3.1996 */
                {
                not_string(a);
                return(-1);
                }
            strcpy(x,a+3);
            i=tutki_str_lauseke(x,&str_var,&str_lag,&str_var_start,&str_var_len,&k);
            if (i<0) return(-1);
            data_alpha_load(&d,jnro+(long)str_lag,str_var,x);
            strncpy(s,x+str_var_start-1,(unsigned int)str_var_len); s[str_var_len]=EOS;
            }
        if (code_ind==2) conv_var((unsigned char *)s);
        return(1);
        }

int strvert(char *a,char rel,char *b,char *c,char *dd,double *y)
        {
        int i,tosi;
        char s1[LLENGTH],s2[LLENGTH];
        int v;

        if (!code_ind)
            {
            code_ind=1;
            for (i=0; i<256; ++i) code[i]=(unsigned char)i;
            i=spfind("FILTER");
            if (i>=0) { code_ind=2; i=load_codes(spb[i],code); if (i<0) return(-1); }
            }
        i=str_arvo(a,s1); if (i<0) return(-1);
        i=str_arvo(b,s2); if (i<0) return(-1);

        tosi=0;
        v=strcmp(s1,s2);
        switch (rel)
            {
          case '=': if (!v) tosi=1; break;
          case '<': if (v<0) tosi=1; break;
          case '>': if (v>0) tosi=1; break;
          case 'E': if (v) tosi=1; break;
          case 'P': if (v<=0) tosi=1; break;
          case 'S': if (v>=0) tosi=1; break;
            }
        return(tosi);
        }

int str_laske(char *lauseke)
        {
        char x[LLENGTH];
        char tulos[LLENGTH];
        char sana[LLENGTH];
        char sana2[LLENGTH];
        char *p,*q;
        int i,k;
        int var1,lag,start,len;

        strcpy(x,lauseke);
        p=x;
        *tulos=EOS;
        strcpy(sana,str_vasen);
        i=tutki_str_lauseke(sana,&str_var,&str_lag,&str_var_start,&str_var_len,&k);
        var[0]=str_var; /* lausekkeen merkintää varten! */
        if (i<0) return(-1);
        while (*p)
            {
            if (*p=='"')
                {
                q=p+1; while (*q && *q!='"') ++q;
                *q=EOS; strcpy(sana,p+1);
                p=q+1;
                }
            else if (sur_strnicmp(p,"str(",4)==0)
                {
                i=tutki_str_lauseke(p+3,&var1,&lag,&start,&len,&k);
                if (i<0) return(-1);
                p+=k+4;
                data_alpha_load(&d,jnro+(long)lag,var1,sana2);
                strncpy(sana,sana2+start-1,(unsigned int)len); sana[len]=EOS;
                }
            else if (sur_strnicmp(p,"comma",5)==0) /* 13.3.1991 */
                {
                strcpy(sana,",");
                p+=5;
                }
            else if (sur_strnicmp(p,"space",5)==0) /* 15.3.1991 */
                {
                strcpy(sana," ");
                p+=5;
                }
            else if (sur_strnicmp(p,"sp",2)==0)
                {
                strcpy(sana," ");
                p+=2;
                }
            else
                {
                sprintf(sbuf,"\nError in %s",lauseke);
                sur_print(sbuf); WAIT; return(-1);
                }
            if (*p=='|')  /* 22.1.1996 */
                {
                ++p;
                i=strlen(sana)-1; while (i>=0 && sana[i]==' ') sana[i--]=EOS;
                strcat(tulos,sana);
                continue;
                }
            strcat(tulos,sana);
            if (*p=='&') { ++p; continue; }
            if (*p==EOS) break;
            sprintf(sbuf,"\n& or \" missing in %s",lauseke);
            sur_print(sbuf); WAIT; return(-1);
            }
        data_alpha_load(&d,jnro+(long)str_lag,str_var,sana);
        for (i=0; i<str_var_len && i<strlen(tulos); ++i)
            sana[i+str_var_start-1]=tulos[i];
            if (d.type==2) fi_alpha_save(&d.d2,jnro+(long)str_lag,str_var,sana);
            else if (d.type==1) ma_save(&d.d1,(int)(jnro+str_lag),str_var,sana);
            else
                {
                sur_print("\nCannot save data values!");
                WAIT; return(-1);
                }
        return(1);
        }



int str_muuttuja(char *s)
        {
/*        int i,k;
        char x[LLENGTH],*osa[3]; */

        strcpy(str_vasen,s+3);
/*
        strcpy(x,s+3);
        i=tutki_str_lauseke(x,&str_var,&str_var_start,&str_var_len,&k);
*/
/*
printf("\nstr_var=%d start=%d len=%d k=%d",str_var,str_var_start,str_var_len,k);
getch();
*/
        return(1);
        }



int muunto0()
        {
        int i,prind;  /* k */
        double y,s1,s2;

        nxx=nx2=0L; s1=s2=0.0;
        xx=(double *)malloc((d.l2-d.l1+1L)*sizeof(double));
        oxx=(long *)malloc((d.l2-d.l1+1L)*sizeof(double));
        if (xx==NULL || oxx==NULL)
            {
            sprintf(sbuf,"\nNot enough memory for %lu observations!",d.l2-d.l1+1L);
            sur_print(sbuf); WAIT; return(-1);
            }
        sur_print("\nReading original data values...");
        for (i=0; i<spn; ++i) spb2[i]=spb[i];
        prind=0;  /* 1; Vaihdettu piirto oletusarvoisesti pois päältä */
/*        i=hae_apu("prind",sbuf); if (i) prind=atoi(sbuf);  */
        i=spfind("PRIND"); if (i>=0) prind=atoi(spb[i]);

        for (jnro=d.l1; jnro<=d.l2; ++jnro)
            {
            if (unsuitable(&d,jnro)) continue;
            if (prind) { sprintf(sbuf," %ld",jnro); sur_print(sbuf); }
            for (i=0; i<spn; ++i) spb[i]=spb2[i];
            i=lue_arvot(jnro); if (i<0) return(-1);

            laske_var(lauseke,&y);
            if (l_virhe) { var_error(lauseke); return(-1); }
            oxx[nxx]=jnro; xx[nxx++]=y;
            if (y!=MISSING8)
                {
                s1+=y; s2+=y*y; ++nx2;
                }

/*            if (sur_kbhit()) { prind=1-prind; getch(); }  12.7.2007  */
            }
        if (nx2<2) { sur_print("\nNot enough observations!"); WAIT; return(-1); }
        mean=s1/nx2; stddev=sqrt((s2-s1*s1/nx2)/(nx2-1.0));
        sum=s1;  /* 11.10.1996 */
        if ((muunnos==9 || muunnos==10) && sum==0.0)
            {
            sur_print("\nSum of data values is 0!. Cannot continue!");
            WAIT; exit(1);
            }
        return(1);
        }

int sort_data(int muunnos)
        {
        long i;
        unsigned long h,k,g;
        char ind;
        double y;
        double a;

        h=nxx;
        while (h>1L)
            {
            h/=2;
            while (1)
                {
                ind='1';
                for (k=0L; k<nxx-h; ++k)
                    {
                    if (xx[k]>xx[k+h])
                        {
                        y=xx[k]; xx[k]=xx[k+h]; xx[k+h]=y;
                        i=oxx[k]; oxx[k]=oxx[k+h]; oxx[k+h]=i;
                        ind='0';
                        }
                    }
                if (ind=='1') break;
                }
            }

        if (muunnos==5 || muunnos==6) return(1);
        k=0L;
        while (k<nxx)
            {
            y=xx[k]; h=1L;
            while (k+h<nxx && y==xx[k+h]) ++h;
            --h;
            if (muunnos==4) a=(double)(k+1);
            else a=(double)(k+1+h/2.0);
            for (g=k; g<=k+h; ++g) xx[g]=a;
            k+=h+1L;
            }
        return(1);
        }

/* 1=#RANK 2=#NORMAL 3=#STD 4=#NRANK 5=#TRUNCP 6=#WINSP 7=#TRUNCL 8=#WINSL */
/* 9=#PROPORTION 10=#PERCENT */
int muunto2(int muunnos)
        {
        int i;
        long jxx,l;
/*        extern double inv_std(); */
/*        double a; */

        if (muunnos==1 || muunnos==2 || muunnos==4 || muunnos==5 || muunnos==6)
            {
            sort_data(muunnos);
            }
        if (muunnos==1 || muunnos==4) return(1);
        if (muunnos==2 || muunnos==3)
            {
            if (stddev<1e-15)
                { sprintf(sbuf,"\nVariance=0. Cannot continue!"); sur_print(sbuf); return(-1); }
            }
        if (muunnos==3)
            {
            for (jxx=0L; jxx<nxx; ++jxx)
                if (xx[jxx]!=MISSING8) xx[jxx]=(xx[jxx]-mean)/stddev;
            return(1);
            }
        if (muunnos==2)
            {
            for (jxx=0L; jxx<nxx; ++jxx)
/* inv_std korvattu R:n qnorm -funktiolla
                if (xx[jxx]!=MISSING8) xx[jxx]=mean+stddev*inv_std((xx[jxx]-0.5)/nx2);
*/
                if (xx[jxx]!=MISSING8) xx[jxx]=mean+stddev*qnorm(((xx[jxx]-0.5)/nx2),0,1,1,0);
            return(1);
            }
        if (muunnos==5 || muunnos==6)
            {
            level=0.95;
            i=spfind("P"); if (i>=0) level=atof(spb[i]);
            l=0.5*(1.0-level)*nx2;
            if (l>nx2) l=nx2;
            }
        if (muunnos==5)
            {
            for (jxx=0L; jxx<l; ++jxx) xx[jxx]=xx[nx2-1-jxx]=MISSING8;
            return(1);
            }
        if (muunnos==6)
            {
            for (jxx=0L; jxx<l; ++jxx) { xx[jxx]=xx[l]; xx[nx2-1-jxx]=xx[nx2-1-l]; }
            return(1);
            }
        if (muunnos==7 || muunnos==8)
            {
            level=1.96;
            i=spfind("LEVEL"); if (i>=0) level=atof(spb[i]);
            }
        if (muunnos==7)
            {
            for (jxx=0L; jxx<nxx; ++jxx)
                if (xx[jxx]==MISSING8 || fabs(xx[jxx]-mean)>level*stddev) xx[jxx]=MISSING8;
            return(1);
            }
        if (muunnos==8)
            {
            for (jxx=0L; jxx<nxx; ++jxx)
                if (xx[jxx]!=MISSING8)
                    {
                    if (xx[jxx]-mean>level*stddev)
                        xx[jxx]=mean+level*stddev;
                    else if (xx[jxx]-mean<-level*stddev)
                        xx[jxx]=mean-level*stddev;
                    }
            return(1);
            }
        if (muunnos==9)
            {
            for (jxx=0L; jxx<nxx; ++jxx)
                if (xx[jxx]!=MISSING8) xx[jxx]/=sum;
            return(1);
            }
        if (muunnos==10)
            {
            for (jxx=0L; jxx<nxx; ++jxx)
                if (xx[jxx]!=MISSING8) xx[jxx]*=100.0/sum;
            return(1);
            }
        if (muunnos==11)
            {
            for (jxx=0L; jxx<nxx; ++jxx)
                if (xx[jxx]!=MISSING8) xx[jxx]-=mean;
            return(1);
            }
        return(1);
        }


int muunto()
        {
        int i,k,prind;
        double y;

/*        sur_print("\n");  */
        for (i=0; i<spn; ++i) spb2[i]=spb[i];
/*
for (i=0; i<spn; ++i) printf("\n%d %s=%s",i,spa[i],spb[i]); getch();
*/
        prind=0;
/*        i=hae_apu("prind",sbuf); if (i) prind=atoi(sbuf);  */
        i=spfind("PRIND"); if (i>=0) prind=atoi(spb[i]);

        for (jnro=d.l1; jnro<=d.l2; ++jnro)
            {
            if (unsuitable(&d,jnro))
                {
                if (first_new_var)    /* 29.3.92 */
                    {
                    if (nvar)
                        for (k=0; k<nvar; ++k)
                            {
                            if (var[k]<first_new_var) continue;
                            data_save(&d,jnro,var[k],MISSING8);
                            }
                    else
                        {
                        if (var[0]>=first_new_var)
                            data_save(&d,jnro,var[0],MISSING8);
                        }
                    }
                continue;
                }
            if (prind) { sprintf(sbuf," %ld",jnro); sur_print(sbuf); }
            for (i=0; i<spn; ++i) spb[i]=spb2[i];
            i=lue_arvot(jnro); if (i<0) return(-1);
            if (!nvar)
                {
                if (str_muunnos)
                    {
                    i=str_laske(lauseke);
                    if (i<0) return(-1);
                    }
                else
                    {
                    laske_var(lauseke,&y);
                    if (l_virhe) { var_error(lauseke); return(-1); }
                    i=data_save(&d,jnro,var[0],y); if (i<0) return(-1);
                    }
                }
            else
                {
                for (k=0; k<nvar; ++k)
                    {
                    int h;
                    char *pvar;

                    h=spfind(word[k+1]);
                    if (h<0)
                        {
                        sprintf(sbuf,"\nValue of %s not found!",word[k+1]);
                        sur_print(sbuf); WAIT;
                        poista_uudet_muuttujat();
                        return(-1);
                        }
/* printf("\nspb[h]=%s|",spb[h]); getch(); // ++++  */
                    if (spb[h]==NULL) y=arvo[h];
                    else
                        {
                        pvar=spa[h]; spa[h]=NULL;
                        laske_var(spb[h],&y);
/* printf("\ny=%g|",y); getch(); // ++++   */
                        if (l_virhe) { var_error(spb[h]); return(-1); }
                        spb[h]=NULL; arvo[h]=y;
                        spa[h]=pvar;
                        if (l_virhe) return(-1);
                        }
                    i=data_save(&d,jnro,var[k],y); if (i<0) return(-1);
                    }
                }
/*            if (kbhit()) { prind=1-prind; getch(); } */
            }
        return(1);
        }


int talletus()
        {
        int i,prind;  /* k 
        double y; */
        long jxx;

        sur_print("\nSaving transformed values...");
/*      prind=1;  */
        prind=0;
        for (jxx=0L; jxx<nxx; ++jxx)
            {
            if (prind) { sprintf(sbuf," %ld",oxx[jxx]); sur_print(sbuf); }
            i=data_save(&d,oxx[jxx],var[0],xx[jxx]); if (i<0) return(-1);
/*            if (kbhit()) { prind=1-prind; getch(); }  */
            }
        return(1);
        }

#define EQ '\176'

void uusi_nimi(int i, char *s)
        {
        char x[LLENGTH];
        int k,len,h;

        len=d.d2.l; if (len<10) return;
        fi_rewind(&(d.d2));
fi_gets(&(d.d2),x,d.d2.l,(long)(d.d2.var+(long)i*((long)len+(long)d.d2.extra)+(long)d.d2.extra));
        x[len]=EOS;

        k=8; while (x[k]==' ' && k<len) ++k;
        if (k==len || x[k]==EQ)
            {
            if (x[k]==EQ) for (h=k; h<len; ++h) x[h]=' ';
            x[9]=EQ;
            h=0; while (h<strlen(s) && h+10<len) { x[h+10]=s[h]; ++h; }
            fi_rewind(&(d.d2));
fi_puts(&(d.d2),x,d.d2.l,(long)(d.d2.var+(long)i*((long)len+(long)d.d2.extra)+(long)d.d2.extra));
            }
        }


int uudet_nimet()
        {
        int i,k;

        if (d.type!=2) return(-1);
        if (!nvar)
            {
            uusi_nimi(var[0],lauseke);
            }
        else
            {
            for (k=0; k<nvar; ++k)
                {
                i=spfind(word[k+1]);
                if (i<0) return(-1);
                uusi_nimi(var[k],spb[i]);
                }
            }
        return(1);
        }

int muuttujat()
        {
        char *p;

        p=strchr(word[1],'=');
        if (p==NULL)
            {
            while (nvar<g-1 && sur_strcmpi(word[1+nvar],"TO")!=0)
                {
                var[nvar]=varhaku(word[1+nvar]);
                if (var[nvar]<0) return(-1);
                ++nvar;
                }
            return(1);
            }
        *p=EOS;

        if (sur_strnicmp(word[1],"str(",4)==0)
            {
            str_muunnos=1; str_muuttuja(word[1]);
            }
        else
            { var[0]=varhaku(word[1]); if (var[0]<0) return(-1); }
        strcpy(lauseke,p+1);
        nvar=0;
        return(1);
        }

/*
VAR <var>=#F(<expression>) TO <data>
0    1                     2  3
*/

void op_var2()
        {
        int i,k;
        char *p,*q;
        char nimi[LLENGTH];
/*        char x[LLENGTH], *pdat[NDATA];  */
        char lauseke2[LLENGTH];

        edread(comline,r1+r-1);
        p=strchr(comline,PREFIX); if (p==NULL) p=comline;
        g=splitp(p+1,word,MAXPARM);
        i=0;
        while (i<g && strcmp(word[i],"/")!=0) ++i;
        g=i;

        if (g<2 || strstr(word[1],"=#")==NULL)
            {
            sur_print("\nUsage:");
            sur_print("\nVAR <var>=#F(<expression>) TO <data>");
            sur_print("\nwhere #F is #RANK,#NORMAL or #STD");
            WAIT; return;
            }

        if (sur_strcmpi(word[g-2],"TO")==0) strcpy(nimi,word[g-1]);
        else  strcpy(nimi,active_data);

        i=data_open2(nimi,&d,1,0,0); if (i<0) return;

        p=strchr(word[1],'=');
        if (p==NULL) return; /* mahdotonta */
        *p=EOS; ++p; strcpy(lauseke2,p);
        var[0]=varhaku(word[1]); nvar=0;  /* muuttujat() poistettu */
        if (var[0]<0) return;
        q=strchr(p,'(');
        if (q==NULL)
            { sprintf(sbuf,"\n( missing in %s",p); sur_print(sbuf); WAIT; return; }
        *q=EOS;
        if (sur_strcmpi(p,"#RANK")==0) muunnos=1;
        else if (sur_strcmpi(p,"#NORMAL")==0) muunnos=2;
        else if (sur_strcmpi(p,"#STD")==0) muunnos=3;
        else if (sur_strcmpi(p,"#NRANK")==0) muunnos=4;
        else if (sur_strcmpi(p,"#TRUNCP")==0) muunnos=5;
        else if (sur_strcmpi(p,"#WINSP")==0) muunnos=6;
        else if (sur_strcmpi(p,"#TRUNCL")==0) muunnos=7;
        else if (sur_strcmpi(p,"#WINSL")==0) muunnos=8;
        else if (sur_strcmpi(p,"#PROPORTION")==0) muunnos=9; /* 11.10.1996 */
        else if (sur_strcmpi(p,"#PERCENT")==0) muunnos=10;
        else if (sur_strcmpi(p,"#CENTER")==0) muunnos=11;
        else
            { sprintf(sbuf,"\nUnknown transformation %s",p); sur_print(sbuf); WAIT; return; }
        ++q;
        i=strlen(q);
        if (q[i-1]!=')')
            { sur_print("\n( missing!"); WAIT; return; }
        q[i-1]=EOS;
        strcpy(lauseke,q);
        mask(&d);
        poista_var();
        vm_act=d.m_act;
        i=sp_init_var(r1+r-1,d.m_act); if (i<0) { spec_error(); return; }
        i=sp2_init(); if (i<0) { spec_error(); return; } /* MISSING,ORDER,N */
/*
printf("\nspec:");
for (i=0; i<spn; ++i) printf("\n%s",spa[i]); getch();
*/
        i=conditions(&d); if (i<0) return;

        ndata=0;
        i=spfind("INDATA");
        if (i>=0)
            {
            strcpy(sdat_list,spb[i]);
            ndata=split(sdat_list,sdat,NDATA);
            for (i=0; i<ndata; ++i)
                {
                k=data_open(sdat[i],&sd[i]); if (k<0) return;
                }
            }

        i=muunto0(); if (i<0) return;
        i=muunto2(muunnos); if (i<0) return;
        for (i=0; i<spn; ++i) spb[i]=spb2[i];
        strcpy(lauseke,lauseke2); uudet_nimet();
        talletus();
        data_close(&d);
        for (i=0; i<ndata; ++i) data_close(&sd[i]);
        }


/*
VAR <var>=<expression> TO <data>
0    1                 2   3
*/

/* int main(int argc, char *argv[]) */
int muste_var(char *argv)
        {
        int i,k;
        char *p;
        char nimi[LLENGTH];
        char x[LLENGTH]; /*, *pdat[NDATA]; */

/*        if (argc==1) return(1); */
        s_init(argv);
/*
        s_init(argv[1]);
*/

        edread(comline,r1+r-1);
        p=strchr(comline,PREFIX); if (p==NULL) p=comline;
        g=splitp(p+1,word,MAXPARM);
        i=0;
        while (i<g && strcmp(word[i],"/")!=0) ++i;
        g=i;

        if (g<2)
            {
            sur_print("\nUsage:");
            sur_print("\nVAR <var>=<expression> TO <data>");
            sur_print("\nor");
            sur_print("\nVAR <list_of_variables> TO <data>");
            WAIT; return(1);
            }

        if (strstr(word[1],"=#")!=NULL)
            {
            op_var2(); s_end(argv);  /*[1]); */
            return(1);
            }

        if (sur_strcmpi(word[g-2],"TO")==0) strcpy(nimi,word[g-1]);
        else  strcpy(nimi,active_data);
        subst_survo_path(nimi); /* 20.10.2001 */

        i=data_open2(nimi,&d,1,0,0); if (i<0) { s_end(argv); /*[1]);*/ return(1); }

        i=muuttujat(); if (i<0) { s_end(argv); /*[1]);*/ return(1); }
        mask(&d);
        poista_var();
        vm_act=d.m_act;

        for (i=0; i<d.m_act; ++i)
            {
            strcpy(x,d.varname[d.v[i]]);
            if (strncmp(x,"IND ",4)==0 || strncmp(x,"CASES ",6)==0 || strncmp(x,"SELECT ",7)==0)
                {
                sur_print("\nField names IND, CASES and SELECT are not allowed!");
                sur_print("\nChange such a name by FILE STATUS and FILE UPDATE.");
                WAIT; return(1);
                }
            }

        i=sp_init_var(r1+r-1,d.m_act); if (i<0) { spec_error(); return(1); }
        i=sp2_init(); if (i<0) { spec_error(); return(1); } /* MISSING,ORDER,N */

/*
printf("\nspec:");
for (i=0; i<spn; ++i) printf("\n%s",spa[i]); getch();
*/
        i=conditions(&d); if (i<0) return(1);

        ndata=0;
        i=spfind("INDATA");
        if (i>=0)
            {
            strcpy(sdat_list,spb[i]);
            ndata=split(sdat_list,sdat,NDATA);
            for (i=0; i<ndata; ++i)
                {
/*              k=data_open(sdat[i],&sd[i]); if (k<0) return;              */
                k=data_read_open(sdat[i],&sd[i]); if (k<0) { s_end(argv); /*[1]);*/ return(1); }
                }
            }
        varaa_earg();   /* 4.11.1998 */
        muunto();
        for (i=0; i<spn; ++i) spb[i]=spb2[i];
        uudet_nimet();
        data_close(&d);
        for (i=0; i<ndata; ++i) data_close(&sd[i]);
        outseed(); /* sur_rand() */
/*********************
printf("\nspn=%d|",spn); getch();  // ++++
for (i=0; i<spn; ++i) printf("\n%s %g",spa[i],arvo[i]); getch();
**************************/
        s_end(argv); /* [1]); */
        return (1);
        }

