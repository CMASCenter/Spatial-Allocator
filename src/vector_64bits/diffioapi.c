#ifdef USE_IOAPI

#include <sys/types.h>
#include <stdlib.h>

#include "iodecl3.h"
#include "io.h"

int main(int argc, char *argv[])
{
    char ORIG_FILE[16] = "ORIG_FILE";
    char NEW_FILE[16]  = "NEW_FILE";
    char TOLERANCE[16] = "TOLERANCE";

    IOAPI_Bdesc3 bdesc_orig;    /* original file description - non-character data */
    IOAPI_Bdesc3 bdesc_new;     /* new file description - non-character data */
    IOAPI_Cdesc3 cdesc_orig;    /* original file description - character data */
    IOAPI_Cdesc3 cdesc_new;     /* new file description - character data */

    int    *intOrigData, *intNewData;   /* arrays for storing original and */
    float  *fltOrigData, *fltNewData;   /*   new data for various I/O API  */
    double *dblOrigData, *dblNewData;   /*   types                         */

    int varnum;                 /* index for variable loop */
    int jdate, jtime;           /* loop date and time */
    int tstep;                  /* index for time step loop */
    int layer;                  /* index for layer loop */
    int row, col;               /* indices for row and column loops */
    int recno;                  /* position in data array based on row and column */
    int error = 0;              /* true: files differ */

    double origVal;             /* original data value */
    double newVal;              /* new data value */
    double pctdiff;             /* percent difference between original and new */
    double tolerance;           /* difference tolerance */

    char orig_name[256];        /* original file name */
    char new_name[256];         /* new file name */
    char tol_str[20];           /* tolerance env. variable value */
    char varname[NAMLEN3+1];    /* variable name */

    char *prog_name="diffioapi";     /* program name */
    
/* ========================================================================== */

    if(!getEnvtValue(ORIG_FILE, orig_name))
    {
        printf("ERROR: %s environment variable is not set\n", ORIG_FILE);
        return(1);
    }
    
    if(!getEnvtValue(NEW_FILE, new_name))
    {
        printf("ERROR: %s environment variable is not set\n", NEW_FILE);
        return(1);
    }

    if(!getEnvtValue(TOLERANCE, tol_str))
    {
        tolerance = 0.01;
    }
    else
    {
        tolerance = atof(tol_str);
    }

    if(!open3c(ORIG_FILE, &bdesc_orig, &cdesc_orig, FSREAD3, prog_name))
    {
        printf("ERROR: Could not open I/O API file %s\n", orig_name);
        return(1);
    }
    
    if(!open3c(NEW_FILE, &bdesc_new, &cdesc_new, FSREAD3, prog_name))
    {
        printf("ERROR: Could not open I/O API file %s\n", new_name);
        return(1);
    }
    
    if(!desc3c(ORIG_FILE, &bdesc_orig, &cdesc_orig))
    {
        printf("ERROR: Could not get description of I/O API file %s\n", orig_name);
        return(1);
    }
    
    if(!desc3c(NEW_FILE, &bdesc_new, &cdesc_new))
    {
        printf("ERROR: Could not get description of I/O API file %s\n", new_name);
        return(1);
    }
    
    /* compare file headers */
    if(!error &&
      ((bdesc_orig.ftype != bdesc_new.ftype)))
    {
        error = 1;
        printf("ERROR: File types do not match between original and new files\n");
    }
    
    if(!error &&
      ((bdesc_orig.p_alp != bdesc_new.p_alp) ||
       (bdesc_orig.p_bet != bdesc_new.p_bet) ||
       (bdesc_orig.p_gam != bdesc_new.p_gam) ||
       (bdesc_orig.xcent != bdesc_new.xcent) ||
       (bdesc_orig.ycent != bdesc_new.ycent)))
    {
        error = 1;
        printf("ERROR: Map projections do not match between original and new files\n");
    }
    
    if(!error &&
      ((bdesc_orig.xorig != bdesc_new.xorig) ||
       (bdesc_orig.yorig != bdesc_new.yorig) ||
       (bdesc_orig.xcell != bdesc_new.xcell) ||
       (bdesc_orig.ycell != bdesc_new.ycell) ||
       (bdesc_orig.ncols != bdesc_new.ncols) ||
       (bdesc_orig.nrows != bdesc_new.nrows)))
    {
        error = 1;
        printf("ERROR: Grid descriptions do not match between original and new files\n");
    }
    
    if(!error &&
      ((bdesc_orig.nlays != bdesc_new.nlays) ||
       (bdesc_orig.vgtyp != bdesc_new.vgtyp) ||
       (bdesc_orig.vgtop != bdesc_new.vgtop)))
    {
        error = 1;
        printf("ERROR: Layer structures do not match between original and new files\n");
    }
    
    if(!error &&
      ((bdesc_orig.sdate != bdesc_new.sdate) ||
       (bdesc_orig.stime != bdesc_new.stime) ||
       (bdesc_orig.tstep != bdesc_new.tstep) ||
       (bdesc_orig.mxrec != bdesc_new.mxrec)))
    {
        error = 1;
        printf("ERROR: Start times, time steps, or durations do not match "
               "between original and new files\n");
    }
    
    if(!error &&
      ((bdesc_orig.nvars != bdesc_new.nvars)))
    {
        error = 1;
        printf("ERROR: Original and new files have different number of variables\n");
    }
    
    if(error)
    {
        printf("Original and new files have different headers\n");
        return(2);
    }
    
    intOrigData = malloc(bdesc_orig.nrows * bdesc_orig.ncols * sizeof(int));
    intNewData  = malloc(bdesc_orig.nrows * bdesc_orig.ncols * sizeof(int));
    
    fltOrigData = malloc(bdesc_orig.nrows * bdesc_orig.ncols * sizeof(float));
    fltNewData  = malloc(bdesc_orig.nrows * bdesc_orig.ncols * sizeof(float));
    
    dblOrigData = malloc(bdesc_orig.nrows * bdesc_orig.ncols * sizeof(double));
    dblNewData  = malloc(bdesc_orig.nrows * bdesc_orig.ncols * sizeof(double));
    
    /* compare each variable */
    for(varnum = 0; varnum < bdesc_orig.nvars; ++varnum)
    {
        if(strncmp(cdesc_orig.vname[varnum], cdesc_new.vname[varnum], NAMLEN3) != 0)
        {
            error = 1;
            printf("ERROR: Variable names do not match between original and new files\n");
            break;
        }
        
        strNullTerminate(varname, cdesc_orig.vname[varnum], NAMLEN3);
    
        jdate = bdesc_orig.sdate;
        jtime = bdesc_orig.stime;
        
        for(tstep = 0; tstep < bdesc_orig.mxrec; ++tstep)
        {
            for(layer = 1; layer <= bdesc_orig.nlays; ++layer)
            {
                switch(bdesc_orig.vtype[varnum])
                {
                case M3INT:
                    if(!read3c(ORIG_FILE, varname, layer, jdate, jtime, intOrigData))
                    {
                        printf("ERROR: Could not read variable %s at time step "
                               "%d:%d and layer %d from I/O API file %s\n", varname,
                               jdate, jtime, layer, orig_name);
                        return(1);
                    }
                    
                    if(!read3c(NEW_FILE, varname, layer, jdate, jtime, intNewData))
                    {
                        printf("ERROR: Could not read variable %s at time step "
                               "%d:%d and layer %d from I/O API file %s\n", varname,
                               jdate, jtime, layer, new_name);
                        return(1);
                    }
                    
                    break;
                case M3REAL:
                    if(!read3c(ORIG_FILE, varname, layer, jdate, jtime, fltOrigData))
                    {
                        printf("ERROR: Could not read variable %s at time step "
                               "%d:%d and layer %d from I/O API file %s\n", varname,
                               jdate, jtime, layer, orig_name);
                        return(1);
                    }
                    
                    if(!read3c(NEW_FILE, varname, layer, jdate, jtime, fltNewData))
                    {
                        printf("ERROR: Could not read variable %s at time step "
                               "%d:%d and layer %d from I/O API file %s\n", varname,
                               jdate, jtime, layer, new_name);
                        return(1);
                    }
                    
                    break;
                case M3DBLE:
                    if(!read3c(ORIG_FILE, varname, layer, jdate, jtime, dblOrigData))
                    {
                        printf("ERROR: Could not read variable %s at time step "
                               "%d:%d and layer %d from I/O API file %s\n", varname,
                               jdate, jtime, layer, orig_name);
                        return(1);
                    }
                    
                    if(!read3c(NEW_FILE, varname, layer, jdate, jtime, dblNewData))
                    {
                        printf("ERROR: Could not read variable %s at time step "
                               "%d:%d and layer %d from I/O API file %s\n", varname,
                               jdate, jtime, layer, new_name);
                        return(1);
                    }
                    
                    break;
                }
                
                for(row = 0; row < bdesc_orig.nrows; ++row)
                {
                    for(col = 0; col < bdesc_orig.ncols; ++col)
                    {
                        recno = (row*bdesc_orig.ncols) + col;
                        
                        switch(bdesc_orig.vtype[varnum])
                        {
                        case M3INT:
                            origVal = (double) intOrigData[recno];
                            newVal = (double) intNewData[recno];
                            break;
                        case M3REAL:
                            origVal = (double) fltOrigData[recno];
                            newVal = (double) fltNewData[recno];
                            break;
                        case M3DBLE:
                            origVal = dblOrigData[recno];
                            newVal = dblNewData[recno];
                            break;
                        }
                        
                        /* NOTE: what if origVal is zero? */
                        if(origVal != 0.)
                        {
                            pctdiff = 100. * (newVal - origVal) / origVal;
                            if(abs(pctdiff) > tolerance)
                            {
                                error = 1;
                                printf("ERROR: Original and new values for variable "
                                       "%s, time step %d:%d, layer %d, row %d, and "
                                       "column %d differ by %.2f%%\n", varname, jdate,
                                       jtime, layer, row+1, col+1, pctdiff);
                            }
                        }
                    }  /* end loop over columns */
                }  /* end loop over rows */
            }  /* end loop over layers */
            
            nextimec(&jdate, &jtime, bdesc_orig.tstep);
        }  /* end loop over time steps */
    }  /* end loop over variables */
    
    if(error)
    {
        printf("Original and new files differ\n");
        return(2);
    }
    else
    {
        printf("Orignal and new files are the same\n");
        return(0);
    }
}

#else

int main(int argc, char *argv[])
{
    printf("I/O API support is required for diffioapi.exe\n");
    return 0;
}

#endif
