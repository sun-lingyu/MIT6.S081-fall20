#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

static int nthread = 1;
static int round = 0;

struct barrier {
  pthread_mutex_t barrier_mutex;
  pthread_cond_t barrier_cond;
  int nthread;      // Number of threads that have reached this round of the barrier
  int round;     // Barrier round
  int flag;
} bstate;

static void
barrier_init(void)
{
  assert(pthread_mutex_init(&bstate.barrier_mutex, NULL) == 0);
  assert(pthread_cond_init(&bstate.barrier_cond, NULL) == 0);
  bstate.nthread = 0;
  bstate.round = 0;
  bstate.flag=0;//是否还有线程在上一轮 
}

static void 
barrier()
{
  // YOUR CODE HERE
  //
  // Block until all threads have called barrier() and
  // then increment bstate.round.
  //
  pthread_mutex_lock(&bstate.barrier_mutex);       // acquire lock
  while(bstate.flag==1)//还有线程在上一轮 
  {
    pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
  }
  bstate.nthread++;

  if(bstate.nthread==nthread)
  {
    bstate.round++;
    bstate.flag=1;
    pthread_cond_broadcast(&bstate.barrier_cond);     // 唤醒由于bstate.nthread!=nthread而等待的线程
  } 
  else
    pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);//当此函数返回时，必有bstate.flag==1
  
  bstate.nthread--;
  if(bstate.nthread==0)//上一轮的全部线程都已经唤醒
  {
    bstate.flag=0;
    pthread_cond_broadcast(&bstate.barrier_cond);     // 唤醒由于bstate.flag==1而等待的线程
  }    
  pthread_mutex_unlock(&bstate.barrier_mutex);     // release lock
}

static void *
thread(void *xa)
{
  long n = (long) xa;
  long delay;
  int i;

  for (i = 0; i < 20000; i++) {
    int t = bstate.round;
    assert (i == t);
    barrier();
    usleep(random() % 100);
  }

  return 0;
}

int
main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  long i;
  double t1, t0;

  if (argc < 2) {
    fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);

  barrier_init();

  for(i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, thread, (void *) i) == 0);
  }
  for(i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  printf("OK; passed\n");
}
