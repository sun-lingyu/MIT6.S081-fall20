#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//copy trapframe to save registers. 
void backuptrapframe(struct trapframe* original, struct trapframe* backup)
{
  backup->kernel_satp=original->kernel_satp; 
  backup->kernel_sp=original->kernel_sp; 
  backup->kernel_trap=original->kernel_trap;   
  backup->epc=original->epc; 
  backup->kernel_hartid=original->kernel_hartid;   
  backup->ra=original->ra;   
  backup->sp=original->sp;   
  backup->gp=original->gp;  
  backup->tp=original->tp;  
  backup->t0=original->t0;  
  backup->t1=original->t1;  
  backup->t2=original->t2;  
  backup->s0=original->s0;  
  backup->s1=original->s1;  
  backup->a0=original->a0;  
  backup->a1=original->a1;  
  backup->a2=original->a2;  
  backup->a3=original->a3;  
  backup->a4=original->a4;  
  backup->a5=original->a5;  
  backup->a6=original->a6;  
  backup->a7=original->a7;   
  backup->s2=original->s2;
  backup->s3=original->s3;
  backup->s4=original->s4;
  backup->s5=original->s5;
  backup->s6=original->s6;
  backup->s7=original->s7;
  backup->s8=original->s8;
  backup->s9=original->s9;
  backup->s10=original->s10;
  backup->s11=original->s11;
  backup->t3=original->t3;
  backup->t4=original->t4;
  backup->t5=original->t5;
  backup->t6=original->t6;
}
//resume backup trapframe to original trapframe. 
void resumetrapframe(struct trapframe* original, struct trapframe* backup)
{
  original->kernel_satp=backup->kernel_satp; 
  original->kernel_sp=backup->kernel_sp; 
  original->kernel_trap=backup->kernel_trap;   
  original->epc=backup->epc; 
  original->kernel_hartid=backup->kernel_hartid;   
  original->ra=backup->ra;   
  original->sp=backup->sp;   
  original->gp=backup->gp;  
  original->tp=backup->tp;  
  original->t0=backup->t0;  
  original->t1=backup->t1;  
  original->t2=backup->t2;  
  original->s0=backup->s0;  
  original->s1=backup->s1;  
  original->a0=backup->a0;  
  original->a1=backup->a1;  
  original->a2=backup->a2;  
  original->a3=backup->a3;  
  original->a4=backup->a4;  
  original->a5=backup->a5;  
  original->a6=backup->a6;  
  original->a7=backup->a7;   
  original->s2=backup->s2;
  original->s3=backup->s3;
  original->s4=backup->s4;
  original->s5=backup->s5;
  original->s6=backup->s6;
  original->s7=backup->s7;
  original->s8=backup->s8;
  original->s9=backup->s9;
  original->s10=backup->s10;
  original->s11=backup->s11;
  original->t3=backup->t3;
  original->t4=backup->t4;
  original->t5=backup->t5;
  original->t6=backup->t6;
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  
  // save user program counter.
  p->trapframe->epc = r_sepc();

  
  if(r_scause() == 8){
    // system call

    if(p->killed)
      exit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sstatus &c registers,
    // so don't enable until done with those registers.
    intr_on();

    syscall();
  } else if((which_dev = devintr()) != 0){
    // ok
  } else {
    printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    p->killed = 1;
  }

  if(p->killed)
    exit(-1);

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2)
  {
    p->tickpassed++;
    if (p->tick > 0 && p->tickpassed >= p->tick && !p->inhandler)
    {
      // save enough state in struct proc context when the timer goes off 
      // that sigreturn can correctly return to the interrupted user code.
      p->alarm_context.epc = p->trapframe->epc;
      p->alarm_context.ra = p->trapframe->ra;
      p->alarm_context.sp = p->trapframe->sp;
      p->alarm_context.gp = p->trapframe->gp;
      p->alarm_context.tp = p->trapframe->tp;
      
      p->alarm_context.s0 = p->trapframe->s0;
      p->alarm_context.s1 = p->trapframe->s1;
      p->alarm_context.s2 = p->trapframe->s2;
      p->alarm_context.s3 = p->trapframe->s3;
      p->alarm_context.s4 = p->trapframe->s4;
      p->alarm_context.s5 = p->trapframe->s5;
      p->alarm_context.s6 = p->trapframe->s6;
      p->alarm_context.s7 = p->trapframe->s7;
      p->alarm_context.s8 = p->trapframe->s8;
      p->alarm_context.s9 = p->trapframe->s9;
      p->alarm_context.s10 = p->trapframe->s10;
      p->alarm_context.s11 = p->trapframe->s11;
      
      p->alarm_context.t0 = p->trapframe->t0;
      p->alarm_context.t1 = p->trapframe->t1;
      p->alarm_context.t2 = p->trapframe->t2;
      p->alarm_context.t3 = p->trapframe->t3;
      p->alarm_context.t4 = p->trapframe->t4;
      p->alarm_context.t5 = p->trapframe->t5;
      p->alarm_context.t6 = p->trapframe->t6;
      
      p->alarm_context.a0 = p->trapframe->a0;
      p->alarm_context.a1 = p->trapframe->a1;
      p->alarm_context.a2 = p->trapframe->a2;
      p->alarm_context.a3 = p->trapframe->a3;
      p->alarm_context.a4 = p->trapframe->a4;
      p->alarm_context.a5 = p->trapframe->a5;
      p->alarm_context.a6 = p->trapframe->a6;
      p->alarm_context.a7 = p->trapframe->a7;
      // return to alarm handler
      p->trapframe->epc = p->handler;
      p->tickpassed = 0;
      p->inhandler = 1;
    }
  }
  yield();
  usertrapret();
}
  /*if(which_dev == 2)
  {
    //added to support alarm.
    //Only invoke the alarm function if the process has a timer outstanding.
    if(p->tick!=0 && p->inhandler==0)//Prevent re-entrant calls to the handler
    {
      p->tickpassed++;
      if(p->tickpassed==p->tick)
      {
        printf("before:\n");
        printf("s1:%p a5:%p\n",p->trapframe->s0,p->trapframe->a5);
        backuptrapframe(p->trapframe,p->trapframe+1);
        or: *(p->trapframe+1)=*(p->trapframe);
        p->backup=*(p->trapframe);
        p->tickpassed=0;
        p->trapframe->epc=(uint64)p->handler;
        p->inhandler=1;

        在从handler返回时置p->inhandler=0，已添加
        难点：在哪里保存大量寄存器状态->保存在trapframe（trapframe的空间足够大（4096B），可以容纳两个struct trapframe）
        注意：在sigalarmret系统调用中实现handler ret！
      }
    }
    yield();
  }
  

  usertrapret();
}*/

//
// return to user space
//
void
usertrapret(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();

  // send syscalls, interrupts, and exceptions to trampoline.S
  w_stvec(TRAMPOLINE + (uservec - trampoline));
  /*if(p->shouldreturn)
  {

    p->shouldreturn=0;
    p->inhandler=0;
    p->tickpassed=0;
    resumetrapframe(p->trapframe,p->trapframe+1);
    or: *(p->trapframe)=*(p->trapframe+1);
    or: *(p->trapframe)=p->backup;
    printf("after:\n");
    printf("s1:%p a5:%p\n",p->trapframe->s0,p->trapframe->a5);
  }*/

  // set up trapframe values that uservec will need when
  // the process next re-enters the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  uint64 satp = MAKE_SATP(p->pagetable);
  
  // jump to trampoline.S at the top of memory, which 
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 fn = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64,uint64))fn)(TRAPFRAME, satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void 
kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    printf("scause %p\n", scause);
    printf("sepc=%p stval=%p\n", r_sepc(), r_stval());
    panic("kerneltrap");
  }

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
    yield();

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  acquire(&tickslock);
  ticks++;
  wakeup(&ticks);
  release(&tickslock);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
  uint64 scause = r_scause();

  if((scause & 0x8000000000000000L) &&
     (scause & 0xff) == 9){
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000001L){
    // software interrupt from a machine-mode timer interrupt,
    // forwarded by timervec in kernelvec.S.

    if(cpuid() == 0){
      clockintr();
    }
    
    // acknowledge the software interrupt by clearing
    // the SSIP bit in sip.
    w_sip(r_sip() & ~2);

    return 2;
  } else {
    return 0;
  }
}

