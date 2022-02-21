
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "proc.h"
#include "global.h"
#include "proto.h"



PUBLIC	PROCESS	proc_table[NR_TASKS + NR_PROCS];

PUBLIC	TASK	task_table[NR_TASKS] = {
	//{task_tty, STACK_SIZE_TTY, "tty"}
	{A, STACK_SIZE_A, "A"},
	{B, STACK_SIZE_B, "B"},
	{C, STACK_SIZE_C, "C"},
	{D, STACK_SIZE_D, "D"},
	{E, STACK_SIZE_E, "E"},
	{F, STACK_SIZE_F, "F"},};

PUBLIC  TASK    user_proc_table[NR_PROCS] = {
	//{TestA, STACK_SIZE_TESTA, "TestA"},
	//{TestB, STACK_SIZE_TESTB, "TestB"},
	//{TestC, STACK_SIZE_TESTC, "TestC"}
	};

PUBLIC	char		task_stack[STACK_SIZE_TOTAL];

PUBLIC	TTY		tty_table[NR_CONSOLES];
PUBLIC	CONSOLE		console_table[NR_CONSOLES];

PUBLIC	irq_handler	irq_table[NR_IRQ];

PUBLIC	system_call	sys_call_table[NR_SYS_CALL] = {sys_get_ticks, sys_write, sys_sleep, sys_p, sys_v};

