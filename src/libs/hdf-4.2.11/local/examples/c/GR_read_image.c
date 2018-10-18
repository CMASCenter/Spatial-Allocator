#include "hdf.h"

#define  FILE_NAME       "General_RImages.hdf"
#define  N_COMPS         2
#define  X_LENGTH        10   /* number of columns of the entire image */
#define  Y_LENGTH        5    /* number of rows of the entire image */
#define  PART_COLS       2    /* number of columns read for partial image */
#define  PART_ROWS       3    /* number of rows read for partial image */
#define  SKIP_COLS       5    /* number of columns read for skipped image */
#define  SKIP_ROWS       3    /* number of rows read for skipped image */
#define  COLS_PART_START 3    /* starting column to read partial image */
#define  ROWS_PART_START 1    /* starting row to read partial image */
#define  COLS_SKIP_START 1    /* starting column to read skipped image */
#define  ROWS_SKIP_START 0    /* starting row to read skipped image */
#define  N_STRIDES       2    /* number of elements to skip on each dim. */

int main( )
{
   /************************* Variable declaration **************************/

   intn  status;        /* status for functions returning an intn */
   int32 index;
   int32 file_id, gr_id, ri_id,
         start[2],      /* start position to write for each dimension */
         edges[2],      /* number of elements to bewritten along 
                           each dimension */
         stride[2],     /* number of elements to skip on each dimension */
         dim_sizes[2];  /* dimension sizes of the image array */
   int16 entire_image[Y_LENGTH][X_LENGTH][N_COMPS],
         partial_image[PART_ROWS][PART_COLS][N_COMPS],
         skipped_image[SKIP_ROWS][SKIP_COLS][N_COMPS];
   int32 i, j;

   /********************** End of variable declaration **********************/

   /*
   * Open the HDF file for reading.
   */
   file_id = Hopen (FILE_NAME, DFACC_READ, 0);

   /*
   * Initialize the GR interface.
   */
   gr_id = GRstart (file_id);

   /*
   * Select the first raster image in the file.
   */
   ri_id = GRselect (gr_id, 0);

   /*
   * Define the size of the data to be read, i.e., start from the origin 
   * and go as long as the length of each dimension.
   */
   start[0] = start[1] = 0;
   edges[0] = X_LENGTH;
   edges[1] = Y_LENGTH;

   /*
   * Read the data from the raster image array.
   */
   status = GRreadimage (ri_id, start, NULL, edges, (VOIDP)entire_image);

   /*
   * Display only the first component of the image since the two components 
   * have the same data in this example.
   */
   printf ("First component of the entire image:\n");
   for (i = 0; i < Y_LENGTH; i++)
   {
      for (j = 0; j < X_LENGTH; j++)
         printf ("%d ", entire_image[i][j][0]);
      printf ("\n");
   }

   /*
   * Define the size of the data to be read.
   */
   start[0] = COLS_PART_START;
   start[1] = ROWS_PART_START;
   edges[0] = PART_COLS;
   edges[1] = PART_ROWS;

   /*
   * Read a subset of the raster image array.
   */
   status = GRreadimage (ri_id, start, NULL, edges, (VOIDP)partial_image);

   /*
   * Display the first component of the read sample.
   */
   printf ("\nThree rows & two cols at 2nd row and 4th column");
   printf (" of the first component:\n");
   for (i = 0; i < PART_ROWS; i++)
   {
      for (j = 0; j < PART_COLS; j++)
         printf ("%d ", partial_image[i][j][0]);
      printf ("\n");
   }

   /*
   * Define the size and the pattern to read the data.
   */
   start[0] = COLS_SKIP_START;
   start[1] = ROWS_SKIP_START;
   edges[0] = SKIP_COLS;
   edges[1] = SKIP_ROWS;
   stride[0] = stride[1] = N_STRIDES;

   /*
   * Read all the odd rows and even columns of the image.
   */
   status = GRreadimage (ri_id, start, stride, edges, (VOIDP)skipped_image);

   /*
   * Display the first component of the read sample.
   */
   printf ("\nAll odd rows and even columns of the first component:\n");
   for (i = 0; i < SKIP_ROWS; i++)
   {
      for (j = 0; j < SKIP_COLS; j++)
         printf ("%d ", skipped_image[i][j][0]);
      printf ("\n");
   }

   /*
   * Terminate access to the raster image and to the GR interface, and
   * close the HDF file.
   */
   status = GRendaccess (ri_id);
   status = GRend (gr_id);
   status = Hclose (file_id);
   return 0;
}
