#include "hdf.h"

#define  FILE_NAME        "General_Vdatas.hdf"
#define  VDATA_NAME       "Solid Particle"
#define  FIELD_NAME       "Mass"
#define  VATTR_NAME       "Site Ident"      /* name of the vdata attribute  */
#define  FATTR_NAME       "Scales"          /* name of the field attribute  */
#define  VATTR_N_VALUES   3      /* number of values in the vdata attribute */
#define  FATTR_N_VALUES   4      /* number of values in the field attribute */

int main( )
{   
   /************************* Variable declaration **************************/

   intn  status_n;      /* returned status for functions returning an intn  */
   int32 status_32,     /* returned status for functions returning an int32 */
         file_id, vdata_ref, vdata_id,
         field_index,   /* index of a field within the vdata */
         n_vdattrs,     /* number of vdata attributes */
         n_fldattrs,    /* number of field attributes */
         vdata_type,    /* to hold the type of vdata's attribute */ 
         vdata_n_values,/* to hold the number of vdata's attribute values   */ 
         vdata_size,    /* to hold the size of vdata's attribute values     */ 
         field_type,    /* to hold the type of field's attribute            */ 
         field_n_values,/* to hold the number of field's attribute values   */ 
         field_size;    /* to hold the size of field's attribute values     */
   char  vd_attr[VATTR_N_VALUES] = {'A', 'B', 'C'};/* vdata attribute values*/
   int32 fld_attr[FATTR_N_VALUES] = {2, 4, 6, 8};  /* field attribute values*/
   char  vattr_buf[VATTR_N_VALUES];     /* to hold vdata attribute's values */
   int32 fattr_buf[FATTR_N_VALUES];     /* to hold field attribute's values */
   char  vattr_name[30],                /* name of vdata attribute */
         fattr_name[30];                /* name of field attribute */

   /********************** End of variable declaration **********************/

   /*
   * Open the HDF file for writing.
   */
   file_id = Hopen (FILE_NAME, DFACC_WRITE, 0);

   /* 
   * Initialize the VS interface. 
   */
   status_n = Vstart (file_id);

   /* 
   * Get the reference number of the vdata named VDATA_NAME.
   */
   vdata_ref = VSfind (file_id, VDATA_NAME);

   /*
   * Attach to the vdata for writing. 
   */
   vdata_id = VSattach (file_id, vdata_ref, "w");

   /*
   * Attach an attribute to the vdata, i.e., indicated by the second parameter.
   */
   status_n = VSsetattr (vdata_id, _HDF_VDATA, VATTR_NAME, DFNT_CHAR,
                                                   VATTR_N_VALUES, vd_attr);

   /*
   * Get the index of the field FIELD_NAME within the vdata.
   */
   status_n = VSfindex (vdata_id, FIELD_NAME, &field_index);

   /*
   * Attach an attribute to the field field_index.
   */
   status_n = VSsetattr (vdata_id, field_index, FATTR_NAME, DFNT_INT32, 
                                                   FATTR_N_VALUES, fld_attr);

   /*
   * Get the number of attributes attached to the vdata's first 
   * field - should be 0. 
   */
   n_fldattrs = VSfnattrs (vdata_id, 0);
   printf ( "Number of attributes of the first field of the vdata: %d\n", 
             n_fldattrs);

   /*
   * Get the number of attributes attached to the field specified by 
   * field_index - should be 1.
   */
   n_fldattrs = VSfnattrs (vdata_id, field_index);
   printf ( "Number of attributes of field %s: %d\n", FIELD_NAME, n_fldattrs);

   /*
   * Get the total number of the field's and vdata's attributes - should be 2. 
   */
   n_vdattrs = VSnattrs (vdata_id);
   printf ( "Number of attributes of the vdata and its fields: %d\n", 
             n_vdattrs);

   /*
   * Get information about the vdata's first attribute, indicated
   * by the third parameter which is the index of the attribute. 
   */
   status_n = VSattrinfo (vdata_id, _HDF_VDATA, 0, vattr_name, 
                          &vdata_type, &vdata_n_values, &vdata_size);

   /*
   * Get information about the first attribute of the field specified by 
   * field_index. 
   */
   status_n = VSattrinfo (vdata_id, field_index, 0, fattr_name, &field_type, 
                          &field_n_values, &field_size);

   /*
   * Get the vdata's first attribute. 
   */
   status_n = VSgetattr (vdata_id, _HDF_VDATA, 0, vattr_buf);
   printf("Values of the vdata attribute = %c %c %c\n", vattr_buf[0],
                          vattr_buf[1], vattr_buf[2]);

   /*
   * Get the first attribute of the field specified by field_index.
   */
   status_n = VSgetattr (vdata_id, field_index, 0, fattr_buf);
   printf("Values of the field attribute = %d %d %d %d\n", fattr_buf[0], 
                          fattr_buf[1], fattr_buf[2], fattr_buf[3]);

   /*
   * Terminate access to the vdata and to the VS interface, then close 
   * the HDF file. 
   */
   status_32 = VSdetach (vdata_id);
   status_n  = Vend (file_id);
   status_32 = Hclose (file_id);
   return 0;
}
