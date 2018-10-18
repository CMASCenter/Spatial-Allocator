#include "hdf.h"

#define  FILE_NAME      "Image_with_Palette.hdf"
#define  IMAGE_NAME     "Image with Palette"
#define  N_ENTRIES      256     /* number of elements of each color */

int main( )
{
   /************************* Variable declaration **************************/

   intn  status,         /* status for functions returning an intn */
         i, j;
   int32 file_id, gr_id, ri_id, pal_id, ri_index;
   int32 data_type, n_comps, n_entries, interlace_mode; 
   uint8 palette_data[N_ENTRIES][3];        /* static because of fixed size */

   /************************* Variable declaration **************************/

   /* 
   * Open the file. 
   */
   file_id = Hopen (FILE_NAME, DFACC_READ, 0);

   /* 
   * Initiate the GR interface. 
   */
   gr_id = GRstart (file_id);

   /* 
   * Get the index of the image IMAGR_NAME.
   */
   ri_index = GRnametoindex (gr_id, IMAGE_NAME);

   /*
   * Get image identifier.
   */
   ri_id = GRselect (gr_id, ri_index);
 
   /* 
   * Get the identifier of the palette attached to the image. 
   */
   pal_id = GRgetlutid (ri_id, ri_index);

   /*
   * Obtain and display information about the palette.
   */
   status = GRgetlutinfo (pal_id, &n_comps, &data_type, &interlace_mode, 
                          &n_entries);
   printf ("Palette: %d components; %d entries\n", n_comps, n_entries); 

   /* 
   * Read the palette data. 
   */
   status = GRreadlut (pal_id, (VOIDP)palette_data);

   /*
   * Display the palette data.  Recall that HDF supports only 256 colors.
   * Each color is defined by its 3 components. Therefore, 
   * verifying the value of n_entries and n_comps is not necessary and 
   * the buffer to hold the palette data can be static.  However, 
   * if more values or colors are added to the model, these parameters 
   * must be checked to allocate sufficient space when reading a palette.
   */
   printf ("  Palette Data: \n");
   for (i=0; i< n_entries; i++)
   {
      for (j = 0; j < n_comps; j++)
         printf ("%i ", palette_data[i][j]);
      printf ("\n");
   }
   printf ("\n");

   /* 
   * Terminate access to the image and to the GR interface, and 
   * close the HDF file. 
   */
   status = GRendaccess (ri_id);
   status = GRend (gr_id);
   status = Hclose (file_id);
   return 0;
}
