
#include "param.h"
#include "user.h"
#include "filsys.h"
#include "file.h"
#include "conf.h"
#include "inode.h"
#include "reg.h"

extern int nchrdev;
extern int nblkdev;
extern struct cdevsw cdevsw[];
extern struct bdevsw bdevsw[];
struct mount {
};

typedef struct mount	mount;
struct file *maxfp;

/* Declarações de funções externas */
struct file *getf(int);
void closei(struct inode *, int);
void iput(struct inode *);
struct inode *namei(int (*)(), int);
int uchar(void);
int suser(void);
struct file *falloc(void);
int ufalloc(void);
void printf(char *, ...);
struct filsys *getfs(int);
void wakeup(int *);

/*
 * Get file pointer from file descriptor
 */
struct file *
getf(int f)
{
    register struct file *fp;
    int *ofile;
    int rf;

    rf = f;
    if (rf < 0 || rf >= NOFILE)
        goto bad;
    
    ofile = (int *)u.u_ofile;
    fp = (struct file *)ofile[rf];
    if (fp == NULL) {
    bad:
        u.u_error = EBADF;
        fp = NULL;
    }
    return(fp);
}

/*
 * Close a file
 */
void
closef(struct file *fp)
{
    register struct inode *ip;

    if (fp->f_flag & FPIPE) {
        ip = (struct inode *)fp->f_inode;
        ip->i_mode = ip->i_mode & ~(IREAD | IWRITE);
        wakeup((int *)((char *)ip + 1));
        wakeup((int *)((char *)ip + 2));
    }
    
    if (fp->f_count <= 1)
        closei((struct inode *)fp->f_inode, fp->f_flag & FWRITE);
        
    fp->f_count--;
}

/*
 * Close inode
 */
void
closei(struct inode *ip, int rw)
{
    int dev, maj;

    dev = ip->i_addr[0];
    maj = ((union { int i; struct { char d_minor; char d_major; } d; } *)&dev)->d.d_major;
    
    if (ip->i_count <= 1) {
        switch (ip->i_mode & IFMT) {
        case IFCHR:
            if (maj < nchrdev)
                (*cdevsw[maj].d_close)(dev, rw);
            break;
            
        case IFBLK:
            if (maj < nblkdev)
                (*bdevsw[maj].d_close)(dev, rw);
            break;
        }
    }
    
    iput(ip);
}

/*
 * Open inode
 */
void
openi(struct inode *ip, int rw)
{
    int dev, maj;

    dev = ip->i_addr[0];
    maj = ((union { int i; struct { char d_minor; char d_major; } d; } *)&dev)->d.d_major;
    
    switch (ip->i_mode & IFMT) {
    case IFCHR:
        if (maj >= nchrdev)
            goto bad;
        (*cdevsw[maj].d_open)(dev, rw);
        break;
        
    case IFBLK:
        if (maj >= nblkdev) {
        bad:
            u.u_error = ENXIO;
            return;
        }
        (*bdevsw[maj].d_open)(dev, rw);
        break;
    }
}

/*
 * Check access permissions
 */
int
access(struct inode *ip, int mode)
{
    int m;

    m = mode;
    
    if (m == IWRITE) {
        if (getfs(ip->i_dev)->s_ronly != 0) {
            u.u_error = EROFS;
            return(1);
        }
        if (ip->i_flag & ITEXT) {
            u.u_error = ETXTBSY;
            return(1);
        }
    }
    
    if (u.u_uid == 0) {
        if (m == IEXEC && (ip->i_mode & (IEXEC | (IEXEC>>3) | (IEXEC>>6))) == 0)
            return(1);
        return(0);
    }
    
    if (u.u_uid != ip->i_uid) {
        m = m >> 3;
        if (u.u_gid != ip->i_gid)
            m = m >> 3;
    }
    
    if ((ip->i_mode & m) != 0)
        return(0);
        
    u.u_error = EACCES;
    return(1);
}

/*
 * Get owner of file
 */
struct inode *
owner(void)
{
    register struct inode *ip;
    extern int uchar(void);

    ip = namei(uchar, 0);
    if (ip == NULL)
        return NULL;
        
    if (u.u_uid == ip->i_uid)
        return(ip);
        
    if (suser())
        return(ip);
        
    iput(ip);
    return NULL;
}

/*
 * Check superuser
 */
int
suser(void)
{
    if (u.u_uid == 0)
        return(1);
        
    u.u_error = EPERM;
    return(0);
}

/*
 * Allocate user file descriptor
 */
int
ufalloc(void)
{
    register int i;
    int *ofile;

    ofile = (int *)u.u_ofile;
    for (i = 0; i < NOFILE; i++) {
        if (ofile[i] == NULL) {
            u.u_ar0[R0] = i;
            return(i);
        }
    }
    
    u.u_error = EMFILE;
    return(-1);
}

/*
 * Allocate file structure
 */
struct file *
falloc(void)
{
    register struct file *fp;
    register int i;
    int *ofile;

    i = ufalloc();
    if (i < 0)
        return NULL;
    
    ofile = (int *)u.u_ofile;
    
    for (fp = &file; fp < &file + NFILE; fp++) {
        if (fp->f_count == 0) {
            ofile[i] = (int)fp;
            fp->f_count++;
            fp->f_offset[0] = 0;
            fp->f_offset[1] = 0;
            if (fp > maxfp)
                maxfp = fp;
            return(fp);
        }
    }
    
    printf("no file\n");
    u.u_error = ENFILE;
    return NULL;
}
