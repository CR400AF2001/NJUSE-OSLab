
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK *p_task = task_table;
	PROCESS *p_proc = proc_table;
	char *p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16 selector_ldt = SELECTOR_LDT_FIRST;
	int i;
	u8 privilege;
	u8 rpl;
	int eflags;
	for (i = 0; i < NR_TASKS + NR_PROCS; i++)
	{
		if (i < NR_TASKS)
		{ /* 任务 */
			p_task = task_table + i;
			privilege = PRIVILEGE_TASK;
			rpl = RPL_TASK;
			eflags = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
		}
		else
		{ /* 用户进程 */
			p_task = user_proc_table + (i - NR_TASKS);
			privilege = PRIVILEGE_USER;
			rpl = RPL_USER;
			eflags = 0x202; /* IF=1, bit 2 is always 1 */
		}

		strcpy(p_proc->p_name, p_task->name); // name of the process
		p_proc->pid = i;					  // pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
			   sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
			   sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
		p_proc->regs.cs = (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ds = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.es = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.fs = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ss = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;

		p_proc->nr_tty = 0;

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	proc_table[0].needTimeSlice = proc_table[0].priority = 2;
	proc_table[1].needTimeSlice = proc_table[1].priority = 3;
	proc_table[2].needTimeSlice = proc_table[2].priority = 3;
	proc_table[3].needTimeSlice = proc_table[3].priority = 3;
	proc_table[4].needTimeSlice = proc_table[4].priority = 4;
	proc_table[5].needTimeSlice = proc_table[5].priority = 1;

	proc_table[0].type = proc_table[1].type = proc_table[2].type = 'r';
	proc_table[3].type = proc_table[4].type = 'w';
	proc_table[5].type = 'n';

	proc_table[0].done = proc_table[1].done = proc_table[2].done = proc_table[3].done = proc_table[4].done = proc_table[5].done = 0;

	//proc_table[1].nr_tty = 0;
	//proc_table[2].nr_tty = 1;
	//proc_table[3].nr_tty = 1;

	k_reenter = 0;
	ticks = 0;

	// 饿死
	hungry = 1;

	p_proc_ready = proc_table;
	p_proc_pre = proc_table;

	// 读者上限
	maxReadNum = readMutex.val = 3;
	maxWriteNum = writeMutex.val = writeMutexMutex.val = 1;
	readCountMutex.val = 1;
	readCount = prereadCount = 0;

	init_clock();
	init_keyboard();

	disp_pos = 0;
	for(i=0;i<80*25;i++){
		disp_str(" ");
	}
	disp_pos = 0;

	restart();

	while (1)
	{
	}
}

/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	int i = 0;
	while (1)
	{
		write("<Ticks:%x>");
		sleep(200);
	}
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	int i = 0x1000;
	while (1)
	{
		write("B");
		sleep(200);
	}
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestC()
{
	int i = 0x2000;
	while (1)
	{
		write("C");
		sleep(200);
	}
}

/*======================================================================*
                               A
 *======================================================================*/
void A()
{
	Reader("A");
}

/*======================================================================*
                               B
 *======================================================================*/
void B()
{
	Reader("B");
}

/*======================================================================*
                               C
 *======================================================================*/
void C()
{
	Reader("C");
}

/*======================================================================*
                               D
 *======================================================================*/
void D()
{
	Writer("D");
}

/*======================================================================*
                               E
 *======================================================================*/
void E()
{
	Writer("E");
}

/*======================================================================*
                               F
 *======================================================================*/
void F()
{
	Normal("F");
}

/*======================================================================*
                               Reader
 *======================================================================*/
void Reader(char *name)
{
	sleep(1000);
	while (1)
	{
		p(&readCountMutex);
		if (prereadCount == 0)
		{
			p(&writeMutex);
		}
		prereadCount++;
		v(&readCountMutex);
		p(&readMutex);
		readCount++;

		write(name);
		write(" start. ");

		for (int i = 0; i < p_proc_ready->needTimeSlice; ++i)
		{
			write(name);
			write(" reading. ");
			if (i == p_proc_ready->needTimeSlice - 1)
			{
				write(name);
				write(" finish. ");
			}
			else
			{
				milli_delay(1000);
			}
		}

		readCount--;
		v(&readMutex);
		p(&readCountMutex);
		if (prereadCount == 1)
		{
			v(&writeMutex);
		}
		prereadCount--;
		v(&readCountMutex);

		p_proc_ready->done = hungry;
		milli_delay(1000);
	}
}

/*======================================================================*
                               Writer
 *======================================================================*/
void Writer(char *name)
{
	while (1)
	{
		p(&writeMutexMutex);
		p(&writeMutex);
		
		write(name);
		write(" start. ");

		for (int i = 0; i < p_proc_ready->needTimeSlice; ++i)
		{
			write(name);
			write(" writing. ");
			if (i == p_proc_ready->needTimeSlice - 1)
			{
				write(name);
				write(" finish. ");
			}
			else
			{
				milli_delay(1000);
			}
		}

		v(&writeMutex);
		v(&writeMutexMutex);

		p_proc_ready->done = hungry;
		milli_delay(1000);
	}
}

/*======================================================================*
                               Normal
 *======================================================================*/
void Normal(char *name)
{
	while (1)
	{
		switch (status)
		{
		case 'r':
			write(" READ! ");
			write("reader num is ");
			char num[4] = {'0' + readCount, '.', ' ', '\0'};
			write(num);
			break;
		case 'w':
			write(" WRITE! ");
			break;
		default:
			write(" START! ");
			break;
		}
		sleep(1000);
	}
}