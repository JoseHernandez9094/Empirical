#include <iostream>
#include <array>

#include "../../tools/assert.h"
#include "../../tools/command_line.h"
#include "../../tools/macro_math.h"
#include "../../tools/unit_tests.h"

int main(int argc, char* argv[])
{
  std::vector<std::string> args = emp::cl::args_to_strings(argc, argv);
  const bool verbose = emp::cl::use_arg(args, "-v");

  // Test converting between binary, decimal, and sum formats.
  EMP_TEST_MACRO( EMP_DEC_TO_BIN(9), "0, 0, 0, 0, 0, 0, 1, 0, 0, 1");
  EMP_TEST_MACRO( EMP_DEC_TO_BIN(91), "0, 0, 0, 1, 0, 1, 1, 0, 1, 1");
  EMP_TEST_MACRO( EMP_DEC_TO_BIN(999), "1, 1, 1, 1, 1, 0, 0, 1, 1, 1");

  EMP_TEST_MACRO( EMP_BIN_TO_DEC(0,0,0,0,0,0,1,0,1,1), "11");
  EMP_TEST_MACRO( EMP_BIN_TO_DEC(0,0,0,1,0,1,1,0,1,1), "91");
  EMP_TEST_MACRO( EMP_BIN_TO_DEC(1,0,1,0,1,0,1,0,1,0), "682");

  EMP_TEST_MACRO( EMP_BIN_TO_SUM(0,0,0,1,0,1,1,0,1,1), "0, 0, 0, 64, 0, 16, 8, 0, 2, 1");
  EMP_TEST_MACRO( EMP_DEC_TO_SUM(91), "0, 0, 0, 64, 0, 16, 8, 0, 2, 1");

  EMP_TEST_MACRO( EMP_BIN_TO_PACK(0,0,0,1,0,1,1,0,1,1), "(64, 16, 8, 2, 1)");
  EMP_TEST_MACRO( EMP_DEC_TO_PACK(91), "(64, 16, 8, 2, 1)");
  
  // Test Boolean logic
  EMP_TEST_MACRO( EMP_NOT(0), "1" );
  EMP_TEST_MACRO( EMP_NOT(EMP_NOT(0)), "0" );
  
  EMP_TEST_MACRO( EMP_EQU(0,0), "1" );
  EMP_TEST_MACRO( EMP_EQU(0,1), "0" );
  EMP_TEST_MACRO( EMP_EQU(1,0), "0" );
  EMP_TEST_MACRO( EMP_EQU(1,1), "1" );

  EMP_TEST_MACRO( EMP_BIT_LESS(0,0), "0" );
  EMP_TEST_MACRO( EMP_BIT_LESS(0,1), "1" );
  EMP_TEST_MACRO( EMP_BIT_LESS(1,0), "0" );
  EMP_TEST_MACRO( EMP_BIT_LESS(1,1), "0" );

  EMP_TEST_MACRO( EMP_BIT_GTR(0,0), "0" );
  EMP_TEST_MACRO( EMP_BIT_GTR(0,1), "0" );
  EMP_TEST_MACRO( EMP_BIT_GTR(1,0), "1" );
  EMP_TEST_MACRO( EMP_BIT_GTR(1,1), "0" );


  
  // Test other helper math functions.
  EMP_TEST_MACRO( EMP_MATH_VAL_TIMES_0(222), "0" );
  EMP_TEST_MACRO( EMP_MATH_VAL_TIMES_1(222), "222" );

  EMP_TEST_MACRO( EMP_MATH_BIN_TIMES_0(0,0,1,0,1,0,1,0,1,0), "0, 0, 0, 0, 0, 0, 0, 0, 0, 0" );
  EMP_TEST_MACRO( EMP_MATH_BIN_TIMES_1(0,0,1,0,1,0,1,0,1,0), "0, 0, 1, 0, 1, 0, 1, 0, 1, 0" );

  EMP_TEST_MACRO( EMP_MATH_COUNT_BITS(1, 1), "2");
  EMP_TEST_MACRO( EMP_MATH_COUNT_BITS(1, N), "0");
  EMP_TEST_MACRO( EMP_MATH_COUNT_BITS(0, N), "N");

  EMP_TEST_MACRO( EMP_MATH_GET_CARRY(2), "1");
  EMP_TEST_MACRO( EMP_MATH_CLEAR_CARRY(2), "0");
  
  // Now in combination...
  EMP_TEST_MACRO( EMP_MATH_COUNT_BITS(EMP_MATH_CLEAR_CARRY(1), EMP_MATH_GET_CARRY(2)), "2" );

  // Basic Addition...
  EMP_TEST_MACRO( EMP_ADD(1, 2), "3");
  EMP_TEST_MACRO( EMP_ADD(5, 5), "10");
  EMP_TEST_MACRO( EMP_ADD(7, 7), "14");
  EMP_TEST_MACRO( EMP_ADD(111, 112), "223");
  EMP_TEST_MACRO( EMP_ADD(127, 1), "128");
  EMP_TEST_MACRO( EMP_ADD(123, 789), "912");
  EMP_TEST_MACRO( EMP_ADD(1023, 1), "0");      // Overflow

  // Basic Subtraction...
  EMP_TEST_MACRO( EMP_SUB(10, 7), "3");
  EMP_TEST_MACRO( EMP_SUB(128, 1), "127");
  EMP_TEST_MACRO( EMP_SUB(250, 250), "0");
  EMP_TEST_MACRO( EMP_SUB(250, 100), "150");
  EMP_TEST_MACRO( EMP_SUB(91, 66), "25");
  EMP_TEST_MACRO( EMP_SUB(99, 100), "1023");   // Underflow

  // Combination of add and sub
  EMP_TEST_MACRO( EMP_ADD( EMP_SUB(250, 100), EMP_SUB(91, 66)), "175");

  // Shifting
  EMP_TEST_MACRO( EMP_SHIFTL(17), "34");
  EMP_TEST_MACRO( EMP_SHIFTL(111), "222");  
  EMP_TEST_MACRO( EMP_SHIFTL(444), "888");  
  EMP_TEST_MACRO( EMP_SHIFTL(1023), "1022");   // Overflow...

  EMP_TEST_MACRO( EMP_SHIFTR(100), "50");
  EMP_TEST_MACRO( EMP_SHIFTR(151), "75");

  // Inc, dec, half...
  EMP_TEST_MACRO( EMP_INC(20), "21");
  EMP_TEST_MACRO( EMP_INC(55), "56");
  EMP_TEST_MACRO( EMP_INC(63), "64");
  EMP_TEST_MACRO( EMP_INC(801), "802");

  EMP_TEST_MACRO( EMP_DEC(20), "19");
  EMP_TEST_MACRO( EMP_DEC(55), "54");
  EMP_TEST_MACRO( EMP_DEC(63), "62");
  EMP_TEST_MACRO( EMP_DEC(900), "899");

  EMP_TEST_MACRO( EMP_HALF(17), "8");
  EMP_TEST_MACRO( EMP_HALF(18), "9");
  EMP_TEST_MACRO( EMP_HALF(60), "30");
  EMP_TEST_MACRO( EMP_HALF(1001), "500");

  // Multiply!
  EMP_TEST_MACRO( EMP_MULT(1, 1), "1");
  EMP_TEST_MACRO( EMP_MULT(200, 0), "0");
  EMP_TEST_MACRO( EMP_MULT(201, 1), "201");
  EMP_TEST_MACRO( EMP_MULT(10, 7), "70");
  EMP_TEST_MACRO( EMP_MULT(25, 9), "225");
  EMP_TEST_MACRO( EMP_MULT(65, 3), "195");
  EMP_TEST_MACRO( EMP_MULT(65, 15), "975");


  if (verbose) std::cout << "All tests passed." << std::endl;
}
