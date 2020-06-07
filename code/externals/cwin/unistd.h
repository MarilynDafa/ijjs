#pragma once
#include <stdio.h>
#include <string.h> 
extern int		opterr;			// global - if nonzero print errors - not implemented
extern int		optopt;			// global - unknown option character - not implemented
extern char* optarg;			// global - currnet option argument pointer
extern int		optind; 	// global - next argv index

#include <process.h>
extern int getopt(int argc, char** argv, const char* options);


int fsync(FILE* fp);