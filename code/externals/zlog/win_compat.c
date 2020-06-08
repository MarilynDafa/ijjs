#ifdef _MSC_VER

#include <windows.h>
#include <time.h>
#include <stdio.h>

struct tm *localtime_r(long *clock,struct tm *res)
{
    _localtime32_s(res,clock);
    return(res);
}
int open(char *f,int m,int p) {
	//HANDLE h = CreateFile(f,FILE_APPEND_DATA,
	HANDLE h = CreateFileA(f,FILE_APPEND_DATA,
			(FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE),
			NULL, 
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  return(h);
}
int write(int fd,char *data,int len) {
  int numb = 0;
  int status;

  status = WriteFile(fd,data,len,&numb,NULL);
  if(status == 0) return(-1);
  return(numb);
}
int close(int fd) {
  if(CloseHandle(fd)) return(0);
  return(-1);
}
#endif
