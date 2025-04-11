// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#define NBUCKET 13
#define HASH(id) id % NBUCKET

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  
  // do not use head anymore
  // struct buf head;
} bcache;


struct {
  struct spinlock lock;
  struct buf head;
} buckets[NBUCKET];

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");


  // Create linked list for each bucket
  for(int i = 0; i < NBUCKET; i ++)
  {
    buckets[i].head.prev = &buckets[i].head;
    buckets[i].head.next = &buckets[i].head;
  }

  // Add all buffers to bucket 0 initially
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = buckets[0].head.next;
    b->prev = &buckets[0].head;
    initsleeplock(&b->lock, "buffer");
    buckets[0].head.next->prev = b;
    buckets[0].head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
// static struct buf*
// bget(uint dev, uint blockno)
// {
//   struct buf *b;

//   int id = HASH(blockno);
//   // printf("bget: id is %d\n", id);
//   int lru;

//   // Is the block already cached?
//   acquire(&buckets[id].lock);
//   for(b = buckets[id].head.next; b != &buckets[id].head; b = b->next){
//     if(b->dev == dev && b->blockno == blockno){
//       b->refcnt++;
//       release(&buckets[id].lock);
//       acquiresleep(&b->lock); // acquire this lock for using buf b
//       return b;
//     }
//   }
  
//   // Not cached.
//   // Recycle the least recently used (LRU) unused buffer.
//   lru = ticks;

//   struct buf * lrubuf = 0;
//   int lrubucket = -1;
//   int lastlru = -2;

//   for(int i = 0; i < NBUCKET; i++){
//     if(i != id) // Avoid repeating acquiring
//       acquire(&buckets[i].lock);
//     for(b = buckets[i].head.next; b != &buckets[i].head; b = b->next){
//       if(b->refcnt == 0 && lru > b->ticks){     // Make sure refcnt is 0
//         lru = b->ticks;
//         lrubuf = b;
//         lastlru = lrubucket;
//         lrubucket = i;
//       }
//     }
//     if(i == lrubucket){
//       // printf("release this lock: %d\n", i);
//       if(lastlru != id && lastlru >= 0)
//         release(&buckets[lastlru].lock);
//       // printf("release success: %d\n", i);
//     }
//     else{
//       if(i != id)
//         release(&buckets[i].lock);
//     }
//   }
//   if (lrubuf == 0) {
//     panic("bget: no free buffers for lru");
//   }
//   // Set this found lru buf
//   lrubuf->dev = dev;
//   lrubuf->blockno = blockno;
//   lrubuf->valid = 0;
//   lrubuf->refcnt = 1;

//   // Add this buf to mapped bucket and remove it from its original bucket
//   // Remove
//   lrubuf->prev->next = lrubuf->next;
//   lrubuf->next->prev = lrubuf->prev;

//   // Add
//   buckets[id].head.prev->next = lrubuf;
//   lrubuf->prev = buckets[id].head.prev;
//   buckets[id].head.prev = lrubuf;
//   lrubuf->next = &buckets[id].head;

//   // printf("release this lru lock: %d\n", lrubucket);
//   release(&buckets[lrubucket].lock);
//   // printf("release this id lock: %d\n", id);
//   if(id != lrubucket)
//     release(&buckets[id].lock);
//   acquiresleep(&lrubuf->lock);
//   // printf("bget: success\n");
//   return lrubuf;

//   panic("bget: no buffers");

// }
static struct buf*
bget(uint dev, uint blockno)
{
    struct buf *b;
    int id = HASH(blockno);
    
    // 首先检查目标桶是否已有缓存
    acquire(&buckets[id].lock);
    for (b = buckets[id].head.next; b != &buckets[id].head; b = b->next) {
        if (b->dev == dev && b->blockno == blockno) {
            b->refcnt++;
            release(&buckets[id].lock);
            acquiresleep(&b->lock);
            return b;
        }
    }
    release(&buckets[id].lock);

    // 全局搜索LRU且未被引用的缓冲区
    struct buf *lrubuf = 0;
    int lrubucket = -1;
    uint lruticks = ~0;

    for (int i = 0; i < NBUCKET; i++) {
        acquire(&buckets[i].lock);
        for (b = buckets[i].head.next; b != &buckets[i].head; b = b->next) {
            if (b->refcnt == 0 && b->ticks < lruticks) {
                lruticks = b->ticks;
                lrubuf = b;
                lrubucket = i;
            }
        }
        release(&buckets[i].lock);
        // if (lrubuf) break; // 找到后提前退出
    }

    if (!lrubuf) 
        panic("bget: no free buffers for lru");

    int target = id;
    int src = lrubucket;

    // 按顺序获取锁（处理相同桶的情况）
    if (target == src) {
        acquire(&buckets[target].lock);
    } else {
        if (target < src) {
            acquire(&buckets[target].lock);
            acquire(&buckets[src].lock);
        } else {
            acquire(&buckets[src].lock);
            acquire(&buckets[target].lock);
        }
    }

    // 重新检查目标桶是否已被其他进程缓存
    for (b = buckets[target].head.next; b != &buckets[target].head; b = b->next) {
        if (b->dev == dev && b->blockno == blockno) {
            b->refcnt++;
            // 释放锁前判断是否相同
            if (target == src) {
                release(&buckets[target].lock);
            } else {
                release(&buckets[target].lock);
                release(&buckets[src].lock);
            }
            acquiresleep(&b->lock);
            return b;
        }
    }

    // 移动缓冲区到目标桶
    lrubuf->dev = dev;
    lrubuf->blockno = blockno;
    lrubuf->valid = 0;
    lrubuf->refcnt = 1;

    // 从原桶移除
    lrubuf->prev->next = lrubuf->next;
    lrubuf->next->prev = lrubuf->prev;

    // 插入目标桶头部
    lrubuf->next = buckets[target].head.next;
    lrubuf->prev = &buckets[target].head;
    buckets[target].head.next->prev = lrubuf;
    buckets[target].head.next = lrubuf;

    // 释放锁
    if (target == src) {
        release(&buckets[target].lock);
    } else {
        release(&buckets[src].lock);
        release(&buckets[target].lock);
    }

    acquiresleep(&lrubuf->lock);
    return lrubuf;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int id = HASH(b->blockno);

  acquire(&buckets[id].lock);
  b->refcnt--;  // should be minusing with bcache lock held
  if (b->refcnt == 0){
    b->ticks = ticks;
  }
  release(&buckets[id].lock);
}

void
bpin(struct buf *b) {
  int bid = HASH(b->blockno);
  acquire(&buckets[bid].lock);
  b->refcnt++;
  release(&buckets[bid].lock);
}

void
bunpin(struct buf *b) {
  int bid = HASH(b->blockno);
  acquire(&buckets[bid].lock);
  b->refcnt--;
  release(&buckets[bid].lock);
}


