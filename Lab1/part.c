/*
Zeid Alameedi
TITLE: Partition Table Project
***************************************************************************************************
"Source code that mimics the Linux program 'fdisk' which will print out the table of all partitions
on a system."
***************************************************************************************************
*/

#include<stdio.h>
#include<stdlib.h>
#include <fcntl.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

struct partition {
	u8 drive;             /* drive number FD=0, HD=0x80, etc. */

	u8  head;             /* starting head */
	u8  sector;           /* starting sector */
	u8  cylinder;         /* starting cylinder */

	u8  sys_type;         /* partition type: NTFS, LINUX, etc. */

	u8  end_head;         /* end head */
	u8  end_sector;       /* end sector */
	u8  end_cylinder;     /* end cylinder */

	u32 start_sector;     /* starting sector counting from 0 */
	u32 nr_sectors;       /* number of of sectors in partition */
};


int main(int argc, char *argv[])
{
    char buff[512];
    int fd = open("vdisk", 0); //open disk for read only
    if (fd == 3)
    {
        printf("Vdisk = 3. opened!\n\n");
    

    read(fd, buff, 512); //read MBR into the buff array

	//KC Wang Book Fig 7.3
	struct partition *p = &buff[0x1BE]; //ptr will point to MBR

	int i=0;
	int end_sec = 0;


		while (p->drive == 0)
		{
		//Print first four partitions
		if (i<4)
		{
				//printf("Device [%d]\t", i+1);
				printf("Start Sector: %d\t", p->start_sector);
				printf("End Sector: %d ", (p->start_sector+p->nr_sectors)-1);
				printf("\tNR Sector: %d", p->nr_sectors);
				printf("\t\tID: %x", p->sys_type);
				printf("\n");
		}
		if(i==3)
		{
		printf("Extended partition: \n");
		}
		if(p->sys_type == 5)
			 //if(p->sys_type == 5) //For the extended partition has an ID of 5
				{
						{

								//seek the next partition using lseek()
								u32 n = (p->start_sector);


								lseek(fd, (long)(n*512), SEEK_SET);


								read(fd, buff, 512);
								p = (struct partition *)&buff[0x1BE];
								printf("Start Sector: %d\t", p->start_sector);
								printf("End Sector: %d ", (p->start_sector+p->nr_sectors)-1);
								printf("\tNR Sector: %d", p->nr_sectors);
								printf("\t\tID: %x", p->sys_type);
								printf("\n");
						}

	}
				p++; //advance ptr to the next partition table in master boot record
				i++;
		}
		
	}




	close(fd);


	
    return 0;
}