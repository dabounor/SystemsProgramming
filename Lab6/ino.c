#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <fcntl.h>
#include<ext2fs/ext2_fs.h>

#define BLKSIZE 1024
typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc GD;
typedef struct ext2_inode INODE;
typedef struct ext2_dir_entry_2 DIR;



SUPER *sp;
GD *gd;
INODE *ip;
DIR *dp;


char ibuf[BLKSIZE];


char *name[64];
char buff[BLKSIZE];
int n, blksize, dev;

int tokenize(char *pathname)
{
    n = 0;
    name[n] = strtok(pathname,"/0");
    while (name[n] != NULL)
    {
        n++;
        name[n] = strtok(NULL,"/0");
    }
    return n;
}

int get_block(int dev, int blk, char *buf)
{
    lseek(dev, blk*BLKSIZE, SEEK_SET);
    return read(dev, buf, BLKSIZE);
}

void show_dir(INODE *ip)
{
        char sbuf[BLKSIZE], temp[256];
        char *cp;
        int i;
 
        for (i=0; i < 12; i++){  // assume DIR at most 12 direct blocks
            if (ip->i_block[i] == 0)
               break;
            // YOU SHOULD print i_block[i] number here
            get_block(dev, ip->i_block[i], sbuf);

            dp = (DIR *)sbuf;
            cp = sbuf;
 
            while(cp < sbuf + BLKSIZE){
               strncpy(temp, dp->name, dp->name_len);
               temp[dp->name_len] = 0;
               printf("%4d %4d %4d %s\n", 
	             dp->inode, dp->rec_len, dp->name_len, temp);

               cp += dp->rec_len;
               dp = (DIR *)cp;
           }
        }
}


int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        printf("Must pass in the diskimage and path.\n");
    }
    else 
    {
         dev = open(argv[1], O_RDONLY);   // OR  O_RDWR //why blksize*BLK?
         if(dev == 3)
         {       
            get_block(dev, (long)1, buff);
            tokenize(argv[2]);
            sp = (SUPER *) buff;
            if(sp->s_magic == 0xEF53)
            {
                printf("Successful. It is an EXT2 File system\n");
                printf("s_inodes_count %d\n", sp->s_inodes_count);
                printf("s_blocks_count %d\n", sp->s_blocks_count);
                printf("s_r_blocks_count %d\n", sp->s_r_blocks_count);
                printf("s_free_inodes_count %d\n", sp->s_free_inodes_count);
                printf("s_free_blocks_count %d\n", sp->s_free_blocks_count);
                printf("s_first_data_block %d\n", sp->s_first_data_block);
                printf("s_log_block_size %d\n", sp->s_log_block_size);
                printf("s_blocks_per_group %d\n", sp->s_blocks_per_group);
                printf("s_inodes_per_group %d\n", sp->s_inodes_per_group);
                printf("s_mnt_count %d\n", sp->s_mnt_count);
                printf("s_max_mnt_count %d\n", sp->s_max_mnt_count);
                printf("%-30s = %8x\n", "s_magic", sp->s_magic);
                printf("s_mtime = %s\n", ctime(&sp->s_mtime));
                printf("s_wtime = %s\n", ctime(&sp->s_wtime));
                blksize = 1024 * (1 << sp->s_log_block_size);
                printf("block size = %d\n", blksize);
                printf("inode size = %d\n\n", sp->s_inode_size);

                //gd = sp->s_first_data_block + 1;
                get_block(dev, 2, buff);
                gd = (GD *) buff;

                printf("bmap %d\n", gd->bg_block_bitmap);
                printf("imap %d\n", gd->bg_inode_bitmap);
                printf("inodes_start %d\n", gd->bg_inode_table);

                read(sp->s_first_data_block, ibuf, BLKSIZE);
                ip = (INODE *)ibuf + 1; 
                show_dir(ip);

            }
            else 
            {
                printf("Warning: Not an EXT2 File system.\n");
            }
            printf("%s\n", name[1]);
         }
         else 
         {
             printf("That disk does not exist\n");
         }
    }
}
