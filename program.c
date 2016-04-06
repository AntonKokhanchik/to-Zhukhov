#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <sys/wait.h>

#define bufsiz 512

void list_dir(char* srs, char* dst, int temporary);
void copy(char* srs_name, char* dst_name);
void _mkdir(const char* dir);
void forker(int temporary, char* m);
//количество потоков, на которые всё распределится, название папки, которую копируем, и название папки, в которую копируем

#define PERMS 0777
int main (int argc, char* argv[])
{
	int temporary = creat("/tmp/tmp", PERMS);

	if (argc != 4)
	{
		printf("Wrong number of parameters");
		return 1;
	}

	list_dir(argv[2],argv[3], temporary);

	forker(temporary, argv[1]);

	return 0;
}

void list_dir(char* srs, char* dst, int temporary)
{
	DIR* dir = opendir(srs);
	struct dirent* current;

	if(!dir)
	{
		printf("Directory error");
		exit(1);
	}

	if(!opendir(dst))
		_mkdir(dst);
	while ((current = readdir(dir)) != NULL)
	{
		if((strcmp(current->d_name, ".") == 0) || (strcmp(current->d_name, "..") == 0))
			continue;

		char srs_name[bufsiz];
		strcpy(srs_name, srs);
		strcat(srs_name,"/");
		strcat(srs_name,current->d_name);

		char dst_name[bufsiz];
		strcpy(dst_name, dst);
		strcat(dst_name,"/");
		strcat(dst_name,current->d_name);

		if(current->d_type == DT_DIR)
		{
			list_dir(srs_name, dst_name, temporary);
			continue;
		}

		char* buf = srs_name;		
		write(temporary, buf, bufsiz)==-1;
		buf = dst_name;
		write(temporary, buf, bufsiz)==-1;

	}
}

void copy(char* srs_name, char* dst_name)
{
	int srs, dst, n;
	char buf[BUFSIZ];
	if ((srs = open(srs_name, O_RDONLY, 0)) == -1)
	{
		printf("open error %s",srs_name);
		exit(1);
	}
	if ((dst = creat(dst_name, PERMS)) == -1)
	{
		printf("create error %s",dst_name);
		exit(1);
	}
	while ((n = read(srs, buf, BUFSIZ)) > 0)
		if (write(dst, buf, n) != n)
		{
			printf("write error %s", dst_name);
			exit(1);
		}
	close(srs);
	close(dst);
}

void _mkdir(const char *dir) 
{
        char tmp[256];
        char *p = NULL;
        size_t len;
 
        snprintf(tmp, sizeof(tmp),"%s",dir);
        len = strlen(tmp);
        if(tmp[len - 1] == '/')
                tmp[len - 1] = 0;
        for(p = tmp + 1; *p; p++)
                if(*p == '/') 
		{
                        *p = 0;
                        mkdir(tmp, PERMS);
                        *p = '/';
                }
        mkdir(tmp, PERMS);
}

void forker(int temporary, char* m)
{
	char buf[2*bufsiz];
	close(temporary);
	
	temporary = open("/tmp/tmp", O_RDONLY, 0);

	int n = atoi(m);
	pid_t pid = fork();

	for(int i = 0; i < n-1; i++)
	{
		if(pid != 0)
			pid = fork();
	}
	
	if(!pid)
	{
		int c = 0;
		char srs[bufsiz];
		char dst[bufsiz];
		while (read(temporary, buf, 2*bufsiz) > 0)
		{
			strncpy(srs, buf, bufsiz);
			strncpy(dst, buf+bufsiz, bufsiz);
			copy(srs, dst);
			c++;
		}
		printf("I`m done, %d, %d files copied\n", getpid(), c);
	}
	if(pid)
	{
		for (int i=0; i<n; i++)
			wait(0);
		remove("/tmp/tmp");
		printf("Parent: I`m done, %d\n", getpid());
	}	
	
}

