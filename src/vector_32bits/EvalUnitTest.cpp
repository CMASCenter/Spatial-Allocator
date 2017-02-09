/***************************************************************************
                          EvalUnitTest.cpp  -  description
                             -------------------
    begin                : Fri Dec 10 2004
    copyright            : (C) 2004 by Benjamin Brunk
    email                : brunk@unc.edu
 ***************************************************************************/

/***************************************************************************
 *                                                                         
 *  Part of MIMS Spatial Allocator Unit Testing
 *
 ***************************************************************************/
#include <stream.h>
#include "EvalUnitTest.h"



extern "C"  int compileExpression(char infixExpression[]);
extern "C" double calculateExpression(double values[]);
extern "C" double evaluatePostfix(char postfix[]);
extern "C" void convertPostfix(char infix[], char postfix[]);

extern "C" char postfixExpression[];
extern "C" int numElements;
extern "C" int weightDBF(char *inFile, char *outFile);

 
CPPUNIT_TEST_SUITE_REGISTRATION( EvalUnitTest );
 

void EvalUnitTest::testCompile()
{

     cout << "\n\nTesting compileExpression a simple expression\n";
     compileExpression("(Length + Width)");  
     CPPUNIT_ASSERT(strcmp(postfixExpression, "Length Width +") == 0);
     cout << "numElements=" << numElements << "\n";
 
     cout << "\n\nTesting compileExpression with a bit more complicated one\n";
     compileExpression("(Length * Width) * (height - 50)");  
     CPPUNIT_ASSERT(strcmp(postfixExpression, "Length Width * height 50 - *") == 0);
     cout << "numElements=" << numElements << "\n";
     cout << "\n\n---===Compile tests completed===---\n\n";


}



void EvalUnitTest::testCalculate()
{
     int i;
     double values[4];
     double result;
     values[0] = 2.0;
     values[1] = 5.0;
     values[2] = 10.0;
     values[3] = 1.0;
     
     cout << "\n\nTesting calculateExpression\n";
     cout << "Evaluating: " << "(Length + Width) * Height - 1\n";
     cout << "Values:     " << "( 2.0   +  5.0 ) *  10.0  - 1\n";  
     compileExpression("(Length + Width) * Height - 1");
     result = calculateExpression(values);
     cout << "result=" << result << "\n";
     CPPUNIT_ASSERT(result == 69);



     cout << "\n\nTesting calculateExpression with different parentheses\n";
     cout << "Evaluating: " << "(Length + Width) * Height - 1\n";
     cout << "Values:     " << "( 2.0   +  5.0 ) *  9\n";  
     compileExpression("(Length + Width) * (Height - 1)");
     result = calculateExpression(values);
     cout << "result=" << result << "\n";
     CPPUNIT_ASSERT(result == 63);


     cout << "\n\nTesting calculateExpression with even more complexity\n";
     cout << "Evaluating: " << "(1/4)*x1+(1/5)*x2\n";
     cout << "Values:     " << "x1=2.0   x2=5.0\n";  
     compileExpression("(1/4)*x1+(1/5)*x2");
     result = calculateExpression(values);
     cout << "result=" << result << "\n";
     CPPUNIT_ASSERT(result == 1.5);
 
     cout << "\n\n---===Calculation tests completed===---\n\n";
}

    

void EvalUnitTest::testDBF()
{

     cout << "Testing setenv & getenv for WEIGHT_FUNCTION environment variable\n";
     setenv("WEIGHT_FUNCTION","HOUSEHOLDS + MARHH_CHD + MHH_CHILD + AGE_22_29 + AGE_3__39 + AGE_4__49 + AGE_5__64 + AGE_65_UP + MALES + FEMALES", 1);
     char *exp = getenv("WEIGHT_FUNCTION");
     cout << "Evaluating: " << exp << "\n";
     CPPUNIT_ASSERT(strcmp (exp, 
            "HOUSEHOLDS + MARHH_CHD + MHH_CHILD + AGE_22_29 + AGE_3__39 + AGE_4__49 + AGE_5__64 + AGE_65_UP + MALES + FEMALES") == 0);
     
     cout << "\n\nTesting dbf read and evaluation of data\n";
     weightDBF("../data/tn_pophous_short.dbf", "testout.txt");


     cout << "\n\nAnother dbf test and evaluation of data\n";
     setenv("WEIGHT_FUNCTION","0.5 * HOUSEHOLDS + MARHH_CHD + MHH_CHILD", 1);
     exp = getenv("WEIGHT_FUNCTION");
     cout << "Evaluating: " << exp << "\n";
     weightDBF("../data/tn_pophous_short.dbf", "testout2.txt");


     cout << "\n\n---===DBF data tests completed===---\n\n";
}

void EvalUnitTest::testDivZero()
{

     double values[4];
     double result;
     values[0] = 2.0;
     values[1] = 5.0;
     values[2] = 10.0;
     values[3] = 1.0;
     
     cout << "\n\nTesting division by zero\n";
     cout << "Evaluating: " << "X1 / 0\n";
     cout << "Values:     " << "2.0 / 0  - 1\n";  
     compileExpression("X1 / 0");
     result = calculateExpression(values);
     cout << "result=" << result << "\n";

     cout << "\n\n---===Divide by zero test completed===---\n\n";
}
