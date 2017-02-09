#include "hdf.h"

#define  FILE_NAME   "General_HDFobjects.hdf"
#define  VG_NAME     "AN Vgroup"

int main( )
{
   /************************* Variable declaration **************************/

   intn   status_n;      /* returned status for functions returning an intn */
   int32  status_32,     /* returned status for functions returning an int32*/
          file_id, an_id, ann_id, 
          n_annots,      /* number of annotations */
         *ann_list,      /* list of annotation identifiers */
          vgroup_ref,    /* reference number of the vgroup */
          index;         /* index of an annotation in the annotation list */
          ann_type annot_type = AN_DATA_DESC;   /* annotation to be obtained*/
   uint16 ann_tag, ann_ref,              /* tag/ref number of an annotation */
          vgroup_tag = DFTAG_VG;         /* tag of the vgroup */

   /********************** End of variable declaration **********************/

   /*
   * Create the HDF file.
   */
   file_id = Hopen (FILE_NAME, DFACC_READ, 0);

   /*
   * Initialize the V interface.
   */
   status_n = Vstart (file_id);

   /*
   * Get the vgroup named VG_NAME.
   */
   vgroup_ref = Vfind (file_id, VG_NAME);

   /*
   * Initialize the AN interface and obtain an interface id.
   */
   an_id = ANstart (file_id);

   /*
   * Get the number of object descriptions.  Note that, since ANnumann takes
   * the tag and reference number as being of type unit16, vgroup_ref must be
   * safely cast to uint16 by checking for FAIL value first.
   */
   if (vgroup_ref != FAIL)
   {
      n_annots = ANnumann (an_id, annot_type, vgroup_tag, (uint16)vgroup_ref);

      /*
      * Allocate space to hold the annotation identifiers.
      */
      ann_list = malloc (n_annots * sizeof (int32));

      /*
      * Get the list of identifiers of the annotations attached to the 
      * vgroup and of type annot_type.
      */
      n_annots = ANannlist (an_id, annot_type, vgroup_tag, (uint16)vgroup_ref, 
                            ann_list);

      /*
      * Get each annotation identifier from the list then display the 
      * tag/ref number pair of the corresponding annotation.
      */
      printf ("List of annotations of type AN_DATA_DESC:\n");
      for (index = 0; index < n_annots; index++)
      {
        /*
         * Get and display the ref number of the annotation from 
         * its identifier.
         */
         status_32 = ANid2tagref (ann_list[index], &ann_tag, &ann_ref);
         printf ("Annotation index %d: tag = %s\nreference number= %d\n", 
           index, ann_tag == DFTAG_DIA ? "DFTAG_DIA (data description)":
           "Incorrect", ann_ref);
      } /* for */
   } /* for */

   /*
   * Get and display an annotation type from an annotation tag.
   */
   annot_type = ANtag2atype (DFTAG_FID);
   printf ("\nAnnotation type of DFTAG_FID (file label) is %s\n", 
              annot_type == AN_FILE_LABEL ? "AN_FILE_LABEL":"Incorrect");

   /*
   * Get and display an annotation tag from an annotation type.
   */
   ann_tag = ANatype2tag (AN_DATA_LABEL);
   printf ("\nAnnotation tag of AN_DATA_LABEL is %s\n", 
              ann_tag == DFTAG_DIL ? "DFTAG_DIL (data label)":"Incorrect");

   /*
   * Terminate access to the AN interface and close the HDF file.
   */
   status_32 = ANend (an_id);
   status_n = Hclose (file_id);

   /*
   * Free the space allocated for the annotation identifier list.
   */
   free (ann_list);
   return 0;
}
