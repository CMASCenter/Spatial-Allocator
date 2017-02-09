/***************************************************************************
                          EvalUnitTest.h  -  description
                             -------------------
    begin                : Wed Nov 10 2004
    copyright            : (C) 2004 by Benjamin Brunk
    email                : brunk@unc.edu
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  Header file
 *                                                                         *
 ***************************************************************************/
#ifndef CPP_UNIT_EVAL_H
#define CPP_UNIT_EVAL_H

//#define UNIT_TEST_MAIN

#include <cppunit/extensions/HelperMacros.h>




/*
 * A test case that is designed to produce
 * example errors and failures
 *
 */

class EvalUnitTest : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( EvalUnitTest );
  CPPUNIT_TEST(testDivZero);
  CPPUNIT_TEST(testCompile);
  CPPUNIT_TEST(testCalculate);
  CPPUNIT_TEST(testDBF);
  CPPUNIT_TEST(testDivZero);
  CPPUNIT_TEST_SUITE_END();

  
protected:
     void testCompile();
     void testCalculate();
     void testDBF();
     void testDivZero();
     

public:




       //void setUp();
       //void tearDown();

 

};


#endif
