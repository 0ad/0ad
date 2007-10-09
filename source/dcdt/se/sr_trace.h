
/** \file sr_trace.h 
 * Macros to easily trace and debug programs
 *
 * To trace your source code, you can define SR_USE_TRACEx (x in {1,..,9}),
 * and then include sr_trace.h. In this way, messages are automatically 
 * sent to sr_out. To stop using the trace, you need just to comment the 
 * defined SR_USE_TRACEx.
 *
 * If SR_USE_TRACE (without any number) is defined, it will have the same
 * effect as defining all SR_USE_TRACEx, x in {1,..,9}.
 *
 * To change the sr_out output stream, it is possible
 * to define SR_TRACE_OSTREAM to ostream or to another SrOutput object. 
 *
 * SR_TRACE_MSG can be defined by the user to customize the tracing messages.
 * If it is not defined, then the default message is displayed, with the option
 * to not display filenames when defining SR_TRACE_NO_FILENAME.
 *
 **/

# ifndef SR_TRACE_H
# define SR_TRACE_H

# if defined(SR_USE_TRACE) || defined(SR_USE_TRACE1) || defined(SR_USE_TRACE2) || defined(SR_USE_TRACE3) || defined(SR_USE_TRACE4) || defined(SR_USE_TRACE5) || defined(SR_USE_TRACE6) || defined(SR_USE_TRACE7) || defined(SR_USE_TRACE8) || defined(SR_USE_TRACE9)
#  ifndef SR_TRACE_OSTREAM
#   include <SR/sr_output.h>
#   define SR_TRACE_OSTREAM sr_out
#  endif 
# endif

# ifndef SR_TRACE_MSG
#  if defined(SR_TRACE_FILENAME)
#   define SR_TRACE_MSG __FILE__ << "::" << __LINE__ << '\t'
#  else
#   define SR_TRACE_MSG "line " << __LINE__ << ": "
#  endif
# endif

# if defined(SR_USE_TRACE1) || defined(SR_USE_TRACE)
# define SR_TRACE1(msg) SR_TRACE_OSTREAM << SR_TRACE_MSG << msg << '\n'
# else
# define SR_TRACE1(msg) 
# endif

# if defined(SR_USE_TRACE2) || defined(SR_USE_TRACE)
# define SR_TRACE2(msg) SR_TRACE_OSTREAM << SR_TRACE_MSG << msg << '\n'
# else
# define SR_TRACE2(msg) 
# endif

# if defined(SR_USE_TRACE3) || defined(SR_USE_TRACE)
# define SR_TRACE3(msg) SR_TRACE_OSTREAM << SR_TRACE_MSG << msg << '\n'
# else
# define SR_TRACE3(msg) 
# endif

# if defined(SR_USE_TRACE4) || defined(SR_USE_TRACE)
# define SR_TRACE4(msg) SR_TRACE_OSTREAM << SR_TRACE_MSG << msg << '\n'
# else
# define SR_TRACE4(msg) 
# endif

# if defined(SR_USE_TRACE5) || defined(SR_USE_TRACE)
# define SR_TRACE5(msg) SR_TRACE_OSTREAM << SR_TRACE_MSG << msg << '\n'
# else
# define SR_TRACE5(msg) 
# endif

# if defined(SR_USE_TRACE6) || defined(SR_USE_TRACE)
# define SR_TRACE6(msg) SR_TRACE_OSTREAM << SR_TRACE_MSG << msg << '\n'
# else
# define SR_TRACE6(msg) 
# endif

# if defined(SR_USE_TRACE7) || defined(SR_USE_TRACE)
# define SR_TRACE7(msg) SR_TRACE_OSTREAM << SR_TRACE_MSG << msg << '\n'
# else
# define SR_TRACE7(msg) 
# endif

# if defined(SR_USE_TRACE8) || defined(SR_USE_TRACE)
# define SR_TRACE8(msg) SR_TRACE_OSTREAM << SR_TRACE_MSG << msg << '\n'
# else
# define SR_TRACE8(msg) 
# endif

# if defined(SR_USE_TRACE9) || defined(SR_USE_TRACE)
# define SR_TRACE9(msg) SR_TRACE_OSTREAM << SR_TRACE_MSG << msg << '\n'
# else
# define SR_TRACE9(msg) 
# endif

#endif // SR_TRACE_H

