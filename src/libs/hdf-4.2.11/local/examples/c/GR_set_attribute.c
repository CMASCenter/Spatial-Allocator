#include "hdf.h"

#define  FILE_NAME          "General_RImages.hdf"
#define  IMAGE_NAME         "Image Array 2"
#define  F_ATT1_NAME        "File Attribute 1"
#define  F_ATT2_NAME        "File Attribute 2"
#define  RI_ATT1_NAME       "Image Attribute 1"
#define  RI_ATT2_NAME       "Image Attribute 2"
#define  F_ATT1_VAL         "Contents of First FILE Attribute"
#define  F_ATT2_VAL         "Contents of Second FILE Attribute"
#define  F_ATT1_N_VALUES    32
#define  F_ATT2_N_VALUES    33
#define  RI_ATT1_VAL        "Contents of IMAGE's First Attribute"
#define  RI_ATT1_N_VALUES   35
#define  RI_ATT2_N_VALUES   6

int main( ) 
{
   /************************* Variable declaration **************************/
   
   intn  status;         /* status for functions returning an intn */
   int32 gr_id, ri_id, file_id,
         ri_index;
   int16 ri_attr_2[RI_ATT2_N_VALUES] = {1, 2, 3, 4, 5, 6};

   /********************** End of variable declaration **********************/

   /*
   * Open the HDF file.
   */
   file_id = Hopen (FILE_NAME, DFACC_WRITE, 0);

   /*
   * Initialize the GR interface.
   */
   gr_id = GRstart (file_id);

   /*
   * Set two file attributes to the file with names, data types, numbers of 
   * values, and values of the attributes specified.
   */
   status = GRsetattr (gr_id, F_ATT1_NAME, DFNT_CHAR8, F_ATT1_N_VALUES, 
                       (VOIDP)F_ATT1_VAL); 

   status = GRsetattr (gr_id, F_ATT2_NAME, DFNT_CHAR8, F_ATT2_N_VALUES, 
                       (VOIDP)F_ATT2_VAL);

   /*
   * Obtain the index of the image named IMAGE_NAME.
   */
   ri_index = GRnametoindex (gr_id, IMAGE_NAME);

   /*
   * Obtain the identifier of this image.
   */
   ri_id = GRselect (gr_id, ri_index);

   /*
   * Set two attributes to the image with names, data types, numbers of 
   * values, and values of the attributes specified.
   */
   status = GRsetattr (ri_id, RI_ATT1_NAME, DFNT_CHAR8, RI_ATT1_N_VALUES, 
                       (VOIDP)RI_ATT1_VAL);

   status = GRsetattr (ri_id, RI_ATT2_NAME, DFNT_INT16, RI_ATT2_N_VALUES, 
                       (VOIDP)ri_attr_2);

   /*
   * Terminate access to the image and to the GR interface, and close the
   * HDF file.
   */
   status = GRendaccess (ri_id);
   status = GRend (gr_id);
   status = Hclose (file_id);
   return 0;
}
