/* test.c : Test Driver for Xeros
 */

#include <xeroskernel.h>

int exit_proc=0;
int exit_cnt=1;
//int result[4];


/*
* testroot
*
* @desc:	Executes the testroot process
* @note:	This process executes the process management test cases
*/	
void testroot(void)
{
	int cnt = MAX_PROC-count()-1;

	// Test Case 3: Able to allocate the max number of processes and push to ready queue 
	//		while able to context switch between the root and child processes
	//		given that this process is the only process creation point in the code
	kprintf("Begin Test Case 3 ... \n");
	kprintf("Add the following pid to the ready queue\n");
	kprintf("ready_q: ");
	while(cnt > 0)
	{
		syscreate(&testproc, PROC_STACK);
		cnt=MAX_PROC-count()-1;
		kprintf("%d ", count());
	}
	kprintf("\n");

	if(count() == MAX_PROC-1)
		kprintf("TC3 Pass: 31 testprocs have been added to ready_q\n");
	else
		kprintf("TC3 Fail: less than 31 testproces have been added to ready_q\n");

	// Test Case 4: Able to reclaim the process blocks and push onto the stop queue,
	//		while able to context switch between the root and child processes,
	//		given that this process is the only process creation point in the code
	exit_proc=1;
	kprintf("\nBegin Test Case 4 ... \n");
	kprintf("Add the following pid to the stop queue\n");
	kprintf("stop_q: ");
	sysyield();

	if(count() == 0)	
		kprintf("\nTC4 Pass: 31 tesprocs have been added to stop_q\n");
	else
		kprintf("\nTC4 Fail: less than 31 testproces have been added to stop_q\n");

	sysstop();
}

/*
* testproc
*
* @desc:	Executes the testproc process
*/
void testproc(void)
{
	for(;;)
	{
		if(exit_proc) break;
		sysyield();
	}
	kprintf("%d ", exit_cnt);
	exit_cnt++;
	sysstop();
}

/*
* testdriver
*
* @desc:	Executes the test root process
*/
void testdriver(void) 
{
#ifdef	MEM_TEST
	int *blka, *blkb, *blkc, *blkd;
	int total_mem=0;

	total_mem = kmemtotalsize();
	// Test Case 1: 
	// Attempt to allocate memory that is bigger than any available space
	kprintf("Begin Test Case 1 ... \n");
	kprintf("Malloc memory that is within any block memory size\n");
	kprintf("kmalloc:\t\t%d\t", kmemhdsize()/2);
	blka = kmalloc(kmemhdsize()/2);
	if(blka)
		kprintf("success\n");
	else
		kprintf("failed, invalid mem_sz\n");

	kprintf("kmalloc:\t\t%d\t", kmemhdsize()/2);
	blkb = kmalloc(kmemhdsize()/2);
	if(blkb)
		kprintf("success\n");
	else
		kprintf("failed, invalid mem_sz\n");

	kprintf("kmalloc:\t\t%d\t", kmemhdsize()/2);
	blkc = kmalloc(kmemhdsize()/2);
	if(blkc)
		kprintf("success\n");
	else
		kprintf("failed, invalid mem_sz\n");

	kprintf("kmalloc:\t\t%d\t", kmemhdsize()*100);
	blkd = kmalloc(kmemhdsize()*100);
	if(blkd)
		kprintf("success\n");
	else
		kprintf("failed, invalid mem_sz\n");

	if(!blkd) 
		kprintf("TC1 Pass: All valid memory sizes have been allocated\n");
	else
		kprintf("TC1 Fail: An invalid memory size has been allocated\n");

	kprintf("\n");

	// Test Case 2: 
	// Free all allocated memory and does not free any memory that is NULL
	kprintf("Begin Test Case 2 ... \n");
	kprintf("Free all allocated memory at following addr\n");
	kprintf("kfree:\t\t\t%d\n", blkc);
	kfree(blkc);
	kprintf("kfree:\t\t\t%d\n", blka);
	kfree(blka);
	kprintf("kfree:\t\t\t%d\n", blkb);
	kfree(blkb);
	kprintf("kfree:\t\t\t%d\n", blkd);
	kfree(blkd);

	kprintf("mem size on startup:\t%d\n", total_mem);
	kprintf("mem size after kfree:\t%d\n", kmemtotalsize());

	// Compare total free memory space
	if(kmemtotalsize() == total_mem) 
		kprintf("TC2 Pass: All allocated memory have been returned\n");
	else
	{
		if(kmemtotalsize() > total_mem)
			kprintf("TC2 Fail: Invalid memory has been added to memory space\n");
		else
			kprintf("TC2 Fail: Memory leak has occurred\n");
	}
	kprintf("\n");
#endif

#ifdef	PROC_TEST
	create(&testroot, PROC_STACK);
	contextinit();
	dispatch();
#endif	

//	testresult();
}


/*
* testresult
*
* @desc:	Print out test result summary
*/
/*
void testresult(void)
{
	int i;

	kprintf("\n----------------------------------------------------\n");
	kprintf("XEROS Test Summary \n");
	kprintf("----------------------------------------------------\n");
	for(i=0; i<4 ; i++) 
	{
		kprintf("Test Case [%d]\t\t", i+1);
		
		if(i<2)
			kprintf("MEM_TEST\t\t");
		else
			kprintf("PROC_TEST\t\t");

		if(result[i])
			kprintf("Pass\n");
		else
			kprintf("Fail\n");
	}
}*/
