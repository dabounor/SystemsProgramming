#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>
#include "type.h"

// globals
MINODE minode[NMINODE];
MINODE *root;
PROC proc[NPROC], *running;
MTABLE mntable[4], *mountPtr;

SUPER *sp;
GD *gp;
INODE *ip;

int dev, fd;
/**** same as in mount table ****/
int nblocks; // from superblock
int ninodes; // from superblock
int bmap;    // bmap block
int imap;    // imap block
int iblk;    // inodes begin block

char line[128], cmd[32], pathname[64];
char *cmds[] = {"ls", "cd", "pwd", "mkdir", "rmdir", "creat", "link", "unlink", "symlink","readlink", "quit", 0};

char gpath[128];
char *name[64];
int n;
char *disk = "mydisk";

char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

int debug = 0;

int findCmd(char *command)
{
    int i = 0;
    while (cmds[i])
    {
        if (strcmp(command, cmds[i]) == 0)
            return i;
        i++;
    }
    return -1;
}

int tokenize(char *pathname)
{
    int i;
    char *temp;
    //char copy[128];
    n = 0;
    strcpy(gpath, pathname);
    temp = strtok(gpath, "/");
    while (temp)
    {
        name[n] = temp;
        temp = strtok(0, "/");
        n++;
    }
}

int get_block(int fd, int blk, char buf[])
{
    lseek(fd, (long)blk * BLKSIZE, 0);
    read(fd, buf, BLKSIZE);
}

int put_block(int fd, int blk, char buf[])
{
    lseek(fd, (long)blk * BLKSIZE, 0);
    write(fd, buf, BLKSIZE);
}

int tst_bit(char *buf, int bit)
{
    int i, j;
    i = bit / 8;
    j = bit % 8;
    if (buf[i] & (1 << j))
        return 1;
    return 0;
}

int set_bit(char *buf, int bit)
{
    int i, j;
    i = bit / 8;
    j = bit % 8;
    buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit)
{
    int i, j;
    i = bit / 8;
    j = bit % 8;
    buf[i] &= ~(1 << j);
}

int decFreeInodes(int dev)
{
    char buf[BLKSIZE];

    // dec free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count--;
    put_block(dev, 2, buf);
}

int ialloc(int dev)
{
    int i;
    char buf[BLKSIZE];

    // read inode_bitmap block
    get_block(dev, imap, buf);

    for (i = 0; i < ninodes; i++)
    {
        if (tst_bit(buf, i) == 0)
        {
            set_bit(buf, i);
            decFreeInodes(dev);

            put_block(dev, imap, buf);

            return i + 1;
        }
    }
    printf("ialloc(): no more free inodes\n");
    return 0;
}

int incFreeInodes(int dev)
{
    char buf[BLKSIZE];

    // inc free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count++;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count++;
    put_block(dev, 2, buf);
}

int idealloc(int dev, int ino)
{
    char buf[BLKSIZE];

    // read inode_bitmap block
    get_block(dev, imap, buf);
    clr_bit(buf, ino - 1); // set to zero
    put_block(dev, imap, buf);
    incFreeInodes(dev);

    return 1;
}

int decFreeBlocks(int dev)
{
    char buf[BLKSIZE];

    // dec free blocks count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_blocks_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count--;
    put_block(dev, 2, buf);
}

int balloc(int dev)
{
    int i;
    char buf[BLKSIZE];

    // read block_bitmap block
    get_block(dev, bmap, buf);

    for (i = 0; i < nblocks; i++)
    {
        if (tst_bit(buf, i) == 0)
        {
            set_bit(buf, i);
            decFreeBlocks(dev);

            put_block(dev, bmap, buf);

            return i + 1;
        }
    }
    printf("balloc(): no more free inodes\n");
    return 0;
}

int incFreeBlocks(int dev)
{
    char buf[BLKSIZE];

    // inc free blocks count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_blocks_count++;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count++;
    put_block(dev, 2, buf);
}

int bdealloc(int dev, int bno)
{
    char buf[BLKSIZE];

    // read inode_bitmap block
    get_block(dev, bmap, buf);
    clr_bit(buf, bno - 1); // set to zero
    put_block(dev, bmap, buf);
    incFreeBlocks(dev);

    return 1;
}

int findmyname(MINODE *parent, u32 myino, char *myname)
{
    // search for myino in parent INODE;
    // copy its name string into myname[ ]

    int i;
    char *cp, c, sbuf[BLKSIZE];
    DIR *dp;
    INODE *ip;

    ip = &(parent->INODE);

    /**********  search for a file name ***************/
    for (i = 0; i < 12; i++)
    { /* search direct blocks only */
        if (debug)
            printf("search: i=%d  i_block[%d]=%d\n", i, i, ip->i_block[i]);
        if (ip->i_block[i] == 0)
            return 0;

        //getchar();

        get_block(dev, ip->i_block[i], sbuf);
        dp = (DIR *)sbuf;
        cp = sbuf;

        while (cp < sbuf + BLKSIZE)
        {
            c = dp->name[dp->name_len];
            dp->name[dp->name_len] = 0;
            if (myino == dp->inode)
            {
                strcpy(myname, dp->name);
                return 1;
            }
            dp->name[dp->name_len] = c;
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    return 0;
}

int findino(MINODE *mip, u32 *myino)
{
    // get DIR's ino into myino AND return parent's ino

    int i, pino;
    char *cp, c, sbuf[BLKSIZE];
    DIR *dp;
    INODE *ip;

    ip = &(mip->INODE);
    *myino = mip->ino;

    /**********  search for a file name ***************/
    for (i = 0; i < 12; i++)
    { /* search direct blocks only */
        if (debug)
            printf("search: i=%d  i_block[%d]=%d\n", i, i, ip->i_block[i]);
        if (ip->i_block[i] == 0)
            return 0;

        //getchar();

        get_block(dev, ip->i_block[i], sbuf);
        dp = (DIR *)sbuf;
        cp = sbuf;

        while (cp < sbuf + BLKSIZE)
        {
            c = dp->name[dp->name_len];
            dp->name[dp->name_len] = 0;

            if (!strcmp(dp->name, ".."))
            {
                return dp->inode;
            }
            dp->name[dp->name_len] = c;
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    return 0;
}

MINODE *iget(int dev, int ino)
{
    char buf[BLKSIZE];
    int blk, disp;
    INODE *ip;

    for (int i = 0; i < NMINODE; i++)
    {
        if (minode[i].ino == ino && minode[i].dev == dev)
        { //Found!
            minode[i].refCount++;
            return &minode[i];
        }
    }
    //Did not find matchning inode
    for (int i = 0; i < NMINODE; i++)
    {

        if (minode[i].refCount == 0)
        {
            minode[i].refCount = 1;
            minode[i].dev = dev;
            minode[i].ino = ino;
            minode[i].dirty = 0;
            minode[i].mounted = 0;
            minode[i].mountptr = 0;

            blk = (ino - 1) / 8 + iblk;
            disp = (ino - 1) % 8;

            get_block(dev, blk, buf);
            ip = (INODE *)buf + disp;
            minode[i].INODE = *ip;
            return &minode[i];
        }
    }
    printf("Insufficient Inodes!\n");
    return 0;
}

int iput(MINODE *mip)
{
    int i, blk, disp;
    char buf[BLKSIZE];
    INODE *ip;

    if (mip == 0)
        return 0;
    mip->refCount--;

    if (mip->refCount > 0)
        return 0; //Other is using this inode
    if (!mip->dirty)
        return 0; //Nothing has changed
    //Need to Write Inode back to disk
    if (debug)
        printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino);

    blk = (mip->ino - 1) / 8 + iblk; //block
    disp = (mip->ino - 1) % 8;       //offset

    get_block(mip->dev, blk, buf);

    ip = (INODE *)buf + disp;
    *ip = mip->INODE; //copy INODE to *ip

    put_block(mip->dev, blk, buf);
}

int getino(int dev, char *pathname)
{
    int i, ino, blk, disp;
    char buf[BLKSIZE];
    INODE *ip;
    MINODE *mip;
    dev = root->dev; // only ONE device so far

    if (debug)
        printf("getino: pathname=%s\n", pathname);
    if (strcmp(pathname, "/") == 0)
        return 2;

    if (pathname[0] == '/')
        mip = iget(dev, 2);
    else
        mip = iget(running->cwd->dev, running->cwd->ino);

    strcpy(buf, pathname);
    tokenize(buf); // n = number of token strings

    for (i = 0; i < n; i++)
    {
        if (debug)
        {
            printf("===========================================\n");
            printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);
        }
        //printf("name[%d]: %s\n",i,name[i]);
        ino = search(mip, name[i]);
        //printf("ino of %s: %d\n",name[i],ino);
        if (ino == 0)
        {
            iput(mip);
            printf("name %s does not exist\n", name[i]);
            return 0;
        }
        iput(mip);
        //printf("Entering iget in getino\n");
        mip = iget(dev, ino);
    }
    return ino;
}

int search(MINODE *mip, char *name)
{
    int i;
    char *cp, c;
    char dbuf[1024];
    DIR *dp;
    INODE *ip;

    if (debug)
        printf("Search for %s in MINODE = [%d, %d]\n", name, mip->dev, mip->ino);

    ip = &(mip->INODE);

    for (i = 0; i < 12; i++)
    { // assume: DIRs have at most 12 direct blocks
        if (debug)
            printf("i_block[%d] = %d\n", i, ip->i_block[i]);
        if (ip->i_block[i] == 0)
            return 0;
        get_block(dev, ip->i_block[i], dbuf);

        cp = dbuf;
        dp = (DIR *)dbuf;

        while (cp < dbuf + 1024)
        {
            //if (dp->name_len == 0)
            //   return 0;

            c = dp->name[dp->name_len];
            dp->name[dp->name_len] = 0;

            if (debug)
                printf("%4d %4d %4d   %s\n", dp->inode, dp->rec_len, dp->name_len, dp->name);
            if (strcmp(dp->name, name) == 0)
            {
                if (debug)
                {
                    printf("Found %s with ino %d\n", dp->name, dp->inode);
                    printf("group=%d inumber=%d\n", ip->i_gid, dp->inode - 1);
                    printf("blk = %d disp = %d\n", ip->i_block[i], 1);
                }
                return (dp->inode);
            }
            dp->name[dp->name_len] = c;
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    return 0;
}

int init(int dev)
{
    MINODE *mip;
    PROC *p;
    char buf[BLKSIZE];
    //INIT ninodes, nblocks, bmap, imap, iblk
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;

    if (sp->s_magic != 0xEF53)
    {
        printf("Not an ext2 FS\n");
        exit(1);
    }

    ninodes = sp->s_inodes_count;
    nblocks = sp->s_blocks_count;

    get_block(dev, 2, buf);
    gp = (GD *)buf;

    bmap = gp->bg_block_bitmap;
    imap = gp->bg_inode_bitmap;
    iblk = gp->bg_inode_table;
    //INIT minodes, procs
    for (int i = 0; i < NMINODE; i++)
    {
        mip = &minode[i];
        mip->dev = 0;
        mip->ino = 0;
        mip->refCount = 0;
        mip->mounted = 0;
        mip->mountptr = 0;
    }
    root = 0;
    for (int i = 0; i < NPROC; i++)
    {
        p = &proc[i];
        p->pid = i + 1;
        p->uid = i; //Not sure if 0 or i?!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        p->cwd = 0;
        for (int i = 0; i < NFD; i++)
        {
            p->fd[i] = 0;
        }
    }
}

mount_root() // mount root file system, establish / and CWDs
{

    root = iget(dev, 2);
    root->mounted = 1;
    root->mountptr = &(mntable[0]);

    mountPtr = &(mntable[0]);
    mountPtr->dev = fd;
    mountPtr->ninodes = ninodes;
    mountPtr->nblocks = nblocks;
    mountPtr->iblk = iblk;
    mountPtr->imap = imap;
    mountPtr->bmap = bmap;
    mountPtr->mountedDirPtr_inode = root;

    strcpy(mountPtr->devName, disk);
    strcpy(mountPtr->mntName, "/");

    //Let cwd of both P0 and P1 point at the root minode (refCount=3)
    //   P0.cwd = iget(dev, 2);
    //  P1.cwd = iget(dev, 2);
    proc[0].cwd = iget(dev, 2);
    proc[1].cwd = iget(dev, 2);
    
    running = &(proc[0]);
    running->cwd = iget(dev, 2);
    // Let running -> P0.
}

my_cd()
{
    char temp[256];
    char buf[BLKSIZE];
    DIR *dp;
    MINODE *ip, *newip, *cwd;
    int dev, ino;
    char c;

    if (pathname[0] == 0)
    {
        iput(running->cwd);
        running->cwd = iget(root->dev, 2);
        return;
    }

    if (pathname[0] == '/')
        dev = root->dev;
    else
        dev = running->cwd->dev;
    strcpy(temp, pathname);
    ino = getino(dev, temp);

    if (!ino)
    {
        printf("cd : no such directory\n");
        return (-1);
    }
    if (debug)
        printf("dev=%d ino=%d\n", dev, ino);
    newip = iget(dev, ino); /* get inode of this ino */

    if (debug)
        printf("mode=%4x   ", newip->INODE.i_mode);
    //if ( (newip->INODE.i_mode & 0040000) == 0){
    if (!S_ISDIR(newip->INODE.i_mode))
    {
        printf("%s is not a directory\n", pathname);
        iput(newip);
        return (-1);
    }

    iput(running->cwd);
    running->cwd = newip;

    if (debug)
        printf("after cd : cwd = [%d %d]\n", running->cwd->dev, running->cwd->ino);
}

int ls_file(MINODE *mip, char *name)
{
    int k;
    u16 mode, mask;
    char mydate[32], *s, *cp, ss[32];
    char buf[BLKSIZE];

    mode = mip->INODE.i_mode;

    if ((mode & 0xF000) == 0x8000)
        printf("%c", '-');
    if ((mode & 0xF000) == 0x4000)
        printf("%c", 'd');
    if ((mode & 0xF000) == 0xA000)
        printf("%c", 'l');

    for (int i = 8; i >= 0; i--)
    {
        if (mode & (1 << i))
            printf("%c", t1[i]);
        else
            printf("%c", t2[i]);
    }

    printf("%4d", mip->INODE.i_links_count);
    printf("%4d", mip->INODE.i_uid);
    printf("%4d", mip->INODE.i_gid);
    printf("  ");

    s = mydate;
    s = (char *)ctime(&mip->INODE.i_ctime);
    s = s + 4;

    strncpy(ss, s, 12);

    ss[12] = 0;

    printf("%s", ss);
    printf("%8ld", mip->INODE.i_size);

    printf("    %s", name);

    if (S_ISLNK(mode))
    {   get_block(dev,mip->INODE.i_block[0],buf);
        printf(" -> %s", buf);
    }
    printf("\n");
}

int ls_dir(MINODE *mip)
{
    int i;
    char sbuf[BLKSIZE], temp[256];
    DIR *dp;
    char *cp;
    MINODE *dip;

    for (i = 0; i < 12; i++)
    { /* search direct blocks only */
        if (debug)
            printf("i_block[%d] = %d\n", i, mip->INODE.i_block[i]);
        if (mip->INODE.i_block[i] == 0)
            return 0;

        get_block(mip->dev, mip->INODE.i_block[i], sbuf);
        dp = (DIR *)sbuf;
        cp = sbuf;

        while (cp < sbuf + BLKSIZE)
        {
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            dip = iget(dev, dp->inode);

            ls_file(dip, temp);
            iput(dip);

            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
}

int my_ls()
{
    MINODE *mip;
    u16 mode;
    int dev, ino;

    if (pathname[0] == 0)
    {

        ls_dir(running->cwd);
    }
    else
    {

        dev = root->dev;
        ino = getino(dev, pathname);
        if (ino == 0)
        {
            printf("no such file %s\n", pathname);
            return -1;
        }
        mip = iget(dev, ino);
        mode = mip->INODE.i_mode;
        if (!S_ISDIR(mode))
        {

            ls_file(mip, (char *)basename(pathname));
        }
        else
        {

            ls_dir(mip);
        }
        iput(mip);
    }
}

int rpwd(MINODE *wd)
{
    char buf[BLKSIZE], myname[256], *cp;
    MINODE *parent, *ip;
    u32 myino, parentino;
    DIR *dp;

    if (wd == root)
        return;

    parentino = findino(wd, &myino);
    parent = iget(dev, parentino);
    printf("pino = %d\n",parentino);
    findmyname(parent, myino, myname);
    // recursively call rpwd()
    rpwd(parent);

    iput(parent);
    printf("/%s", myname);

    return 1;
}

void my_pwd(MINODE *wd)
{
    if (wd == root)
    {
        printf("/\n");
        return;
    }
    rpwd(wd);
    printf("\n");
}

void my_pwd_cmdln(MINODE *wd)
{
    if (wd == root)
    {
        printf("/");
        return;
    }
    rpwd(wd);
}

int enter_name(MINODE *pip, int ino, char *name)
{
    int i;
    char buf[BLKSIZE];
    char *cp;
    DIR *dp;

    // Find last used block
    for (i = 0; i < 12; i++)
    {
        if (pip->INODE.i_block[i] == 0)
            break;
        get_block(dev, pip->INODE.i_block[i], buf);
        cp = buf;
        dp = (DIR *)buf;

        while (cp + dp->rec_len < buf + 1024)
        {
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        int need_length = 4 * ((8 + strlen(name) + 3) / 4);
        int last_entry_ideal_len = 4 * ((8 + dp->name_len + 3) / 4);
        int remain = dp->rec_len - dp->name_len;
        if (remain >= need_length)
        {
            dp->rec_len = last_entry_ideal_len;
            cp += dp->rec_len;
            dp = (DIR *)cp;
            dp->rec_len = remain;
            dp->inode = ino;
            dp->name_len = strlen(name);
            strncpy(dp->name, name, dp->name_len);
            put_block(dev, pip->INODE.i_block[i], buf);
            return 1;
        }
    }
    // No space in existing datablocks
    int bno = balloc(dev);
    if (bno)
    {
        pip->INODE.i_size += BLKSIZE;
        pip->INODE.i_block[i] = bno;
        get_block(dev, bno, buf);
        cp = buf;
        dp = (DIR *)buf;
        dp->name_len = strlen(name);
        dp->rec_len = BLKSIZE;
        dp->inode = ino;
        strncpy(dp->name, name, dp->name_len);
        put_block(dev, pip->INODE.i_block[i], buf);
    }
    else
        printf("Could not allocate new block\n");

    return 0;
}

int mymkdir(MINODE *pip, char *name)
{
    int ino, bno;
    MINODE *mip;
    char buf[BLKSIZE];
    ino = ialloc(dev);
    if (!ino)
    {
        printf("Could not allocate new inode\n");
        return -1;
    }
    bno = balloc(dev);
    if (!bno)
    {
        printf("could not allocate new block\n");
        return -1;
    }

    mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    ip->i_mode = 0x41ED;                                // OR 040755: DIR type and permissions
    ip->i_uid = running->uid;                           // Owner uid
    ip->i_gid = running->gid;                           // Group Id
    ip->i_size = BLKSIZE;                               // Size in bytes
    ip->i_links_count = 2;                              // Links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); // set to current time
    ip->i_blocks = 2;                                   // LINUX: Blocks count in 512-byte chunks
    ip->i_block[0] = bno;                               // new DIR has one data block
    for (int i = 0; i < 15; i++)
        ip->i_block[i] = 0;

    ip->i_block[0] = bno;
    mip->dirty = 1; // mark minode dirty
    iput(mip);      // write INODE to disk

    // Add . and .. to dir
    DIR *dp;
    char *cp;
    memset(buf, 0, BLKSIZE);
    dp = (DIR *)buf;
    dp->inode = ino;
    strncpy(dp->name, ".", 1);
    dp->name_len = 1;
    dp->rec_len = 12;

    cp = buf;
    cp += dp->rec_len;
    dp = (DIR *)cp;

    dp->inode = pip->ino;
    strncpy(dp->name, "..", 2);
    dp->name_len = 2;
    dp->rec_len = BLKSIZE - 12;
    put_block(dev, bno, buf);

    // Add new dir to parent dir
    enter_name(pip, ino, name);
}

int my_mkdir()
{
    MINODE *mip, *pmip;
    char *parent, *child, *dircopy, *basecopy;
    int pino;

    if (pathname[0] == '/')
    {
        mip = root;
        dev = root->dev;
    }
    else
    {
        mip = running->cwd;
        dev = running->cwd->dev;
    }

    dircopy = strdup(pathname);
    basecopy = strdup(pathname);
    parent = dirname(dircopy);
    child = basename(basecopy);
    if (debug)
        printf("parent: %s  child: %s\n", parent, child);

    pino = getino(dev, parent);
    if (!pino)
        return -1;
    pmip = iget(dev, pino);

    //check parent is a directory
    int mode = pmip->INODE.i_mode;
    if (!S_ISDIR(mode))
    {
        printf("%s is not a directory\n", parent);
        return -1;
    }
    //check if new dir already exists
    if (search(pmip, child))
    {
        printf("%s already exists in %s\n", child, parent);
        return -1;
    }

    mymkdir(pmip, child);

    return 0;
}

int rm_child(MINODE *pip, char *name)
{
    char *cp, buf[BLKSIZE];
    DIR *dp, *prev;
    int current_position;

    for (int i = 0; i < 12; i++)
    {
        current_position = 0;
        if (pip->INODE.i_block[i] == 0)
        {
            printf("%s does not exist\n", name);
            return -1;
        }
        get_block(dev, pip->INODE.i_block[i], buf);

        cp = buf;
        dp = (DIR *)buf;
        prev = 0;
        while (cp < buf + BLKSIZE)
        {
            if (strlen(name) == dp->name_len)
                if (strncmp(dp->name, name, dp->name_len) == 0)
                { // FOUND
                    int ideal_len = 4 * ((8 + dp->name_len + 3) / 4);
                    if (ideal_len != dp->rec_len)
                    { // IT IS LAST ENTRY IN BLOCK
                        prev->rec_len += dp->rec_len;
                    }
                    else if (prev == 0)
                    { // IT IS FIRST ENTRY IN BLOCK
                        bdealloc(dev, pip->INODE.i_block[i]);
                        pip->INODE.i_size -= BLKSIZE;
                        for (int j = i; j < 12; j++)
                            if (j == 11)
                                pip->INODE.i_block[11] = 0;
                            else
                                pip->INODE.i_block[j] = pip->INODE.i_block[j + 1];
                    }
                    else
                    { // IT IS IN MIDDLE OF BLOCK
                        int removed_len = dp->rec_len;
                        char *temp = cp;
                        DIR *last = (DIR *)temp;
                        while (cp < buf + BLKSIZE)
                        { // get last entry
                            temp += last->rec_len;
                            last = (DIR *)temp;
                        }
                        last->rec_len += removed_len;
                        //move all entries after current to where current begins
                        memcpy(cp, cp + removed_len, BLKSIZE - current_position - removed_len);
                    }
                    put_block(dev, pip->INODE.i_block[i], buf);
                    pip->dirty = 1;
                    return 1;
                }
            cp += dp->rec_len;
            current_position += dp->rec_len;
            prev = dp;
            dp = (DIR *)cp;
        }
    }
    return 0;
}

int my_rmdir()
{
    int ino, pino;
    MINODE *mip, *pip;
    char *parent, *child, *dircopy, *basecopy, buf[BLKSIZE], *cp;
    DIR *dp;

    dircopy = strdup(pathname);
    basecopy = strdup(pathname);
    parent = dirname(dircopy);
    child = basename(basecopy);

    ino = getino(dev, pathname);
    mip = iget(dev, ino);
    pino = getino(dev, parent);

    /*
    if(running->uid != mip->INODE.i_uid)
    {
        printf("You do not have permission to remove this dir\n");
        return -1;
    }*/
    int mode = mip->INODE.i_mode;
    if (!S_ISDIR(mode))
    {
        printf("%s is not a directory\n", child);
        iput(mip);
        return -1;
    }
    if (mip->INODE.i_links_count > 2)
    {
        printf("dir is not empty. cannot remove\n");
        iput(mip);
        return -1;
    }
    if (mip->INODE.i_links_count == 2)
    { // check if empty. May still have files
        get_block(dev, mip->INODE.i_block[0], buf);
        cp = buf;
        dp = (DIR *)buf;
        cp += dp->rec_len;
        dp = (DIR *)cp;          //get second entry ".."
        if (dp->rec_len != 1012) // dir is non empty
        {
            printf("dir is not empty. cannot remove\n");
            iput(mip);
            return -1;
        }
    }
    // dir can be removed

    // deallocate blocks and inode
    for (int i = 0; i < 12; i++)
    {
        if (mip->INODE.i_block[i] == 0)
            continue;
        bdealloc(mip->dev, mip->INODE.i_block[i]);
    }
    idealloc(mip->dev, mip->ino);
    iput(mip);

    pip = iget(dev, pino);
    rm_child(pip, child);
    pip->INODE.i_links_count--;
    pip->INODE.i_atime = pip->INODE.i_mtime = time(0L);
    pip->dirty = 1;
    iput(pip);

    return 1;
}

int mycreat(MINODE *pip, char *name)
{
    int ino, bno;
    MINODE *mip;
    char buf[BLKSIZE];
    ino = ialloc(dev);
    if (!ino)
    {
        printf("Could not allocate new inode\n");
        return -1;
    }
    bno = balloc(dev);
    if (!bno)
    {
        printf("could not allocate new block\n");
        return -1;
    }

    mip = iget(dev, ino);
    INODE *ip = &(mip->INODE);
    ip->i_mode = 0x81A4;                                // OR 040755: DIR type and permissions
    ip->i_uid = running->uid;                           // Owner uid
    ip->i_gid = running->gid;                           // Group Id
    ip->i_size = 0;                                     // Size in bytes
    ip->i_links_count = 2;                              // Links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); // set to current time
    ip->i_blocks = 1;                                   // LINUX: Blocks count in 512-byte chunks
    ip->i_block[0] = bno;                               // new DIR has one data block
    for (int i = 1; i < 15; i++)
        ip->i_block[i] = 0;

    mip->dirty = 1; // mark minode dirty
    iput(mip);      // write INODE to disk

    // Add new dir to parent dir
    enter_name(pip, ino, name);
}

int my_creat()
{
    MINODE *mip, *pmip;
    char *parent, *child, *dircopy, *basecopy;
    int pino;

    if (pathname[0] == '/')
    {
        mip = root;
        dev = root->dev;
    }
    else
    {
        mip = running->cwd;
        dev = running->cwd->dev;
    }

    dircopy = strdup(pathname);
    basecopy = strdup(pathname);
    parent = dirname(dircopy);
    child = basename(basecopy);
    printf("parent: %s  child: %s\n", parent, child);

    pino = getino(dev, parent);
    if (!pino)
        return -1;
    pmip = iget(dev, pino);

    //check parent is a directory
    int mode = pmip->INODE.i_mode;
    if (!S_ISDIR(mode))
    {
        printf("%s is not a directory\n", parent);
        return -1;
    }
    //check if new dir already exists
    if (search(pmip, child))
    {
        printf("%s already exists in %s\n", child, parent);
        return -1;
    }

    mycreat(pmip, child);

    return 0;
}

int my_link()
{
    MINODE *pmipNew, *mipOld;
    char *oldf_dname, *oldf_bname, *newf_dname, *newf_bname;
    char *dircopy, *basecopy;
    char oldf_pathname[64], newf_pathname[64];
    int pino, pinoNew, inoOld, mode;

    sscanf(pathname, "%s %s", oldf_pathname, newf_pathname);

    basecopy = strdup(oldf_pathname);
    oldf_bname = basename(basecopy);

    inoOld = getino(dev, oldf_pathname);
    if (!inoOld) //Check a/b/c exists
        return -1;
    mipOld = iget(dev, inoOld);

    //Check old file is a reg or link file type

    if (!S_ISREG(mipOld->INODE.i_mode) && !S_ISLNK(mipOld->INODE.i_mode))
    {
        printf("%s is not a regular or link file\n", oldf_bname);
        return -1;
    }
    printf("newfile pathname: %s\n", newf_pathname);
    dircopy = strdup(newf_pathname);
    basecopy = strdup(newf_pathname);
    newf_dname = dirname(dircopy);
    newf_bname = basename(basecopy);
    printf("New file name to link: %s\n", newf_bname);
    pinoNew = getino(dev, newf_dname);
    if (!pinoNew) //Check x/y dir exists
        return -1;
    pmipNew = iget(dev, pinoNew);
    mode = pmipNew->INODE.i_mode;

    //Check x/y is a dir
    if (!S_ISDIR(mode))
    {
        printf("%s is not a dir\n", newf_dname);
        return -1;
    }

    //check if newfile already exists
    if (search(pmipNew, newf_bname))
    {
        printf("%s already exists in %s\n", newf_bname, newf_dname);
        return -1;
    }
    //Must make sure it is setting it as a link file not reg file! Will need to modify enter_name
    enter_name(pmipNew, inoOld, newf_bname); // add newfile z to datablock of x/y with ino of oldfile a/b/c

    mipOld->INODE.i_links_count++; // increment link count of old file
    mipOld->dirty = 1;             //put some dirt on it.
    iput(mipOld);
    //pmipNew->dirty = 1;
    //iput(pmipNew);
}

int my_truncate(INODE *ip)
{
    int x;
    int size = ip->i_size;
    int blocks = ip->i_size / BLKSIZE + 1;
    int blocks_remaining = blocks;
    char *dbuf[1024];
    INODE *ipl;

    for (x = 0; ip->i_block[x] != 0 && x < 12; x++)
    {

        bdealloc(dev, ip->i_block[x]);
        blocks_remaining--;
    }
    if (blocks_remaining)
    {
        get_block(dev, ip->i_block[12], dbuf);
        char *cp = dbuf;
        dp = (DIR *)dbuf;

        while (cp < dbuf + 1024 && blocks_remaining && dp->inode)
        {
            ipl = iget(dev, dp->inode);
            for (int i = 0; ipl->i_block[i] != 0 && i < 12; i++)
            {
                bdealloc(dev, ipl->i_block[i]);
            }
            cp += 4;
            dp = (DIR *)cp;
            blocks_remaining--;
        }
    }
    if (blocks_remaining)
    {
        get_block(dev, ip->i_block[13], dbuf);
        char *cp = dbuf;
        dp = (DIR *)dbuf;

        while (cp < dbuf + 1024 && blocks_remaining && dp->inode)
        {
            printf("%d ", dp->inode);
            get_block(dev, dp->inode, dbuf);
            while (cp < dbuf + 1024 && blocks_remaining && dp->inode)
            {
                ipl = iget(dev, dp->inode);
                for (int i = 0; ipl->i_block[i] != 0 && i < 12; i++)
                {
                    bdealloc(dev, ipl->i_block[i]);
                }
                cp += 4;
                dp = (DIR *)cp;
                blocks_remaining--;
            }
        }
        //printf("\nBlocks remaining = %d\n", blocks_remaining);
    }
}

int my_unlink()
{
    MINODE *mip, *pmip;
    INODE *ip;
    int ino, pino, mode;
    char *basecopy, *dircopy, *bnamefile, *dnamefile;

    basecopy = strdup(pathname);
    dircopy = strdup(pathname);
    bnamefile = basename(basecopy);
    dnamefile = dirname(dircopy);

    ino = getino(dev, pathname);
    if (!ino)
        return -1;
    mip = iget(dev, ino);
    mode = mip->INODE.i_mode;
    //Check it is a reg or lnk file
    if (!S_ISREG(mode) && !S_ISLNK(mode))
    {
        printf("%s is not a regular or link file\n", bnamefile);
        return -1;
    }
    mip->INODE.i_links_count--; //decrement link count

    if (!mip->INODE.i_links_count) //link count is 0
    {
        printf("link count is 0, removing %s\n", bnamefile);
        my_truncate(&mip->INODE); //deallocate all data blocks of INODE
        idealloc(dev, mip->ino);  //deallocate INODE
        //iput(mip);
    }
    printf("Getting parent inode");
    pino = getino(dev, dnamefile); //get ino of parent
    if (!pino)
        return -1;
    pmip = iget(dev, pino); //parent minode
    printf("removing %s from parent inode\n", bnamefile);
    rm_child(pmip, bnamefile); // remove it
}

int my_symlink()
{
    MINODE *pmipNew, *mipOld, *mipNew;
    char *old_dname, *old_bname, *new_dname, *new_bname;
    char *dircopy, *basecopy;
    char old_pathname[64], new_pathname[64];
    int pinoOld, pinoNew, inoOld, inoNew, mode;

    //old_pathname:= a/b/c   new_pathname:= x/y/z  link z to c
    sscanf(pathname, "%s %s", old_pathname, new_pathname);

    basecopy = strdup(old_pathname);
    old_bname = basename(basecopy);

    inoOld = getino(dev, old_pathname);
    if (!inoOld) //Check a/b/c exists
        return -1;
    mipOld = iget(dev, inoOld);

    //Check oldname is a directory or reg file type

    if (!S_ISREG(mipOld->INODE.i_mode) && !S_ISDIR(mipOld->INODE.i_mode))
    {
        printf("%s is not a directory or a regular file\n", old_bname);
        return -1;
    }

    strcpy(pathname, new_pathname); //Setting up Pathname:=new_pathname for use in my_creat()
    my_creat();                     //Create file x/y/z
    inoNew = getino(dev, new_pathname);
    if (!inoNew) //Check x/y/z was created
        return -1;
    mipNew = iget(dev, inoNew);
    mipNew->INODE.i_mode = 0120000; //Make it a link file
    enter_name_link_file(mipNew,old_pathname);
    mipNew->dirty = 1;
    iput(mipNew);
}


int enter_name_link_file(MINODE *ip, char *name)
{
    char buf[BLKSIZE];
   
    if (ip->INODE.i_block[0] == 0){
        return -1;
    }
    get_block(dev, ip->INODE.i_block[0], buf);
    memset(buf,'\0',sizeof(buf));

    if (strlen(name) < 84)
    {
        ip->INODE.i_size = strlen(name);
        strncpy(buf, name, strlen(name));
        buf[strlen(name)] = '\0';
        put_block(dev, ip->INODE.i_block[0], buf);
        return 1;
    }
    else
    {
        ip->INODE.i_size = strlen(name);
        strncpy(buf, name, 83); //up to 84 bytes
        buf[strlen(name)]='\0';
        put_block(dev, ip->INODE.i_block[0], buf);
        return 1;
    }
}

int my_readlink()
{
    MINODE *mip;
    int ino, mode;
    char buf[BLKSIZE], *basecopy,*filename;

    basecopy = strdup(pathname);
    filename = basename(basecopy);

    ino = getino(dev, pathname);
    if (!ino)
        return -1;
    mip = iget(dev, ino);
    mode = mip->INODE.i_mode;
    //Check it is a reg or lnk file
    if (!S_ISLNK(mode))
    {
        printf("%s is not a link file\n", filename);
        return -1;
    }
    get_block(dev, mip->INODE.i_block[0], buf);
    printf("%s\n",buf);
}

int quit()
{
    printf("quiting...\n");
    for (int i = 0; i < NMINODE; i++)
    {
        if (minode[i].refCount > 0 && minode[i].dirty)
        {
            iput(&minode[i]);
        }
    }
    exit(0);
}




// MAIN PROGRAM STARTS HERE //
int (*fptr[])(char *) = {(int (*)())my_ls, my_cd, my_pwd, my_mkdir, my_rmdir, my_creat, quit, 0}; 
int main(int argc, char *argv[])
{
    int ino, index;
    char buf[BLKSIZE], hostname[64], username[64];

    gethostname(hostname, 64);        //Get hostname via built in system call
    strcpy(username, getenv("USER")); //Get username

    if (argc > 1)
        disk = argv[1];
    fd = open(disk, O_RDWR);

    if (fd < 0)
    {
        printf("Failed to open %s\n", disk);
        exit(1);
    }

    dev = fd;
    init(dev);
    mount_root();

    while (1)
    {
        for (int i = 0; i < 64; i++)
            name[i] = 0;
        *pathname = 0;

        printf("\nCMDs:= [ls|cd|pwd|mkdir|rmdir|creat|link|unlink|symlink|readlink|quit]\n");

        printf("%s@%s ~", username, hostname);
        my_pwd_cmdln(running->cwd);
        printf(" $ ");

        fgets(line, 128, stdin);
        sscanf(line, "%s %[^\n]s", cmd, pathname);
        if (debug)
            printf("cmd=%s pathname=%s\n", cmd, pathname);
        //strcpy(buf, pathname);
        index = findCmd(cmd);
        switch (index)
        {
        case (-1):
            printf("invalid command\n");
            break;
        case (0):
            my_ls();
            break;
        case (1):
            my_cd();
            break;
        case (2):
            my_pwd(running->cwd);
            break;
        case (3):
            my_mkdir();
            break;
        case (4):
            my_rmdir();
            break;
        case (5):
            my_creat();
            break;
        case (6):
            my_link();
            break;
        case (7):
            my_unlink();
            break;
        case (8):
            my_symlink();
            break;
        case (9):
            my_readlink();
            break;
        case (10):
            quit();
            break;
        }
    }
}