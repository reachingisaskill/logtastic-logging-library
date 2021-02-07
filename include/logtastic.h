
#ifndef LOGTASTIC_H_
#define LOGTASTIC_H_

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
#include <condition_variable>
#include <atomic>
#include <chrono>

#include <cstdlib>
#include <csignal>


///////////////////////////////////////////////

#define LOGTASTIC_VERSION "1.0"

///////////////////////////////////////////////
//
// IF DEFINED LOGTASTIC_DEBUG_OFF
// THEN all debug log statements are disabled!
//
// IF DEFINED LOGTASTIC_ALL_LOGGING_DISABLED
// All logging statements become null conditional expressions
// To optimize away the logging statements, compile with -O1 or higher
// 
////////////////////////////////////////////////

// Define the function name string

#if defined _WIN32 // Windows

#define LOGTASTIC_NUMBER_SIGNALS 6

#ifndef LOGTASTIC_LOG_FILE_DIRECTORY
#define LOGTASTIC_LOG_FILE_DIRECTORY ".\\"
#endif


#elif defined __linux__ // linux

#define LOGTASTIC_NUMBER_SIGNALS 25

#ifndef LOGTASTIC_LOG_FILE_DIRECTORY
#define LOGTASTIC_LOG_FILE_DIRECTORY "./"
#endif

#else // Just use the default

#define LOGTASTIC_NUMBER_SIGNALS 6

#ifndef LOGTASTIC_LOG_FILE_DIRECTORY
#define LOGTASTIC_LOG_FILE_DIRECTORY "./"
#endif

#endif

///////////////////////////////////////////////
#define LOGTASTIC_LOG_FUNCTION( d, f, t ) logtastic::push(d, f, t)
#define LOGTASTIC_VARIABLE_FUNCTION( i, n, f, v, c) logtastic::pushVariable< i >( n, f, v, c )

///////////////////////////////////////////////

#if defined(LOGTASTIC_DEBUG_OFF) || defined(LOGTASTIC_ALL_LOGGING_DISABLED)

#ifdef LOGTASTIC_FUNCTION_NAME

#define DEBUG_LOG( log_message ) do{}while(0)
#define DEBUG_STREAM             if(0) logtastic::message( logtastic::debug, LOGTASTIC_FUNCTION_NAME )

#else // Not Define LOGTASTIC_FUNCTION_NAME

#define DEBUG_LOG( func, log_message ) do{}while(0)
#define DEBUG_STREAM( func )           if(0) logtastic::message( logtastic::debug, func )

#endif // Defined LOGTASTIC_FUNCTION_NAME

#else

#ifdef LOGTASTIC_FUNCTION_NAME

#define DEBUG_LOG( log_message ) LOGTASTIC_LOG_FUNCTION( logtastic::debug, LOGTASTIC_FUNCTION_NAME, log_message )
#define DEBUG_STREAM             logtastic::message( logtastic::debug, LOGTASTIC_FUNCTION_NAME )

#else // Not Define LOGTASTIC_FUNCTION_NAME

#define DEBUG_LOG( func, log_message ) LOGTASTIC_LOG_FUNCTION( logtastic::debug, LOGTASTIC_FUNCTION_NAME, log_message )
#define DEBUG_STREAM( func )           logtastic::message( logtastic::debug, func )

#endif // Defined LOGTASTIC_FUNCTION_NAME

#endif

////////////////////////////////////////////////

#ifndef LOGTASTIC_ALL_LOGGING_DISABLED
// MACROS ENABLED

#ifdef LOGTASTIC_FUNCTION_NAME

#define INFO_LOG( log_message )    LOGTASTIC_LOG_FUNCTION( logtastic::info, LOGTASTIC_FUNCTION_NAME, log_message )
#define WARN_LOG( log_message )    LOGTASTIC_LOG_FUNCTION( logtastic::warn, LOGTASTIC_FUNCTION_NAME, log_message )
#define ERROR_LOG( log_message )   LOGTASTIC_LOG_FUNCTION( logtastic::error, LOGTASTIC_FUNCTION_NAME, log_message )
#define FAILURE_LOG( log_message ) LOGTASTIC_LOG_FUNCTION( logtastic::failure, LOGTASTIC_FUNCTION_NAME, log_message )

#define INFO_STREAM logtastic::message( logtastic::info, LOGTASTIC_FUNCTION_NAME )
#define WARN_STREAM logtastic::message( logtastic::warn, LOGTASTIC_FUNCTION_NAME )
#define ERROR_STREAM logtastic::message( logtastic::error, LOGTASTIC_FUNCTION_NAME )
#define FAILURE_STREAM logtastic::message( logtastic::failure, LOGTASTIC_FUNCTION_NAME )

#define VARIABLE_LOG( id, variable, count ) LOGTASTIC_VARIABLE_FUNCTION( id, #variable, LOGTASTIC_FUNCTION_NAME, variable, count )

#else // Not Defined LOGTASTIC_FUNCTION_NAME

#define INFO_LOG( function_name, log_message )    LOGTASTIC_LOG_FUNCTION( logtastic::info, function_name, log_message )
#define WARN_LOG( function_name, log_message )    LOGTASTIC_LOG_FUNCTION( logtastic::warn, function_name, log_message )
#define ERROR_LOG( function_name, log_message )   LOGTASTIC_LOG_FUNCTION( logtastic::error, function_name, log_message )
#define FAILURE_LOG( function_name, log_message ) LOGTASTIC_LOG_FUNCTION( logtastic::failure, function_name, log_message )

#define INFO_STREAM( function_name ) logtastic::message( logtastic::info, function_name )
#define WARN_STREAM( function_name ) logtastic::message( logtastic::warn, function_name )
#define ERROR_STREAM( function_name ) logtastic::message( logtastic::error, function_name )
#define FAILURE_STREAM( function_name ) logtastic::message( logtastic::failure, function_name )

#define VARIABLE_LOG( function_name, id, variable, count ) LOGTASTIC_VARIABLE_FUNCTION( id, #variable, function_name, variable, count )

#endif // Defined LOGTASTIC_FUNCTION_NAME

#else // Not defined LOGTASTIC_ALL_LOGGING_DISABLED
// MACROS DISABLED

#ifdef LOGTASTIC_FUNCTION_NAME

#define INFO_LOG( log_message )     do{}while(0)
#define WARN_LOG( log_message )     do{}while(0)
#define ERROR_LOG( log_message )    do{}while(0)
#define FAILURE_LOG( log_message )  do{}while(0)

#define INFO_STREAM     if(0) logtastic::message( logtastic::info, LOGTASTIC_FUNCTION_NAME )
#define WARN_STREAM     if(0) logtastic::message( logtastic::warn, LOGTASTIC_FUNCTION_NAME )
#define ERROR_STREAM    if(0) logtastic::message( logtastic::error, LOGTASTIC_FUNCTION_NAME )
#define FAILURE_STREAM  if(0) logtastic::message( logtastic::failure, LOGTASTIC_FUNCTION_NAME )

#define VARIABLE_LOG( id, variable, count ) do{}while(0)

#else // Not Defined LOGTASTIC_FUNCTION_NAME

#define INFO_LOG( func, log_message )     do{}while(0)
#define WARN_LOG( func, log_message )     do{}while(0)
#define ERROR_LOG( func, log_message )    do{}while(0)
#define FAILURE_LOG( func, log_message )  do{}while(0)

#define INFO_STREAM( function_name )     if(0) logtastic::message( logtastic::info, function_name )
#define WARN_STREAM( function_name )     if(0) logtastic::message( logtastic::warn, function_name )
#define ERROR_STREAM( function_name )    if(0) logtastic::message( logtastic::error, function_name )
#define FAILURE_STREAM( function_name )  if(0) logtastic::message( logtastic::failure, function_name )

#define VARIABLE_LOG( func, id, variable, count ) do{}while(0)

#endif // Defined LOGTASTIC_FUNCTION_NAME

#endif // Defined LOGTASTIC_ALL_LOGGING_DISABLED

////////////////////////////////////////////////


namespace logtastic
{

  enum log_depth { debug, info, warn, error, failure, off };

  class message;
  class logger;


////////////////////////////////////////////////


  // Stremer type interface
  class message
  {
    private:
      int _identifier;
      log_depth _depth;
      std::stringstream _message;
      const char* _func_name;

    public:
      message( log_depth, int );
      message( log_depth, const char* );
      message( const message& );
      ~message();

      template < typename T >
        message& operator<<( const T& );
  };


////////////////////////////////////////////////

  // Internal entry structure
  struct statement
  {
    log_depth depth;
    const char* function;
    std::string text;

    // Is a singly linked list
    std::mutex mutex;
    statement* next;
  };

////////////////////////////////////////////////


  class logger
  {
    typedef void (*SignalHandler)(int);

    private:
      logger( std::ostream& st );// Constructor

      logger( const logger& ) = delete;
      logger& operator=( const logger& ) = delete;
      logger( logger&& ) = delete;
      logger& operator=( logger&& ) = delete;

      ~logger(); // Destructor

      bool _isRunning;
      void initialise( const char*, const char* );// Setup and run

      static logger* _theInstance; // THE INSTANCE!

      ////////////////////////////////////////////////////////////////

      // Thread specific data
      std::thread _loggingThread;
      size_t _queueSizeWarning;
      unsigned long _maxFileSize;

      // Program data
      std::string _programName;
      std::string _programVersion;
      std::ostream& _output;

      // File options
      std::string _logDirectory;
      std::string _baseFilename;
      size_t _numberFiles;
      size_t _currentFileID;
      unsigned long _currentFileNumber;
      std::vector< std::string > _files;
      std::ofstream _currentFile;

      // Time data
      std::chrono::system_clock::time_point _startTime;
      std::chrono::steady_clock::time_point _startClock;

      // Signal action object
      SignalHandler _userSignalHandlers[LOGTASTIC_NUMBER_SIGNALS];

      // Options configuration
      bool _flushOnCall;
      log_depth _screenDepth;
      log_depth _variableLogDepth;
      bool _haltOnSignal[LOGTASTIC_NUMBER_SIGNALS];


      ////////////////////////////////////////////////////////////////
      // Mutexed list variables

      // Control the start and end only
      statement* _start;
      std::mutex _startMutex;
      statement* _end;
      std::mutex _endMutex;

      // Count the number of elements
      size_t _elementCount;
      std::mutex _elementCountMutex;

      // Condition variables to wait on
      std::condition_variable _waitData;

      // Stopping the thread
      bool _stopThread;


      ////////////////////////////////////////////////////////////////
      // Helper functions
      std::string getPrefix( log_depth );
      void outputAll( log_depth, std::string& );


      ////////////////////////////////////////////////////////////////
      // Functions that write to the file
      logger& Log_Statement( log_depth, const char*, std::string );

      template < typename T >
      logger& Log_Variable( log_depth, const char*, const char*, T& );

      // Prettyness in the files
      void writeIntro();
      void writeFileOutro();
      void writeOutro();
      void flush();

      // Update behaviour
      void nextFile();
      void stopThread();

      // Implement the buffer behaviour
      size_t getSize();
      // Push onto front of the buffer
      void pushStatement( statement* );

    public:

      static logger* get() { return logger::_theInstance; }
     
      ////////////////////////////////////////////////////////////////
      // Friend functions to define the logging interface
      friend void logging_thread_function();
      friend bool notify_check();

      friend void log( log_depth, const char*, std::string );

      friend void push( log_depth, const char*, std::string );

      template < unsigned int N, typename T >
      friend void pushVariable( const char* name, const char* func, T& var, unsigned long int skipNum );

      ////////////////////////////////////////////////////////////////
      // Static functions to initialise/kill the logger
      friend void init( std::ostream& );
      friend void start( const char*, const char* );
      friend void stop();

      ////////////////////////////////////////////////////////////////
      // Functions to configure the options (User should never interact directly with the "logger" object
      friend void setLogFile( const char* );
      friend void setLogFileDirectory( const char* );

      friend void setQueueSizeWarning( size_t );
      friend void setMaxFileSize( unsigned long );
      friend void setMaxNumberFiles( unsigned long size );

      friend void setFlushOnEveryCall( bool );
      friend void setPrintToScreenLimit( log_depth );
      friend void setFormat( log_depth, const char* );
      friend void setFormatAll( const char * );
      friend void setVariableLogDepth( log_depth );

      friend void setHaltOnSignal( int, bool );
      friend void registerSignalHandler( int, void (*)(int) );

      // Can't be a member function
      friend void logtastic_signal_handler( int );

  };


  ////////////////////////////////////////////////////////////////
  // Friend Function Declarations
  ////////////////////////////////////////////////////////////////

  void log( log_depth, const char*, std::string );

  void push( log_depth, const char*, std::string );

  template < unsigned int N, typename T >
  void pushVariable( const char*, const char*, T&, unsigned long int );

  void init( std::ostream& os = std::cout );
  void start( const char*, const char* );
  void stop();

  void setLogFile( const char* );
  void setLogFileDirectory( const char* );

  void setQueueSizeWarning( size_t );
  void setMaxFileSize( unsigned long );
  void setMaxNumberFiles( unsigned long size );

  void setFlushOnEveryCall( bool );
  void setPrintToScreenLimit( log_depth );
  void setFormat( log_depth, const char* );
  void setFormatAll( const char * );
  void setVariableLogDepth( log_depth );

  void setHaltOnSignal( int, bool );
  void registerSignalHandler( int, void (*)(int) );

  void logtastic_signal_handler( int );

  ////////////////////////////////////////////////////////////////
  // Template Definitions
  ////////////////////////////////////////////////////////////////

  template < unsigned int N, typename T >
  void pushVariable( const char* name, const char* function, T& var, unsigned long int skipNum )
  {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lk( mutex );

    logger* theLogger = logtastic::logger::get();
    if ( theLogger == nullptr ) return;

    static unsigned long int count = 0;

    if ( count == 0 )
    {
      std::stringstream ss;
      ss << "Monitoring variable id number : " << N << " (" << name << ") - Every " << skipNum << " steps with initial value = " << var;
      LOGTASTIC_LOG_FUNCTION( theLogger->_variableLogDepth, function, ss.str() );
    }

    if ( count >= skipNum )
    {
      std::stringstream ss;
      ss << "Variable : " << name << " = " << var;

      statement* st = new statement();
      st->depth = theLogger->_variableLogDepth;
      st->function = function;
      st->text = ss.str();

      count = 1;
      theLogger->pushStatement( st );
    }
    else
      ++count;
  }


  ////////////////////////////////////////////////////////////////
  // Message templated stream operator

  template < typename T >
  message& message::operator<<( const T& arg )
  {
    _message << arg;
    return *this;
  }

}

#endif // LOGTASTIC_H_
