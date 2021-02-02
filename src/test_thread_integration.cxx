
#include "logtastic.h"

#include <thread>
#include <chrono>

const int numThreads = 4;
const int numIterations = 1000;

void threadFunc();

int main ( int, char** )
{
  logtastic::init();
  logtastic::setLogFileDirectory( "./dat" );
  logtastic::setPrintToScreenLimit( logtastic::failure );
  logtastic::setLogFile("integration.log");
  logtastic::setMaxFileSize( 100 );
  logtastic::setMaxNumberFiles( 1000 );

  logtastic::start("Logtastic Tests", "1.0");


  ERROR_LOG( "HELLO" );
  ERROR_LOG( "HELLO" );
  ERROR_LOG( "HELLO" );

  std::thread the_threads[numThreads];

  for ( size_t i = 0; i < numThreads; ++i )
  {
    the_threads[i] = std::thread( threadFunc );
  }

  for ( size_t i = 0; i < numThreads; ++i )
  {
    the_threads[i].join();
  }


  logtastic::stop();

  return 0;
}


void threadFunc()
{
  for ( int i = 0; i < numIterations; ++i )
  {
    ERROR_STREAM << std::this_thread::get_id() << " " << i;
    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
  }
}



