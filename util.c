#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int zwm_util_getenv(unsigned long pid, const char* key, char* value, int len)
{
	char path[1024];
	sprintf(path, "/proc/%lu/environ", pid);
	//printf("path: %s\n",path);
	int fd = open(path, O_RDONLY);
	if (fd > 0) {
		char environ[1024 * 64];
		char* c = environ;
		while (read(fd, c, 1)) {
			if (*c == 0) {
				char *k, *v;
				k = v = environ;
				while (*v != '=')
					v++;

				*v++ = 0;
				//printf("[%s, %s]",k,v);
				if (strcmp(k, key) == 0) {
					//printf("%s %s\n",k,v);
					strncpy(value, v, len);
                    value[len]=0;
					return strlen(value);
				}
				c = environ;
			} else {
				c++;
			}
		}
		close(fd);
	}
    return 0;
}

#if TEST
int main(int argc, char** argv)
{

	unsigned long pid = atoi(argv[1]);
	const char* key = argv[2];
	char valu[1024] = { 0 };
	if (zwm_util_getenv(pid, key, valu))
		printf("%s\n", valu);
}
#endif
