#include "param.h"
#include "systm.h"
#include "user.h"
#include "proc.h"

#define UMODE       0170000
#define SCHMAG      10
#define HZ          60

extern void display(void);
extern int spl5(void);
extern int spl1(void);
extern int spl7(void);
extern void splx(int);
extern void wakeup(int *);
extern void incupc(int, int *);
extern void setpri(struct proc *);
extern int issig(void);
extern void psig(void);

extern int lbolt;
extern int time[2];
extern int tout[2];
extern char runin;
extern char runrun;

struct callo callout_table[NCALL]; 

void
clock(int dev, int sp, int r1, int nps, int r0, int pc, int ps)
{
    struct callo *p1, *p2;
    struct proc *pp;
    int s;

    /* Reinicia o clock */
    *lks = 0115;

    /* Atualiza o display */
    display();

    /*
     * Processa callout_tables
     */
    if (callout_table[0].c_func == 0)
        goto out;

    p2 = &callout_table[0];
    while (p2->c_time <= 0 && p2->c_func != 0)
        p2++;
    p2->c_time--;

    if ((ps & 0340) != 0)
        goto out;

    s = spl5();
    if (callout_table[0].c_time <= 0) {
        p1 = &callout_table[0];
        while (p1->c_func != 0 && p1->c_time <= 0) {
            (*p1->c_func)(p1->c_arg);
            p1++;
        }
        p2 = &callout_table[0];
        while (p1->c_func != 0) {
            p2->c_func = p1->c_func;
            p2->c_time = p1->c_time;
            p2->c_arg = p1->c_arg;
            p1++;
            p2++;
        }
        p2->c_func = 0;
    }
    splx(s);

out:
    /* Atualiza contadores de tempo */
    if ((ps & UMODE) == UMODE) {
        u.u_utime++;
        if (u.u_prof[3] != 0)
            incupc(pc, u.u_prof);
    } else {
        u.u_stime++;
    }

    pp = (struct proc *)u.u_procp;
    if (++pp->p_cpu == 0)
        pp->p_cpu--;

    /* Lightning bolt */
    if (++lbolt >= HZ) {
        if ((ps & 0340) != 0)
            return;

        lbolt = -HZ;

        /* Atualiza time of day */
        if (++time[1] == 0)
            ++time[0];

        s = spl1();
        if (time[1] == tout[1] && time[0] == tout[0])
            wakeup((int *)tout);

        if ((time[1] & 03) == 0) {
            runrun = 1;
            wakeup(&lbolt);
        }

        /* Atualiza prioridades */
           for (pp = &proc[0]; pp < &proc[NPROC]; pp++) {
            if (pp->p_stat != 0) {
                if (pp->p_time != 127)
                    pp->p_time++;

                if ((pp->p_cpu & 0377) > SCHMAG)
                    pp->p_cpu = -SCHMAG;
                else
                    pp->p_cpu = 0;

                if (pp->p_pri > PUSER)
                    setpri(pp);
            }
        }

        if (runin != 0) {
            runin = 0;
            wakeup(&runin);
        }

        if ((ps & UMODE) == UMODE) {
            u.u_ar0 = &r0;
            if (issig() != 0)
                psig();
            setpri((struct proc *)u.u_procp);
        }
        splx(s);
    }
}

/*
 * timeout - agenda função para ser chamada após tim ticks
 */
void
timeout(int (*fun)(), int arg, int tim)
{
    struct callo *p1, *p2;
    int t;
    int s;

    t = tim;
    p1 = &callout_table[0];
    
    s = spl7();
    
    while (p1->c_func != 0 && p1->c_time <= t) {
        t = -p1->c_time;
        p1++;
    }
    
    p1->c_time = -t;
    
    p2 = p1;
    while (p2->c_func != 0)
        p2++;
    
    while (p2 > p1) {
        p2[1].c_time = p2[0].c_time;
        p2[1].c_func = p2[0].c_func;
        p2[1].c_arg = p2[0].c_arg;
        p2--;
    }
    
    p1->c_time = t;
    p1->c_func = fun;
    p1->c_arg = arg;
    
    splx(s);
}
