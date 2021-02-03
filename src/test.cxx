//#define __COUNTER__ 5
#include <iostream>

#define LOGTASTIC_FUNCTION_NAME __func__
#include "logtastic.h"


void test( int )
{
  std::cout << "TESTING" << std::endl;
}


int main ( int, char** )
{
  logtastic::init();
  logtastic::setLogFileDirectory( "./dat" );

  logtastic::registerSignalHandler( SIGINT, test );
  logtastic::setHaltOnSignal( SIGINT, true );
  logtastic::setFlushOnEveryCall( true );

  logtastic::setLogFile("test.log");


  logtastic::start("Logtastic Tests", "1.0");

//  logtastic::log( logtastic::info, "main", "Hello Again!");
  logtastic::start( "HELLO!", "4000" );

  INFO_LOG( "Check out my new logging system!" );

  WARN_LOG( "A warning!" );

  ERROR_LOG( "Broke It.." );

  DEBUG_LOG( "Debugging Info Here..." );

  logtastic::setLogFile( "hello!" );

  std::cout << "HELLO WORLD!" << std::endl;

  WARN_STREAM << "HELLO!" << 4;


  for ( unsigned int i = 0; i < 1; ++i )
  {
    VARIABLE_LOG( 1, i, 10 );
    for ( int j = 0; j < 200; ++j )
    {
      VARIABLE_LOG( 2, j, 50 );
    }
  }


  while(1) {}

  logtastic::stop();

  return 0;
}
