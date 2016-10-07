#include <stdio.h>
#include <fcntl.h>


#include "structs.h"
#include "comm.h"
#include "house.h"

main()
{
	int i;
	int fd = open("hcontrol", O_RDWR);
	int num_read;
	struct house_control_rec hc;
	struct house_control_rec_old hc_old;
	int fd_out = open("hcontrol.new", O_WRONLY);

	if (fd == -1)
	{
		printf("No file\n");
		exit(0);
	}
	printf("Sizeof house_control_rec is %d\n", sizeof(struct house_control_rec));
	
	num_read = read(fd, &hc_old, sizeof(struct house_control_rec_old));
	while (num_read)
	{
		hc.vnum = hc_old.vnum;
		hc.atrium = hc_old.atrium;
		hc.built_on = hc_old.built_on;
		hc.mode = hc_old.mode;
		hc.owner = hc_old.owner;
		hc.num_of_guests = 0;
		hc.last_payment = hc_old.last_payment;

	printf("House vnum %d\n", hc.vnum);

		for (i=0; i<MAX_GUESTS; i++)
			hc.guests[i] = 0;

		write(fd_out, &hc, sizeof(struct house_control_rec));
		num_read = read(fd, &hc_old, sizeof(struct house_control_rec_old));
	}	
	close(fd);
	close(fd_out);
} 
