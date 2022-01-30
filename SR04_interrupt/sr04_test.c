#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <signal.h>



//sr04test /dev/sr04_dev

int main(int argc, char *argv[])
{
	int fd;
	unsigned int time_ns = 0;
	unsigned int time_us = 0;
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
		if(read(fd, &time_ns, 4) != 4)
		{
			close(fd);
			printf("read error \n");
			return -1;
		}
		time_us = time_ns / 1000;
		printf("read time = %d us \n", time_us);
		printf("read distance = %d \n", time_us * 340 / 2 / 1000);
		sleep(1);
	}

	close(fd);

	return 0;
}

