//another implementation for bio.c
//user a seperate hash bucket for unused buffers
//can't pass bcachetest because of too many contentions
//but I think this is a good idea
//it achieves LRU, and serialized the allocation part in bget.


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
  struct buf head[NHASH+1];
  //a lock for each bucket.
  struct spinlock bucketlock[NHASH+1];
  //+1: one bucket for usued buffers
} bcache;

void
binit(void)
{
  struct buf *b;

  for(int i=0;i<NHASH+1;i++)
  {
    initlock(&bcache.bucketlock[i], "bcache.bucket");
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){//add all to head[0]
    b->next = bcache.head[NHASH].next;
    b->prev = &bcache.head[NHASH];
    initsleeplock(&b->lock, "buffer");
    bcache.head[NHASH].next->prev = b;
    bcache.head[NHASH].next = b;
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
  acquire(&bcache.bucketlock[NHASH]);
  b=bcache.head[NHASH].prev;
  if(b!=&bcache.head[NHASH]) {
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    //remove from bcache.head[NHASH]
    b->next->prev = b->prev;
    b->prev->next=b->next;
    release(&bcache.bucketlock[NHASH]);
    //add to bcache.head[bucketnum]
    b->next = bcache.head[bucketnum].next;
    b->prev = &bcache.head[bucketnum];
    bcache.head[bucketnum].next->prev = b;
    bcache.head[bucketnum].next = b;
    release(&bcache.bucketlock[bucketnum]);
    acquiresleep(&b->lock);
    return b;
  }
  release(&bcache.bucketlock[NHASH]);
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
    acquire(&bcache.bucketlock[NHASH]);
    b->next = bcache.head[NHASH].next;
    b->prev = &bcache.head[NHASH];
    bcache.head[NHASH].next->prev = b;
    bcache.head[NHASH].next = b;
    release(&bcache.bucketlock[NHASH]);
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


