# Logtastic

## Introduction

My home grown logging library.

I implemented this during the first year of my PhD in experimental particle physics. I was eager to demonstrate my ability and obstinantly refused to use existing libraries and favour of learning everything I could. The initial version was pre-C++11. I have since been improving it. If you look through the code and some of the conventions are a mix of C, C++ and C++11, I generally work on a "if it ain't broke don't fix it approach", and this was originally working in 2013 on an outdated system.

I only recently added the library to git hub (a requirement for modern job hunting...) and quickly created a multithreaded branch, which runs a mutex-controlled logging buffer and a separate logging thread. The biggest time sink is writing to the console or file, hence this was moved to a separate thread. It includes functionality to limit the number of calls to the thread if the queue gets to large which is configurable at runtime. I've found it can be used in a real time application (60fps game engine) and still 10s-100s of statements per frame without trouble. 

## Basic design

 * It uses a series of hash defines to implement the logging functions, which call functions from a
singleton logging class.
 * Compile-time definitions can disable the logging statements to test for stability and speed.
   "LOGTASTIC_ALL_LOGGING_DISABLED" disables all logging statements by turning them into `do{}while( false )` statements, which are optimized away when the GCC optmisation options are used.
 * A separate flag "LOGTASTIC_DEBUG_OFF\" disables only the debugging statements for a release version.
 * Both function call-type interface and a iostream interface is provided. e.g.:
	- ERROR_LOG( *log-message* );
	- ERROR_STREAM << *message one* << *some other data* << *more text* ;

 * A variable tracking function is provided to uniquely track simple variables within as the
   program progresses. A unique identifier must be provided to identify the variable in question.
   The user must also supply a "count" option with only logs the variable every <count> times the
   statement is encounted in execution (useful for lots of looping!) e.g.:
	- VARIABLE_LOG( *id*, *variable*, *count* );

 * Signal handling was the last piece added into the system and must be enabled at runtime by
   provding a function pointer for a signal handler, e.g.:
	- logtastic::registerSignalHandler( void (\*signal_handler)( int ) );
	
Had a lot of development when I wrote the FES simulations. Around 2015. Been a while since I've looked
at it in details but it stills works perfectly!



## TODO:
 
 * Finer controls for the signal handling functionality. It was added quite quickly to meet a need
   but it could be generalised to a more flexible interface.
   
 * Implement a multithreaded buffer, rather than a std::queue with a mutex. At the moment the whole queue is mutex locked in order to add a statement to the queue. This generally seems fine, however I'd rather lock the front and the back of the queue independently. This would allow periods of high logging calls to happen without being accidentally blocked. I expect this to only make a difference to game engines running at their limit - when FPS is crucial.





