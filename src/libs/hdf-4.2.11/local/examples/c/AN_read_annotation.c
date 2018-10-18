#include "hdf.h"

#define  FILE_NAME   "General_HDFobjects.hdf"

int main( )
{
   /************************* Variable declaration **************************/

   intn  status_n;      /* returned status for functions returning an intn  */
   int32 status_32,     /* returned status for functions returning an int32 */
         file_id,       /* HDF file identifier */
         an_id,         /* AN interface identifier */
         ann_id,        /* an annotation identifier */
         index,         /* position of an annotation in all of the same type*/
         ann_length,    /* length of the text in an annotation */
         n_file_labels, n_file_descs, n_data_labels, n_data_descs;
   char *ann_buf;       /* buffer to hold the read annotation */

   /********************** End of variable declaration **********************/

   /*
   * Open the HDF file.
   */
   file_id = Hopen (FILE_NAME, DFACC_READ, 0);

   /*
   * Initialize the AN interface.
   */
   an_id = ANstart (file_id);

   /*
   * Get the annotation information, e.g., the numbers of file labels, file
   * descriptions, data labels, and data descriptions.
   */
   status_n = ANfileinfo (an_id, &n_file_labels, &n_file_descs,
                        &n_data_labels, &n_data_descs);

   /*
   * Get the data labels.  Note that this for loop can be used to
   * obtain the contents of each kind of annotation with the appropriate
   * number of annotations and the type of annotation, i.e., replace
   * n_data_labels with n_file_labels, n_file_descs, or n_data_descs, and
   * AN_DATA_LABEL with AN_FILE_LABEL, AN_FILE_DESC, or AN_DATA_DESC,
   * respectively.
   */
   for (index = 0; index < n_data_labels; index++)
   {
      /*
      * Get the identifier of the current data label.
      */
      ann_id = ANselect (an_id, index, AN_DATA_LABEL);

      /*
      * Get the length of the data label.
      */
      ann_length = ANannlen (ann_id);

      /*
      * Allocate space for the buffer to hold the data label text.
      */
      ann_buf = malloc ((ann_length+1) * sizeof (char));

      /*
      * Read and display the data label.  Note that the size of the buffer,
      * i.e., the third parameter, is 1 character more than the length of
      * the data label; that is for the null character.  It is not the case
      * when a description is retrieved because the description does not 
      * necessarily end with a null character.
      * 
      */
      status_32 = ANreadann (ann_id, ann_buf, ann_length+1);
      printf ("Data label index: %d\n", index);
      printf ("Data label contents: %s\n", ann_buf);

      /*
      * Terminate access to the current data label.
      */
      status_n = ANendaccess (ann_id);

      /*
      * Free the space allocated for the annotation buffer.
      */
      free (ann_buf);
   }

   /*
   * Terminate access to the AN interface and close the HDF file.
   */
   status_32 = ANend (an_id);
   status_n = Hclose (file_id);
   return 0;
}
