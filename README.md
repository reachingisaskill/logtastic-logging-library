Logtastic

My home grown logging library.

 * It uses a series of hash defines to implement the logging functions, which call functions from a
singleton logging class.
 * Compile-time definitions can disable the logging statements to test for stability and speed.
   "__ALL_LOGGING_OFF__" disables all logging statements, which are optimized away when the GCC 
   optmisation options are used.
 * A separate flag "__DEBUG_OFF__" disables only the debugging statements for a release version.
 * Both function call-type interface and a iostream interface is provided. e.g.:
	- ERROR_LOG( /log-message/ );
	- ERROR_LOG_STREAM << /message one/ << /some other data/ << /more text/ ;

 * A variable tracking function is provided to uniquely track simple variables within as the
   program progresses. A unique identifier must be provided to identify the variable in question.
   The user must also supply a "count" option with only logs the variable every <count> times the
   statement is encounted in execution (useful for lots of looping!) e.g.:
	- VARIABLE_LOG( <id>, <variable>, <count> );

 * Signal handling was the last piece added into the system and must be enabled at runtime by
   provding a function pointer for a signal handler, e.g.:
	- logtastic::registerSignalHandler( void (\*signal_handler)( int ) );


TODO:
 * Improve robustness for compilers that don't define __COUNTER__ - probably don't need this exact
   interface can change it. I'd like to call a template function using the data objects adress as 
   the template variable, but that functionality wasn't available when I wrote it. Maybe an
   inherited "monitor" class could be used instead?
 * Finer controls for the signal handling functionality. It was added quite quickly to meet a need
   but it could be generalised to a more flexible interface.





