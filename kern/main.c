/*
 *	Copyright 1973 Bell Telephone Laboratories Inc
 */
/* SPDX License Indentifier: BSD-2.0 OR MPL-2.0 */

#include "param.h"
#include "user.h"
#include "systm.h"
#include "proc.h"
#include "text.h"
#include "inode.h"
#include "seg.h"

extern struct proc k_proc[];
// extern struct proc proc[];
extern struct inode* u_rootdir; 
// extern struct inode* rootdir; 
/* Declarações de funções externas */
extern int fubyte(int);
extern int fuword(int*);
extern void clearseg(int);
extern void mfree(int*, int, int);
extern void printf(const char*, ...);
extern int min(int, int);
extern void panic(const char*);
extern void cinit(void);
extern void binit(void);
extern void iinit(void);
extern struct inode* iget(int, int);
extern int newproc(void);
extern void expand(int);
extern void copyout(int*, int, int);
extern void sched(void);
extern int nseg(int);

/* Variáveis globais */
int lksp[] = {
    0177546,
    0172540,
    0
};

int icode[] = {
    0104413,
    0000014,
    0000010,
    0000777,
    0000014,
    0000000,
    0062457,
    0061564,
    0064457,
    0064556,
    0000164,
};

int updlock;
int* lks;

void sureg(void);
int estabur(int nt, int nd, int ns);

int main(void)
{
    register int i1, *p;

    /*
     * zero and free all of core
     */

    updlock = 0;
    /* Casting para o formato esperado pelo UNIX V6 */
    ((int*)UISA)[0] = *ka6 + USIZE;
    ((int*)UISD)[0] = 077406;
    
    for(; fubyte(0) >= 0; ((int*)UISA)[0]++) {
        clearseg(((int*)UISA)[0]);
        maxmem++;
        mfree((int*)coremap, 1, ((int*)UISA)[0]);
    }
    printf("mem = %l\n", maxmem*10/32);
    maxmem = min(maxmem, MAXMEM);
    mfree((int*)swapmap, nswap, swplo);

    /*
     * determine clock
     */

    ((int*)UISA)[7] = ka6[1]; /* io segment */
    ((int*)UISD)[7] = 077406;
    
    for(p=lksp;; p++) {
        if(*p == 0)
            panic("no clock");
        if(fuword(p) != -1) {
            lks = (int*)*p;
            break;
        }
    }

    /*
     * set up system process
     */

    k_proc[0].p_addr = *ka6;
    k_proc[0].p_size = USIZE;
    k_proc[0].p_stat = SRUN;
    k_proc[0].p_flag = SLOAD | SSYS;
    u.u_procp = (int)&k_proc[0]; 

    /*
     * set up 'known' i-nodes
     */

    sureg();
    *lks = 0115;
    cinit();
    binit();
    iinit();
    u_rootdir = iget(rootdev, ROOTINO);
    u_rootdir->i_flag &= ~ILOCK;
    u.u_cdir = iget(rootdev, ROOTINO);
    ((struct inode*)u.u_cdir)->i_flag &= ~ILOCK;

    /*
     * make init process
     * enter scheduling loop
     * with system process
     */

    if(newproc()) {
        expand(USIZE+1);
        u.u_uisa[0] = USIZE;
        u.u_uisd[0] = 6;
        sureg();
        copyout(icode, 0, 30);
        return 0;
    }
    sched();
    return 0;
}

void sureg(void)
{
    register int *up, *rp, a;

    a = ((struct proc*)u.u_procp)->p_addr; 
    up = &u.u_uisa[0];
    rp = &((int*)UISA)[0];
    while(rp < &((int*)UISA)[8])
        *rp++ = *up++ + a;
    if((up = (int*)((struct proc*)u.u_procp)->p_textp) != NULL)
        a = -((struct text*)up)->x_caddr;
    up = &u.u_uisd[0];
    rp = &((int*)UISD)[0];
    while(rp < &((int*)UISD)[8]) {
        *rp = *up++;
        if((*rp++ & WO) == 0)
            rp[((int*)UISA - (int*)UISD)/2 - 1] = -a;
    }
}

int estabur(int nt, int nd, int ns)
{
    register int a, *ap, *dp;

    if(nseg(nt)+nseg(nd)+nseg(ns) > 8 || nt+nd+ns+USIZE > maxmem) {
        u.u_error = ENOMEM;
        return(-1);
    }
    a = 0;
    ap = &u.u_uisa[0];
    dp = &u.u_uisd[0];
    while(nt >= 128) {
        *dp++ = (127<<8) | RO;
        *ap++ = a;
        a += 128;
        nt -= 128;
    }
    if(nt) {
        *dp++ = ((nt-1)<<8) | RO;
        *ap++ = a;
        a += nt;
    }
    a = USIZE;
    while(nd >= 128) {
        *dp++ = (127<<8) | RW;
        *ap++ = a;
        a += 128;
        nd -= 128;
    }
    if(nd) {
        *dp++ = ((nd-1)<<8) | RW;
        *ap++ = a;
        a += nd;
    }
    while(ap < &u.u_uisa[8]) {
        *dp++ = 0;
        *ap++ = 0;
    }
    a += ns;
    while(ns >= 128) {
        a -= 128;
        ns -= 128;
        *--dp = (127<<8) | RW;
        *--ap = a;
    }
    if(ns) {
        *--dp = ((128-ns)<<8) | RW | ED;
        *--ap = a-128;
    }
    sureg();
    return(0);
}

int nseg(int n)
{
    return((n+127)>>7);
}
