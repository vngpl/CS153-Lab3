#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

#define SHM_TABLE_SIZE 64

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< SHM_TABLE_SIZE; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

// for uint size in mappages() use pgsize variable for default page size

int shm_open(int id, char **pointer) {
  int i;
  struct proc *p = myproc();

  acquire(&(shm_table.lock));
  // case 1: shm with id exists
  for (i = 0; i < SHM_TABLE_SIZE; i++) {
    if (shm_table.shm_pages[i].id == (uint)id) {
      shm_table.shm_pages[i].refcnt++;
      mappages(p->pgdir, (void*)PGROUNDUP(p->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U);
      *pointer = (char*)PGROUNDUP(p->sz);
      p->sz += PGSIZE;
      release(&(shm_table.lock));
      return 0;
    }
  }


  return 0; //added to remove compiler warning -- you should decide what to return
}


int shm_close(int id) {
//you write this too!




  return 0; //added to remove compiler warning -- you should decide what to return
}
