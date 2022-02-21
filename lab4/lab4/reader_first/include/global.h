
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* EXTERN is defined as extern except in global.c */
#ifdef	GLOBAL_VARIABLES_HERE
#undef	EXTERN
#define	EXTERN
#endif

EXTERN	int		ticks;

EXTERN	int		disp_pos;
EXTERN	u8		gdt_ptr[6];	// 0~15:Limit  16~47:Base
EXTERN	DESCRIPTOR	gdt[GDT_SIZE];
EXTERN	u8		idt_ptr[6];	// 0~15:Limit  16~47:Base
EXTERN	GATE		idt[IDT_SIZE];

EXTERN	u32		k_reenter;

EXTERN	TSS		tss;
EXTERN	PROCESS*	p_proc_ready;
EXTERN	PROCESS*	p_proc_pre;

EXTERN	int		nr_current_console;

extern	PROCESS		proc_table[];
extern	char		task_stack[];
extern  TASK            task_table[];
extern  TASK            user_proc_table[];
extern	irq_handler	irq_table[];
extern	TTY		tty_table[];
extern  CONSOLE         console_table[];

typedef struct semaphore {
    int val;
    PROCESS* queue[NR_TASKS];
}Semaphore;

EXTERN Semaphore readMutex;
EXTERN Semaphore writeMutex;
EXTERN Semaphore readCountMutex;
EXTERN Semaphore writeMutexMutex;

EXTERN int maxReadNum;
EXTERN int maxWriteNum;
EXTERN int readCount;
EXTERN int prereadCount;

EXTERN char status;
EXTERN int  hungry;