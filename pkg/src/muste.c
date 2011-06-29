#include <R.h>
#include <Rinternals.h>
#include <R_ext/Parse.h>
#include <stdio.h>
#include "survolib.h"

extern int s_init();
extern int s_end();
extern int op_arit();
extern int muste_corr();
extern int muste_var();
extern int muste_file_show();
extern int muste_editor();
extern int headline();


static char komento[256];


SEXP Muste_Editor(SEXP session)
{
    int dummy;
    
    
    muste_editor();
    return(session);
}

/*
int muste_checkstack(void)
{
   int dummy;
   intptr_t usage = R_CStackDir * (R_CStackStart - (uintptr_t)&dummy);
   if(usage > 0.95 * R_CStackLimit) return(-1);
   return(1);
} 
*/

SEXP Muste_EvalRExpr(char *cmd)
{
   ParseStatus status;
   SEXP cmdsexp, cmdexpr, ans = R_NilValue;
   int i;

   sprintf(komento,"if (inherits(try(.muste.ans<-%s,silent=TRUE), \"try-error\")) FALSE else TRUE",cmd);

// Rprintf("EvalR: %s\n",komento); // RS DEBUG

   PROTECT(cmdsexp = allocVector(STRSXP, 1));
   SET_STRING_ELT(cmdsexp, 0, mkChar(komento));
   cmdexpr = PROTECT(R_ParseVector(cmdsexp, -1, &status, R_NilValue));
   if (status != PARSE_OK) {
       UNPROTECT(2);
// RS REM      error("Invalid call %s",cmd);
       return (R_NilValue);
   }
   for(i=0; i<length(cmdexpr); i++) ans = eval(VECTOR_ELT(cmdexpr,i),R_GlobalEnv);
   UNPROTECT(2); 
   if (INTEGER(ans)[0]==FALSE) return (R_NilValue);
   ans = findVar(install(".muste.ans"),R_GlobalEnv);
   return ans;
}

int muste_evalr(char *cmd)
   {
   int retstat;
   SEXP status;
   retstat=1;
   status=Muste_EvalRExpr(cmd);
   if (status==R_NilValue) retstat=-1;
   return retstat;
   }

/*

SEXP Muste_EditorialArithmetics(SEXP session)
{
    s_init("A");
    op_arit();
    s_end("A");
    return(session);
}

SEXP Muste_CorrModule(SEXP session)
{
    muste_corr("A");
    return(session);
}

SEXP Muste_HeaderLine(SEXP session)
{
    headline();
    return(session);
}


SEXP Muste_VarOperation(SEXP session)
{
    muste_var("A");
    return(session);
}


SEXP Muste_FileShow(SEXP session)
{
    muste_file_show("A");
    return(session);
}


*/

//int Muste_GetKey(SEXP funktio, SEXP ymparisto)
//{
/* 
  const char funktio[] = "MusteGetKey\0";
   const char ymparisto[] = "muste.environment\0";

   SEXP a = allocVector(STRSXP, 2);
   PROTECT(a);
   SET_STRING_ELT(a, 0, mkChar(funktio));
   SET_STRING_ELT(a, 1, mkChar(ymparisto));
   UNPROTECT(1);
   return a;
*/   
//    return(INTEGER(eval(funktio,ymparisto))[0]);
//}

/*
SEXP Muste_WaitKoe(SEXP funktio, SEXP ymparisto)
{
    int i;
    i=Muste_GetKey(funktio,ymparisto);
    Rprintf("\n%d\n",i);
    return(funktio);
}
*/

/* .Call("Muste_SetCursorKoe",as.integer(10),as.integer(10)) 
SEXP Muste_SetCursorKoe(SEXP row, SEXP col)
{
    int i,j,k;
    i=INTEGER(row)[0];
    j=INTEGER(col)[0];
    k=sur_locate(i,j);
    return(row);
}

SEXP Muste_Write(SEXP row, SEXP col,SEXP sha)
{
    const char testi[] = "Kokeilu";
    int i,j,k;
    i=INTEGER(row)[0];
    j=INTEGER(col)[0];
    k=INTEGER(sha)[0];
    write_string(testi,7,k,i,j);
    return(row);
}

*/
