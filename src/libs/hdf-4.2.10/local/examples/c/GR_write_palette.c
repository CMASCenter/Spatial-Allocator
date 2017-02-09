#include "hdf.h"

#define  FILE_NAME         "Image_with_Palette.hdf"
#define  NEW_IMAGE_NAME    "Image with Palette"
#define  N_COMPS_IMG       2       /* number of image components */
#define  X_LENGTH          5       /* number of rows in the image */
#define  Y_LENGTH          5       /* number of columns in the image */
#define  N_ENTRIES         256     /* number of entries in the palette */
#define  N_COMPS_PAL       3       /* number of palette's components */

int main( )
{
   /************************* Variable declaration **************************/

   intn  status,         /* status for functions returning an intn */
         i, j;
   int32 file_id, gr_id, ri_id, pal_id, 
         interlace_mode, 
         start[2],     /* holds where to start to write for each dimension  */
         edges[2],     /* holds how long to write for each dimension */
         dim_sizes[2];  /* sizes of the two dimensions of the image array   */
   uint8 image_buf[Y_LENGTH][X_LENGTH][N_COMPS_IMG]; /* data of first image */
   uint8 palette_buf[N_ENTRIES][N_COMPS_PAL];

   /********************** End of variable declaration **********************/

   /* 
   * Open the HDF file.
   */
   file_id = Hopen (FILE_NAME, DFACC_CREATE, 0);

   /* 
   * Initialize the GR interface.
   */
   gr_id = GRstart (file_id);

   /* 
   * Define the dimensions and interlace mode of the image. 
   */
   dim_sizes[0] = X_LENGTH;
   dim_sizes[1] = Y_LENGTH;
   interlace_mode = MFGR_INTERLACE_PIXEL;

   /* 
   * Create the image named NEW_IMAGE_NAME.
   */
   ri_id = GRcreate (gr_id, NEW_IMAGE_NAME, N_COMPS_IMG, DFNT_UINT8, 
                     interlace_mode, dim_sizes);

   /*
   * Fill the image data buffer with values.
   */
   for (i = 0; i < Y_LENGTH; i++)
   {
      for (j = 0; j < X_LENGTH; j++)
      {
         image_buf[i][j][0] = (i + j) + 1;
         image_buf[i][j][1] = (i + j) + 2;
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
   status = GRwriteimage (ri_id, start, NULL, edges, (VOIDP)image_buf);

   /* 
   * Initialize the palette to grayscale. 
   */
   for (i = 0; i < N_ENTRIES; i++) {
      palette_buf[i][0] = i;
      palette_buf[i][1] = i;
      palette_buf[i][2] = i;
   }

   /*
   * Define palette interlace mode.
   */
   interlace_mode = MFGR_INTERLACE_PIXEL;

   /* 
   * Get the identifier of the palette attached to the image NEW_IMAGE_NAME.
   */
   pal_id = GRgetlutid (ri_id, 0);

   /*
   * Write data to the palette.
   */
   status = GRwritelut (pal_id, N_COMPS_PAL, DFNT_UINT8, interlace_mode,
                        N_ENTRIES, (VOIDP)palette_buf);

   /* 
   * Terminate access to the image and to the GR interface, and 
   * close the HDF file. 
   */
   status = GRendaccess (ri_id);
   status = GRend (gr_id);
   status = Hclose (file_id);
   return 0;
}
