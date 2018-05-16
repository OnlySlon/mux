#define _FILE_OFFSET_BITS  64 // for log files over 2Gb

#include <stdarg.h> // va_*
#include <string.h> // memset
#include <stdio.h> // vsnprintf
#include <time.h> // localtime
#include <stdlib.h> // free
#include <stdlib.h> // malloc
#include <assert.h>
#include <string.h> // strerror
#include <errno.h> // errno
#include <sys/types.h>
#include <unistd.h>


#include <syslog.h>
#include "clog.h"

struct tdx_strrb *tdx_strrb_create(size_t rb_size, size_t msg_len, int key);
int tdx_strrb_add(struct tdx_strrb *rb, const char *data);

static int first = 1;
static unsigned long long first_ts;
static int time_profiling = 0;



char *(g_text[]) = { "MARK ", "FATAL", "ERROR", "WARN ", "DEBUG", "TRACE"};	// !! 5 Letters !!
char *(g_dbg[])  = { "SCK", "TCP", "PRT", "FLW", "SHA", "TRA", "TUN", "COM", "CRY", "SYS", "FEC", "RHC", "AUT", "TUX", "CNG" };


unsigned long long get_time(void)
{
#ifdef LOW_RES_TIMER
        struct timeval tv;
        tdx_gettimeofday(&tv, NULL);
        return(unsigned long long)((unsigned long long) tv.tv_sec) * 1000000 + ((unsigned long)tv.tv_usec);

        unsigned long long microtime = 0;
        struct timeval time_camera = { .tv_sec=0, .tv_usec=0 };
        tdx_gettimeofday(&time_camera,NULL);
        microtime = time_camera.tv_sec;
        microtime = microtime * 1000000;
        microtime = microtime  + time_camera.tv_usec;
        return microtime;
#else

        struct timeval tv;
        gettimeofday(&tv, NULL);
        return(unsigned long long)((unsigned long long) tv.tv_sec) * 1000000 + ((unsigned long)tv.tv_usec);


        unsigned long long microtime = 0;
        struct timeval time_camera = { .tv_sec=0, .tv_usec=0 };
        gettimeofday(&time_camera,NULL);
        microtime = time_camera.tv_sec;
        microtime = microtime * 1000000;
        microtime = microtime  + time_camera.tv_usec;
        return microtime;
#endif
}




int clog_time_profiling_enable()
{
	time_profiling = 1;
	return 0;
}

int clog_time_profiling_disable()
{
	time_profiling = 0;
	return 0;
}


int clog_debugmask_parse(char *parse_me, unsigned *debug_mask)
{

	*debug_mask = 0;
	if (strstr(parse_me, "SCK") != NULL)
		*debug_mask = *debug_mask|DBG_SOCKETS;
	if (strstr(parse_me, "TCP") != NULL)
		*debug_mask = *debug_mask|DBG_TCPSTACK;
	if (strstr(parse_me, "PRT") != NULL)
		*debug_mask = *debug_mask|DBG_PROTO;
	if (strstr(parse_me, "FLW") != NULL)
		*debug_mask = *debug_mask|DBG_FLOWCTL;
	if (strstr(parse_me, "SHA") != NULL)
		*debug_mask = *debug_mask|DBG_SHAPER;
	if (strstr(parse_me, "TRA") != NULL)
		*debug_mask = *debug_mask|DBG_TRANSPORT;
	if (strstr(parse_me, "TUN") != NULL)
		*debug_mask = *debug_mask|DBG_TUNTAP;
	if (strstr(parse_me, "COM") != NULL)
		*debug_mask = *debug_mask|DBG_COMPRESS;
	if (strstr(parse_me, "CRY") != NULL)
		*debug_mask = *debug_mask|DBG_CRYPTO;
	if (strstr(parse_me, "SYS") != NULL)
		*debug_mask = *debug_mask|DBG_SYSTEM;
	if (strstr(parse_me, "FEC") != NULL)
		*debug_mask = *debug_mask|DBG_FEC;
	if (strstr(parse_me, "RHC") != NULL)
		*debug_mask = *debug_mask|DBG_ROHC;
	if (strstr(parse_me, "AUT") != NULL)
		*debug_mask = *debug_mask|DBG_AUTH;
	if (strstr(parse_me, "TUX") != NULL)
		*debug_mask = *debug_mask|DBG_TAPMUX;
	if (strstr(parse_me, "CNG") != NULL)
		*debug_mask = *debug_mask|DBG_CONG;
	if (strstr(parse_me, "ALL") != NULL)
		*debug_mask = *debug_mask|DBG_ALL;
	if (strstr(parse_me, "NUL") != NULL)
		*debug_mask = 0;

	return 0;
} 





char *get_thread_name()
{
	return "";
//	return "_FIXME_";
}

int easy_log_2base(int log2me)
{
	int i = 0;
	if (log2me == 1)
		return 1;
	if (log2me == 0)
		return 0;
	while (log2me%2 == 0)
	{
		log2me = log2me / 2;
		i++;
	}
	return i;
}

extern unsigned long long get_time(void);
/*
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (unsigned long long)((unsigned long long) tv.tv_sec) * 1000000 + ((unsigned long long)tv.tv_usec);
}
*/

void clog_first(void)
{
	first = 1;
}

int clog_subsys_enabled(CLOG_INFO* info, int log_system)
{
	if (info->log_mask&log_system)
		return 1;
	return 0;
}


void clog(CLOG_INFO* info, 
		  int level, 
		  int log_system, 
		  const char *format, ...)
{
	int light = 0;
	int stdout_mode = 0;
	int ringbuf_mode = 0;

	if (first)
	{
		first = 0;
		first_ts = get_time();
	}


	if (level == 666)
		light = 1;

	if (!(-1 < level && 6 > level))	level = CMARK;

	if (info)
	{
		if (!((level <= info->s_logfile_level ) && (info->log_mask&log_system || level == CFATAL ))) return;
		if (info->log2stdout) stdout_mode = 1;
		if (info->log2rb)	 ringbuf_mode = 1;
	}

	if (info == NULL || stdout_mode || ringbuf_mode)
	{
		va_list ap;	// for the variable args
		char message[2048];
		unsigned tlen;
		unsigned long long t;
		unsigned long nanosec_rem;
//		static unsigned long = get_time();

		memset((char*)message, 0, 2048);
		va_start(ap, format);
		vsnprintf((char *)message, 2047, format, ap);
		va_end(ap);

//		syslog(LOG_NOTICE, "%s", message);
//		printf("%s\n", message);


//		t = clock_time();
		t = get_time();
		t -= first_ts;
		if (t > 10000000000)
		{
			first_ts = get_time();
			t = get_time();
			t -= first_ts;
		}
		nanosec_rem =  (t - ((t/1000000) *1000000));

		if (stdout_mode || info == NULL)
		{
			printf("[%5llu.%03lu] %s %s\n", (unsigned long long) t/1000000, nanosec_rem/1000, g_dbg[easy_log_2base(log_system)], message);

			first_ts = get_time();

		}
		if (ringbuf_mode)
		{
			static char logstr[180];
			if (time_profiling)
			{
				sprintf(logstr, "[%5llu.%03lu] %s %s", (unsigned long long) t/1000000, nanosec_rem/1000, g_dbg[easy_log_2base(log_system)], message);
				if (info->rb)
				{
//					tdx_strrb_add(info->rb, logstr);
				}
			}
			else
			{
				struct tm *ts;
				time(&t);
				ts = localtime(&t);

				sprintf(logstr, "[%d.%.2d.%.2d %.2d:%.2d:%.2d] %s %s %s",         
						ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min, ts->tm_sec,                       
						g_text[level],
						g_dbg[easy_log_2base(log_system)], message);
				if (info->rb)
				{
//					tdx_strrb_add(info->rb, logstr);
				}


			}
		}
		return;
	}

	// if we are logging to the function or file
	if ((level <= info->s_logfile_level ) && (info->log_mask&log_system || level == CFATAL ))
	{

		va_list ap;	// for the variable args
		time_t    t;
		struct tm *ts;
		char message[2048];

		memset((char*)message, 0, 2048);
		//  int vsnprintf(char *str, size_t size, const char *format, va_list ap);
		va_start(ap, format);
		vsnprintf((char *) message,  (size_t) 2047, (const char *) format, ap);
		va_end(ap);

		// log to file
		if (NULL!=info->s_logfile && level <= info->s_logfile_level)
		{
			if (NULL!=info->s_logfile)
			{ // log to text file
				time(&t);
				ts = localtime(&t);

				if (NULL!=ts)
				{ // add the time to the beginning
// #ifndef WIN32
					if (!info->log2rb)
					{
						if (!light)
							fprintf(info->s_logfile, "[%d.%.2d.%.2d %.2d:%.2d:%.2d] %.4d %s %s %s ",         
									ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min, ts->tm_sec, 
									getpid(), (get_thread_name() != NULL) ? get_thread_name() : "MAIN   ",
									g_text[level],
									g_dbg[easy_log_2base(log_system)]);

						if (light)
							fprintf(info->s_logfile, "[%d.%.2d.%.2d %.2d:%.2d:%.2d] ",         
									ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min, ts->tm_sec);
					}
					else
					{

						static char rbuff[512];
						if (!light)
							sprintf(rbuff, "[%d.%.2d.%.2d %.2d:%.2d:%.2d] %.4d %s %s %s %s",         
									ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min, ts->tm_sec, 
									getpid(), (get_thread_name() != NULL) ? get_thread_name() : "MAIN   ",
									g_text[level],
									g_dbg[easy_log_2base(log_system)], message);
						if (light)
							sprintf(rbuff, "[%d.%.2d.%.2d %.2d:%.2d:%.2d] %s",         
									ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min, ts->tm_sec, message);
//					printf("___LEN=%u\n", strlen(rbuff));

					}

				}
				if (info->s_logfile && !info->log2rb)
				{
					fprintf(info->s_logfile, "%s",  message);
					fprintf(info->s_logfile, "\n");
					fflush(info->s_logfile);
				}
				ts = NULL;
			}
		}
	}


}



void clog_perror(CLOG_INFO* info, int level, int log_system,  const char *s)
{
	if (NULL == info)
	{
//		return;
	}
	// check here so we do not waste time doing strerror this will also
	// be done in clog (the first if below) so it is redundant to save time.
//	if (level <= info->s_logfile_level || level <= info->s_callback_level)
	{
		if (NULL!=s && 0!=*s)
		{
			clog(info, level, log_system,  "%s %s", s, strerror(errno));
		}
		else
		{
			clog(info, level, log_system, "%s", strerror(errno));
		}
	}
}


void clog_close(CLOG_INFO* info)
{

	if (NULL==info)	return;

	clog_fileclose(info);

	memset(info, 0, sizeof(CLOG_INFO));

	free(info);
	info = NULL;
}

void clog_fileclose(CLOG_INFO * info)
{
	if (NULL!=info)
	{
		// flush/close logfile
		if (NULL!=info->s_logfile)
		{
			fflush(info->s_logfile);
			fclose(info->s_logfile);
		}
		info->s_logfile = NULL;
	}
}

FILE * clog_fileopen(char *file)
{
	FILE * retval = NULL;

	if (!file)	return NULL;

	retval = fopen(file, "a");

	if (NULL == retval)
		perror("clog_fileopen");
	return retval;
}


int clog_getlevel(const char *level)
{
	int retval=CTRACE;
	char tmp=0;

	if (NULL==level) return CMARK;

	tmp = level[0];

	if (tmp)
	{
		switch (tmp)
		{
			case 'M':
			case 'm':
				retval=CMARK; break;
			case 'F':
			case 'f':
				retval=CFATAL; break;
			case 'E':
			case 'e':
				retval=CERROR; break;
			case 'W':
			case 'w':
				retval=CWARN;  break;
			case 'D':
			case 'd':
				retval=CDEBUG; break;
			case 'T':
			case 't':
				retval=CTRACE; break;
			default:
				retval=0;
				break;
		}
	}

	return retval;
}


CLOG_INFO* clog_open(char * file, unsigned int f_level,  unsigned int log_mask)
{
	CLOG_INFO *info=NULL;

	putenv("TZ=GMT0");
	tzset();

	info = (CLOG_INFO*) malloc(sizeof(CLOG_INFO));  
	memset(info, 0, sizeof(CLOG_INFO));


	if (strcmp(file, "-") == 0 || strcmp(file, "stdout") == 0)
	{
		info->log2stdout = 1;
//		printf("Logging mode: Stdout\n");
//		printf("CLOG_STDOUT mode\n");
		goto set_params;
	}

	if (strcmp(file, "rb") == 0)
	{
//		printf("Logging mode: SHM ringbuffer\n");
		info->log2rb = 1;
		goto set_params;
	}

	if (strcmp(file, "syslog") == 0)
	{
//		printf("Logging mode: SysLog\n");
//		printf("CLOG_SYSLOG mode\n");
		info->log2syslog = 1;
		goto set_params;
	}


	printf("Logging mode: File (%s)\n", file);
	info->s_logfile         = clog_fileopen(file);
	if (info->s_logfile == NULL)
		info->log2stdout = 1;

	set_params:
	{
//		printf("CLOG SETPARAMS level=%u mask=%u\n", f_level, log_mask);
		info->s_logfile_level   = f_level;
		info->log_mask           = log_mask;
		return info;
	}
}




int clog_setfile(CLOG_INFO* info, char * file)
{
	int retval=0;

	if (NULL==info)	return 0;

	clog_fileclose(info);
	info->s_logfile = clog_fileopen(file);
	if (NULL!=info->s_logfile)
	{
		retval = 1;
	}

	return retval;
}


int clog_setfilelevel(CLOG_INFO* info, unsigned int level)
{
	int retval = 0;

	if (NULL==info)	return 0;

	if (CTRACE >= level)
	{
		info->s_logfile_level=level;
		retval = 1;
	}
	else
	{
		info->s_logfile_level=CMARK;
	}

	return retval;
}

int clog_set_strrb(CLOG_INFO *info, unsigned size, int key, int max_msg_len)
{
	return 0;
	info->log2rb = 1;
    printf("Create Log2RB with key=%u\n", key);
	if (size <= 1)
		size = 2000;
	info->rb      = tdx_strrb_create(size, max_msg_len, key);
	if (!info->rb)
	{
		printf("Can't create ring buffer for logging...\n");
		info->log2rb = 0;
	}
}


