#include "hdf.h"

#define   FILE_NAME        "General_Vgroups.hdf"

int main( )
{
   /************************* Variable declaration **************************/

   intn   status_n;     /* returned status for functions returning an intn  */
   int32  status_32,    /* returned status for functions returning an int32 */
          file_id, vgroup_id, vgroup_ref,
          obj_index,    /* index of an object within a vgroup */
          num_of_pairs, /* number of tag/ref number pairs, i.e., objects */
          obj_tag, obj_ref,     /* tag/ref number of an HDF object */
          vgroup_pos = 0;       /* position of a vgroup in the file */

   /********************** End of variable declaration ***********************/

   /*
   * Open the HDF file for reading.
   */
   file_id = Hopen (FILE_NAME, DFACC_READ, 0);

   /*
   * Initialize the V interface.
   */
   status_n = Vstart (file_id);

   /*
   * Obtain each vgroup in the file by its reference number, get the
   * number of objects in the vgroup, and display the information about
   * that vgroup.
   */
   vgroup_ref = -1;        /* set to -1 to search from the beginning of file */
   while (TRUE)
   {
      /*
      * Get the reference number of the next vgroup in the file.
      */
      vgroup_ref = Vgetid (file_id, vgroup_ref);

      /*
      * Attach to the vgroup for reading or exit the loop if no more vgroups
      * are found.
      */
      if (vgroup_ref == -1) break;
      vgroup_id = Vattach (file_id, vgroup_ref, "r"); 

      /*
      * Get the total number of objects in the vgroup.
      */
      num_of_pairs = Vntagrefs (vgroup_id);

      /*
      * If the vgroup contains any object, print the tag/ref number 
      * pair of each object in the vgroup, in the order they appear in the
      * file, and indicate whether the object is a vdata, vgroup, or neither.
      */
      if (num_of_pairs > 0)
      {
         printf ("\nVgroup #%d contains:\n", vgroup_pos);
         for (obj_index = 0; obj_index < num_of_pairs; obj_index++)
         {
            /*
            * Get the tag/ref number pair of the object specified 
            * by its index, obj_index, and display them.
            */
            status_n = Vgettagref (vgroup_id, obj_index, &obj_tag, &obj_ref);
            printf ("tag = %d, ref = %d", obj_tag, obj_ref);

            /*
            * State whether the HDF object referred to by obj_ref is a vdata,
            * a vgroup, or neither.
            */
            if (Visvg (vgroup_id, obj_ref))
               printf ("  <-- is a vgroup\n");
            else if (Visvs (vgroup_id, obj_ref))
               printf ("  <-- is a vdata\n");
            else
               printf ("  <-- neither vdata nor vgroup\n");
         } /* for */
      } /* if */

      else
         printf ("Vgroup #%d contains no HDF objects\n", vgroup_pos);

      /*
      * Terminate access to the current vgroup.
      */
      status_32 = Vdetach (vgroup_id);

      /*
      * Move to the next vgroup position.
      */
      vgroup_pos++;
   } /* while */

   /*
   * Terminate access to the V interface and close the file.
   */
   status_n = Vend (file_id);
   status_n = Hclose (file_id);
   return 0;
}
