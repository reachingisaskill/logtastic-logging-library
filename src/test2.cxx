
#include <iostream>

#define __DEBUG_OFF__
#include "logtastic.h"

int main ( int, char** )
{

  ERROR_LOG( "main", "TEST!" );

  ERROR_STREAM( "main" ) << "some bullcrap here...";

  logtastic::init();
  logtastic::setLogFileDirectory( "./dat" );
  logtastic::setPrintToScreenLimit( logtastic::warn );
  logtastic::setLogFile("test2.log");
  logtastic::setMaxFileSize( 1000 );
  logtastic::setMaxNumberFiles( 2 );

  logtastic::start("Logtastic Tests", "1.0");

  for ( size_t i = 0 ; i < 10000; ++i )
  {
    INFO_LOG( "test", "Hello" );
    VARIABLE_LOG( "yo", 0, i, 100 );
  }


  logtastic::stop();
  return 0;
}
