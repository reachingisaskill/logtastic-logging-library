# Logtastic

## Introduction

My home grown logging library.

I implemented this during the first year of my PhD in experimental particle physics. I was eager to demonstrate my ability and obstinantly refused to use existing libraries and favour of learning everything I could. The initial version was pre-C++11. I have since been improving it. If you look through the code and some of the conventions are a mix of C, C++ and C++11, I generally work on a "if it ain't broke don't fix it approach", and this was originally working in 2013 on an outdated system.

I only recently added the library to git hub (a requirement for modern job hunting...) and quickly created a multithreaded branch, which runs a mutex-controlled logging buffer and a separate logging thread. The biggest time sink is writing to the console or file, hence this was moved to a separate thread. It includes functionality to limit the number of calls to the thread if the queue gets to large which is configurable at runtime. I've found it can be used in a real time application (60fps game engine) and still 10s-100s of statements per frame without trouble. 

Most recent additions include 1) added signal handlers so it records the signals and optionally calls a user specified function as well, and 2) a mutex controlled queue.
The queue has separate front, back and counter mutexes so that the front and back can be handled separately and the counter mutex can control overall access to throttle the calling thread (don't want a segault because accidentally generated 10^47 error messages...). The calling threads also do no block if there is a data race, rather they repeatedly keep trying until they get through. This turns out to be faster, but may slightly change the order of log statements in rare cases.

## Basic usage

It uses a series of hash defines to implement the logging functions, which call functions from a singleton logging class and compile-time definitions can disable the logging statements to test for stability and speed.

 * `LOGTASTIC_ALL_LOGGING_DISABLED` disables all logging statements by turning them into `do{}while( false )` statements, which are optimized away when the GCC optmisation options are used.

 * `LOGTASTIC_DEBUG_OFF` disables only the debugging statements for a release version.

 * `ERROR_LOG( *log-message* );` Log single string type interface

 * `WARN_STREAM << *message one* << *some other data* << *more text*;` Log a string stream interface

 * The macro `LOGTASTIC_FUNCTION_NAME` determines where logtastic will generate a function name for you, for every invocation. If you define it before the header file is included logtastic will use your defined string/macro to determine the calling function.
   If it is not defined before the header file is included, you must specify a function name for every invocation. e.g.

   - With `LOGTASTIC_FUNCTION_NAME __fun__` :
     `ERROR_LOG( "An error occured" );`

   - Without any definition :
     `ERROR_LOG( "test_function", "An error occured" );`
     `WARN_STREAM( "initialisation" ) << "Initialisation error :" << errno;`


A variable tracking function is provided to uniquely track simple variables within as the program progresses. A unique identifier must be provided to identify the variable in question.
The user must also supply a "count" option with only logs the variable every <count> times the statement is encounted in execution (useful for lots of looping!) e.g.:

 * `VARIABLE_LOG( *id*, *variable*, *count* );` (If `LOGTASTIC_FUNCTION_NAME` is defined)

 * `VARIABLE_LOG( *func_name*, *id*, *variable*, *count* );`

Signal handling was the last piece added into the system and must be enabled at runtime by provding a function pointer for a signal handler, e.g.:
 * `logtastic::registerSignalHandler( void (*signal_handler)( int ) );`
	

## TODO:
 
 * Finer controls for the signal handling functionality. It was added quite quickly to meet a need
   but it could be generalised to a more flexible interface.

 * Look at the mutex handling again. I think I could use shared\_mutexes for a lot of locks to increase average access speed. There is also the potential to uses a spinlock but I think that is unecessary and I have already found that "cheap" (read: "terrible"...) implementations are not reliable with a high number of threads.

