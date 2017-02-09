#include "hdf.h"

#define  FILE_NAME    "General_RImages.hdf"

int main( ) 
{
   /************************* Variable declaration **************************/

   intn  status;            /* status for functions returning an intn */
   int32 file_id, gr_id, ri_id,
         n_rimages,         /* number of raster images in the file */
         n_file_attrs,      /* number of file attributes */
         ri_index,          /* index of a image */
         dim_sizes[2],      /* dimensions of an image */
         n_comps,           /* number of components an image contains */
         interlace_mode,    /* interlace mode of an image */ 
         data_type,         /* number type of an image */
         n_attrs;           /* number of attributes belong to an image */
   char  name[H4_MAX_GR_NAME], /* name of an image */
        *type_string,       /* mapped text of a number type */
        *interlace_string;  /* mapped text of an interlace mode */

   /********************** End of variable declaration **********************/

   /*
   * Open the file for reading.
   */
   file_id = Hopen (FILE_NAME, DFACC_READ, 0);

   /*
   * Initialize the GR interface.
   */
   gr_id = GRstart (file_id);

   /*
   * Determine the contents of the file.
   */
   status = GRfileinfo (gr_id, &n_rimages, &n_file_attrs);

   /*
   * For each image in the file, get and display the image information.
   */
   printf ("RI#    Name       Components  Type         Interlace     \
   Dimensions   Attributes\n\n");
   for (ri_index = 0; ri_index < n_rimages; ri_index++)
   {
      ri_id = GRselect (gr_id, ri_index);
      status = GRgetiminfo (ri_id, name, &n_comps, &data_type, 
                          &interlace_mode, dim_sizes, &n_attrs);
      /*
      * Map the number type and interlace mode into text strings for output 
      * readability.  Note that, in this example, only two possible types 
      * are considered because of the simplicity of the example.  For real 
      * problems, all possible types should be checked and, if reading the
      * data is desired, the size of the type must be determined based on the
      * machine where the program resides.
      */
      if (data_type == DFNT_CHAR8)
         type_string = "Char8";
      else if (data_type == DFNT_INT16)
         type_string = "Int16";
      else
         type_string = "Unknown";

      switch (interlace_mode)
      {
         case MFGR_INTERLACE_PIXEL:
            interlace_string = "MFGR_INTERLACE_PIXEL";
            break;
         case MFGR_INTERLACE_LINE:
            interlace_string = "MFGR_INTERLACE_LINE";
            break;
         case MFGR_INTERLACE_COMPONENT:
            interlace_string = "MFGR_INTERLACE_COMPONENT";
            break;
         default:
            interlace_string = "Unknown";
            break;
      } /* switch */
 
      /*
      * Display the image information for the current raster image.
      */
          printf ("%d  %s       %d      %s   %s     %2d,%2d         %d\n", 
                 ri_index, name, n_comps, type_string, interlace_string,
                 dim_sizes[0], dim_sizes[1], n_attrs);

      /*
      * Terminate access to the current raster image.
      */
      status = GRendaccess (ri_id);
   }

   /*
   * Terminate access to the GR interface and close the HDF file.
   */
   status = GRend (gr_id);
   status = Hclose (file_id);
   return 0;
}
