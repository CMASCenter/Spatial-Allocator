#include "hdf.h"

#define  FILE_NAME      "General_Vdatas.hdf"
#define  FIELD_SIZE     80         /* maximum length of all the field names */

int main( )
{
   /************************* Variable declaration **************************/

   intn  status_n;      /* returned status for functions returning an intn  */
   int32 status_32,     /* returned status for functions returning an int32 */
         n_records,     /* to retrieve the number of records in the vdata   */
         interlace_mode,/* to retrieve the interlace mode of the vdata      */
         vdata_size,    /* to retrieve the size of all specified fields     */
         file_id, vdata_ref, vdata_id;
   char  fieldname_list[FIELD_SIZE], /* buffer to retrieve the vdata data   */
         vdata_name[VSNAMELENMAX];   /* buffer to retrieve the vdata name   */

   /********************** End of variable declaration **********************/

   /*
   * Open the HDF file for reading. 
   */
   file_id = Hopen (FILE_NAME, DFACC_READ, 0);

   /*
   * Initialize the VS interface. 
   */
   status_n = Vstart (file_id);

   /*
   * Set vdata_ref to -1 to start the search from the beginning of file.
   */
   vdata_ref = -1;

   /*
   * Use VSgetid to obtain each vdata by its reference number then attach 
   * to the vdata and get its information.  The loop terminates when 
   * the last vdata is reached.
   */
   while ((vdata_ref = VSgetid (file_id, vdata_ref)) != FAIL)
   {
      /*
      * Attach to the current vdata for reading.
      */
      vdata_id = VSattach (file_id, vdata_ref, "r");

      /*
      * Test whether the current vdata is not a storage of an attribute, then
      * obtain and display its information.
      */
      if( VSisattr (vdata_id) != TRUE )
      {
         status_n = VSinquire (vdata_id, &n_records, &interlace_mode, 
                            fieldname_list, &vdata_size, vdata_name);
         printf ("Vdata %s: - contains %d records\n\tInterlace mode: %s \
                 \n\tFields: %s - %d bytes\n\t\n", vdata_name, n_records,
                 interlace_mode == FULL_INTERLACE ? "FULL" : "NONE", 
                 fieldname_list, vdata_size );
      }

      /*
      * Detach from the current vdata.
      */
      status_32 = VSdetach (vdata_id);
   } /* while */

   /*
   * Terminate access to the VS interface and close the HDF file. 
   */
   status_n = Vend (file_id);
   status_32 = Hclose (file_id);
   return 0;
}
