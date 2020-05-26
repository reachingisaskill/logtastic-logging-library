
#include "logtastic.h"

#include <iomanip>
#include <ctime>


// Local definitions
#if defined __linux__

#define LOGTASTIC_SIGNAL_ID( signal ) ( signal == SIGABRT   ? 0 : \
                                        signal == SIGFPE    ? 1 : \
                                        signal == SIGILL    ? 2 : \
                                        signal == SIGINT    ? 3 : \
                                        signal == SIGSEGV   ? 4 : \
                                        signal == SIGTERM   ? 5 : \
                                        signal == SIGHUP    ? 6 : \
                                        signal == SIGQUIT   ? 7 : \
                                        signal == SIGBUS    ? 8 : \
                                        signal == SIGXCPU   ? 9 : \
                                        signal == SIGXFSZ   ? 10 : \
                                        signal == SIGUSR1   ? 11 : \
                                        signal == SIGUSR2   ? 12 : \
                                        signal == SIGPIPE   ? 13 : \
                                        signal == SIGCHLD   ? 14 : \
                                        signal == SIGTTIN   ? 15 : \
                                        signal == SIGTTOU   ? 16 : \
                                        signal == SIGALRM   ? 17 : \
                                        signal == SIGVTALRM ? 18 : \
                                        signal == SIGPROF   ? 19 : \
                                        signal == SIGSTOP   ? 20 : \
                                        signal == SIGTSTP   ? 21 : \
                                        signal == SIGCONT   ? 22 : \
                                        signal == SIGTRAP   ? 23 : \
                                        signal == SIGWINCH  ? 24 : 25 )

#else

#define LOGTASTIC_SIGNAL_ID( signal ) ( signal == SIGABRT   ? 0 : \
                                        signal == SIGFPE    ? 1 : \
                                        signal == SIGILL    ? 2 : \
                                        signal == SIGINT    ? 3 : \
                                        signal == SIGSEGV   ? 4 : \
                                        signal == SIGTERM   ? 5 : 6 )

#endif


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
    _userSignalHandlers(),
    _logDirectory( LOGTASTIC_LOG_FILE_DIRECTORY ),
    _filenames( 0, std::string("") ),
    _flushOnCall( true ),
    _screenDepth( logtastic::warn ),
    _variableLogDepth( logtastic::info ),
    _haltOnSignal()
  {
    for ( unsigned int i = 0; i < LOGTASTIC_NUMBER_SIGNALS; ++i )
    {
      _userSignalHandlers[i] = nullptr;
      _haltOnSignal[i] = false;
    }
  }


  void logger::initialise( const char* name, const char* version )
  {
    // Configure the details
    _programName = name;
    _programVersion = version;

    _startTime = std::chrono::system_clock::now();
    _startClock = std::chrono::steady_clock::now();

    // Register exiting functions
    std::atexit( stop );
    std::at_quick_exit( stop );

    // Register signal handlers
    std::signal( SIGABRT, logtastic_signal_handler ); // Abort
    std::signal( SIGFPE , logtastic_signal_handler ); // Floating Point Exception
    std::signal( SIGILL , logtastic_signal_handler ); // Illegal Instruction
    std::signal( SIGINT , logtastic_signal_handler ); // Interrupt
    std::signal( SIGSEGV, logtastic_signal_handler ); // Segmentation Violation
    std::signal( SIGTERM, logtastic_signal_handler ); // Termination

    // POSIX Signals
#ifdef __linux__
    std::signal( SIGHUP ,   logtastic_signal_handler ); // Terminal Hangup
    std::signal( SIGQUIT,   logtastic_signal_handler ); // Quit & Core dump (user error detection)
    std::signal( SIGBUS ,   logtastic_signal_handler ); // Bus Error ( Illegal Address )
    std::signal( SIGXCPU,   logtastic_signal_handler ); // CPU time exceeded
    std::signal( SIGXFSZ,   logtastic_signal_handler ); // File Size Exceeded
    std::signal( SIGUSR1,   logtastic_signal_handler ); // USER 1
    std::signal( SIGUSR2,   logtastic_signal_handler ); // USER 2
    std::signal( SIGPIPE,   logtastic_signal_handler ); // Broken Pipe
    std::signal( SIGCHLD,   logtastic_signal_handler ); // Child Process Termination/Stop
    std::signal( SIGTTIN,   logtastic_signal_handler ); // Access STDIN while in background
    std::signal( SIGTTOU,   logtastic_signal_handler ); // Using STDOUT while in background (if TOSTOP mode is set)
    std::signal( SIGALRM,   logtastic_signal_handler ); // Alarm using real or clock time
    std::signal( SIGVTALRM, logtastic_signal_handler ); // Current process Virtual Time Alarm
    std::signal( SIGPROF,   logtastic_signal_handler ); // Profiling Alarm
    std::signal( SIGSTOP,   logtastic_signal_handler ); // Stop
    std::signal( SIGTSTP,   logtastic_signal_handler ); // Polite Stop ( C-z )
    std::signal( SIGCONT,   logtastic_signal_handler ); // Program Continue
    std::signal( SIGTRAP,   logtastic_signal_handler ); // Breakpoint trapping
    std::signal( SIGWINCH,  logtastic_signal_handler ); // Window size change
    // std::signal( SIGLOST,   logtastic_signal_handler ); // Lost remote resource // NOT RECOGNISED
    // sigaction( SIGINFO, logtastic_signal_handler ); // Status request from lead process // NOT RECOGNISED
#endif


    // Make sure directories exist
    std::stringstream command;
#if defined _WIN32
    command << "md " << _logDirectory;
#elif defined __linux__
    command << "mkdir -p " << _logDirectory;
#else // Might as well try this one
    command << "mkdir " << _logDirectory;
#endif
    int mkdir_result = std::system( command.str().c_str() );

    if ( mkdir_result )
    {
      std::cerr << "Could not create and verify logfile directory.\nIs your operating system supported?" << std::endl;
      std::abort();
    }


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


    std::time_t start_time = std::chrono::system_clock::to_time_t( _startTime );
    std::tm tm = *std::localtime(&start_time);

    // Log initialisation statements
    std::stringstream outputStr;
    outputStr << "\n\tLOGTASTIC LOGGING\n";
    outputStr << "Version - " << LOGTASTIC_VERSION << "\n\n";
    outputStr << "Program Name         : " << _programName << "\n";
    outputStr << "Program Version      : " << _programVersion << "\n\n";
    outputStr << "Log File Directory   : " << _logDirectory << "\n";
    outputStr << "Logging Initialised  : " << std::put_time( &tm, "%c %Z" ) << "\n\n";

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
    _startTime()
  {
    _startTime = std::chrono::system_clock::now();
  }


  logger::~logger()
  {
    std::time_t end_time = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
    std::tm tm = *std::localtime(&end_time);

    // Close all the streams
    std::stringstream outputStr;
    outputStr << "\nEND OF PROGRAM OPERATION\n";
    outputStr << "\nLogtastic Logging completed at : " << std::put_time( &tm, "%c %Z" ) << "\n";

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

  void setHaltOnSignal( int signal, bool value )
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_haltOnSignal[LOGTASTIC_SIGNAL_ID( signal )] = value;
    else
      ERROR_LOG( "Attempted to change signal handling behaviour after logger initialisation" );
  }

  void registerSignalHandler( int signal, void (*hndl)(int) )
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_userSignalHandlers[LOGTASTIC_SIGNAL_ID( signal)] = hndl;
    else
      ERROR_LOG( "Attempted to set signal handler after logger initialisation" );
  }

  void logtastic_signal_handler( int sig )
  {
    if ( ! logger::_theInstance->_isRunning ) return;
    logger::SignalHandler handler = logger::_theInstance->_userSignalHandlers[ LOGTASTIC_SIGNAL_ID( sig ) ];
    bool kill = logger::_theInstance->_haltOnSignal[ LOGTASTIC_SIGNAL_ID( sig ) ];


    switch( sig )
    {
      case SIGABRT :
        FAILURE_LOG( "Abort Signal Received." );
        if ( handler != nullptr ) handler( sig );
        stop();
        std::exit( sig );
        break;

      case SIGFPE  :
        FAILURE_LOG( "Floating Point Exception Received." );
        if ( handler != nullptr ) handler( sig );
        stop();
        std::exit( sig );
        break;

      case SIGILL  :
        FAILURE_LOG( "Illegal Instruction Signal Received." );
        if ( handler != nullptr ) handler( sig );
        stop();
        std::exit( sig );
        break;

      case SIGSEGV :
        FAILURE_LOG( "Segmentation Violoation Signal Received." );
        if ( handler != nullptr ) handler( sig );
        stop();
        std::exit( sig );
        break;

      case SIGINT  :
        if ( kill ) 
        {
          FAILURE_LOG( "Interrupt Signal Received. Halting." );
          if ( handler != nullptr ) handler( sig );
          stop();
          std::exit( sig );
        }
        else
        {
          ERROR_LOG( "Interrupt Signal Received." );
          if ( handler != nullptr ) handler( sig );
        }
        break;

      case SIGTERM :
        if ( kill ) 
        {
          FAILURE_LOG( "Termination Signal Received. Halting." );
          if ( handler != nullptr ) handler( sig );
          stop();
          std::exit( sig );
        }
        else
        {
          ERROR_LOG( "Termination Signal Received." );
          if ( handler != nullptr ) handler( sig );
        }
        break;

#ifdef __linux__
      case SIGTSTP :
        INFO_LOG( "Stopping program execution." );
        if ( handler != nullptr ) handler( sig );
        if ( kill ) std::raise( SIGSTOP );
        break;

      case SIGCONT :
        INFO_LOG( "Resuming program execution." );
        if ( handler != nullptr ) handler( sig );
        break;

      case SIGALRM :
        WARN_STREAM << "Alarm Signal Received: " << sig;
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          FAILURE_LOG( "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

      case SIGVTALRM :
        WARN_STREAM << "Virtual Alarm Signal Received: " << sig;
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          FAILURE_LOG( "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

      case SIGPROF :
        WARN_STREAM << "Profiling Timer Expiration Signal Received: " << sig;
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          FAILURE_LOG( "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

      case SIGHUP :
        if ( kill ) 
        {
          FAILURE_LOG( "Hang Up Signal Received. Halting." );
          if ( handler != nullptr ) handler( sig );
          stop();
          std::exit( sig );
        }
        else
        {
          ERROR_LOG( "Hang Up Signal Received." );
          if ( handler != nullptr ) handler( sig );
        }
        break;

      case SIGQUIT :
        FAILURE_LOG( "Quit Signal Received." );
        if ( handler != nullptr ) handler( sig );
        stop();
        std::exit( sig );
        break;

      case SIGBUS :
        FAILURE_LOG( "Bus Error Signal Received." );
        if ( handler != nullptr ) handler( sig );
        stop();
        std::exit( sig );
        break;

      case SIGXCPU :
        FAILURE_LOG( "CPU Time Exceeded Signal Received." );
        if ( handler != nullptr ) handler( sig );
        stop();
        std::exit( sig );
        break;

      case SIGXFSZ :
        FAILURE_LOG( "File Size Exceeded Signal Received." );
        if ( handler != nullptr ) handler( sig );
        stop();
        std::exit( sig );
        break;

      case SIGUSR1 :
        INFO_LOG( "User Signal 1 Received." );
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          FAILURE_LOG( "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

      case SIGUSR2 :
        INFO_LOG( "User Signal 2 Received." );
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          FAILURE_LOG( "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

      case SIGPIPE :
        INFO_LOG( "Broken Pipe Signal Received." );
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          FAILURE_LOG( "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

      case SIGCHLD :
        INFO_LOG( "Child Process Termination Signal Received." );
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          FAILURE_LOG( "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

      case SIGTTIN :
        INFO_LOG( "Terminal Input For Background Process Signal Received." );
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          WARN_LOG( "Stopping program." );
          std::raise( SIGSTOP );
        }
        break;

      case SIGTTOU :
        INFO_LOG( "Terminal Output For Background Process Signal Received." );
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          FAILURE_LOG( "Forcing program exit on signal." );
          std::raise( SIGSTOP );
        }
        break;

      case SIGTRAP :
        INFO_LOG( "Break point reached." );
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          FAILURE_LOG( "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

      case SIGWINCH : // Window resize
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          INFO_LOG( "Window Resize Signal Received." );
          FAILURE_LOG( "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

    // case SIGSTOP : // Un catchable!
    // case SIGLOST :
    // case SIGINFO :
#endif

      default:
        FAILURE_STREAM << "Unexpected and Unknown Signal " << sig << " Received. Halting process.";
        stop();
        std::exit( sig );
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

//    std::chrono::duration<double> timediff = std::chrono::steady_clock::now() - _startClock;
//    ss << std::fixed << std::setw( 11 ) << std::setfill('0') << std::setprecision( 4 ) << timediff.count();
    std::chrono::milliseconds timediff = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() - _startClock );
    ss << std::setw( 11 ) << std::setfill('0') << timediff.count();
     
    ss << "] ";
    return ss.str();
  }
  
  void logger::outputAll( log_depth depth, std::string& str )
  {
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

