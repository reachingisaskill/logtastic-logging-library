
#include "logtastic.h"

#include <chrono>
#include <iomanip>

namespace logtastic
{

  // Forward declarations
  void logging_thread_function();

///////////////////////////////////////////////////////////////

  messege::messege( log_depth depth, int identifier ) :
    _identifier( identifier ),
    _depth( depth ),
    _messege( "" ),
    _func_name( nullptr )
  {
  }
  
  messege::messege( log_depth depth, const char* func ) :
    _identifier( 0 ),
    _depth( depth ),
    _messege( "" ),
    _func_name( func )
  {
  }

  messege::messege( const messege& mess ) :
    _identifier( mess._identifier ),
    _depth( mess._depth ),
    _messege( mess._messege.str() ),
    _func_name( mess._func_name )
  {
  }

  messege::~messege()
  {
    std::string result = _messege.str();

//    logtastic::log( _depth, _func_name, result );
    LOGTASTIC_LOG_FUNCTION( _depth, _func_name, result );
  }

  template <>
  messege& messege::operator<<( const log_depth& depth )
  {
    _depth = depth;
    return *this;
  }


///////////////////////////////////////////////////////////////
  //  Con/De-structors

  logger* logger::_theInstance( nullptr );


  logger::logger( std::ostream& stream ) :
    _isRunning( false ),
    _loggingThread(),
    _statementQueue(),
    _statementQueueMutex(),
    _stopThread( false ),
    _queueSizeWarning( 10000 ),

    _programName(),
    _programVersion(),
    _output( stream ),
    _startTime(),
    _startClock(),
    _logDirectory( LOGTASTIC_LOG_FILE_DIRECTORY ),
    _filenames( 0, std::string("") ),
    _flushOnCall( true ),
    _screenDepth( logtastic::warn ),
    _variableLogDepth( logtastic::info ),
    _handleSignals( true ),
    _haltOnSignal( true ),
    _lastSignal( 0 ),
    _userHandler( 0 )
  {
  }


  void logger::initialise( const char* name, const char* version )
  {
    // Configure the details
    _programName = name;
    _programVersion = version;

    _startTime = time(0);
    gettimeofday( &_startClock, 0 );

    // Register exiting functions
    atexit( stop );
    // at_quick_exit( logtastic::stop );

    // Register signal handlers
    if ( _handleSignals == true )
    {
      _sigAction.sa_handler = logtastic_signal_handler;
      sigemptyset( &_sigAction.sa_mask );
      _sigAction.sa_flags = SA_RESTART;

      sigaction( SIGHUP , &_sigAction, 0 ); // Terminal Hangup
      sigaction( SIGABRT, &_sigAction, 0 ); // Abort
      sigaction( SIGINT , &_sigAction, 0 ); // Interrupt
      sigaction( SIGTERM, &_sigAction, 0 ); // Termination
      sigaction( SIGQUIT, &_sigAction, 0 ); // Quit & Core dump (user error detection)

      sigaction( SIGFPE , &_sigAction, 0 ); // Floating Point Exception
      sigaction( SIGILL , &_sigAction, 0 ); // Illegal Instruction
      sigaction( SIGSEGV, &_sigAction, 0 ); // Segmentation Violation
      sigaction( SIGBUS , &_sigAction, 0 ); // Bus Error ( Illegal Address )
      sigaction( SIGXCPU, &_sigAction, 0 ); // CPU time exceeded
      sigaction( SIGXFSZ, &_sigAction, 0 ); // File Size Exceeded

      sigaction( SIGUSR1, &_sigAction, 0 ); // USER 1
      sigaction( SIGUSR2, &_sigAction, 0 ); // USER 2
      sigaction( SIGPIPE, &_sigAction, 0 ); // Broken Pipe
      // sigaction( SIGLOST, &_sigAction, 0 ); // Lost remote resource // NOT RECOGNISED

      sigaction( SIGCHLD, &_sigAction, 0 ); // Child Process Termination/Stop
      sigaction( SIGTTIN, &_sigAction, 0 ); // Access STDIN while in background
      sigaction( SIGTTOU, &_sigAction, 0 ); // Using STDOUT while in background (if TOSTOP mode is set)

      sigaction( SIGALRM, &_sigAction, 0 ); // Alarm using real or clock time
      sigaction( SIGVTALRM, &_sigAction, 0 ); // Current process Virtual Time Alarm
      sigaction( SIGPROF, &_sigAction, 0 ); // Profiling Alarm

      sigaction( SIGSTOP, &_sigAction, 0 ); // Stop
      sigaction( SIGTSTP, &_sigAction, 0 ); // Polite Stop ( C-z )
      sigaction( SIGCONT, &_sigAction, 0 ); // Program Continue
      sigaction( SIGTRAP, &_sigAction, 0 ); // Breakpoint trapping

      sigaction( SIGWINCH, &_sigAction, 0 ); // Window size change
      // sigaction( SIGINFO, &_sigAction, 0 ); // Status request from lead process // NOT RECOGNISED
    }

    // Make sure directories exist | TODO: #defines for windows machines!
    std::stringstream command;
    command << "mkdir -p " << _logDirectory;
    system( command.str().c_str() );


    // Initialise output fstreams
    for ( std::list< std::string >::iterator it = _filenames.begin(); it != _filenames.end(); ++it )
    {
      std::ofstream* ofst = new std::ofstream( it->c_str() );
      if ( ! ofst->is_open() )
      {
        std::cerr << "Could Not Open File (" << (*it) << ") For Writing - Trying again\n";
        ofst = new std::ofstream( it->c_str() );
        if ( ! ofst->is_open() )
          std::cerr << "Could Not Open File For Writing - Skipping\n";
      } 
      _files.push_back( ofst );
    }


    // Log initialisation statements
    std::stringstream outputStr;
    outputStr << "\n\tLOGTASTIC LOGGING\n";
    outputStr << "Version - " << LOGTASTIC_VERSION << "\n\n";
    outputStr << "Program Name         : " << _programName << "\n";
    outputStr << "Program Version      : " << _programVersion << "\n\n";
    outputStr << "Log File Directory   : " << _logDirectory << "\n";
    outputStr << "Logging Initialised  : " << ctime(&_startTime) << "\n\n";

    std::string result = outputStr.str();
    this->outputAll( logtastic::info, result );
    this->flush();

    // Start the logging thread
    _loggingThread = std::thread( logging_thread_function );
    _isRunning =true;
  }


  logger::logger( const logger& log ) :
    _programName( log._programName ),
    _programVersion( log._programVersion ),
    _output( log._output ),
    _startTime( time(0) )//,
    //_startClock( gettimeofday() )
  {
    gettimeofday( &_startClock, 0 );
  }


  logger::~logger()
  {
    // Reset the timer
    _startTime = time(0);

    // Close all the streams
    std::stringstream outputStr;
    outputStr << "\nEND OF PROGRAM OPERATION\n";
    outputStr << "\nLogtastic Logging completed at : " << ctime( &_startTime ) << "\n";

    std::string result = outputStr.str();

    this->outputAll( logtastic::info, result );
    this->flush();
    
    for ( std::list< std::ofstream* >::iterator it = _files.begin(); it != _files.end(); ++it )
    {
      (*it)->close();
      delete (*it);
    }
  }


  void logger::stopThread()
  {
    // Close the buffer thread
    _stopThread = true;
    _loggingThread.join();
  }


////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Friend Functions 

  void log( log_depth depth, const char* func_name, std::string string )
  {
    if ( logger::_theInstance != 0 )
    {
      logger::_theInstance->Log_Statement( depth, func_name, string );
    }
  }


  void push( log_depth depth, const char* function, std::string text )
  {
    logger* theLogger = logger::get();
    if ( theLogger == nullptr ) return;
    statement st;
    st.depth = depth;
    st.function = function;
    st.text = text;

    {
      std::lock_guard<std::mutex> lock( theLogger->_statementQueueMutex );
      theLogger->_statementQueue.push( st );
    }
  }


  void init()
  {
    if ( logger::_theInstance == nullptr )
    {
      logger::_theInstance = new logger();
    }
  }

  void start( const char* name, const char* version )
  {
    if ( logger::_theInstance == 0 )
    {
      logger::_theInstance = new logger();
    }

    if ( ! logger::_theInstance->_isRunning )
    {
      logger::_theInstance->initialise( name, version );
    }
    else
    {
      ERROR_LOG( "Trying to start logging while it is already running." );
    }
  }

  void stop()
  {
    if ( logger::_theInstance != 0 )
    {
      // Stop the thread before we destory the object
      logger::_theInstance->stopThread();

      delete logger::_theInstance;
      logger::_theInstance = 0;
    }
  }


  void addLogFile( const char* fileName )
  {
    std::string fileString = logger::_theInstance->_logDirectory + "/" + fileName;
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_filenames.push_back( fileString );
    else
      ERROR_LOG( "Attempted to add another output file after logger initialisation" );
  }

  void setLogFileDirectory( const char* directoryName )
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_logDirectory = directoryName;
    else
      ERROR_LOG( "Attempted to change the Log File Directory after initialisation" );
  }

  void setQueueSizeWarning( size_t limit )
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_queueSizeWarning = limit;
    else
      ERROR_LOG( "Attempted to change the queue size limit after initialisation" );
  }

  void setFlushOnEveryCall( bool flush )
  {
    logger::_theInstance->_flushOnCall = flush;
  }
  
  void setPrintToScreenLimit( log_depth depth )
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_screenDepth = depth;
    else
      ERROR_LOG( "Attempted to change logging setting (Print to terminal depth) after logger initialisation" );
  }

  bool setFormat( log_depth, const char* )
  {
    std::cout << "FUNCTION NOT YET IMPLEMENTED!" << std::endl;
    return false;
  }

  bool setFormatAll( const char * )
  {
    std::cout << "FUNCTION NOT YET IMPLEMENTED!" << std::endl;
    return false;
  }

  void setVariableLogDepth( log_depth depth )
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_variableLogDepth = depth;
    else
      ERROR_LOG( "Attempted to change logging setting (Variable log depth) after logger initialisation" );
  }


  // Signal Handling

  void preventSignalHandling()
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_handleSignals = false;
    else
      ERROR_LOG( "Attempted to change signal handling behaviour after logger initialisation" );
  }

  void preventHaltOnSignal()
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_haltOnSignal = false;
    else
      ERROR_LOG( "Attempted to change signal handling behaviour after logger initialisation" );
  }

  int signalReceived()
  {
    if ( logger::_theInstance->_isRunning == false )
      return 0;
    else
      return logger::_theInstance->_lastSignal;
  }

  void registerSignalHandler( void (*hndl)(int) )
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_userHandler = hndl;
    else
      ERROR_LOG( "Attempted to set signal handler after logger initialisation" );
  }

  void logtastic_signal_handler( int sig )
  {
    if ( ! logger::_theInstance->_isRunning ) return;

    logger::_theInstance->_lastSignal = sig;
    if ( logger::_theInstance->_userHandler != 0 ) logger::_theInstance->_userHandler( sig );

    switch( sig )
    {
      case SIGABRT :
        FAILURE_STREAM << "Program aborted internally.";
        stop();
        exit( sig );
        break;
      case SIGHUP  :
      case SIGINT  :
      case SIGTERM :
      case SIGQUIT :
        FAILURE_STREAM << "Signal " << sig << " Received. Forcing Program Stop";
        stop();
        exit( sig );
        break;
      case SIGFPE  :
      case SIGILL  :
      case SIGSEGV :
      case SIGBUS  :
      case SIGXCPU :
      case SIGXFSZ :
        ERROR_STREAM << "A fatal error has occured in program operation.";
        FAILURE_STREAM << "Signal " << sig << " Recieved. Forcing Program Stop";
        stop();
        exit( sig );
        break;
      case SIGUSR1 :
      case SIGUSR2 :
      case SIGPIPE :
      // case SIGLOST :
      case SIGCHLD :
      case SIGTTIN :
      case SIGTTOU :
        ERROR_STREAM << "Signal " << sig << " Recieved.";
        if ( logger::_theInstance->_haltOnSignal ) 
        {
          FAILURE_LOG( "Forcing Program Stop" );
          stop();
          exit( sig );
        }
        break;

      case SIGALRM :
      case SIGVTALRM :
      case SIGPROF :
        WARN_STREAM << "Alarm Signal Recieved, signal " << sig;
        break;

      case SIGSTOP :
      case SIGTSTP :
        WARN_LOG( "Program Operation Stopped" );
        raise( SIGSTOP );
        break;

      case SIGCONT :
        WARN_LOG( "Program Operation Resumed" );
        break;

      case SIGTRAP :
        ERROR_LOG( "Breakpoint Signal Received" );
        if ( logger::_theInstance->_haltOnSignal )
        {
          stop();
          exit( sig );
        }
        break;

      case SIGWINCH :
        INFO_LOG( "Window Size Changed" );
        break;

//      case SIGINFO :
//        INFO_LOG( "Request for Status signal received" ); 
//        break;

      default:
        FAILURE_STREAM << "Unexpected Signal " << sig << " Received";
        if ( logger::_theInstance->_haltOnSignal )
        {
          stop();
          exit( sig );
        }
        break;
    }
  }


  //  Member Functions 
  
  std::string logger::getPrefix( log_depth depth )
  {
    static std::stringstream ss;
    ss.str("");
    ss << "[";
    switch (depth)
    {
      case debug :
        ss << "DEBUG ";
        break;
      case info :
        ss << "INFO  ";
        break;
      case warn :
        ss << "WARN  ";
        break;
      case failure:
        ss << "FAIL  ";
        break;

      case error :
      default:
        ss << "ERROR ";
        break;
    }
    ss << "- ";
    // USE INSTRUCTIONS HERE!

    timeval timeDiff;
    gettimeofday( &timeDiff, 0 );
//    ss << std::setw(8) << std::setfill('0') << (unsigned long int)( timeDiff.tv_sec - _startClock.tv_sec );// << std::setw(3) << ( timeDiff.tv_usec - _startClock.tv_usec ); //time(0);
//    ss << std::setfill('0') << std::setw(3) << (unsigned long int)( ( ( timeDiff.tv_usec - _startClock.tv_usec ) / 1000 ) % 1000 );
    ss << std::setw( 11 ) << std::setfill('0') << (unsigned long int)( timeDiff.tv_sec - _startClock.tv_sec ) * 1000 + ( timeDiff.tv_usec - _startClock.tv_usec ) / 1000;
     
    ss << "] ";
    return ss.str();
  }
  
  void logger::outputAll( log_depth depth, std::string& str )
  {
    if ( (!_outputDebug) && ( depth == logtastic::debug ) ) return;

    if ( depth >= _screenDepth ) 
    {
      _output << str;
    }

    for ( std::list< std::ofstream* >::iterator it = _files.begin(); it != _files.end(); ++it )
    {
      *(*it) << str;
    }
  }
  

  void logger::flush()
  {
    _output.flush();
    //return;
    for ( std::list< std::ofstream* >::iterator it = _files.begin(); it != _files.end(); ++it )
    {
      (*it)->flush();
    }
  }


  logger& logger::Log_Statement( log_depth depth, const char* func_name, std::string string )
  {
    std::stringstream ss;
    ss << getPrefix( depth ) << func_name << " : " << string << "\n";

    std::string result = ss.str();
    outputAll( depth, result );

    if ( _flushOnCall )
    {
      this->flush();
    }

    return *this;
  }


////////////////////////////////////////////////////////////////////////////////
  // The logging thread.

  bool notify_check()
  {
    static logger* theLogger = logger::get();
    std::lock_guard< std::mutex > statementQueueLock( theLogger->_statementQueueMutex );
    bool answer = theLogger->_statementQueue.size() > 0;
    return answer;
  }


  void logging_thread_function()
  {
    static logger* theLogger = logger::get();
    std::unique_lock< std::mutex > lock( theLogger->_statementQueueMutex, std::defer_lock );
    statement current_statement;
    size_t number = 0;
    size_t warning_limit = theLogger->_queueSizeWarning;
    size_t throttling_limit = 0.8*theLogger->_queueSizeWarning;

    lock.lock();
    number = theLogger->_statementQueue.size();
    lock.unlock();

//    while ( (! logger::_theInstance->_stopThread) ||  notify_check() )
    do
    {
      if ( number > warning_limit )
      {
        lock.lock(); // Block the submitting threads - reduce queue size to more sensible level

        std::stringstream ss;
        ss << "Logging buffer size: " << number << ", exceeding limits, throttling to 80%";
        theLogger->Log_Statement( warn, "logging_thread_function", ss.str() );

        while ( number > throttling_limit )
        {
          current_statement = theLogger->_statementQueue.front();
          theLogger->_statementQueue.pop();

          theLogger->Log_Statement( current_statement.depth, current_statement.function, current_statement.text );
          --number;
        }
        lock.unlock();
      }
      else if ( number > 0 )
      {
        lock.lock();
        current_statement = theLogger->_statementQueue.front();
        theLogger->_statementQueue.pop();
        lock.unlock();

        theLogger->Log_Statement( current_statement.depth, current_statement.function, current_statement.text );
      }
      else
      {
        std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
      }

      lock.lock();
      number = theLogger->_statementQueue.size();
      lock.unlock();
    }
    while ( (! logger::_theInstance->_stopThread) || ( number > 0 ) );

    // Empty what's left
    lock.lock();
    while ( ! theLogger->_statementQueue.empty() )
    {
      current_statement = theLogger->_statementQueue.front();
      theLogger->_statementQueue.pop();

      theLogger->Log_Statement( current_statement.depth, current_statement.function, current_statement.text );
    }
    lock.unlock();
  }

} // logtastic

