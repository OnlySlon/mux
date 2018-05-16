

#ifndef __CLOG_H__
	#define __CLOG_H__ 1

	#include <stdio.h>




/*! \brief The log levels in int format.

  Use these as the second parameter to clog

  \sa clog
*/
enum
{
	CMARK  = 0,	/*! MARK is used as a non-reporting level */
	/*! FATAL level only reports errors that cause the application to EXIT */
	CFATAL = 1,
	/*! ERROR level also reports errors that are serious but it can keep processing */
	CERROR = 2,
	/*! WARN level also reports errors that may be important */
	CWARN  = 3,
	/*! DEBUG level also reports information on what is happening so errors can be reported better */
	CDEBUG = 4,
	/*! TRACE level reports everything that is loggable */
	CTRACE = 5
};


enum
{
	DBG_SOCKETS    = 1,
	DBG_TCPSTACK   = 2,
	DBG_PROTO      = 4,
	DBG_FLOWCTL    = 8,
	DBG_SHAPER     = 16,
	DBG_TRANSPORT  = 32,
	DBG_TUNTAP     = 64,
	DBG_COMPRESS   = 128,
	DBG_CRYPTO     = 256,
	DBG_SYSTEM     = 512,
	DBG_FEC        = 1024,
	DBG_ROHC       = 2048,
	DBG_AUTH       = 4096,
	DBG_TAPMUX     = 8192,
	DBG_CONG       = 16384,
	DBG_ALL        = 0xFFFFFFFF
};


struct tdx_strrb 
{
	unsigned int start;				/* index of the first slot */
	unsigned int end;				/* index of the last slot */
	unsigned int size;				/* max number of messages to store */
	unsigned int msg_len;
//    char  **buffer;          /* storage for messages */
};


/*!
  This is the struct that holds the destination info. Do not modify the values directly
*/
struct clog_info_struct
{
	FILE          * s_logfile;
	/** Log level for logging to file. */
	int    s_logfile_level;
	int    log_mask;
	int    log2rb;
	struct tdx_strrb *rb;
	int    log2stdout;
	int    log2syslog;
};

/**
  typedef so that it is easier to create clog_info_struct variables.
*/
typedef struct clog_info_struct CLOG_INFO;

/** \brief Convert char* log level to int value of log level

 Convert text to int log level. This function takes a char* and compares the
 first letter of the string and returns the associated log level.

 \param level Char pointer containing one of "FATAL","ERROR","WARN","DEBUG" ,"TRACE".

 \return returns an int of the associated ENUM (Should be greater than 0)

 \sa CFATAL CERROR CWARN CDEBUG CTRACE

 Example:
 \code
 // define variables
 int loglevel=0;
 CLOG_INFO *log=NULL;

 // get log level (perhaps the string was from a config file)
 loglevel=clog_getlevel("ERROR");

 // open the log
 log = clog_open("/tmp/mylog.txt", loglevel, NULL, 0);

 // close the log
 clog_close(log);
 \endcode
*/
extern int                clog_getlevel(const char *level);

/** \brief sets the logging to a new file

 Closes existing log file (if open) and then opens a new file to log to

 \param info The CLOG_INFO struct that was returned from the clog_open call.
 \param file The file name that you want to now log to.

 \return If successfull it returns 1, On failure it returns 0.

 \sa clog_open clog_setfilelevel clog_getlevel

 Example:
 \code
 // variable to hold the logging info
 CLOG_INFO* log=NULL;

 // open the log file
 log = clog_open("/var/tmp1.txt", clog_getlevel("TRACE"), NULL, 0);

 // change the log file and change the log level
 clog_setfile(log, "/var/tmp2.txt");

 // close the log and free the memory 'log' is pointing to
 clog_close(log);
 \endcode
*/
extern int                clog_setfile(CLOG_INFO* info, char * file);

/** \brief sets the logging to a new log level

 Sets the log level for logging to a file to the new log level.

 \param info The CLOG_INFO struct that was returned from the clog_open call.
 \param level The int that was returned from the clog_getlevel call.

 \return If successfull it returns 1, On failure it returns 0.

 \sa clog_open clog_setfile clog_getlevel

 Example:
 \code
 // variable to hold the logging info
 CLOG_INFO* log=NULL;

 // open the log file
 log = clog_open("/var/tmp1.txt", clog_getlevel("TRACE"), NULL, 0);

 // change the log file and change the log level
 clog_setfilelevel(log, clog_getlevel("FATAL"));

 // close the log and free the memory 'log' is pointing to
 clog_close(log);
 \endcode
*/
extern int                clog_setfilelevel(CLOG_INFO* info, unsigned int level);



/** \brief Opens the log file

  Opens the log file and returns a CLOG_INFO structure that has been allocated with malloc().

  \param file Filename of the log file you want to open. Directory must already exist, File will be appended to.
  \param f_level A message will only be saved to file if the message's log level is at least this level.
  \param callback A function that will also be called on a logging.
  \param c_level The callback function will only be called if the message's log level is at least this level.

  \sa clog_getlevel clog_close
*/
extern CLOG_INFO* clog_open(char * file, unsigned int f_level, unsigned int debug_mask);

/** \brief logs to the file and function

  Logs to the file and function that were specified in the clog_open function. This function's last 2 parameters are the same as printf's 2 parameters.

  \param info The CLOG_INFO struct that was returned from the clog_open call.
  \param level The int that was returned from the clog_getlevel call.
  \param format see the manual on printf for the format parameter and the arguments

  \sa clog_open clog_getlevel printf

  Example:
  \code
  // define variables
  CLOG_INFO *info=NULL;
  int messagenumber = 0;

  // open the log
  info = clog_open("/tmp/mylog.txt", clog_getlevel("DEBUG"), NULL, 0);

  // will get logged
  clog(info, CFATAL, "fatal message %d", messagenumber++);

  // will get logged
  clog(info, CERROR, "error message %d", messagenumber++);

  // will get logged
  clog(info, CWARN , "warn  message %d", messagenumber++);

  // will NOT get logged
  clog(info, CDEBUG, "debug message %d", messagenumber++);

  // will NOT get logged
  clog(info, CTRACE, "trace message %d", messagenumber++);

  // close the log and free the memory
  clog_close(log);
  \endcode


*/
extern void               clog(CLOG_INFO* info, int level, int log_system, const char *format, ...);

/** \brief Closes the file and free()s memory.

  Closes the open file pointed to in the CLOG_INFO* structure. Also free()s
  memory and that is in the CLOG_INFO* structure after setting it all to
  the defaults (NULLs and Zeros)

  \param info The CLOG_INFO struct that was returned from the clog_open call.

  \sa clog_open
*/
extern void               clog_close(CLOG_INFO* info);

/*@}*/

extern void clog_perror(CLOG_INFO* info, int level, int log_system, const char *s);

void  clog_fileclose(CLOG_INFO * info);
FILE* clog_fileopen(char *file);

int clog_subsys_enabled(CLOG_INFO* info, int log_system);


int clog_time_profiling_enable();
int clog_time_profiling_disable();

#endif
