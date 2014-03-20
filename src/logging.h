#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <inttypes.h>

#define _LOG_DEBUG  1 
#define _LOG_INFO   2
#define _LOG_WARN   3
#define _LOG_ERR	4
#define _LOG_MSG    5
#define _LOG_DEFAULT _LOG_WARN

#define _LOG_SDEBUG   "DEBUG"
#define _LOG_SINFO    "INFO "
#define _LOG_SWARN    "WARN "
#define _LOG_SERR     "ERROR"
#define _LOG_SMSG     "MSG  "

#define _LOG_SSDEBUG   "DEBUG"
#define _LOG_SSINFO    "INFO"
#define _LOG_SSWARN    "WARN"
#define _LOG_SSERR     "ERROR"
#define _LOG_SSMSG     "MSG"


#ifdef LOG_FILELINE
#define	_LOG_FILELINE	1
#else
#define _LOG_FILELINE	0
#endif

#define log_debug(F, ...)	{log_str(_LOG_DEBUG, __FILE__, __LINE__, _LOG_SDEBUG, F, ##__VA_ARGS__);}
#define log_info(F, ...)	{log_str(_LOG_INFO, __FILE__, __LINE__, _LOG_SINFO, F, ##__VA_ARGS__);}
#define log_warn(F, ...)	{log_str(_LOG_WARN, __FILE__, __LINE__, _LOG_SWARN, F, ##__VA_ARGS__);}
#define log_err(F, ...)		{log_str(_LOG_ERR, __FILE__, __LINE__, _LOG_SERR, F, ##__VA_ARGS__);}
#define log_msg(F, ...)		{log_str(_LOG_MSG, __FILE__, __LINE__, _LOG_SMSG, F, ##__VA_ARGS__);}

// No new line
#define log_ndebug(F, ...)	{log_nstr(_LOG_DEBUG, __FILE__, __LINE__, _LOG_SDEBUG, F, ##__VA_ARGS__);}
#define log_ninfo(F, ...)	{log_nstr(_LOG_INFO, __FILE__, __LINE__, _LOG_SINFO, F, ##__VA_ARGS__);}
#define log_nwarn(F, ...)	{log_nstr(_LOG_WARN, __FILE__, __LINE__, _LOG_SWARN, F, ##__VA_ARGS__);}
#define log_nerr(F, ...)	{log_nstr(_LOG_ERR, __FILE__, __LINE__, _LOG_SERR, F, ##__VA_ARGS__);}
#define log_nmsg(F, ...)	{log_nstr(_LOG_MSG, __FILE__, __LINE__, _LOG_SMSG, F, ##__VA_ARGS__);}

static int _LOG_LEVEL = _LOG_DEFAULT;
static FILE *_LOG_FD = NULL;

static inline int log_level(int level)
{
	if(level != 0) _LOG_LEVEL = level;
	return _LOG_LEVEL;
}

static inline FILE* log_fd(FILE *fd)
{
	if(fd != NULL) _LOG_FD = fd;
	return _LOG_FD;
}

static inline void log_str(int level, const char *file, const int line, const char
		*level_str, const char *format, ...)
{
	if(_LOG_LEVEL > level)
		return;

	if(_LOG_FD == NULL)
		_LOG_FD = stdout;

	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	if(_LOG_FILELINE)
	{
		fprintf(_LOG_FD, "%d-%02d-%02d %02d:%02d:%02d [%s] %s(%d) ",
				(tm->tm_year) + 1900, (tm->tm_mon) + 1, tm->tm_mday, tm->tm_hour,
				tm->tm_min, tm->tm_sec, level_str, file, line);
	}else
	{
		fprintf(_LOG_FD, "%d-%02d-%02d %02d:%02d:%02d [%s] ", (tm->tm_year) +
				1900, (tm->tm_mon) + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,
				tm->tm_sec, level_str);
	}

	va_list argptr;
	va_start(argptr, format);
	vfprintf(_LOG_FD, format, argptr);
	va_end(argptr);
	fprintf(_LOG_FD, "\n");
	fflush(_LOG_FD);
}

static inline void log_nstr(int level, const char *file, const int line, const char
		*level_str, const char *format, ...)
{
	if(_LOG_LEVEL > level)
		return;

	if(_LOG_FD == NULL)
		_LOG_FD = stdout;

	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	if(_LOG_FILELINE)
	{
		fprintf(_LOG_FD, "%d-%02d-%02d %02d:%02d:%02d [%s] %s(%d) ",
				(tm->tm_year) + 1900, (tm->tm_mon) + 1, tm->tm_mday, tm->tm_hour,
				tm->tm_min, tm->tm_sec, level_str, file, line);
	}else
	{
		fprintf(_LOG_FD, "%d-%02d-%02d %02d:%02d:%02d [%s] ", (tm->tm_year) +
				1900, (tm->tm_mon) + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,
				tm->tm_sec, level_str);
	}

	va_list argptr;
	va_start(argptr, format);
	vfprintf(_LOG_FD, format, argptr);
	va_end(argptr);
	fflush(_LOG_FD);
}

static inline int log_level_compare(int cur_level, int cmp_level)
{
    return cur_level <= cmp_level ? 1 : 0;
}

static inline int log_debug_enable()
{
    return log_level_compare(log_level(0), _LOG_DEBUG);
}

static inline int log_info_enable()
{
    return log_level_compare(log_level(0), _LOG_INFO);
}

static inline int log_warn_enable()
{
    return log_level_compare(log_level(0), _LOG_WARN);
}

static inline int log_err_enable()
{
    return log_level_compare(log_level(0), _LOG_ERR);
}

static inline int log_msg_enable()
{
    return 1;
}

#endif // _LOGGING_H_
