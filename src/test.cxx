#define __COUNTER__ 5
#include <iostream>

#include "logtastic.h"


void test( int )
{
  std::cout << "TESTING" << std::endl;
}


int main ( int, char** )
{
  logtastic::setDataFileDirectory( "./dat" );
  logtastic::setLogFileDirectory( "./dat" );
  logtastic::preventHaltOnSignal();

  logtastic::registerSignalHandler( test );

  logtastic::addLogFile("test.log");


  logtastic::init("Logtastic Tests", "1.0");

  logtastic::log( logtastic::info, "Hello Again!");
  logtastic::init( "HELLO!", "4000" );

  INFO_LOG( "Check out my new logging system!" );

  WARN_LOG( "A warning!" );

  ERROR_LOG( "Broke It.." );

  DEBUG_LOG( "Debugging Info Here..." );

  logtastic::addLogFile( "hello!" );

  std::cout << "HELLO WORLD!" << std::endl;

  WARN_STREAM << "HELLO!" << 4;


  for ( unsigned int i = 0; i < 100; ++i )
  {
    VARIABLE_LOG( 1, i, 10 );
    for ( int j = 0; j < 200; ++j )
    {
      VARIABLE_LOG( 2, j, 50 );
    }
  }


  INFO_LOG( "Now testing Data File System!" );

  logtastic::addDataFile( 1, "data_test1.dat" );

  DATA_STREAM( 1 ) << 1 << "\t" << "2";

  logtastic::closeDataFile( 1 );
  logtastic::closeDataFile( 1 );

  while(1) {}

  logtastic::stop();

  return 0;
}
