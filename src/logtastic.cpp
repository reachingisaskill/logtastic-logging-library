
#include "logtastic.h"

#include <iomanip>

namespace logtastic
{

  unsigned long int messege::_instCount = 0;
  
  messege::messege( log_depth depth, int identifier ) :
    _identifier( identifier ),
    _depth( depth ),
    _messege( "" )
  {
    ++_instCount;
  }

  messege::messege( const messege& mess ) :
    _identifier( mess._identifier ),
    _depth( mess._depth ),
    _messege( mess._messege.str() )
  {
    ++_instCount;
  }

  messege::~messege()
  {
    --_instCount;
    if ( _instCount == 0 )
    {
      std::string result = _messege.str();

      if ( _depth == logtastic::data )
        logtastic::logData( _identifier, result.c_str() );
      else
        logtastic::log( _depth, result.c_str() );
    }
  }

  template <>
    messege& messege::operator<<( const log_depth& depth )
    {
      _depth = depth;
      return *this;
    }


///////////////////////////////////////////////////////////////


  std::string logger::_dataDirectory( __LOGTASTIC_LOG_FILE_DIRECTORY__ );
  std::string logger::_logDirectory( __LOGTASTIC_LOG_FILE_DIRECTORY__ );
  logger* logger::_theInstance = 0;
  std::list< std::string > logger::_filenames( 0, std::string("") );
  std::map< int, std::ofstream* > logger::_dataFiles;
  bool logger::_flushOnCall = true;
  log_depth logger::_screenDepth = logtastic::warn;
  log_depth logger::_variableLogDepth = logtastic::info;
  bool logger::_handleSignals = true;
  bool logger::_haltOnSignal = true;
  int logger::_lastSignal( 0 );

  void (*logger::_userHandler)(int) = 0;

  //std::map< log_depth, instr_list > _formatting;


  //  Con/De-structors

  logger::logger( const char* name, const char* version, std::ostream& stream ) :
    _programName( name ),
    _programVersion( version ),
    _output( stream ),
    _startTime( time(0) )//,
    //_startClock( gettimeofday() )
  {
    gettimeofday( &_startClock, 0 );


    // Register exiting functions

    atexit( logtastic::stop );
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


    // Make sure directories exist

    std::stringstream command;
    command << "mkdir -p " << _logDirectory << " " << _dataDirectory;
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
    outputStr << "Version - " << __LOGTASTIC_VERSION__ << "\n\n";
    outputStr << "Program Name         : " << _programName << "\n";
    outputStr << "Program Version      : " << _programVersion << "\n\n";
    outputStr << "Log File Directory   : " << _logDirectory << "\n";
    outputStr << "Data File Directory  : " << _dataDirectory << "\n\n";
    outputStr << "Logging Initialised  : " << ctime(&_startTime) << "\n\n";

    std::string result = outputStr.str();
    this->outputAll( logtastic::info, result );
    this->flush();
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
    // Close output fstreams

    _startTime = time(0);

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

    for ( std::map< int, std::ofstream* >::iterator it = _dataFiles.begin(); it != _dataFiles.end(); ++it )
    {
      it->second->close();
      delete it->second;
    }
  }


  //  Static Functions 

  void log( log_depth depth, const char* string )
  {
    if ( logger::_theInstance != 0 )
    {
      logger::_theInstance->Log_Statement( depth, string );
    }
  }

  void logData( int identifier, const char* string )
  {
    if ( logger::_theInstance != 0 )
    {
      logger::_theInstance->Log_Data( identifier, string );
    }
  }

  void init( const char* name, const char* version, std::ostream& stream )
  {
    if ( logger::_theInstance == 0 )
    {
      logger::_theInstance = new logger( name, version, stream );
    }
    else
    {
      ERROR_LOG( "Attempting to initialise when logger already initialised" );
    }
  }

  void stop()
  {
    if ( logger::_theInstance != 0 )
    {
      delete logger::_theInstance;
      logger::_theInstance = 0;
    }
  }


  void addLogFile( const char* fileName )
  {
    std::string fileString = logger::_logDirectory + "/" + fileName;
    if ( logger::_theInstance == 0 )
      logger::_filenames.push_back( fileString );
    else
      ERROR_LOG( "Attempted to add another output file after logger initialisation" );
  }

  void setLogFileDirectory( const char* directoryName )
  {
    if ( logger::_theInstance == 0 )
      logger::_logDirectory = directoryName;
    else
      ERROR_LOG( "Attempted to change the Log File Directory after initialisation" );
  }

  void addDataFile( int identifier, const char* fileName )
  {
    std::string fileString = logger::_dataDirectory + "/" + fileName;
    if ( logger::_dataFiles.find( identifier ) == logger::_dataFiles.end() )
    {
      std::ofstream* tmp = new std::ofstream( fileString.c_str() );
      if ( tmp->fail() ) 
      {
        ERROR_STREAM << "Could Not Open Data File: " << fileName;
        delete tmp;
        return;
      }
      logger::_dataFiles.insert( std::make_pair( identifier, tmp ) );
      INFO_STREAM << "Successfully Added Data File: " << identifier << " - " << fileName;
    }
    else
    {
      delete logger::_dataFiles[ identifier ];
      logger::_dataFiles.erase( identifier );
      logger::_dataFiles.insert( std::make_pair( identifier, new std::ofstream( fileString.c_str() ) ) );
      INFO_STREAM << "Replaced Data File: " << identifier << " With: " << fileName;
    }
  }

  void setDataFileDirectory( const char* directoryName )
  {
    if ( logger::_theInstance == 0 )
      logger::_dataDirectory = directoryName;
    else
      ERROR_LOG( "Attempted to change the Data File Directory after initialisation" );
  }

  void closeDataFile( int n )
  {
    if ( logger::_dataFiles.find( n ) == logger::_dataFiles.end() )
      ERROR_LOG( "Could not find file handle. No Data Files will be closed" );
    else
    {
      logger::_dataFiles[ n ]->close();
      delete logger::_dataFiles[ n ];
      logger::_dataFiles.erase( n );
      INFO_STREAM << "Data File Number: " << n << " successfully closed";
    }
  }

  void setFlushOnEveryCall( bool flush )
  {
    logger::_flushOnCall = flush;
  }
  
  void setPrintToScreenLimit( log_depth depth )
  {
    if ( logger::_theInstance == 0 )
      logger::_screenDepth = depth;
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
    if ( logger::_theInstance == 0 )
      logger::_variableLogDepth = depth;
    else
      ERROR_LOG( "Attempted to change logging setting (Variable log depth) after logger initialisation" );
  }


  // Signal Handling

  void preventSignalHandling()
  {
    if ( logger::_theInstance == 0 )
      logger::_handleSignals = false;
    else
      ERROR_LOG( "Attempted to change signal handling behaviour after logger initialisation" );
  }

  void preventHaltOnSignal()
  {
    if ( logger::_theInstance == 0 )
      logger::_haltOnSignal = false;
    else
      ERROR_LOG( "Attempted to change signal handling behaviour after logger initialisation" );
  }

  int signalReceived()
  {
    if ( logger::_theInstance == 0 )
      return 0;
    else
      return logger::_lastSignal;
  }

  void registerSignalHandler( void (*hndl)(int) )
  {
    if ( logger::_theInstance == 0 )
      logger::_userHandler = hndl;
    else
      ERROR_LOG( "Attempted to set signal handler after logger initialisation" );
  }

  void logtastic_signal_handler( int sig )
  {
    if ( logger::_theInstance == 0 ) return;

    logger::_lastSignal = sig;
    if ( logger::_userHandler != 0 ) logger::_userHandler( sig );

    switch( sig )
    {
      case SIGABRT :
        FAILURE_STREAM << "Program aborted internally.";
        exit( sig );
        break;
      case SIGHUP  :
      case SIGINT  :
      case SIGTERM :
      case SIGQUIT :
        FAILURE_STREAM << "Signal " << sig << " Received. Forcing Program Stop";
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
        if ( logger::_haltOnSignal ) 
        {
          FAILURE_LOG( "Forcing Program Stop" );
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
        if ( logger::_haltOnSignal )
        {
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
        if ( logger::_haltOnSignal )
        {
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

  logger& logger::Log_Statement( log_depth depth, const char* string )
  {
    std::stringstream ss;
    ss << getPrefix( depth ) << string << "\n";

    std::string result = ss.str();
    outputAll( depth, result );

    if ( _flushOnCall )
    {
      this->flush();
    }

    return *this;
  }

  logger& logger::Log_Data( int identifier, const char* string )
  {
    std::map< int, std::ofstream* >::iterator it = _dataFiles.find( identifier );
    
    if ( it != _dataFiles.end() )
      (*it->second) << string << "\n";

    return *this;
  }

} // logtastic

