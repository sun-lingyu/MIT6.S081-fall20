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

#define NHASH 13

struct {
  struct buf buf[NBUF];

  //hash table for all disk blocks
  struct buf head[NHASH];
  //a lock for each bucket.
  struct spinlock bucketlock[NHASH];
  //+1: one bucket for usued buffers
} bcache;

void
binit(void)
{
  struct buf *b;

  for(int i=0;i<NHASH;i++)
  {
    initlock(&bcache.bucketlock[i], "bcache.bucket");
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){//add all to head[0]
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
    b->refcnt=0;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bucketnum=blockno%NHASH;

  acquire(&bcache.bucketlock[bucketnum]);

  // Is the block already cached?
  for(b = bcache.head[bucketnum].next; b != &bcache.head[bucketnum]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucketlock[bucketnum]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  int curr=bucketnum;
  for(int i=1;i<NHASH;i++){
    curr=(curr+1)%NHASH;
    acquire(&bcache.bucketlock[curr]);
    for (b = bcache.head[curr].prev; b != &bcache.head[curr]; b = b->prev)
    {
      if(b->refcnt == 0) 
      {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        //remove from bcache.head[NHASH]
        b->next->prev = b->prev;
        b->prev->next=b->next;
        release(&bcache.bucketlock[curr]);
        //add to bcache.head[bucketnum]
        b->next = bcache.head[bucketnum].next;
        b->prev = &bcache.head[bucketnum];
        bcache.head[bucketnum].next->prev = b;
        bcache.head[bucketnum].next = b;
        release(&bcache.bucketlock[bucketnum]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.bucketlock[curr]);
  }
  panic("bget: no buffers");
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
  int bucketnum=b->blockno%NHASH;

  acquire(&bcache.bucketlock[bucketnum]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;

    b->next = bcache.head[bucketnum].next;
    b->prev = &bcache.head[bucketnum];
    bcache.head[bucketnum].next->prev = b;
    bcache.head[bucketnum].next = b;
  }
  
  release(&bcache.bucketlock[bucketnum]);
}

void
bpin(struct buf *b) {
  int bucketnum=b->blockno%NHASH;
  acquire(&bcache.bucketlock[bucketnum]);
  b->refcnt++;
  release(&bcache.bucketlock[bucketnum]);
}

void
bunpin(struct buf *b) {
  int bucketnum=b->blockno%NHASH;
  acquire(&bcache.bucketlock[bucketnum]);
  b->refcnt--;
  release(&bcache.bucketlock[bucketnum]);
}


