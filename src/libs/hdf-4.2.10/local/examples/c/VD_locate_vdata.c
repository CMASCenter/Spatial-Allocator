#include "hdf.h"

#define  FILE_NAME         "General_Vdatas.hdf"
#define  SEARCHED_FIELDS   "Position,Temperature"

int main( )
{
   /************************* Variable declaration **************************/

   intn  status_n;      /* returned status for functions returning an intn  */
   int32 status_32,     /* returned status for functions returning an int32 */
         file_id, vdata_id, vdata_ref,
         index = 0;     /* index of the vdata in the file - manually kept   */
   int8  found_fields;  /* TRUE if the specified fields exist in the vdata  */

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
   * Set the reference number to -1 to start the search from 
   * the beginning of file.
   */
   vdata_ref = -1;

   /*
   * Assume that the specified fields are not found in the current vdata.
   */
   found_fields = FALSE;
   
   /*
   * Use VSgetid to obtain each vdata by its reference number then
   * attach to the vdata and search for the fields.  The loop
   * terminates when the last vdata is reached or when a vdata which
   * contains the fields listed in SEARCHED_FIELDS is found.  
   */
   while ((vdata_ref = VSgetid (file_id, vdata_ref)) != FAIL)
   {
      vdata_id = VSattach (file_id, vdata_ref, "r");
      if ((status_n = VSfexist (vdata_id, SEARCHED_FIELDS)) != FAIL)
      {
         found_fields = TRUE;
         break;
      }

      /*
      * Detach from the current vdata before continuing searching.
      */
      status_32 = VSdetach (vdata_id);

      index++;		/* advance the index by 1 for the next vdata */
   }
   
   /*
   * Print the index of the vdata containing the fields or a "not found" 
   * message if no such vdata is found.  Also detach from the vdata found.
   */
   if (!found_fields) 
      printf ("Fields Position and Temperature were not found.\n");
   else 
   {
      printf
     ("Fields Position and Temperature found in the vdata at position %d\n", 
       index);
      status_32 = VSdetach (vdata_id);
   }

   /*
   * Terminate access to the VS interface and close the HDF file.
   */
   status_n = Vend (file_id);
   status_32 = Hclose (file_id);
   return 0;
}
