#include "mfhdf.h"

#define FILE_NAME     "SDS.hdf"
#define SUB1_LENGTH   5
#define SUB2_LENGTH   4
#define SUB3_LENGTH1  2 
#define SUB3_LENGTH2  3 

int main()
{
   /************************* Variable declaration **************************/

   int32 sd_id, sds_id, sds_index;
   intn  status;
   int32 start[2], edges[2], stride[2];
   int32 sub1_data[SUB1_LENGTH];
   int32 sub2_data[SUB2_LENGTH];
   int32 sub3_data[SUB3_LENGTH2][SUB3_LENGTH1];
   int   i, j;

   /********************* End of variable declaration ***********************/
        
   /*
   * Open the file for reading and initialize the SD interface.
   */
   sd_id = SDstart (FILE_NAME, DFACC_READ);

   /*
   * Select the first data set.
   */ 
   sds_index = 0;
   sds_id = SDselect (sd_id, sds_index);
   /*
   *         Reading the first subset.
   *
   * Set elements of start, edges, and stride arrays to read
   * every 3rd element in the 2nd column starting at 4th row.   
   */
   start[0] = 3;   /* 4th row */
   start[1] = 1;   /* 2nd column */
   edges[0] = SUB1_LENGTH; /* SUB1_LENGTH elements are read along 2nd column*/
   edges[1] = 1;
   stride[0] = 3;  /* every 3rd element is read along 2nd column */
   stride[1] = 1;

   /*
   * Read the data from the file into sub1_data array.
   */
   status = SDreaddata (sds_id, start, stride, edges, (VOIDP)sub1_data);

   /* 
   * Print what we have just read; the following numbers should be displayed:
   *
   *             5 8 1000 14 17   
   */
   for (j = 0; j < SUB1_LENGTH; j++) printf ("%d ", sub1_data[j]);
   printf ("\n");

   /* 
   *         Reading the second subset.
   *
   * Set elements of start and edges arrays to read
   * first 4 elements of the 10th row. 
   */
   start[0] = 9;  /* 10th row  */
   start[1] = 0;  /* 1st column */
   edges[0] = 1; 
   edges[1] = SUB2_LENGTH; /* SUB2_LENGTH elements are read along 10th row */

   /*
   * Read data from the file into sub2_data array. Note that the third
   * parameter is set to NULL for contiguous reading.
   */
   status = SDreaddata (sds_id, start, NULL, edges, (VOIDP)sub2_data);

   /* 
   * Print what we have just read; the following numbers should be displayed:
   *
   *            10 1000 12 13 
   */
   for (j = 0; j < SUB2_LENGTH; j++) printf ("%d ", sub2_data[j]);
   printf ("\n");

   /* 
   *         Reading the third subset.
   *
   * Set elements of the arrays start, edges, and stride to read
   * every 6th element in the column and 4th element in the row
   * starting at 1st column, 3d row.    
   */
   start[0] = 2;  /* 3d row */
   start[1] = 0;  /* 1st column */
   edges[0] = SUB3_LENGTH2; /* SUB3_LENGTH2 elements are read along
                               each column */
   edges[1] = SUB3_LENGTH1; /* SUB3_LENGTH1 elements are read along  
                               each row */
   stride[0] = 6; /* read every 6th element along each column */
   stride[1] = 4; /* read every 4th element along each row */

   /*
   * Read the data from the file into sub3_data array.
   */
   status = SDreaddata (sds_id, start, stride, edges, (VOIDP)sub3_data);

   /* 
   * Print what we have just read; the following numbers should be displayed:
   *
   *            3 7 
   *            9 13  
   *            15 19 
   */
   for ( j = 0; j < SUB3_LENGTH2; j++ ) {
       for (i = 0; i < SUB3_LENGTH1; i++) printf ("%d ", sub3_data[j][i]);
       printf ("\n");
   }
   /*
   * Terminate access to the data set.
   */
   status = SDendaccess (sds_id);

   /*
   * Terminate access to the SD interface and close the file.
   */
   status = SDend (sd_id);

   return 0;
}
