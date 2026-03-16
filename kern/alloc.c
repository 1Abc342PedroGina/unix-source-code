/*
 *	Copyright 1973 Bell Telephone Laboratories Inc
 */

#include "param.h"
#include "systm.h"
#include "filsys.h"
#include "buf.h"
#include "inode.h"
#include "user.h"


    struct mount mount[NMOUNT];  // Declarar antes de usar
    extern struct mount mount[]; // Ou declarar como extern
struct buf *bread(int, int);
struct buf *getblk(int, int);
void brelse(struct buf *);
void bcopy(char *, char *, int);
extern void panic(const char*);
extern void printf(const char*, ...);
void sleep(int *, int);
void wakeup(int *);
void prdev(char *, int);
void clrbuf(struct buf *);
void bwrite(struct buf *);
struct inode *iget(int, int);
void iput(struct inode *);
void iupdat(struct inode *, int *);
void prele(struct inode *);
void bflush(int);
int badblock(struct filsys *, int, int);
struct filsys *getfs(int);

void
iinit(void)
{
    register char *cp;
    register struct buf *bp;

    bp = bread(rootdev, 1);
    cp = (char *)getblk(NODEV, 1);
    if (u.u_error)
        panic("iinit");
    bcopy(bp->b_addr, cp, 256);
    brelse(bp);
    mount[0].m_bufp = (struct buf *)cp;
    mount[0].m_dev = rootdev;
    cp = (char *)((struct buf *)cp)->b_addr;
    ((struct filsys *)cp)->s_flock = 0;
    ((struct filsys *)cp)->s_ilock = 0;
    ((struct filsys *)cp)->s_ronly = 0;
    time[0] = ((struct filsys *)cp)->s_time[0];
    time[1] = ((struct filsys *)cp)->s_time[1];
}

struct buf *
alloc(int dev)
{
    int bno;
    register struct buf *bp;
    register struct filsys *fp;
    register int *ip;

    fp = getfs(dev);
    while (fp->s_flock)
        sleep(&fp->s_flock, PINOD);
    
    do {
        bno = fp->s_free[--fp->s_nfree];
        if (bno == 0) {
            fp->s_nfree++;
            prdev("no space", dev);
            u.u_error = ENOSPC;
            return (struct buf *)NULL;
        }
    } while (badblock(fp, bno, dev));
    
    if (fp->s_nfree <= 0) {
        fp->s_flock++;
        bp = bread(dev, bno);
        ip = (int *)bp->b_addr;
        fp->s_nfree = *ip++;
        bcopy((char *)ip, (char *)fp->s_free, 100);
        brelse(bp);
        fp->s_flock = 0;
        wakeup(&fp->s_flock);
    }
    
    bp = getblk(dev, bno); 
    clrbuf(bp);
    fp->s_fmod = 1;
    return(bp);
}

void
free(int dev, int bno)
{
    register struct filsys *fp;
    register struct buf *bp;
    register int *ip;

    fp = getfs(dev);
    fp->s_fmod = 1;
    
    while (fp->s_flock)
        sleep(&fp->s_flock, PINOD);
        
    if (badblock(fp, bno, dev))
        return;
        
    if (fp->s_nfree >= 100) {
        fp->s_flock++;
        bp = getblk(dev, bno);
        ip = (int *)bp->b_addr;
        *ip++ = fp->s_nfree;
        bcopy((char *)fp->s_free, (char *)ip, 100);
        fp->s_nfree = 0;
        bwrite(bp);
        fp->s_flock = 0;
        wakeup(&fp->s_flock);
    }
    
    fp->s_free[fp->s_nfree++] = bno;
    fp->s_fmod = 1;
}

int
badblock(struct filsys *fp, int abn, int dev)
{
    if (abn < fp->s_isize + 2 || abn >= fp->s_fsize) {
        prdev("bad block", dev);
        return(1);
    }
    return(0);
}

struct inode *
ialloc(int dev)
{
    register struct filsys *fp;
    register struct buf *bp;
    register char *ip;
    int i, j, k, ino;

    fp = getfs(dev);
    while (fp->s_ilock)
        sleep(&fp->s_ilock, PINOD);
        
loop:
    if (fp->s_ninode > 0) {
        ino = fp->s_inode[--fp->s_ninode];
        ip = (char *)iget(dev, ino);
        if (((struct inode *)ip)->i_mode == 0) {
            for (ip = (char *)&((struct inode *)ip)->i_mode; 
                 ip < (char *)&((struct inode *)ip)->i_addr[8];)
                *ip++ = 0;
            fp->s_fmod = 1;
            return (struct inode *)ip;
        }
        printf("busy i\n");
        iput((struct inode *)ip);
        goto loop;
    }
    
    fp->s_ilock++;
    ino = 0;
    
    for (i = 0; i < fp->s_isize; i++) {
        bp = bread(dev, i + 2);
        ip = bp->b_addr;
        
        for (j = 0; j < 256; j = j + 16) {
            ino++;
            if (ip[j] != 0)
                continue;
                
            for (k = 0; k < NINODE; k++)
                if (dev == (&inode)[k].i_dev && ino == (&inode)[k].i_number)
                    goto cont;
                    
            fp->s_inode[fp->s_ninode++] = ino;
            if (fp->s_ninode >= 100)
                break;
        cont:
            ;
        }
        brelse(bp);
        if (fp->s_ninode >= 100)
            break;
    }
    
    if (fp->s_ninode <= 0)
        panic("out of inodes");
        
    fp->s_ilock = 0;
    wakeup(&fp->s_ilock);
    goto loop;
}

void
ifree(int dev, int ino)
{
    register struct filsys *fp;

    fp = getfs(dev);
    if (fp->s_ilock)
        return;
    if (fp->s_ninode >= 100)
        return;
        
    fp->s_inode[fp->s_ninode++] = ino;
    fp->s_fmod = 1;
}

struct filsys *
getfs(int dev)
{
    register struct mount *p;
    register struct filsys *fp;
    register int n1, n2;

    for (p = &mount[0]; p < &mount[NMOUNT]; p++) {
        if (p->m_bufp != NULL && p->m_dev == dev) {
            fp = (struct filsys *)((struct buf *)p->m_bufp)->b_addr;
            n1 = fp->s_nfree;
            n2 = fp->s_ninode;
            
            if (n1 > 100 || n2 > 100) {
                prdev("bad count", dev);
                fp->s_nfree = 0;
                fp->s_free[0] = 0;
                fp->s_ninode = 0;
                fp->s_inode[0] = 0;
            }
            return(fp);
        }
    }
    panic("no fs");
    return NULL;
}

void
update(void)
{
    register struct inode *ip;
    register struct mount *mp;
    register struct buf *bp;

    if (updlock)
        return;
        
    updlock++;
    
    for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++) {
        if (mp->m_bufp != NULL) {
            ip = (struct inode *)((struct buf *)mp->m_bufp)->b_addr;
            
            if (((struct filsys *)ip)->s_fmod == 0 || 
                ((struct filsys *)ip)->s_ilock != 0 ||
                ((struct filsys *)ip)->s_flock != 0 || 
                ((struct filsys *)ip)->s_ronly != 0)
                continue;
                
            bp = getblk(mp->m_dev, 1);
            ((struct filsys *)ip)->s_fmod = 0;
            ((struct filsys *)ip)->s_time[0] = time[0];
            ((struct filsys *)ip)->s_time[1] = time[1];
            bcopy((char *)ip, bp->b_addr, 256);
            bwrite(bp);
        }
    }
    
    for (ip = &inode; ip < &inode + NINODE; ip++) {
        if ((ip->i_flag & ILOCK) == 0) {
            ip->i_flag = ip->i_flag | ILOCK;
            iupdat(ip, time);
            prele(ip);
        }
    }
    
    updlock = 0;
    bflush(NODEV);
}
