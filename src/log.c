#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "log.h"

#define MAX_FORMAT_TIME_LEN     30
#define MAX_LOG_BUFF            (2 * 1024)

extern pid_t g_pid;
char LOG_FILE_PATH[256];

// 默认会打DEBUG日志
unsigned int g_ulLogLevel = 0;

// 0:not print log
// 1:pirnt log
unsigned int g_isPrintLog = 1;

void log_init(unsigned int level)
{
	sprintf(LOG_FILE_PATH, "/var/log/cloudfs/debug.%d.log", g_pid);
	set_log_level(level);
}

void set_log_level(unsigned int level)
{
	if (level < LOG_BUTT)
		g_ulLogLevel = level;
}

void set_log_enable()
{
	g_isPrintLog = 1;
}

void set_log_disable()
{
	g_isPrintLog = 0;
}

void formatTime(char * pcTime)
{
    time_t now;
    struct tm timenow;
    struct timeval tv;

    // get current time
    time(&now);
    localtime_r(&now, &timenow);

    // get the time of millisecend
    gettimeofday(&tv, NULL);

    // format the current time in string
    sprintf(pcTime, 
            "%04d/%02d/%02d %02d:%02d:%02d.%06d", 
            timenow.tm_year + (int)1900, 
            timenow.tm_mon + 1, 
            timenow.tm_mday, 
            timenow.tm_hour, 
            timenow.tm_min, 
            timenow.tm_sec, 
            (int)tv.tv_usec);
}


void writeLogToFile(const char * pBuff)
{
    FILE * fp = NULL;

    fp = fopen(LOG_FILE_PATH, "at+");
    if(NULL == fp)
    {
        return;
    }

    if(EOF == fputs(pBuff, fp))
    {
        return;
    }

    fclose(fp);
}


void formatLog(const char * format, 
               const char * fileName, 
               const char * funcName, 
               unsigned int line, 
               unsigned int level, 
               va_list ap)
{
    char aucTime[MAX_FORMAT_TIME_LEN] = {0};
    char aucBuff[MAX_LOG_BUFF] = {0};
    char * pBuff = NULL;

    // format time
    formatTime(aucTime);

    pBuff = aucBuff;

    pBuff += sprintf(pBuff, "%s ", aucTime);

    // format the log level
    switch(level)
    {
        case DEBUG:
            pBuff += sprintf(pBuff, "%s", "[D]");
            break;
        case ERROR:
            pBuff += sprintf(pBuff, "%s", "[E]");
            break;
        default:
            return;
    }

    // format the function name
    pBuff += sprintf(pBuff, "[%s]", funcName);

    // format the data which need display
    pBuff += vsprintf(pBuff, format, ap);

    // format the file name
    pBuff += sprintf(pBuff, "[%s, %d]\r\n", fileName, line);

    // write the format string into a file
    writeLogToFile(aucBuff);
}


void writeLog(const char * fileName,       
              const char * funcName, 
              unsigned int line, 
              unsigned int level, 
              const char * format, 
              ...)
{
    va_list ap;

	if (g_isPrintLog && level >= g_ulLogLevel)
	{
	    va_start(ap, format);

	    formatLog(format, fileName, funcName, line, level, ap);

	    va_end(ap);
	}
}
