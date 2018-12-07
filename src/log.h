#ifndef _LOG__H__
#define _LOG__H__

#ifdef __cplusplus  
extern "C" {
#endif


enum tagLOGLEVEL
{
    DEBUG, 
    ERROR, 
    LOG_BUTT
};

void log_init(unsigned int level);

void set_log_level(unsigned int level);

void set_log_enable();

void set_log_disable();

void writeLog(const char * fileName,       
              const char * funcName, 
              unsigned int level, 
              unsigned int line, 
              const char * format, 
              ...);

#define log_debug(format, ...) \
    do \
    { \
        writeLog(__FILE__, __FUNCTION__, __LINE__, DEBUG, format, ##__VA_ARGS__); \
    }while(0)

#define log_error(format, ...) \
    do \
    { \
        writeLog(__FILE__, __FUNCTION__, __LINE__, ERROR, format, ##__VA_ARGS__); \
    }while(0)

#ifdef __cplusplus  
}
#endif

#endif

