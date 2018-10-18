#include "hdf.h"

#define  FILE_NAME         "General_Vgroups.hdf"
#define  N_RECORDS         30       /* number of records in the vdatas */
#define  ORDER             3        /* order of field FIELD_VD2 */
#define  VG_NAME           "Vertices"
#define  VG_CLASS          "Vertex Set"
#define  VD1_NAME          "X,Y Coordinates"   /* first vdata to hold X,Y...*/
#define  VD1_CLASS         "Position"          /*...values of the vertices */
#define  VD2_NAME          "Temperature"       /* second vdata to hold the...*/
#define  VD2_CLASS         "Property List"     /*...temperature field */
#define  VD3_NAME          "Node List"         /* third vdata to hold...*/
#define  VD3_CLASS         "Mesh"              /*...the list of nodes */
#define  FIELD1_VD1        "PX"    /* first field of first vdata - X values */
#define  FIELD2_VD1        "PY"/* second field of first vdata - Y values */
#define  FIELD_VD2         "TMP"/* field of third vdata */
#define  FIELD_VD3         "PLIST"/* field of second vdata */
#define  FIELDNAME_LIST    "PX,PY" /* field name list for first vdata */
/* Note that the second and third vdatas can use the field names as 
   the field name lists unless more fields are added to a vdata.
   Then a field name list is needed for that vdata */

int main( )
{
   /************************* Variable declaration **************************/

   intn     status_n;   /* returned status for functions returning an intn  */
   int32    status_32,  /* returned status for functions returning an int32 */
            file_id, vgroup_id, 
            vdata1_id, vdata2_id, vdata3_id; 
   int32    num_of_records,           /* number of records actually written */
            vd_index;                 /* position of a vdata in the vgroup  */
   int8     i, j, k = 0;
   float32  pxy[N_RECORDS][2] =       /* buffer for data of the first vdata */
		  	{-1.5, 2.3, -1.5, 1.98, -2.4, .67,
   			-3.4, 1.46, -.65, 3.1, -.62, 1.23,
   			-.4, 3.8, -3.55, 2.3, -1.43, 2.44,
   			.23, 1.13, -1.4, 5.43, -1.4, 5.8,
   			-3.4, 3.85, -.55, .3, -.21, 1.22,
   			-1.44, 1.9, -1.4, 2.8, .94, 1.78,
   			-.4, 2.32, -.87, 1.99, -.54, 4.11,
   			-1.5, 1.35, -1.4, 2.21, -.22, 1.8,
   			-1.1, 4.55, -.44, .54, -1.11, 3.93,
   			-.76, 1.9, -2.34, 1.7, -2.2, 1.21};
   float32  tmp[N_RECORDS];          /* buffer for data of the second vdata */
   int16    plist[N_RECORDS][3];     /* buffer for data of the third vdata */

   /********************** End of variable declaration ***********************/

   /*
   * Open the HDF file for writing.
   */
   file_id = Hopen (FILE_NAME, DFACC_WRITE, 0);

   /*
   * Initialize the V interface.
   */
   status_n = Vstart (file_id);

   /*
   * Buffer the data for the second and third vdatas.
   */
   for (i = 0; i < N_RECORDS; i++)
      for (j = 0; j < ORDER; j++)
         plist[i][j] = ++k;

   for (i = 0; i < N_RECORDS; i++)
      tmp[i] = i * 10.0;

   /*
   * Create the vgroup then set its name and class.  Note that the vgroup's
   * reference number is set to -1 for creating and the access mode is "w" for
   * writing.
   */
   vgroup_id = Vattach (file_id, -1, "w");
   status_32 = Vsetname (vgroup_id, VG_NAME);
   status_32 = Vsetclass (vgroup_id, VG_CLASS);

   /*
   * Create the first vdata then set its name and class. Note that the vdata's
   * reference number is set to -1 for creating and the access mode is "w" for
   * writing.
   */
   vdata1_id = VSattach (file_id, -1, "w");
   status_32 = VSsetname (vdata1_id, VD1_NAME);
   status_32 = VSsetclass (vdata1_id, VD1_CLASS);

   /*
   * Introduce and define the fields of the first vdata.
   */
   status_n = VSfdefine (vdata1_id, FIELD1_VD1, DFNT_FLOAT32, 1);
   status_n = VSfdefine (vdata1_id, FIELD2_VD1, DFNT_FLOAT32, 1);
   status_n = VSsetfields (vdata1_id, FIELDNAME_LIST);

   /*
   * Write the buffered data into the first vdata with full interlace mode.
   */
   num_of_records = VSwrite (vdata1_id, (uint8 *)pxy, N_RECORDS, 
                             FULL_INTERLACE);

   /*
   * Insert the vdata into the vgroup using its identifier.
   */
   vd_index = Vinsert (vgroup_id, vdata1_id);

   /*
   * Detach from the first vdata.
   */
   status_32 = VSdetach (vdata1_id);

   /*
   * Create, write, and insert the second vdata to the vgroup using
   * steps similar to those used for the first vdata.
   */ 
   vdata2_id = VSattach (file_id, -1, "w");
   status_32 = VSsetname (vdata2_id, VD2_NAME);
   status_32 = VSsetclass (vdata2_id, VD2_CLASS);
   status_n = VSfdefine (vdata2_id, FIELD_VD2, DFNT_FLOAT32, 1);
   status_n = VSsetfields (vdata2_id, FIELD_VD2);
   num_of_records = VSwrite (vdata2_id, (uint8 *)tmp, N_RECORDS, 
                             FULL_INTERLACE);
   vd_index = Vinsert (vgroup_id, vdata2_id);
   status_32 = VSdetach (vdata2_id);

   /*
   * Create, write, and insert the third vdata to the vgroup using 
   * steps similar to those used for the first and second vdatas.
   */
   vdata3_id = VSattach (file_id, -1, "w");
   status_32 = VSsetname (vdata3_id, VD3_NAME);
   status_32 = VSsetclass (vdata3_id, VD3_CLASS);
   status_n = VSfdefine (vdata3_id, FIELD_VD3, DFNT_INT16, 3);
   status_n = VSsetfields (vdata3_id, FIELD_VD3);
   num_of_records = VSwrite (vdata3_id, (uint8 *)plist, N_RECORDS, 
                             FULL_INTERLACE);
   vd_index = Vinsert (vgroup_id, vdata3_id);
   status_32 = VSdetach (vdata3_id);

   /*
   * Terminate access to the vgroup "Vertices".
   */
   status_32 = Vdetach (vgroup_id);

   /*
   * Terminate access to the V interface and close the HDF file.
   */
   status_n = Vend (file_id);
   status_n = Hclose (file_id);
   return 0;
}
