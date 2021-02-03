
#include "logtastic.h"

#include <thread>
#include <chrono>

const int numThreads = 3;
const int numIterations = 1000000;

void threadFunc();

int main ( int, char** )
{
  logtastic::init();
  logtastic::setLogFileDirectory( "./dat" );
  logtastic::setPrintToScreenLimit( logtastic::warn );
  logtastic::setLogFile("integration.log");
  logtastic::setMaxFileSize( 100000 );
  logtastic::setMaxNumberFiles( 99 );

  logtastic::start("Logtastic Tests", "1.0");


  ERROR_LOG( "intro", "HELLO" );
  ERROR_LOG( "intro", "HELLO" );
  ERROR_LOG( "intro", "HELLO" );

  std::thread the_threads[numThreads];

  std::cout << "Starting threads" << std::endl;
  for ( size_t i = 0; i < numThreads; ++i )
  {
    the_threads[i] = std::thread( threadFunc );
  }

  std::cout << "Joining threads" << std::endl;
  for ( size_t i = 0; i < numThreads; ++i )
  {
    the_threads[i].join();
  }


  std::cout << "Stopping logtastic" << std::endl;
  logtastic::stop();

  return 0;
}


void threadFunc()
{
  for ( int i = 0; i < numIterations; ++i )
  {
    INFO_STREAM( "test_thread" ) << std::this_thread::get_id() << " " << i;
//    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
  }
}



