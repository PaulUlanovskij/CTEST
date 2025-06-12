#include"../ctest.h"
#include <stdio.h>

CTEST(printing_test){
  puts("I love CTEST");
}

CTEST(test_destined_to_fail){
  puts("Sorry boss, I failed hard");
  return 69;
}
