
#include "logtastic.h"


int main ( int, char** )
{
  logtastic::init();
  logtastic::setLogFileDirectory( "./dat" );
  logtastic::setPrintToScreenLimit( logtastic::failure );
  logtastic::addLogFile("integration.log");

  logtastic::start("Logtastic Tests", "1.0");


  ERROR_LOG( "HELLO" );
  ERROR_LOG( "HELLO" );
  ERROR_LOG( "HELLO" );


  logtastic::stop();

  return 0;
}



