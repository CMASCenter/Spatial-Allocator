/***************************************************************************
                          ParseAttributeUnitTest.cpp  -  description
                             -------------------
    begin                : Tue Nov 23 2004
    copyright            : (C) 2004 by Benjamin Brunk
    email                : brunk@unc.edu
 ***************************************************************************/

/***************************************************************************
 *                                                                         
 *  Part of MIMS Spatial Allocator Unit Testing
 *
 ***************************************************************************/
#include <stream.h>
#include <fstream.h>
#include "ParseAttributeUnitTest.h"

 extern "C"
 {
  #include "parse_weight_attributes.h"
 }

 
CPPUNIT_TEST_SUITE_REGISTRATION( ParseAttributeUnitTest );
 

void ParseAttributeUnitTest::filterFileTest()
{
     
     // NO ERRORS, NO WARNINGS
     cout << "\n\ntest #0 no errors\n";
     CPPUNIT_ASSERT(parseWeightSubsets("filters/attribs.txt") == 0);     
     cleanUp();
     
     //

     // ERRORS
     
     cout << "test #1 missing attribute name error\n";
     CPPUNIT_ASSERT(parseWeightSubsets("filters/attribs01.txt") == 1);   
     cleanUp();
     cout << "test #2 duplicate attribute name error\n";
     CPPUNIT_ASSERT(parseWeightSubsets("filters/attribs02.txt") == 2);   
     cleanUp();
     cout << "test #3 missing attribute_type error\n";
     CPPUNIT_ASSERT(parseWeightSubsets("filters/attribs03.txt") == 3);  
     cleanUp();
     cout << "test #4 multiple include_type error\n";
     CPPUNIT_ASSERT(parseWeightSubsets("filters/attribs04.txt") == 4);  
     cout << "test #5 multiple includes error\n";
     CPPUNIT_ASSERT(parseWeightSubsets("filters/attribs05.txt") == 5);   
     cout << "test #6 multiple excludes error\n";
     CPPUNIT_ASSERT(parseWeightSubsets("filters/attribs06.txt") == 6);   
     

     // WARNINGS
     cout << "test #7 missing include_values right side--warning only\n";
     CPPUNIT_ASSERT(parseWeightSubsets("filters/attribs_missing_include_warn.txt") == 0);   
     cleanUp();
     cout << "test #8 missing exclude_values right side--warning only\n";
     CPPUNIT_ASSERT(parseWeightSubsets("filters/attribs_missing_exclude_warn.txt") == 0);   
     cleanUp();
    

 
     cout << "test #10 no errors, checks use of multiple includes/excludes\n";
     CPPUNIT_ASSERT(parseWeightSubsets("filters/attribs10.txt") == 0);  
     cleanUp();
}


void ParseAttributeUnitTest::filterAttribTest()
{
     int parseReturnValue;

     cout << "\n\ntest #11 no errors, checks compilation" <<
                 "of discrete and continuous includes and excludes\n";

      parseReturnValue = parseWeightSubsets("filters/attribs.txt");
     
      CPPUNIT_ASSERT(parseAndSaveFilters() == 1);
         
     cleanUp();

}               

void ParseAttributeUnitTest::discreteTest()
{
     cout << "\n\ntest #12 no errors, checks DISCRETE attribute" <<
                 "of INCLUDE_VALUES\n";
     CPPUNIT_ASSERT(filterDBF("../data/tn_roads", "filters/road_county.txt", "output/tn_roads_12") == 1);

    

}

void ParseAttributeUnitTest::rangeTest()
{



     cout << "\n\ntest #13 no errors, checks CONTINUOUS attribute" <<
                 "of INCLUDE_VALUES\n";
 
     CPPUNIT_ASSERT(filterDBF("output/tn_roads_12", "filters/road_length_cont.txt", "output/tn_roads_13") == 1);


     cout << "\n\ntest #14 no errors, checks CONTINUOUS attribute" <<
                 "of INCLUDE_VALUES\n";

     CPPUNIT_ASSERT(filterDBF("output/tn_roads_12", "filters/road_length_cont_ex.txt", "output/tn_roads_14") == 1);

     cout << "\n\ntest #15 no errors, checks CONTINUOUS attribute" <<
                 "of INCLUDE_VALUES with big numbers\n";
     CPPUNIT_ASSERT(filterDBF("../data/tn_pophous", "filters/filter_tn_pop.txt", "output/tn_test") == 1);
     

}
    


    
