
#ifndef __LOGTASTIC_H__
#define __LOGTASTIC_H__

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <list>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>

#include <cstdlib>
#include <csignal>
#include <ctime>
#include <sys/time.h>


///////////////////////////////////////////////

#ifndef __LOGTASTIC_VERSION__
#define __LOGTASTIC_VERSION__ "0.5"
#endif

#ifndef __LOGTASTIC_LOG_FILE_DIRECTORY__
#define __LOGTASTIC_LOG_FILE_DIRECTORY__ "./"
#endif

///////////////////////////////////////////////
//
// IF DEFINED __DEBUG_OFF__
// THEN all debug log statements are disabled!
// 
////////////////////////////////////////////////

// Define the function name string
#ifndef LOGTASTIC_FUNCTION_NAME

#ifdef WIN32 // Windows
#define LOGTASTIC_FUNCTION_NAME __FUNCTION__
#else // *nix
#define LOGTASTIC_FUNCTION_NAME __func__
#endif

#endif

///////////////////////////////////////////////
#define LOGGER_LOG_FUNCTION( d, f, t ) logtastic::push(d, f, t)
#define LOGGER_VARIABLE_FUNCTION( i, n, f, v, c) logtastic::pushVariable< i >( n, f, v, c )
//#define LOGGER_LOG_FUNCTION( d, f, t ) logtastic::log( d, f, t )
//#define LOGGER_VARIABLE_FUNCTION( i, n, f, v, c) logtastic::recordVariable< i >( n, f, v, c )

///////////////////////////////////////////////

#if defined(__DEBUG_OFF__) || defined(__ALL_LOGGING_DISABLED__)

#define DEBUG_LOG( log_messege ) do{}while(0)
#define DEBUG_STREAM             if(0) logtastic::messege( logtastic::debug, LOGTASTIC_FUNCTION_NAME )

#else

#define DEBUG_LOG( log_messege ) LOGGER_LOG_FUNCTION( logtastic::debug, LOGTASTIC_FUNCTION_NAME, log_messege )
#define DEBUG_STREAM             logtastic::messege( logtastic::debug, LOGTASTIC_FUNCTION_NAME )

#endif

////////////////////////////////////////////////

#ifndef __ALL_LOGGING_DISABLED__
// MACROS ENABLED

#define INFO_LOG( log_messege )    LOGGER_LOG_FUNCTION( logtastic::info, LOGTASTIC_FUNCTION_NAME, log_messege )
#define WARN_LOG( log_messege )    LOGGER_LOG_FUNCTION( logtastic::warn, LOGTASTIC_FUNCTION_NAME, log_messege )
#define ERROR_LOG( log_messege )   LOGGER_LOG_FUNCTION( logtastic::error, LOGTASTIC_FUNCTION_NAME, log_messege )
#define FAILURE_LOG( log_messege ) LOGGER_LOG_FUNCTION( logtastic::failure, LOGTASTIC_FUNCTION_NAME, log_messege )

#define INFO_STREAM logtastic::messege( logtastic::info, LOGTASTIC_FUNCTION_NAME )
#define WARN_STREAM logtastic::messege( logtastic::warn, LOGTASTIC_FUNCTION_NAME )
#define ERROR_STREAM logtastic::messege( logtastic::error, LOGTASTIC_FUNCTION_NAME )
#define FAILURE_STREAM logtastic::messege( logtastic::failure, LOGTASTIC_FUNCTION_NAME )

#define VARIABLE_LOG( id, variable, count ) LOGGER_VARIABLE_FUNCTION( id, #variable, LOGTASTIC_FUNCTION_NAME, variable, count )

#else
// MACROS DISABLED
// To optimize away the while(0) statements, compile with -O1 or higher

#define INFO_LOG( log_messege )     do{}while(0)
#define WARN_LOG( log_messege )     do{}while(0)
#define ERROR_LOG( log_messege )    do{}while(0)
#define FAILURE_LOG( log_messege )  do{}while(0)

#define INFO_STREAM     if(0) logtastic::messege( logtastic::info, LOGTASTIC_FUNCTION_NAME )
#define WARN_STREAM     if(0) logtastic::messege( logtastic::warn, LOGTASTIC_FUNCTION_NAME )
#define ERROR_STREAM    if(0) logtastic::messege( logtastic::error, LOGTASTIC_FUNCTION_NAME )
#define FAILURE_STREAM  if(0) logtastic::messege( logtastic::failure, LOGTASTIC_FUNCTION_NAME )

#define VARIABLE_LOG( id, variable, count ) do{}while(0)

#endif

////////////////////////////////////////////////


namespace logtastic
{

  enum log_depth { debug, info, warn, error, failure };

  class messege;
  class logger;


////////////////////////////////////////////////


  class messege
  {
    private:
      int _identifier;
      log_depth _depth;
      std::stringstream _messege;
      const char* _func_name;

    public:
      messege( log_depth, int );
      messege( log_depth, const char* );
      messege( const messege& );
      ~messege();

      template < typename T >
        messege& operator<<( const T& );
  };


////////////////////////////////////////////////

  struct statement
  {
    log_depth depth;
    const char* function;
    int id;
    std::string text;
  };

////////////////////////////////////////////////


  class logger
  {
    private:
      logger( std::ostream& st = std::cout );// Constructor
      logger( const logger& ); // Copy Constructor
      logger& operator=( const logger& ) { return *this; }; // Assignment Operator
      ~logger(); // Destructor

      bool _isRunning;
      void initialise( const char*, const char* );// Setup and run

      static logger* _theInstance; // THE INSTANCE!

      ////////////////////////////////////////////////////////////////

#ifdef __DEBUG_OFF__
      static const bool _outputDebug = false;
#else
      static const bool _outputDebug = true;
#endif

      // Thread specific data
      std::thread _loggingThread;
      std::queue< statement > _statementQueue;
      std::mutex _statementQueueMutex;
      std::atomic_bool _stopThread;
      size_t _queueSizeWarning;

      // Program data
      std::string _programName;
      std::string _programVersion;
      std::list< std::ofstream* > _files;
      std::ostream& _output;

      // Time data
      time_t _startTime;
      timeval _startClock;

      // Helper functions
      std::string getPrefix( log_depth );
      void outputAll( log_depth, std::string& );

      // Signal action object
      struct sigaction _sigAction;

      // Options configuration
      std::string _logDirectory;
      std::list< std::string > _filenames;
      bool _flushOnCall;
      log_depth _screenDepth;
      log_depth _variableLogDepth;
      bool _handleSignals;
      bool _haltOnSignal;
      int _lastSignal;

      void (*_userHandler)(int);


      ////////////////////////////////////////////////////////////////
      // Functions that do the actual logging - only accessible through the static functions provided
      logger& Log_Statement( log_depth, const char*, std::string );

      template < typename T >
      logger& Log_Variable( log_depth, const char*, const char*, T& );

      void flush();

      void stopThread();

    public:

      static logger* get() { return logger::_theInstance; }
     
      ////////////////////////////////////////////////////////////////
      // Friend functions to define the logging interface
      friend void logging_thread_function();
      friend bool notify_check();

      friend void log( log_depth, const char*, std::string );

      friend void push( log_depth, const char*, std::string );

      template < unsigned int N, typename T >
      friend void recordVariable( const char* name, const char* func, T& var, unsigned long int skipNum );
      template < unsigned int N, typename T >
      friend void pushVariable( const char* name, const char* func, T& var, unsigned long int skipNum );

      ////////////////////////////////////////////////////////////////
      // Static functions to initialise/kill the logger
      friend void init();
      friend void start( const char*, const char* );
      friend void stop();

      ////////////////////////////////////////////////////////////////
      // Functions to configure the options (User should never interact directly with the "logger" object
      friend void addLogFile( const char* );
      friend void setLogFileDirectory( const char* );

      friend void setQueueSizeWarning( size_t );

      friend void setFlushOnEveryCall( bool );
      friend void setPrintToScreenLimit( log_depth );
      friend bool setFormat( log_depth, const char* );
      friend bool setFormatAll( const char * );
      friend void setVariableLogDepth( log_depth );

      friend void preventSignalHandling();
      friend void preventHaltOnSignal();
      friend int signalReceived();
      friend void registerSignalHandler( void (*)(int) );

      // Can't be a member function
      friend void logtastic_signal_handler( int );

  };


  ////////////////////////////////////////////////////////////////
  // Friend Function Declarations
  ////////////////////////////////////////////////////////////////

  void log( log_depth, const char*, std::string );

  void push( log_depth, const char*, std::string );

  template < unsigned int N, typename T >
  void recordVariable( const char*, const char*, T&, unsigned long int );
  template < unsigned int N, typename T >
  void pushVariable( const char*, const char*, T&, unsigned long int );

  void init();
  void start( const char*, const char* );
  void stop();

  void addLogFile( const char* );
  void setLogFileDirectory( const char* );

  void setQueueSizeWarning( size_t );

  void setFlushOnEveryCall( bool );
  void setPrintToScreenLimit( log_depth );
  bool setFormat( log_depth, const char* );
  bool setFormatAll( const char * );
  void setVariableLogDepth( log_depth );

  void preventSignalHandling();
  void preventHaltOnSignal();
  int signalReceived();
  void registerSignalHandler( void (*)(int) );

  void logtastic_signal_handler( int );

  ////////////////////////////////////////////////////////////////
  // Template Definitions
  ////////////////////////////////////////////////////////////////

  // Friend functions
  template < unsigned int N, typename T >
  void recordVariable( const char* name, const char* function, T& var, unsigned long int skipNum )
  {
    static unsigned long int count = 0;
    static logger* theLogger = logtastic::logger::get();
    if ( count == 0 )
    {
      std::stringstream ss;
      ss << "Monitoring variable id number : " << N << " (" << name << ") - Every " << skipNum << " steps with initial value = " << var;
//        logtastic::log( logger::_variableLogDepth, ss.str().c_str() );
      LOGGER_LOG_FUNCTION( theLogger->_variableLogDepth, function, ss.str() );
    }

    if ( count >= skipNum )
    {
      theLogger->Log_Variable( theLogger->_variableLogDepth, function, name, var );
      count = 1;
    }
    else
      ++count;
  }


  template < unsigned int N, typename T >
  void pushVariable( const char* name, const char* function, T& var, unsigned long int skipNum )
  {
    logger* theLogger = logtastic::logger::get();
    if ( theLogger == nullptr ) return;

    static unsigned long int count = 0;
    static statement st = { theLogger->_variableLogDepth, nullptr, 0, std::string() };

    if ( count == 0 )
    {
      std::stringstream ss;
      ss << "Monitoring variable id number : " << N << " (" << name << ") - Every " << skipNum << " steps with initial value = " << var;
//        logtastic::log( logger::_variableLogDepth, ss.str().c_str() );
      LOGGER_LOG_FUNCTION( theLogger->_variableLogDepth, function, ss.str() );
    }

    if ( count >= skipNum )
    {
      std::stringstream ss;
      ss << "Variable : " << name << " = " << var;
      st.function = function;
      st.text = ss.str();
      count = 1;

      std::lock_guard<std::mutex> lock( theLogger->_statementQueueMutex );
      theLogger->_statementQueue.push( st );
    }
    else
      ++count;
  }


  ////////////////////////////////////////////////////////////////
  // Member functions
  template < typename T >
  logger& logger::Log_Variable( log_depth depth, const char* function, const char* varName, T& var )
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
