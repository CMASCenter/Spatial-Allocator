/* This program transforms BELD3 land use data to the modeling grid of interest. */

#ifdef USE_IOAPI

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "iodecl3.h"
#include "io.h"

#define MAX_TILES 24    /* number of tiles of data */
#define MAX_VARS  230   /* number of variables in data */
#define NUM_FILES 3     /* number of files per tile */

/* list of abbreviations to use for each file */     
const char abbrev[NUM_FILES][5] = {"_a", "_b", "_tot"};

/* environment variables */
const char IOAPI_FILE_OUT[16] = "IOAPI_FILE_OUT";
const char IOAPI_FILE_IN[16] = "IOAPI_FILE_IN";

/* program name */
char *prog_name;

int main(int argc, char *argv[])
{
  /* environment variable values */
  char debug[10];            /* print debug messages flag */
  int  debug_output;
  char gridname[NAMLEN3+1];  /* output grid name */
  char griddesc[256];        /* grid description file */
  char input_dir[128];       /* input data directory */
  char tmp_dir[128];         /* tmp directory for intermediate files */
  char output_pre[128];      /* output prefix */

  /* allocatable arrays */
  float *inData;             /* values read in from I/O API files */
  float *outData;            /* summed output values */
  float *qaData;             /* summed data across variables for QA */

  /* fixed size arrays */
  char var_names[MAX_VARS][NAMLEN3+1];    /* variable names */
  int  var_files[MAX_VARS][MAX_TILES][2]; /* variable-to-file mapping array */
  int  tilelist[MAX_TILES];               /* list of tiles to process */

  /* other local variables */
  int  filenum;              /* loop indices */
  int  tileidx, tilenum;
  int  varidx, varnum;
  int  recno;
  int  row, col;
  int  num_tiles;            /* number of tiles to process */
  int  files_processed;      /* number of files processed */

  float pctdiff;             /* percent difference in summed values */

  IOAPI_Bdesc3 bdesc;        /* I/O API file description - non-character data */
  IOAPI_Bdesc3 bdesc_grid;
  IOAPI_Cdesc3 cdesc;        /* I/O API file description - character data */
  IOAPI_Cdesc3 cdesc_grid;
  
  FILE *fileptr;             /* file pointer */
  struct stat status;        /* file status buffer */

  char file_name[256];       /* generated complete file name */
  char varname[NAMLEN3+1];   /* variable name */
  char mesg[256];            /* message buffer */

/* ========================================================================== */

  prog_name = (char *) strdup("beld3smk");

  /* get values for environment variables */
  debug_output = 0;
  if(getEnvtValue("DEBUG_OUTPUT", debug))
  {
    if(strcmp(debug, "Y") == 0) debug_output = 1;
  }
  
  if(!getEnvtValue("OUTPUT_GRID_NAME", gridname))
  {
    sprintf(mesg, "%s environment variable not set", "OUTPUT_GRID_NAME");
    ERROR(prog_name, mesg, 1);
  }
  
  if(!getEnvtValue("GRIDDESC", griddesc))
  {
    sprintf(mesg, "%s environment variable not set", "GRIDDESC");
    ERROR(prog_name, mesg, 1);
  }

  if(!getEnvtValue("INPUT_DATA_DIR", input_dir))
  {
    sprintf(mesg, "%s environment variable not set", "INPUT_DATA_DIR");
    ERROR(prog_name, mesg, 1);
  }

  if(!getEnvtValue("TMP_DATA_DIR", tmp_dir))
  {
    sprintf(mesg, "%s environment variable not set", "TMP_DATA_DIR");
    ERROR(prog_name, mesg, 1);
  }
  
  if(!getEnvtValue("OUTPUT_FILE_PREFIX", output_pre))
  {
    sprintf(mesg, "%s environment variable not set", "OUTPUT_FILE_PREFIX");
    ERROR(prog_name, mesg, 1);
  }

  /* use overlay mode of spatial allocator to determine which tiles to process */
  if(debug_output)
  {
    setenv("DEBUG_OUTPUT", "Y", 1);
  }
  else
  {
    setenv("DEBUG_OUTPUT", "N", 1);
  }
  
  setenv("MIMS_PROCESSING", "OVERLAY", 1);
  
  setenv("OVERLAY_TYPE", "RegularGrid", 1);
  setenv("OVERLAY_SHAPE", gridname, 1);
  setenv("OVERLAY_MAP_PRJN", gridname, 1);
  setenv("OVERLAY_ELLIPSOID", "SPHERE", 1);
  setenv("GRIDDESC", griddesc, 1);
  
  setenv("OVERLAY_ATTRS", "TILES24_ID", 1);
  setenv("OVERLAY_OUT_TYPE", "DelimitedFile", 1);
  sprintf(file_name, "%stiles.txt", tmp_dir);
  setenv("OVERLAY_OUT_NAME", file_name, 1);
  setenv("OVERLAY_OUT_DELIM", " ", 1);
  setenv("WRITE_HEADER", "N", 1);
  
  setenv("INPUT_FILE_TYPE", "ShapeFile", 1);
  sprintf(file_name, "%stiles24", input_dir);
  setenv("INPUT_FILE_NAME", file_name, 1);
  setenv("INPUT_FILE_MAP_PRJN", 
         "+proj=lcc,+lon_0=-90.0,+lat_1=30.0,+lat_2=60.0,+lat_0=40.0", 1);
  setenv("INPUT_FILE_ELLIPSOID", "SPHERE", 1);
 
  
  /* read tile list output from spatial allocator */
  sprintf(file_name, "%stiles.txt", tmp_dir);
  if((fileptr = fopen(file_name, "r")) == NULL)
  {
    sprintf(mesg, "Could not open intermediate file %s", file_name);
    ERROR(prog_name, mesg, 2);
  }
  
  tilenum = 0;
  while(fgets(mesg, sizeof(mesg), fileptr) != NULL)
  {
    tilelist[tilenum] = atoi(mesg);
    ++tilenum;    
  }
  num_tiles = tilenum;
  
  /* use spatial allocator to process each tile */
  if(debug_output)
  {
    setenv("DEBUG_OUTPUT", "Y", 1);
  }
  else
  {
    setenv("DEBUG_OUTPUT", "N", 1);
  }
  
  setenv("MIMS_PROCESSING", "ALLOCATE", 1);
  
  setenv("GRIDDESC", griddesc, 1);

  
  /* create output file headers and store master list of variable names */
  
  /* get grid description from just created intermediate file */
  buildFileName(file_name, input_dir, 0, tilelist[0], 0);
  getDescription(file_name, &bdesc_grid, &cdesc_grid);
  
  /* get variable names from master variable files */
  varidx = 0;
  for(filenum = 0; filenum < NUM_FILES; ++filenum)
  {
    sprintf(file_name, "%svarlist%s.ncf", input_dir, abbrev[filenum]);
    getDescription(file_name, &bdesc, &cdesc);
    
    /* store list of variable names */
    if(filenum != 2)
    {
      for(varnum = 0; varnum < bdesc.nvars; ++varnum)
      {
        strNullTerminate(var_names[varidx], cdesc.vname[varnum], NAMLEN3);
        ++varidx;
      }
    }
    
    /* reset grid description */
    bdesc.p_alp = bdesc_grid.p_alp;
    bdesc.p_bet = bdesc_grid.p_bet;
    bdesc.p_gam = bdesc_grid.p_gam;
    bdesc.xcent = bdesc_grid.xcent;
    bdesc.ycent = bdesc_grid.ycent;
    bdesc.xorig = bdesc_grid.xorig;
    bdesc.yorig = bdesc_grid.yorig;
    bdesc.xcell = bdesc_grid.xcell;
    bdesc.ycell = bdesc_grid.ycell;
    
    bdesc.gdtyp = bdesc_grid.gdtyp;
    memcpy(cdesc.gdnam, cdesc_grid.gdnam, sizeof(cdesc.gdnam));
    bdesc.ncols = bdesc_grid.ncols;
    bdesc.nrows = bdesc_grid.nrows;

  }
  
  /* loop through intermediate files and create variable-to-file mapping */
  memset(var_files, 0, MAX_VARS * MAX_TILES * 2 * sizeof(int));
  
  for(tileidx = 0; tileidx < num_tiles; ++tileidx)
  {
    tilenum = tilelist[tileidx];
  
    for(filenum = 0; filenum < NUM_FILES-1; ++filenum)
    {
      buildFileName(file_name, input_dir, filenum, tilenum, 0);
      if(stat(file_name, &status) == 0)
      {
        getDescription(file_name, &bdesc, &cdesc);
        
        for(varidx = 0; varidx < bdesc.nvars; ++varidx)
        {
          strNullTerminate(varname, cdesc.vname[varidx], NAMLEN3);
          
          for(varnum = 0; varnum < MAX_VARS; ++varnum)
          {
            if(strcmp(varname, var_names[varnum]) == 0)
            {
              var_files[varnum][tilenum-1][filenum] = 1;
              break;
            }
          }  /* end loop over master variables list */
        }  /* end loop over variables in file */
      }
    }  /* end loop over files */
  }  /* end loop over tiles */
  

  return 0;
}

/* ========================================================================== */

int buildFileName(char *dest, const char *dir, 
                  int filenum, int tilenum, int isIntermediate)
{
  return(sprintf(dest, 
                 isIntermediate ? "%sb3%s.tile%d.nzero.out.ncf"
                                : "%sb3%s.tile%d.nzero.ncf", 
                 dir, abbrev[filenum], tilenum));
}

/* ========================================================================== */

int getDescription(const char *file_name, IOAPI_Bdesc3 *bdesc,
                   IOAPI_Cdesc3 *cdesc)
{
  char mesg[256];

  setenv(IOAPI_FILE_IN, file_name, 1);
  
  if(!open3c(IOAPI_FILE_IN, bdesc, cdesc, FSREAD3, prog_name))
  {
    sprintf(mesg, "Could not open I/O API file %s", file_name);
    ERROR(prog_name, mesg, 2);
  }
  
  if(!desc3c(IOAPI_FILE_IN, bdesc, cdesc))
  {
    sprintf(mesg, "Could not get description of I/O API file %s", file_name);
    ERROR(prog_name, mesg, 2);
  }
  
  if(!close3c(IOAPI_FILE_IN))
  {
    sprintf(mesg, "Could not close I/O API file %s", file_name);
    ERROR(prog_name, mesg, 2);
  }
  
  return 0;
}

#else

int main(int argc, char *argv[])
{
    printf("I/O API support is required for beld3smk.exe\n");
    return 0;
}

#endif
