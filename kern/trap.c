/*
 *	Copyright 1973 Bell Telephone Laboratories Inc
 */

#include "param.h"
#include "systm.h"
#include "user.h"
#include "proc.h"
#include "reg.h"
#include "seg.h"

#define EBIT    1 
#define UMODE   0170000
#define SETD    0170011

struct sysent {
    int count;
    int (*call)(void);
} sysent[64];

char regloc[8] = {
    R0, R1, R2, R3, R4, R5, R6, R7
};

/* Declarações de funções externas */
void psignal(struct proc *, int);
int fuword(int *);
int backup(int *);
int estabur(int, int, int);
void expand(int);
void copyseg(int, int);
void clearseg(int);
void savfp(void);
void savu(int *);
void swtch(void);
void panic(char *);
void printf(char *, ...);
int issig(void);
void psig(void);
void trap1(int (*)(void));

void
trap(int dev, int sp, int r1, int nps, int r0, int pc, int ps)
{
    register int i, a;
    register struct sysent *callp;
    struct proc *p;

    savfp();
    u.u_ar0 = &r0;
    
    if (dev == 8) {
        psignal((struct proc *)u.u_procp, SIGFPT);
        if ((ps & UMODE) == UMODE)
            goto err;
        return;
    }
    
    if (dev == 1 && fuword((int *)(pc-2)) == SETD)
        return;
        
    if ((ps & UMODE) != UMODE)
        goto bad;
        
    u.u_error = 0;
    
    if (dev == 9 && sp < -u.u_ssize*64) {
        if (backup(&r0) == 0)
            if (!estabur(u.u_tsize, u.u_dsize, u.u_ssize + SINCR)) {
                p = (struct proc *)u.u_procp;
                expand(p->p_size + SINCR);
                a = p->p_addr + p->p_size;
                for (i = u.u_ssize; i; i--) {
                    a--;
                    copyseg(a - SINCR, a);
                }
                for (i = SINCR; i; i--)
                    clearseg(--a);
                u.u_ssize = u.u_ssize + SINCR;
                goto err;
            }
    }
    
    switch (dev) {
    case 0:
        i = SIGBUS;
        goto def;
    case 1:
        i = SIGINS;
        goto def;
    case 2:
        i = SIGTRC;
        goto def;
    case 3:
        i = SIGIOT;
        goto def;
    case 5:
        i = SIGEMT;
        goto def;
    case 9:
        i = SIGSEG;
        goto def;
    def:
        psignal((struct proc *)u.u_procp, i);
    default:
        u.u_error = dev + 100;
    case 6:
        break;
    }
    
    if (u.u_error)
        goto err;
        
    ps = ps & ~EBIT;
    callp = &sysent[fuword((int *)(pc-2)) & 077];
    
    if (callp == sysent) { /* indirect */
        a = fuword((int *)pc);
        pc = pc + 2;
        callp = &sysent[fuword((int *)a) & 077];
        a = a + 2;
    } else {
        a = pc;
        pc = pc + callp->count * 2;
    }
    
    for (i = 0; i < callp->count; i++) {
        u.u_arg[i] = fuword((int *)a);
        a = a + 2;
    }
    
    u.u_dirp = (char *)u.u_arg[0];
    trap1(callp->call);
    
    if (u.u_error >= 100)
        psignal((struct proc *)u.u_procp, SIGSYS);
        
err:
    if (issig())
        psig();
        
    if (u.u_error != 0) {
        ps = ps | EBIT;
        r0 = u.u_error;
    }
    
    p = (struct proc *)u.u_procp;
    p->p_pri = PUSER + u.u_nice;
    
    if (u.u_dsleep++ > 15) {
        u.u_dsleep = 0;
        p->p_pri++;
        swtch();
    }
    
    return;

bad:
    printf("ka6 = %o\n", *ka6);
    printf("aps = %o\n", &ps);
    panic("trap");
}

void
trap1(int (*f)(void))
{
    savu(u.u_qsav);
    (*f)();
}

void
nosys(void)
{
    u.u_error = 100;
}

void
nullsys(void)
{
}
