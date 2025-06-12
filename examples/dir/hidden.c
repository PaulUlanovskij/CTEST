#include "../../ctest.h"
#include <stdio.h>
CTEST(hidden){
   puts("This is a very secret test, hidden deep inside the codebase.\n\n\nAlso I have a lot of newlines.");
}

int main(){
  puts("I forgot about ctest limitations");
}
