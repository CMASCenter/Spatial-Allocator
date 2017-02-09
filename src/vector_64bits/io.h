/************************************************************
 *  Filename: io.h                                          *
 ************************************************************
 *  Prototypes for warning and error functions              *   
 *  as well as other I/O related processing                 *
 *  used throughout the Spatial Allocator code              *
 *                                                          *
 *  added digiCheck on 4/22/2005 BDB                        *
 *************************************************************/
#ifndef IO__H
#define IO__H
int WARN(char *);
int WARN2(char *msg1, char *msg2);
int MESG(char *);
int MESG2(char *msg1, char *msg2);
void ERROR(char *, char *, int);
void ERROR_NO_EXIT(char *, char *);
int getEnvtValue(char *evName, char *evValue); 
void trim(char *inString, char *outString);
int digiCheck(char *val);
char *convertToUpper(char *input);
int strNullTerminate(char *dest, const char *src, int n);
int strBlankCopy(char *desc, const char *src, int n);
#endif
