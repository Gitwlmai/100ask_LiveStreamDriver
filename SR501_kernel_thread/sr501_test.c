#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <signal.h>



int main(int argc, char *argv[])
{
	int fd;
	int val = 0;
	if(argc != 2)
	{
		printf("usage: %s + dev_name \n", argv[0]);
		return -1;
	}

	fd = open(argv[1], O_RDWR);
	if(fd == -1)
	{
		printf("open error \n");
		return -1;
	}

	while(1)
	{
		if(read(fd, &val, 4) != 4)
		{
			close(fd);
			printf("read error \n");
			return -1;
		}

		printf("read val = %d \n", val);
	}

	close(fd);

	return 0;
}

