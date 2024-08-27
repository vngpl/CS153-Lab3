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
  } shm_pages[SHM_TABLE_SIZE];
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

void map_shm_page(struct proc *p, struct shm_page *page, char **pointer) {
  void *va = (void*)PGROUNDUP(p->sz);
  uint pa = V2P(page->frame);
  mappages(p->pgdir, va, PGSIZE, pa, PTE_W|PTE_U);
  *pointer = (char*)va;
  p->sz += PGSIZE;
}

int shm_open(int id, char **pointer) {
  int i;
  struct proc *curproc = myproc();
  struct shm_page *pg = 0;

  acquire(&(shm_table.lock));

  // case 1: shm with id exists
  for (i = 0; i < SHM_TABLE_SIZE; i++) {
    pg = &shm_table.shm_pages[i];
    if (pg->id == (uint)id) {
      pg->refcnt++;
      map_shm_page(curproc, pg, pointer);
      release(&(shm_table.lock));
      return 0;
    }
  }

  // case 2: shm with id does not exist
  for (i = 0; i < SHM_TABLE_SIZE; i++) {
    pg = &shm_table.shm_pages[i];
    if (!pg->id) {
      pg->id = (uint)id;
      pg->frame = kalloc();
      memset(pg->frame, 0, PGSIZE);
      pg->refcnt = 1;
      map_shm_page(curproc, pg, pointer);
      release(&(shm_table.lock));
      return 0;
    }
  }

  release(&(shm_table.lock));
  return -1; // reaches if shm_table is full
}

int shm_close(int id) {
  int i;
  struct shm_page *pg = 0;

  acquire(&(shm_table.lock));

  for (i = 0; i < SHM_TABLE_SIZE; i++) {
    pg = &shm_table.shm_pages[i];
    if (pg->id == (uint)id) {
      pg->refcnt--;
      if (pg->refcnt > 0) {
        release(&(shm_table.lock));
        return 0;
      }
      pg->frame = 0;
      pg->id = 0;
      // unmap pages on PTE
      // use walkpgdir() to free memory
      release(&(shm_table.lock));
      return 0;
    }
  }

  release(&(shm_table.lock));
  return 1; // reaches if match not found
}