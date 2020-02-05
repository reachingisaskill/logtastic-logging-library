
#include <iostream>
//#include "logtastic_check.h"

#define __DEBUG_OFF__
#include "logtastic.h"

int main ( int, char** )
{

  ERROR_LOG( "TEST!" );

  ERROR_STREAM << "some bullcrap here...";

  std::cout << "\nHello World!\n" << std::endl;
  return 0;
}
