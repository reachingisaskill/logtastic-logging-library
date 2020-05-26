
#include "logtastic.h"

#include <iostream>
#include <thread>


void test()
{
  for ( unsigned int i = 0; i < 100000; ++i )
  {
    INFO_LOG( "Trouble" );
  }
  std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
  for ( unsigned int i = 0; i < 100000; ++i )
  {
    INFO_LOG( "Trouble" );
  }
}


int main ( int, char** )
{
  logtastic::init();
  logtastic::setLogFileDirectory( "./dat" );
  logtastic::setPrintToScreenLimit( logtastic::failure );
  logtastic::addLogFile("test3.log");

  logtastic::start("Logtastic Tests", "1.0");


  std::thread naughty_thread( test );

  for ( unsigned int i = 0; i < 1000; ++i )
  {
    INFO_LOG( "test" );
    for ( int j = 0; j < 2000; ++j )
    {
      ERROR_LOG( "HELLO" );
    }
  }
  std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
  for ( int j = 0; j < 100000; ++j )
  {
    INFO_LOG( "HELLO" );
  }


  naughty_thread.join();

  logtastic::stop();

  return 0;
}



