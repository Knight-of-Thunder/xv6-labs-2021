// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

// struct {
//   struct spinlock lock;
//   struct run *freelist;
// } kmem;

// allocate a freelist for each cpu
struct {
  struct spinlock lock;
  struct run *freelist;
} kcpumem[NCPU];

void
kinit()
{
  // initlock(&kmem.lock, "kmem");

  // init each cpu's freelist instead of the whole kmem
  char cpumemname[16];
  for(int i = 0; i < NCPU; i++){
    snprintf(cpumemname, 16, "kmem_cpu%d", i);
    // printf("%s\n",cpumemname);
    initlock(&kcpumem[i].lock, cpumemname);
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // get current cpu_id
  push_off();
  int id = cpuid();
  pop_off();

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kcpumem[id].lock);
  r->next = kcpumem[id].freelist;
  kcpumem[id].freelist = r;
  release(&kcpumem[id].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  
  // get current cpu_id
  push_off();
  int id = cpuid();
  pop_off();
  
  acquire(&kcpumem[id].lock);
  r = kcpumem[id].freelist;
  if(r)
    kcpumem[id].freelist = r->next;
  release(&kcpumem[id].lock);

  // steal page from other cpu's
  if(r == 0){
    for(int i = 0; i < NCPU; i++){
      acquire(&kcpumem[i].lock);
      r = kcpumem[i].freelist;
      if(r){
        kcpumem[i].freelist = r -> next;
        release(&kcpumem[i].lock);
        break;
      }
      release(&kcpumem[i].lock);
    }
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk

  return (void*)r;
}
