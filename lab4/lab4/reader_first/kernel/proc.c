
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	disable_irq(CLOCK_IRQ);

	int allDone = 1;
	for (PROCESS *p = proc_table; p < proc_table + NR_TASKS + NR_PROCS - 1; ++p)
	{
		if (p->done == 0)
		{
			allDone = 0;
			break;
		}
	}

	if (allDone == 1)
	{
		for (PROCESS *p = proc_table; p < proc_table + NR_TASKS + NR_PROCS - 1; ++p)
		{
			p->done = 0;
		}
	}

	if ((proc_table + NR_TASKS + NR_PROCS - 1)->block == 0 && get_ticks() >= (proc_table + NR_TASKS + NR_PROCS - 1)->sleep_to_ticks)
	{
		p_proc_ready = proc_table + NR_TASKS + NR_PROCS - 1;
	}
	else
	{
		int flag = 0;
		while (1)
		{
			if (p_proc_pre->done == 0 && p_proc_pre->block == 0 && get_ticks() >= p_proc_pre->sleep_to_ticks)
			{
				p_proc_ready = p_proc_pre;
				flag = 1;
			}
			p_proc_pre++;
			if (p_proc_pre == (proc_table + NR_TASKS + NR_PROCS - 1))
			{
				p_proc_pre = proc_table;
			}
			if (flag == 1)
			{
				break;
			}
		}
	}

	if (p_proc_ready->type != 'n')
	{
		status = p_proc_ready->type;
	}

	enable_irq(CLOCK_IRQ);
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

/*======================================================================*
                           sys_sleep
 *======================================================================*/
PUBLIC void sys_sleep(int milli_seconds)
{
	p_proc_ready->sleep_to_ticks = get_ticks() + milli_seconds / (1000 / HZ);
	schedule();
}

/*======================================================================*
                           sys_p
 *======================================================================*/
PUBLIC void sys_p(void *mutex)
{
	disable_irq(CLOCK_IRQ);
	Semaphore *semaphore = (Semaphore *)mutex;
	semaphore->val--;
	if (semaphore->val < 0)
	{
		getDown(semaphore);
	}
	enable_irq(CLOCK_IRQ);
}

/*======================================================================*
                           sys_v
 *======================================================================*/
PUBLIC void sys_v(void *mutex)
{
	disable_irq(CLOCK_IRQ);
	Semaphore *semaphore = (Semaphore *)mutex;
	semaphore->val++;
	if (semaphore->val <= 0)
	{
		wakeUp(semaphore);
	}
	enable_irq(CLOCK_IRQ);
}

/*======================================================================*
                              getDown
 *======================================================================*/
PUBLIC void getDown(Semaphore *mutex)
{
	int index = -(mutex->val + 1);
	mutex->queue[index] = p_proc_ready;
	p_proc_ready->block = 1;
	schedule();
}

/*======================================================================*
                              wakeUp
 *======================================================================*/
PUBLIC void wakeUp(Semaphore *mutex)
{
	PROCESS *process = mutex->queue[0];
	int max = 0;
	int index = -mutex->val + 1;
	for (int i = 0; i < index; ++i)
	{
		if (mutex->queue[i]->priority > process->priority)
		{
			max = i;
			process = mutex->queue[i];
		}
	}
	for (int i = max + 1; i < index; ++i)
	{
		mutex->queue[i - 1] = mutex->queue[i];
	}
	process->block = 0;
}