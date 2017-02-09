/***************************************************************************
                          ParseAttributeUnitTest.h  -  description
                             -------------------
    begin                : Wed Nov 10 2004
    copyright            : (C) 2004 by Benjamin Brunk
    email                : brunk@unc.edu
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  Header file for unit_test.cpp
 *                                                                         *
 ***************************************************************************/
#ifndef CPP_UNIT_PARSE_ATTRIBUTE_H
#define CPP_UNIT_PARSE_ATTRIBUTE_H

//#define UNIT_TEST_MAIN

#include <fstream.h>
#include "cppunit/extensions/HelperMacros.h"



/*
 * A test case that is designed to produce
 * example errors and failures
 *
 */

class ParseAttributeUnitTest : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( ParseAttributeUnitTest );
  CPPUNIT_TEST( filterFileTest );
  CPPUNIT_TEST( filterAttribTest );
  CPPUNIT_TEST( discreteTest );
  CPPUNIT_TEST( rangeTest );
  CPPUNIT_TEST_SUITE_END();

  
protected:
      void filterFileTest();
      void filterAttribTest();
      void discreteTest();
      void rangeTest();

public:




 

};


#endif
