#include "hdf.h"

#define  FILE_NAME    "General_RImages.hdf"
#define  X1_LENGTH    5     /* number of columns in the first image 
                              being modified */
#define  Y1_LENGTH    2     /* number of rows in the first image 
                              being modified */
#define  N1_COMPS     2     /* number of components in the first image */
#define  IMAGE1_NAME  "Image Array 1"
#define  IMAGE2_NAME  "Image Array 2"
#define  X2_LENGTH    6     /* number of columns in the second image */
#define  Y2_LENGTH    4     /* number of rows in the second image */
#define  N2_COMPS     3     /* number of components in the second image */

int main( ) 
{
   /************************* Variable declaration **************************/

   intn  status;         /* status for functions returning an intn */
   int32 file_id,        /* HDF file identifier */
         gr_id,          /* GR interface identifier */
         ri1_id,         /* raster image identifier */
         start1[2],      /* start position to write for each dimension */
         edges1[2],      /* number of elements to be written along
                           each dimension */
         ri2_id,         /* raster image identifier */
         start2[2],      /* start position to write for each dimension */
         edges2[2],      /* number of elements to be written along 
                           each dimension */
         dims_sizes[2],  /* sizes of the two dimensions of the image array */
         data_type,      /* data type of the image data */
         interlace_mode; /* interlace mode of the image */
   int16 i, j;           /* indices for the dimensions */
   int16 image1_buf[Y1_LENGTH][X1_LENGTH][N1_COMPS]; /* data of first image */
   char  image2_buf[Y2_LENGTH][X2_LENGTH][N2_COMPS]; /* data of second image*/

   /********************** End of variable declaration **********************/

   /*
   * Open the HDF file for writing.
   */
   file_id = Hopen (FILE_NAME, DFACC_WRITE, 0);

   /*
   * Initialize the GR interface.
   */
   gr_id = GRstart (file_id);

   /*
   * Select the first raster image in the file.
   */
   ri1_id = GRselect (gr_id, 0);

   /*
   * Fill the first image data buffer with values.
   */
   for (i = 0; i < Y1_LENGTH; i++)
   {
      for (j = 0; j < X1_LENGTH; j++)
      {
         image1_buf[i][j][0] = 0;  /* first component */ 
         image1_buf[i][j][1] = 0;  /* second component */ 
      }
    }

   /*
   * Define the size of the data to be written, i.e., start from the origin
   * and go as long as the length of each dimension.
   */
   start1[0] = start1[1] = 0;
   edges1[0] = X1_LENGTH;
   edges1[1] = Y1_LENGTH;

   /*
   * Write the data in the buffer into the image array.
   */
   status = GRwriteimage (ri1_id, start1, NULL, edges1, (VOIDP)image1_buf);


   /*
   * Set the interlace mode and dimensions of the second image.
   */
   data_type = DFNT_CHAR8;
   interlace_mode = MFGR_INTERLACE_PIXEL;
   dims_sizes[0] = X2_LENGTH;
   dims_sizes[1] = Y2_LENGTH;

   /*
   * Create the second image in the file.
   */
   ri2_id = GRcreate (gr_id, IMAGE2_NAME, N2_COMPS, data_type,
                                interlace_mode, dims_sizes);

   /*
   * Fill the second image data buffer with values.
   */
   for (i = 0; i < Y2_LENGTH; i++)
   {
      for (j = 0; j < X2_LENGTH; j++)
      {
         image2_buf[i][j][0] = 'A';     /* first component */ 
         image2_buf[i][j][1] = 'B';     /* second component */
         image2_buf[i][j][2] = 'C';     /* third component */
      }
    }

   /*
   * Define the size of the data to be written, i.e., start from the origin
   * and go as long as the length of each dimension.
   */
   for (i = 0; i < 2; i++) {
      start2[i] = 0;
      edges2[i] = dims_sizes[i];
   }

   /*
   * Write the data in the buffer into the second image array.
   */
   status = GRwriteimage (ri2_id, start2, NULL, edges2, (VOIDP)image2_buf);

   /*
   * Terminate access to the raster images and to the GR interface, and
   * close the HDF file.
   */
   status = GRendaccess (ri1_id);
   status = GRendaccess (ri2_id);
   status = GRend (gr_id);
   status = Hclose (file_id);
   return 0;
}
