
#ifndef __LOGTASTIC_H__
#define __LOGTASTIC_H__

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <map>

#include <cstdlib>
#include <csignal>
#include <ctime>
#include <sys/time.h>


///////////////////////////////////////////////

#ifndef __LOGTASTIC_VERSION__
#define __LOGTASTIC_VERSION__ "0.3"
#endif

#ifndef __LOGTASTIC_LOG_FILE_DIRECTORY__
#define __LOGTASTIC_LOG_FILE_DIRECTORY__ "./"
#endif

///////////////////////////////////////////////



///////////////////////////////////////////////
//
// IF DEFINED __DEBUG_OFF__
// THEN all debug log statements are disabled!!!
// 
////////////////////////////////////////////////

////////////////////////////////////////////////
#if defined(__DEBUG_OFF__) || defined(__ALL_LOGGING_DISABLED__)

#define DEBUG_LOG( log_messege ) do{}while(0)
#define DEBUG_STREAM             if(0) logtastic::messege( logtastic::debug )

#else

#define DEBUG_LOG( log_messege ) logtastic::log( logtastic::debug, log_messege )
#define DEBUG_STREAM             logtastic::messege( logtastic::debug )

#endif


#ifndef __ALL_LOGGING_DISABLED__
// MACRO DEFINITIONS

#define INFO_LOG( log_messege )    logtastic::log( logtastic::info, log_messege )
#define WARN_LOG( log_messege )    logtastic::log( logtastic::warn, log_messege )
#define ERROR_LOG( log_messege )   logtastic::log( logtastic::error, log_messege )
#define FAILURE_LOG( log_messege ) logtastic::log( logtastic::failure, log_messege )

#define INFO_STREAM logtastic::messege( logtastic::info )
#define WARN_STREAM logtastic::messege( logtastic::warn )
#define ERROR_STREAM logtastic::messege( logtastic::error )
#define FAILURE_STREAM logtastic::messege( logtastic::failure )

// ONLY FOR USE WITH GCC - or others that define __COUNTER__!
// #define VARIABLE_LOG( variable, count ) logtastic::recordVariable< __COUNTER__ >( #variable , variable, count )
#define VARIABLE_LOG( id, variable, count ) logtastic::recordVariable< id >( #variable , variable, count )

#define DATA_STREAM( data_num ) logtastic::messege( logtastic::data, data_num )

#else
// MACROS DISABLED
// To optimize away the while(0) statements, compile with -O1 or higher

#define INFO_LOG( log_messege )     do{}while(0)
#define WARN_LOG( log_messege )     do{}while(0)
#define ERROR_LOG( log_messege )    do{}while(0)
#define FAILURE_LOG( log_messege )  do{}while(0)

#define INFO_STREAM     if(0) logtastic::messege( logtastic::info )
#define WARN_STREAM     if(0) logtastic::messege( logtastic::warn )
#define ERROR_STREAM    if(0) logtastic::messege( logtastic::error )
#define FAILURE_STREAM  if(0) logtastic::messege( logtastic::failure )

// #define VARIABLE_LOG( variable, count ) do{}while(0)
#define VARIABLE_LOG( id, variable, count ) do{}while(0)

#define DATA_STREAM( data_num ) if(0) logtastic::messege( logtastic::data, data_num )

#endif
////////////////////////////////////////////////


namespace logtastic
{

  enum log_depth { debug, info, warn, error, failure, data };

  class messege;
  class logger;


////////////////////////////////////////////////


  class messege
  {
    private:
      int _identifier;
      log_depth _depth;
      std::stringstream _messege;

      static unsigned long int _instCount;
    public:
      messege( log_depth = logtastic::info, int = 0 );
      messege( const messege& );
      ~messege();

      template < typename T >
        messege& operator<<( const T& );
  };


////////////////////////////////////////////////


  class logger
  {
    private:
      logger( const char*, const char*, std::ostream& stream );// Constructor
      logger( const logger& ); // Copy Constructor
      logger& operator=( const logger& ) { return *this; }; // Assignment Operator
      ~logger(); // Destructor

      static logger* _theInstance; // THE INSTANCE!

      ////////////////////////////////////////////////////////////////

#ifdef __DEBUG_OFF__
      static const bool _outputDebug = false;
#else
      static const bool _outputDebug = true;
#endif

      std::string _programName;
      std::string _programVersion;
      std::list< std::ofstream* > _files;
      std::ostream& _output;

      // std::dequeue< std::string > _buffer;
      time_t _startTime;
      timeval _startClock;

      std::string getPrefix( log_depth );
      void outputAll( log_depth, std::string& );

      struct sigaction _sigAction;

      ////////////////////////////////////////////////////////////////

      static std::string _dataDirectory;
      static std::string _logDirectory;
      static std::list< std::string > _filenames;
      static std::map< int, std::ofstream* > _dataFiles;
      static bool _flushOnCall;
      static log_depth _screenDepth;
      static log_depth _variableLogDepth;
      static bool _handleSignals;
      static bool _haltOnSignal;
      static int _lastSignal;

      static void (*_userHandler)(int);

    public:

      static logger* get() { return logger::_theInstance; }
     
      // friend logger const* log();
      friend void log( log_depth, const char* );
      friend void logData( int, const char* );

      friend void init( const char*, const char*, std::ostream& );

      template < unsigned int N, typename T >
      friend void recordVariable( const char* name, T& var, unsigned long int skipNum );

      friend void stop();

      ////////////////////////////////////////////////////////////////

      friend void addLogFile( const char* );
      friend void setLogFileDirectory( const char* );
      friend void addDataFile( int, const char* );
      friend void setDataFileDirectory( const char* );
      friend void closeDataFile( int );

      friend void setFlushOnEveryCall( bool );
      friend void setPrintToScreenLimit( log_depth );
      friend bool setFormat( log_depth, const char* );
      friend bool setFormatAll( const char * );
      friend void setVariableLogDepth( log_depth );

      friend void preventSignalHandling();
      friend void preventHaltOnSignal();
      friend int signalReceived();
      friend void registerSignalHandler( void (*)(int) );

      friend void logtastic_signal_handler( int );

      ////////////////////////////////////////////////////////////////
      
      logger& Log_Statement( log_depth, const char* );
      logger& Log_Data( int, const char* );

      template < typename T >
      logger& Log_Variable( log_depth, const char*, T& );

      void flush();
  };


  ////////////////////////////////////////////////////////////////
  // Friend Function Declarations
  ////////////////////////////////////////////////////////////////

  void log( log_depth, const char* );
  void logData( int, const char* );

  void init( const char*, const char*, std::ostream& stream = std::cout );

  void stop();


  void addLogFile( const char* );
  void setLogFileDirectory( const char* );
  void addDataFile( int, const char* );
  void setDataFileDirectory( const char* );
  void closeDataFile( int );

  void setFlushOnEveryCall( bool );
  void setPrintToScreenLimit( log_depth );
  bool setFormat( log_depth, const char* );
  bool setFormatAll( const char * );
  void setVariableLogDepth( log_depth );


  ////////////////////////////////////////////////////////////////
  // Signal Handling
  ////////////////////////////////////////////////////////////////

  void preventSignalHandling();
  void preventHaltOnSignal();
  int signalReceived();
  void registerSignalHandler( void (*)(int) );

  void logtastic_signal_handler( int );


  ////////////////////////////////////////////////////////////////
  // Template Definitions
  ////////////////////////////////////////////////////////////////

  template < unsigned int N, typename T >
    void recordVariable( const char* name, T& var, unsigned long int skipNum )
    {
      static unsigned long int count = 0;
      if ( count == 0 )
      {
        std::stringstream ss;
        ss << "Monitoring variable id number : " << N << " (" << name << ") - Every " << skipNum << " steps with initial value = " << var;
        logtastic::log( logger::_variableLogDepth, ss.str().c_str() );
      }

      if ( count >= skipNum )
      {
        logtastic::logger::get()->Log_Variable( logger::_variableLogDepth, name, var );
        count = 1;
      }
      else
        ++count;
    }


  template < typename T >
    logger& logger::Log_Variable( log_depth depth, const char* varName, T& var )
    {
        std::stringstream ss;
        ss << getPrefix( depth ) << "Variable : " << varName << " = " << var << "\n";
        std::string result = ss.str();
        outputAll( depth, result );

        if ( _flushOnCall )
        {
          this->flush();
        }

        return *this;
    }


  ////////////////////////////////////////////////////////////////

  template < typename T >
    messege& messege::operator<<( const T& arg )
    {
      _messege << arg;
      return *this;
    }

}

#endif // __LOGTASTIC_H__
