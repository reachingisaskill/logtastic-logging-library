
#include "logtastic.h"

#include <algorithm>
#include <iomanip>
#include <ctime>
#include <cmath>


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

  message::message( log_depth depth, int identifier ) :
    _identifier( identifier ),
    _depth( depth ),
    _message( "" ),
    _func_name( nullptr )
  {
  }
  
  message::message( log_depth depth, const char* func ) :
    _identifier( 0 ),
    _depth( depth ),
    _message( "" ),
    _func_name( func )
  {
  }

  message::message( const message& mess ) :
    _identifier( mess._identifier ),
    _depth( mess._depth ),
    _message( mess._message.str() ),
    _func_name( mess._func_name )
  {
  }

  message::~message()
  {
    std::string result = _message.str();

//    logtastic::log( _depth, _func_name, result );
    LOGTASTIC_LOG_FUNCTION( _depth, _func_name, result );
  }

  template <>
  message& message::operator<<( const log_depth& depth )
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
    _queueSizeWarning( 10000 ),
    _maxFileSize( 100000 ),

    _programName(),
    _programVersion(),
    _output( stream ),
    _logDirectory( LOGTASTIC_LOG_FILE_DIRECTORY ),
    _baseFilename( "logtastic.log" ),
    _numberFiles( 1 ),
    _currentFileID( 0 ),
    _currentFileNumber( 0 ),
    _files(),
    _currentFile(),

    _startTime(),
    _startClock(),

    _enableSignalHandling( false ),
    _userSignalHandlers(),
    _flushOnCall( true ),
    _screenDepth( logtastic::warn ),
    _variableLogDepth( logtastic::info ),
    _haltOnSignal(),

    _start( nullptr ),
    _end( nullptr ),
    _elementCount( 0 ),
    _stopThread( false )
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

    if ( _enableSignalHandling )
    {

      // Register exiting functions
      std::atexit( stop );
#ifdef __linux__
      at_quick_exit( stop );
#endif

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
    }

    // Default to linux style
    std::replace( _logDirectory.begin(), _logDirectory.end(), '\\', '/' );
    // Ends in a slash
    if ( _logDirectory.back() != '/' )
      _logDirectory.push_back( '/' );

    // Make sure directories exist
    int mkdir_result;
    std::stringstream command;
#if defined _WIN32
    std::replace( _logDirectory.begin(), _logDirectory.end(), '/', '\\' );
    command << "dir " << _logDirectory << " 1>nul 2>nul";
    mkdir_result = std::system( command.str().c_str() );
    if ( mkdir_result )
    {
      command.str( "" );
      command << "md " << _logDirectory << " 1>nul 2>nul";
      mkdir_result = std::system( command.str().c_str() );
    }
#elif defined __linux__
    command << "mkdir -p " << _logDirectory;
    mkdir_result = std::system( command.str().c_str() );
#else // Might as well try this one
    command << "mkdir " << _logDirectory;
    mkdir_result = std::system( command.str().c_str() );
#endif

    if ( mkdir_result )
    {
      std::cerr << "Could not create and verify logfile directory.\nIs your operating system supported?" << std::endl;
      std::abort();
    }


    // Initialise output
    _files.reserve( _numberFiles );
    int width = std::log10( _numberFiles ) + 1;
    for( size_t i = 0; i < _numberFiles; ++i )
    {
      std::stringstream file_path;
      file_path << _logDirectory << _baseFilename << '.' << std::setw( width ) << std::setfill( '0' ) << i;
      _files.push_back( file_path.str() );
    }

    _currentFile = std::ofstream( _files[_currentFileID++].c_str(), std::ios_base::out );
    if ( ! _currentFile.is_open() )
    {
      std::cerr << "Could Not Open File (" << _files[0] << ") For Writing" << std::endl;
      std::abort();
    } 

    // Start the logging thread
    _isRunning = true;
    _loggingThread = std::thread( logging_thread_function );
  }


  logger::~logger()
  {
    if ( _currentFile.is_open() )
      _currentFile.close();
  }


  void logger::writeIntro()
  {
    std::time_t start_time = std::chrono::system_clock::to_time_t( _startTime );
    std::tm tm = *std::localtime(&start_time);
    std::time_t file_start_time = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
    std::tm file_tm = *std::localtime(&file_start_time);

    // Log initialisation statements
    std::stringstream outputStr;
    outputStr << "\nLOGTASTIC LOGGING\n";
    outputStr << "Version - " << LOGTASTIC_VERSION << "\n\n";
    outputStr << "Program Name         : " << _programName << "\n";
    outputStr << "Program Version      : " << _programVersion << "\n\n";
    outputStr << "Log File Directory   : " << _logDirectory << "\n";
    outputStr << "Number Log Files     : " << _numberFiles << '\n';
    outputStr << "Logging Initialised  : " << std::put_time( &tm, "%c %Z" ) << "\n\n";

    outputStr << "Current File ID      : " << _currentFileID-1 << '\n';
    outputStr << "File Opened          : " << std::put_time( &file_tm, "%c %Z" ) << '\n';
    outputStr << "Log File Number      : " << _currentFileNumber << "\n\n";

    std::string result = outputStr.str();
    this->outputAll( logtastic::info, result );
    this->flush();
  }


  void logger::writeFileOutro()
  {
    std::time_t file_start_time = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
    std::tm file_tm = *std::localtime(&file_start_time);

    // Log initialisation statements
    std::stringstream outputStr;
    outputStr << "\nEND OF FILE\n";
    outputStr << "File Closed at: " << std::put_time( &file_tm, "%c %Z" ) << "\n\n";

    std::string result = outputStr.str();
    this->outputAll( logtastic::info, result );
    this->flush();
  }

  void logger::writeOutro()
  {
    std::time_t stop_time = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
    std::tm tm = *std::localtime(&stop_time);

    std::stringstream outputStr;
    outputStr << "\nEND OF FILE\n";
    outputStr << "\nEND OF PROGRAM OPERATION\n";
    outputStr << "Program finished at: " << std::put_time( &tm, "%c %Z" ) << "\n\n";

    std::string result = outputStr.str();
    this->outputAll( logtastic::info, result );
    this->flush();
  }

  void logger::nextFile()
  {
    if ( _currentFileID == _numberFiles )
      _currentFileID = 0;

    // Close current
    _currentFile.close();
    // Open the next
    _currentFile = std::ofstream( _files[ _currentFileID ], std::ios_base::out );

    if ( ! _currentFile.is_open() )
    {
      std::cerr << "Could Not Open File " << _currentFileID << " (" << _files[_currentFileID] << ") For Writing" << std::endl;
    } 
    
    // Update the counter
    ++_currentFileID;
    ++_currentFileNumber;
  }

  void logger::stopThread()
  {
    // Close the buffer thread
    {
      std::lock_guard<std::mutex> lk( _elementCountMutex );
      _stopThread = true;
    }
    // Signal that we changed something
    _waitData.notify_all();
    // Join the logging thread
    _loggingThread.join();
  }


////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Buffer implementation

  size_t logger::getSize()
  {
    std::lock_guard<std::mutex> lock( _elementCountMutex );
    return _elementCount;
  }

  void logger::pushStatement( statement* element )
  {
    std::unique_lock<std::mutex> startLock( _startMutex, std::defer_lock );
    std::unique_lock<std::mutex> endLock( _endMutex, std::defer_lock );
    std::unique_lock<std::mutex> countLock( _elementCountMutex, std::defer_lock );

    // Lock the start of the buffer
    startLock.lock();
    // Lock the counter to check the status
    countLock.lock();

    switch ( _elementCount )
    {
      case 0 :
        {
          endLock.lock();
          _start = element;
          _end = element;
          ++_elementCount;
          countLock.unlock();
          endLock.unlock();
        }
        break;
      case 1 : // If there's only one element - block the pop
        {
          endLock.lock();
          ++_elementCount;
          countLock.unlock();

          // Update the start/end pointer and release the end lock - no longer critical
          _start->next = element;
          endLock.unlock();

          // set the start element
          _start = element;
        }
        break;
      default :
        {
          ++_elementCount;
          // Update the start/end pointer and release the end lock - no longer critical
          _start->next = element;
          countLock.unlock();

          // set the start element
          _start = element;
        }
        break;
    }
    // Notify anyone waiting that an element was pushed.
    _waitData.notify_all();
    startLock.unlock();
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
    statement* st = new statement();
    st->depth = depth;
    st->function = function;
    st->text = text;

    theLogger->pushStatement( st );
  }


  void init( std::ostream& os )
  {
    if ( logger::_theInstance == nullptr )
    {
      logger::_theInstance = new logger( os );
    }
  }

  void start( const char* name, const char* version )
  {
    if ( logger::_theInstance == nullptr )
    {
      std::cerr << "Unable to start an uninitialised logger." << std::endl;
      std::abort();
    }

    if ( ! logger::_theInstance->_isRunning )
    {
      logger::_theInstance->initialise( name, version );
    }
    else
    {
      push( warn, "logtastic::start", "Trying to start logging while it is already running." );
    }
  }

  void stop()
  {
    if ( logger::_theInstance != 0 )
    {
      // Stop the thread before we destory the object
      logger::_theInstance->stopThread();

      delete logger::_theInstance;
      logger::_theInstance = nullptr;
    }
  }


  void setLogFile( const char* fileName )
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_baseFilename = std::string( fileName );
    else
      push( error, "logtastic::setLogFile", "Attempted to add another output file after logger initialisation" );
  }

  void setLogFileDirectory( const char* directoryName )
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_logDirectory = directoryName;
    else
      push( error, "logtastic::setLogFileDirectory", "Attempted to change the Log File Directory after initialisation" );
  }

  void setQueueSizeWarning( size_t limit )
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_queueSizeWarning = limit;
    else
      push( error, "logtastic::setQueueSizeWarning", "Attempted to change the queue size limit after initialisation" );
  }

  void setMaxFileSize( unsigned long size )
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_maxFileSize = size;
    else
      push( error, "logtastic::setMaxFileSize", "Attempted to change the max file size after initialisation" );
  }

  void setMaxNumberFiles( unsigned long size )
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_numberFiles = size;
    else
      push( error, "logtastic::setMaxNumberFiles", "Attempted to change the max file size after initialisation" );
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
      push( error, "logtastic::setPrintToScreenLimit", "Attempted to change logging setting (Print to terminal depth) after logger initialisation" );
  }

  void setFormat( log_depth, const char* )
  {
    std::cerr << "logtastic::setFormat : FUNCTION NOT YET IMPLEMENTED!" << std::endl;
    std::abort();
  }

  void setFormatAll( const char * )
  {
    std::cerr << "logtastic::setFormatAll : FUNCTION NOT YET IMPLEMENTED!" << std::endl;
    std::abort();
  }

  void setVariableLogDepth( log_depth depth )
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_variableLogDepth = depth;
    else
      push( error, "logtastic::setVariableLogDepth", "Attempted to change logging setting (Variable log depth) after logger initialisation" );
  }


  // Signal Handling

  void setEnableSignalHandling( bool value )
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_enableSignalHandling = value;
    else
      push( error, "logtastic::setHaltOnSignal", "Attempted to change signal handling behaviour after logger initialisation" );
  }

  void setHaltOnSignal( int signal, bool value )
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_haltOnSignal[LOGTASTIC_SIGNAL_ID( signal )] = value;
    else
      push( error, "logtastic::setHaltOnSignal", "Attempted to change signal handling behaviour after logger initialisation" );
  }

  void registerSignalHandler( int signal, void (*hndl)(int) )
  {
    if ( logger::_theInstance->_isRunning == false )
      logger::_theInstance->_userSignalHandlers[LOGTASTIC_SIGNAL_ID( signal)] = hndl;
    else
      push( error, "logtastic::registerSignalHandler", "Attempted to set signal handler after logger initialisation" );
  }

  void logtastic_signal_handler( int sig )
  {
    if ( ! logger::_theInstance->_isRunning ) return;
    logger::SignalHandler handler = logger::_theInstance->_userSignalHandlers[ LOGTASTIC_SIGNAL_ID( sig ) ];
    bool kill = logger::_theInstance->_haltOnSignal[ LOGTASTIC_SIGNAL_ID( sig ) ];


    switch( sig )
    {
      case SIGABRT :
        push( failure, "logtastic::signal_handler", "Abort Signal Received." );
        if ( handler != nullptr ) handler( sig );
        stop();
        std::exit( sig );
        break;

      case SIGFPE  :
        push( failure, "logtastic::signal_handler", "Floating Point Exception Received." );
        if ( handler != nullptr ) handler( sig );
        stop();
        std::exit( sig );
        break;

      case SIGILL  :
        push( failure, "logtastic::signal_handler", "Illegal Instruction Signal Received." );
        if ( handler != nullptr ) handler( sig );
        stop();
        std::exit( sig );
        break;

      case SIGSEGV :
        push( failure, "logtastic::signal_handler", "Segmentation Violoation Signal Received." );
        if ( handler != nullptr ) handler( sig );
        stop();
        std::exit( sig );
        break;

      case SIGINT  :
        if ( kill ) 
        {
          push( failure, "logtastic::signal_handler", "Interrupt Signal Received. Halting." );
          if ( handler != nullptr ) handler( sig );
          stop();
          std::exit( sig );
        }
        else
        {
          push( error, "logtastic::signal_handler", "Interrupt Signal Received." );
          if ( handler != nullptr ) handler( sig );
        }
        break;

      case SIGTERM :
        if ( kill ) 
        {
          push( failure, "logtastic::signal_handler", "Termination Signal Received. Halting." );
          if ( handler != nullptr ) handler( sig );
          stop();
          std::exit( sig );
        }
        else
        {
          push( error, "logtastic::signal_handler", "Termination Signal Received." );
          if ( handler != nullptr ) handler( sig );
        }
        break;

#ifdef __linux__
      case SIGTSTP :
        push( info, "logtastic::signal_handler", "Stopping program execution." );
        if ( handler != nullptr ) handler( sig );
        if ( kill ) std::raise( SIGSTOP );
        break;

      case SIGCONT :
        push( info, "logtastic::signal_handler", "Resuming program execution." );
        if ( handler != nullptr ) handler( sig );
        break;

      case SIGALRM :
        message( warn, "logtastic::signal_handler" ) << "Alarm Signal Received: " << sig;
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          push( failure, "logtastic::signal_handler", "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

      case SIGVTALRM :
        message( warn, "logtastic::signal_handler" ) << "Virtual Alarm Signal Received: " << sig;
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          push( failure, "logtastic::signal_handler", "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

      case SIGPROF :
        message( warn, "logtastic::signal_handler" ) << "Profiling Timer Expiration Signal Received: " << sig;
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          push( failure, "logtastic::signal_handler", "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

      case SIGHUP :
        if ( kill ) 
        {
          push( failure, "logtastic::signal_handler", "Hang Up Signal Received. Halting." );
          if ( handler != nullptr ) handler( sig );
          stop();
          std::exit( sig );
        }
        else
        {
          push( error, "logtastic::signal_handler", "Hang Up Signal Received." );
          if ( handler != nullptr ) handler( sig );
        }
        break;

      case SIGQUIT :
        push( failure, "logtastic::signal_handler", "Quit Signal Received." );
        if ( handler != nullptr ) handler( sig );
        stop();
        std::exit( sig );
        break;

      case SIGBUS :
        push( failure, "logtastic::signal_handler", "Bus Error Signal Received." );
        if ( handler != nullptr ) handler( sig );
        stop();
        std::exit( sig );
        break;

      case SIGXCPU :
        push( failure, "logtastic::signal_handler", "CPU Time Exceeded Signal Received." );
        if ( handler != nullptr ) handler( sig );
        stop();
        std::exit( sig );
        break;

      case SIGXFSZ :
        push( failure, "logtastic::signal_handler", "File Size Exceeded Signal Received." );
        if ( handler != nullptr ) handler( sig );
        stop();
        std::exit( sig );
        break;

      case SIGUSR1 :
        push( info, "logtastic::signal_handler", "User Signal 1 Received." );
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          push( failure, "logtastic::signal_handler", "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

      case SIGUSR2 :
        push( info, "logtastic::signal_handler", "User Signal 2 Received." );
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          push( failure, "logtastic::signal_handler", "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

      case SIGPIPE :
        push( info, "logtastic::signal_handler", "Broken Pipe Signal Received." );
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          push( failure, "logtastic::signal_handler", "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

      case SIGCHLD :
        push( info, "logtastic::signal_handler", "Child Process Termination Signal Received." );
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          push( failure, "logtastic::signal_handler", "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

      case SIGTTIN :
        push( info, "logtastic::signal_handler", "Terminal Input For Background Process Signal Received." );
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          push( warn, "logtastic::signal_handler", "Stopping program." );
          std::raise( SIGSTOP );
        }
        break;

      case SIGTTOU :
        push( info, "logtastic::signal_handler", "Terminal Output For Background Process Signal Received." );
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          push( failure, "logtastic::signal_handler", "Forcing program exit on signal." );
          std::raise( SIGSTOP );
        }
        break;

      case SIGTRAP :
        push( info, "logtastic::signal_handler", "Break point reached." );
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          push( failure, "logtastic::signal_handler", "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

      case SIGWINCH : // Window resize
        push( info, "logtastic::signal_handler", "Window Resize Signal Received." );
        if ( handler != nullptr ) handler( sig );
        if ( kill )
        {
          push( failure, "logtastic::signal_handler", "Forcing program exit on signal." );
          stop();
          std::exit( sig );
        }
        break;

    // case SIGSTOP : // Un catchable!
    // case SIGLOST :
    // case SIGINFO :
#endif

      default:
        message( failure, "logtastic::signal_handler" ) << "Unexpected and Unknown Signal " << sig << " Received. Halting process.";
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
    std::chrono::milliseconds timediff = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() - _startClock );
    ss << std::setw( 11 ) << std::setfill('0') << timediff.count() << "] ";
    return ss.str();
  }
  
  void logger::outputAll( log_depth depth, std::string& str )
  {
    if ( depth >= _screenDepth ) 
    {
      _output << str;
    }

    _currentFile << str;
  }
  

  void logger::flush()
  {
    _output.flush();
    _currentFile.flush();
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

  void logging_thread_function()
  {
    logger* theLogger = logger::get();
    statement* current_statement;
    size_t warning_limit = theLogger->_queueSizeWarning;
    size_t throttling_limit = 0.8*theLogger->_queueSizeWarning;
    unsigned int max_file_size = theLogger->_maxFileSize;
    unsigned int numberEntries = 0;

    std::unique_lock<std::mutex> counterLock( theLogger->_elementCountMutex, std::defer_lock );
    std::unique_lock<std::mutex> endLock( theLogger->_endMutex, std::defer_lock );

    theLogger->writeIntro();

    while ( true )
    {
      // Grab the counter
      counterLock.lock();

      if ( theLogger->_elementCount == 0 )
      {
        theLogger->_waitData.wait( counterLock, [&]()->bool{ return (theLogger->_elementCount > 0) || theLogger->_stopThread; } );

        if ( (theLogger->_elementCount == 0) && theLogger->_stopThread )
        {
          counterLock.unlock();
          break;
        }
      }

      if ( theLogger->_elementCount > warning_limit )
      {
        std::stringstream ss;
        ss << "Logging buffer size: " << theLogger->_elementCount << ", exceeding limits, throttling to 80%";
        theLogger->Log_Statement( warn, "logging_thread_function", ss.str() );

        endLock.lock();
        while ( theLogger->_elementCount > throttling_limit )
        {
          // Decrement counter
          --theLogger->_elementCount;
          // Grab the element
          current_statement = theLogger->_end;
          // update the end pointer
          theLogger->_end = theLogger->_end->next;
          // write it
          theLogger->Log_Statement( current_statement->depth, current_statement->function, current_statement->text );
          ++numberEntries;
          // Delete the object
          delete current_statement;

          if ( numberEntries > max_file_size )
          {
            theLogger->writeFileOutro();
            theLogger->nextFile();
            theLogger->writeIntro();

            numberEntries = 0;
          }
        }
        counterLock.unlock();
        endLock.unlock();
      }
      else
      {
        // Aquire the end pointer - no one else can pop
        endLock.lock();
        // Update and release the counter
        --theLogger->_elementCount;
        counterLock.unlock();

        // Grab the element
        current_statement = theLogger->_end;
        // update the end pointer
        theLogger->_end = theLogger->_end->next;
        // Release the end pointer
        endLock.unlock();

        // write it
        theLogger->Log_Statement( current_statement->depth, current_statement->function, current_statement->text );
        ++numberEntries;
        // Delete the object
        delete current_statement;

        if ( numberEntries > max_file_size )
        {
          theLogger->writeFileOutro();
          theLogger->nextFile();
          theLogger->writeIntro();

          numberEntries = 0;
        }
      }
    }

    theLogger->writeOutro();
  }

} // logtastic

