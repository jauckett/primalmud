/*        
MODULE_START
================================================================================
FILE
	debug.h		  

MODULE     
	DEBUG_LOG
	
DESCRIPTION  
	Header file for the debug and logging library

NOTES                                            
	includes debug, malloc_debug, logging                                         
================================================================================
MODULE_END
*/        

void DebugInit(char *fname);         
void WriteDebug(char *SourceFile, long SourceLine, char *fmt, ...);
void DebugClose();                                 
void TimeStamp(FILE *fd);

void MemDebugInit(char *fname);
 
#define DEBUG(x) WriteDebug(__FILE__, __LINE__, (x)) 