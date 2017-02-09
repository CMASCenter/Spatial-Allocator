/*
 * A C version of EDSS / Models-3 I/O API grid manipulation routines.
 * This allows the spatial tool to be developed in C only, with no FORTRAN
 * linking required.
 *
 * Developed by Atanas Trayanov of the MCNC Environmental Modeling Center
 * in support of EPA's Multimedia Integrated Modeling System, 2002.
 */
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "parms3.h"

typedef struct _RdVar
{
    void *ptr;
    int type;
} RdVar;

typedef enum
{ INTEGER, REAL, STRING } Type;

#define MAXLINE 256

typedef struct _CoordList
{
    double p_alp;               /*  first, second, third map */
    double p_bet;               /*  projection descriptive   */
    double p_gam;               /*  parameters               */
    double xcent;               /*  lon for coord-system X=0 */
    double ycent;               /*  lat for coord-system Y=0 */
    int ctype;                  /*  coord sys type           */
    char *name;
    struct _CoordList *next;
} CoordList;

typedef struct _GridList
{
    double xorig;
    double yorig;
    double xcell;
    double ycell;
    int ncols;
    int nrows;
    int nthik;
    char *cname;
    char *gname;
    struct _GridList *next;
} GridList;


/* ================================================== */
int advance2NextField(char **buf)
{
    char *p;
  scan_again:
    p = *buf;
    if(p[0] == '\0')
        return 0;
    if(isspace(p[0]))
    {
        (*buf)++;
        goto scan_again;
    }
    if(p[0] == ',')
    {
        (*buf)++;
        return 2;
    }
    return 1;
}

/* ================================================== */
int readVar(int n, RdVar * p, FILE * fp)
{
    char buffer[MAXLINE];
    int k;
    int m;
    int ignore_first_comma;
    char *t;
    char *endp;
    int r, *pi;
    double d, *pd;
    char c, *str;

    if(n <= 0)
        return 0;

    m = 0;
  rd_next_line:
    if(fgets(buffer, MAXLINE, fp) == NULL)
        return 0;
    ignore_first_comma = 0;
    t = buffer;

  rd_next_field:
    k = advance2NextField(&t);
    if(k == 0)
    {                           /* End of buffer */
        goto rd_next_line;
    }
    else if(k == 1)
    {                           /* valid entry */
        ignore_first_comma = 1;
        if(p[m].type == INTEGER)
        {
            r = strtol(t, &endp, 10);
            if(r == 0 && endp == t)
            {                   /* error on READ */
                return 0;
            }
            else
            {
                pi = (int *) (p[m].ptr);
                *pi = r;
                t = endp;
                goto store_input;
            }
        }
        else if(p[m].type == REAL)
        {
            endp = t;
            while(*endp != '\0' && !isspace(*endp) && *endp != ',')
            {
                if(toupper(*endp) == 'D')
                    *endp = 'E';
                endp++;
            }
            d = strtod(t, &endp);
            if(d == 0.0 && endp == t)
            {                   /* error on READ */
                return 0;
            }
            else
            {
                pd = (double *) (p[m].ptr);
                *pd = d;
                t = endp;
                goto store_input;
            }
        }
        else if(p[m].type == STRING)
        {
            str = p[m].ptr;
            if(*t == '\'' || *t == '"')
            {
                c = *t;
                endp = strchr(++t, c);
                if(endp != NULL)
                {
                    *endp = '\0';
                    strncpy(str, t, NAMLEN3);
                    str[NAMLEN3] = '\0';
                    t = endp + 1;
                    goto trim_trail;
                }
                else
                {
                    strncpy(str, t, NAMLEN3);
                    str[NAMLEN3] = '\0';
                    do
                    {
                        r = strlen(p[m].ptr);
                        if(fgets(buffer, MAXLINE, fp) == NULL)
                            return 0;
                        endp = strchr(buffer, c);
                        if(endp != NULL)
                        {
                            *endp = '\0';
                        }
                        if(r < NAMLEN3)
                        {
                            strncat(str, buffer, NAMLEN3 - r);
                            str[NAMLEN3] = '\0';
                        }
                    }
                    while(endp == NULL);
                    t = endp + 1;
                }
                /* remove trailing blanks */
              trim_trail:
                endp = p[m].ptr;
                r = strlen(endp) - 1;
                while(r >= 0)
                {
                    if(isspace(endp[r]))
                    {
                        endp[r--] = '\0';
                    }
                    else
                        break;
                }
                goto store_input;
            }
        }
    }
    else if(k == 2)
    {                           /* comma */
        if(ignore_first_comma)
        {
            ignore_first_comma = 0;
            goto rd_next_field;
        }
        else
            goto store_input;
    }

  store_input:
    m++;
    if(m < n)
        goto rd_next_field;
    return 1;

}

/* ================================================== */
void new_grid(GridList ** list, char *gname, char *cname,
              double xorig, double yorig, double xcell, double ycell,
              int ncols, int nrows, int nthik)
{
    GridList *p;
    GridList **t;

    p = (GridList *) malloc(sizeof(p[0]));
    if(p)
    {
        p->xorig = xorig;
        p->yorig = yorig;
        p->xcell = xcell;
        p->ycell = ycell;
        p->ncols = ncols;
        p->nrows = nrows;
        p->nthik = nthik;
        p->gname = (char *) strdup(gname);
        p->cname = (char *) strdup(cname);
        p->next = NULL;
        for(t = list; *t != NULL; t = &(*t)->next)
        {
        };
        *t = p;

    }
    else
    {
        WARN("Allocation error in new_coord");
    }
    return;
}

/* ================================================== */
void new_coord(CoordList ** list, char *name, int ctype,
               double p_alp, double p_bet, double p_gam,
               double xcent, double ycent)
{
    CoordList *p;
    CoordList **t;

    p = (CoordList *) malloc(sizeof(p[0]));
    if(p)
    {
        p->p_alp = p_alp;
        p->p_bet = p_bet;
        p->p_gam = p_gam;
        p->xcent = xcent;
        p->ycent = ycent;
        p->ctype = ctype;
        p->name = (char *) strdup(name);
        p->next = NULL;
        if(*list == NULL)
        {
            *list = p;
        }
        else
        {
            for(t = list; *t != NULL; t = &(*t)->next);
            *t = p;
        }

    }
    else
    {
        WARN("Allocation error in new_grid");
    }
    return;
}


/* ================================================== */
int dscgridc(const char *gname,
             char *cname,
             int *ctype,
             double *p_alp,
             double *p_bet,
             double *p_gam,
             double *xcent,
             double *ycent,
             double *xorig,
             double *yorig,
             double *xcell, double *ycell, int *ncols, int *nrows, int *nthik)
{
    static int firstime = 1;
    static CoordList *cl_head = NULL;
    static GridList *gl_head = NULL;

    char *fname;
    char *default_gname = "GRIDDESC";
    char string[NAMLEN3 + 1];
    char buffer[MAXLINE];
    FILE *fp;
    CoordList *c;
    GridList *g;
    int found;
    RdVar rv[10];
    int i;
    char mesg[256];


    /*  begin body of dscgridc() */

    if(firstime)
    {
        fname = getenv(default_gname);
        if(!fname)
            fname = default_gname;
        sprintf(mesg, "griddesc file name = %s\n", fname);
        MESG(mesg);
        if((fp = fopen(fname, "r")) == NULL)
        {
            WARN2("Cannot open GRIDDESC file ", fname);
            goto error;
        }

        /* get the first line and discard */
        if(fgets(buffer, MAXLINE, fp) == NULL)
            return 0;


      coord_section:
        i = 0;
        rv[i].ptr = cname;      /* name of map projection coordinate system */
        rv[i++].type = STRING;
        if(!readVar(i, rv, fp))
        {
            WARN("Read error for coordinate name");
            goto error;
        }

        if(cname[0] == '\0')
            goto grid_loop;
        i = 0;
        rv[i].ptr = ctype;
        rv[i++].type = INTEGER;
        rv[i].ptr = p_alp;
        rv[i++].type = REAL;
        rv[i].ptr = p_bet;
        rv[i++].type = REAL;
        rv[i].ptr = p_gam;
        rv[i++].type = REAL;
        rv[i].ptr = xcent;
        rv[i++].type = REAL;
        rv[i].ptr = ycent;
        rv[i++].type = REAL;
        if(!readVar(i, rv, fp))
        {
            WARN("Read error in coordinate section");
            goto error;
        }
        new_coord(&cl_head, cname, *ctype,
                  *p_alp, *p_bet, *p_gam, *xcent, *ycent);
        goto coord_section;

      grid_loop:
        i = 0;
        rv[i].ptr = string;
        rv[i++].type = STRING;
        if(!readVar(i, rv, fp))
        {
            WARN("Read error for grid name");
            goto error;
        }
        if(string[0] == '\0')
            goto endinit;
        i = 0;
        rv[i].ptr = cname;
        rv[i++].type = STRING;
        rv[i].ptr = xorig;
        rv[i++].type = REAL;
        rv[i].ptr = yorig;
        rv[i++].type = REAL;
        rv[i].ptr = xcell;
        rv[i++].type = REAL;
        rv[i].ptr = ycell;
        rv[i++].type = REAL;
        rv[i].ptr = ncols;
        rv[i++].type = INTEGER;
        rv[i].ptr = nrows;
        rv[i++].type = INTEGER;
        rv[i].ptr = nthik;
        rv[i++].type = INTEGER;
        if(!readVar(i, rv, fp))
        {
            WARN("Read error in grid section");
            goto error;
        }
        new_grid(&gl_head, string, cname,
                 *xorig, *yorig, *xcell, *ycell, *ncols, *nrows, *nthik);
        goto grid_loop;
      endinit:
        fclose(fp);
        firstime = 0;
    }

    found = 0;
    for(g = gl_head; g != NULL; g = g->next)
    {
        if(!strcmp(g->gname, gname))
        {
            for(c = cl_head; c != NULL; c = c->next)
            {
                if(!strcmp(g->cname, c->name))
                {
                    found = 1;
                    break;
                }
            }
            break;
        }
    }
    if(!found)
    {
        sprintf(buffer, "Could not find grid %s in GRIDDESC file %s", gname,
                fname);
        WARN(buffer);
        goto error;
    }
    strncpy(cname, g->cname, NAMLEN3);
    cname[NAMLEN3] = '\0';
    *ctype = c->ctype;
    *p_alp = c->p_alp;
    *p_bet = c->p_bet;
    *p_gam = c->p_gam;
    *xcent = c->xcent;
    *ycent = c->ycent;
    *xorig = g->xorig;
    *yorig = g->yorig;
    *xcell = g->xcell;
    *ycell = g->ycell;
    *ncols = g->ncols;
    *nrows = g->nrows;
    *nthik = g->nthik;
    return 1;
  error:
    strcpy(cname, "");
    *ctype = IMISS3;
    *p_alp = BADVAL3;
    *p_bet = BADVAL3;
    *p_gam = BADVAL3;
    *xcent = BADVAL3;
    *ycent = BADVAL3;
    *xorig = BADVAL3;
    *yorig = BADVAL3;
    *xcell = BADVAL3;
    *ycell = BADVAL3;
    *ncols = IMISS3;
    *nrows = IMISS3;
    *nthik = IMISS3;
    return 0;
}
