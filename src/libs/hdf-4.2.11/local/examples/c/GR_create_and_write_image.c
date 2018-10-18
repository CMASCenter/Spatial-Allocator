#include "hdf.h"

#define  FILE_NAME     "General_RImages.hdf"
#define  IMAGE_NAME    "Image Array 1"
#define  X_LENGTH      10    /* number of columns in the image */
#define  Y_LENGTH      5     /* number of rows in the image */
#define  N_COMPS       2     /* number of components in the image */

int main( ) 
{
   /************************* Variable declaration **************************/

   intn  status;         /* status for functions returning an intn */
   int32 file_id,        /* HDF file identifier */
         gr_id,          /* GR interface identifier */
         ri_id,          /* raster image identifier */
         start[2],       /* start position to write for each dimension */
         edges[2],       /* number of elements to be written 
                           along each dimension */
         dim_sizes[2],   /* dimension sizes of the image array */
         interlace_mode, /* interlace mode of the image */
         data_type,      /* data type of the image data */
         i, j;
   int16 image_buf[Y_LENGTH][X_LENGTH][N_COMPS];

   /********************** End of variable declaration **********************/

   /*
   * Create and open the file.
   */
   file_id = Hopen (FILE_NAME, DFACC_CREATE, 0);

   /*
   * Initialize the GR interface.
   */
   gr_id = GRstart (file_id);

   /*
   * Set the data type, interlace mode, and dimensions of the image.
   */
   data_type = DFNT_INT16;
   interlace_mode = MFGR_INTERLACE_PIXEL;
   dim_sizes[0] = X_LENGTH;
   dim_sizes[1] = Y_LENGTH;

   /*
   * Create the raster image array.
   */
   ri_id = GRcreate (gr_id, IMAGE_NAME, N_COMPS, data_type, 
                     interlace_mode, dim_sizes);

   /*
   * Fill the image data buffer with values.
   */
   for (i = 0; i < Y_LENGTH; i++)
   {
      for (j = 0; j < X_LENGTH; j++)
      {
         image_buf[i][j][0] = (i + j) + 1;     /* first component */
         image_buf[i][j][1] = (i + j) + 1;     /* second component */
      }
    }

   /*
   * Define the size of the data to be written, i.e., start from the origin
   * and go as long as the length of each dimension.
   */
   start[0] = start[1] = 0;
   edges[0] = X_LENGTH;
   edges[1] = Y_LENGTH;

   /*
   * Write the data in the buffer into the image array.
   */
   status = GRwriteimage(ri_id, start, NULL, edges, (VOIDP)image_buf);

   /*
   * Terminate access to the raster image and to the GR interface and, 
   * close the HDF file.
   */
   status = GRendaccess (ri_id);
   status = GRend (gr_id);
   status = Hclose (file_id);
   return 0;
}
