#include<stdio.h>
#include<errno.h>

int main()
{
	char buf[256], *s;
	int r;
	r=mkdir("newdir", 0766);
	if(r<0)
	{
		printf("errno=%d : %s\n", errno, strerror(errno));
	}
	r=chdir("newdir");
	s=getcwd(buf,256);
	printf("CWD = %s\n", s);
}
