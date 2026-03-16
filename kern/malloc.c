/*
 *	Copyright 1973 Bell Telephone Laboratories Inc
 */

struct map {
    char *m_size;
    char *m_addr;
};

/*
 * Allocate memory from a map
 */
int
malloc(struct map *mp, int size)
{
    register int a;
    register struct map *bp;

    for (bp = mp; bp->m_size != 0; bp++) {
        if (bp->m_size >= size) {
            a = (int)bp->m_addr;
            bp->m_addr = (char *)((int)bp->m_addr + size);
            bp->m_size = (char *)((int)bp->m_size - size);
            if (bp->m_size == 0) {
                do {
                    bp++;
                    (bp-1)->m_addr = bp->m_addr;
                } while (((bp-1)->m_size = bp->m_size) != 0);
            }
            return(a);
        }
    }
    return(0);
}

/*
 * Free memory from a map
 */
void
mfree(struct map *mp, int size, int aa)
{
    register struct map *bp;
    register int t;
    register int a;

    a = aa;
    for (bp = mp; (int)bp->m_addr <= a && bp->m_size != 0; bp++)
        ;
    
    if (bp > mp && (int)(bp-1)->m_addr + (int)(bp-1)->m_size == a) {
        (bp-1)->m_size = (char *)((int)(bp-1)->m_size + size);
        
        if (a + size == (int)bp->m_addr) {
            (bp-1)->m_size = (char *)((int)(bp-1)->m_size + (int)bp->m_size);
            
            while (bp->m_size != 0) {
                bp++;
                (bp-1)->m_addr = bp->m_addr;
                (bp-1)->m_size = bp->m_size;
            }
        }
    } else {
        if (a + size == (int)bp->m_addr && bp->m_size != 0) {
            bp->m_addr = (char *)((int)bp->m_addr - size);
            bp->m_size = (char *)((int)bp->m_size + size);
        } else if (size != 0) {
            do {
                t = (int)bp->m_addr;
                bp->m_addr = (char *)a;
                a = t;
                t = (int)bp->m_size;
                bp->m_size = (char *)size;
                bp++;
            } while ((size = t) != 0);
        }
    }
}
