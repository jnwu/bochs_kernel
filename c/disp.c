/* disp.c : dispatcher
* 
* name:		Jack Wu
* student id:	17254079
*/

#include <xeroskernel.h>
#include <stdarg.h>

/* Your code goes here */
extern pcb_t *stop_q;
pcb_t *ready_q;
pcb_t *block_q;

/*
* dispatch
*
* @desc:	executes the dispatcher
*/
void dispatch() 
{
	unsigned int request,pid;
	pcb_t *p=NULL;
    	va_list ap;

	/* create arg(s) */
	unsigned int stack=0;
	void (*funcptr)(void);

	/* puts arg(s) */
	char* str=NULL;

	/* sleep arg(s) */
	unsigned int sleep_ms=0;

	/* ipc arg(s) */
	pcb_t *proc=NULL;
	void *buffer;
	int buffer_len;
	

	/* start dispatcher */
	for(;;) 
	{
		p = next();

		/* execute the idle proc only when there are no other proc in ready_q */
		if(p->pid == 0 && count() > 0) 
		{
			ready(p);
			continue;
		}
		request = contextswitch(p);

		// service syscall/interrupt requests
		switch(request) {
 			case TIMER_INT:
				/* signal sleep device is there's at least 1 sleeping proc */
				if(sleeper() > 0 && tick())
					ready(wake());

				ready(p);			
				end_of_intr();
				break;

			case CREATE:	
				/* retrieve args passed from syscall() */
				ap = (va_list)p->args;
				funcptr = va_arg(ap, void*);
				stack = va_arg(ap, int);

				/* create new process */
				create(funcptr, stack);

				ready(p);
				break;

			case YIELD:
				ready(p);
				break;

			case STOP:
				/* free allocated memory and put process on stop queue */
				stop(p);
				kfree(p->mem);
				break;
			
			case GETPID:
				/* sets the proc pid as the proc return code for next ctsw */
				p->rc = p->pid;
				ready(p);								
				break;

			case PUTS:
				/* synchronous kernel print handler*/
				ap = (va_list)p->args;
				str = va_arg(ap, char*);
				kprintf("[kernel]: %s", str);
				ready(p);				
				break;

			case SLEEP:
				ap = (va_list)p->args;
				sleep_ms = va_arg(ap, unsigned int);
				p->delta_slice = sleep_to_slice(sleep_ms);

				/* proc requested no sleep or syssleep is blocked for the time requested*/
				if(!p->delta_slice || !sleep(p))
					ready(p);

				break;
		
			case SEND:
				ap = (va_list)p->args;
				pid = va_arg(ap, unsigned int);
				buffer = va_arg(ap, void*);
				buffer_len = va_arg(ap, int);
			
				proc = unblock(pid);
				if(proc)
				{
					ready(p);
					ready(proc);
					//send(p, proc);
				}
				else
				{
					block(p);
				}
				break;
				
			case RECV:
				ap = (va_list)p->args;
				pid = va_arg(ap, unsigned int);
				buffer = va_arg(ap, void*);
				buffer_len = va_arg(ap, int);
			
				proc = unblock(pid);
				if(proc)
				{
					ready(p);
					ready(proc);
					//recv(p, proc);
				}
				else
				{
					block(p);
				}

				break;
		}
	}
}

/*
* next
*
* @desc: 	pop the head of ready queue
*
* @output:	p	current head of the ready queue
*/
pcb_t* next ()
{
    	pcb_t *p = ready_q;
    	if(p) ready_q = p->next;
    	return p;
}

/*
* ready
*
* @desc:	push pcb block to the end of ready queue
*/
void ready(pcb_t *p) 
{
	pcb_t *tmp = ready_q;

	p->next=NULL;

	if(!tmp) 
	{
		ready_q = p;
		ready_q->next = NULL;
		return;
	}

	while(tmp) 
	{
		if(!tmp->next) break;
		tmp = tmp->next;
	}
	tmp->next = p;
}

/*
* count
*
* @desc:	count the number of pcb in the ready queue
* @note:	the stop queue count is (MAX_PROC-cnt)
*/
int count (void)
{
	int cnt=0;
	pcb_t *tmp = ready_q;

	while(tmp) 
	{
		cnt++;
		tmp=tmp->next;
	}

	return cnt;
}

/*
* stop
*
* @desc: 	add pcb block to the end of stop queue
*/
void stop (pcb_t *p)
{
	pcb_t *tmp = stop_q;

	p->next=NULL;
	if(!tmp) 
	{
		stop_q = p;
		stop_q->next = NULL;
		return;
	}

	while(tmp) 
	{
		if(!tmp->next) break;
		tmp = tmp->next;
	}
	tmp->next = p;
}

/*
* block
*
* @desc: 	add pcb block to the end of block queue
*
* @note:	a proc can be blocked by the following events,
*		1. ipc_send();
*		2. ipc_recv();
*/
void block(pcb_t *p)
{
	pcb_t *tmp = block_q;

	p->next=NULL;
	if(!tmp) 
	{
		block_q = p;
		block_q->next = NULL;
		blocker();
		return;
	}

	while(tmp) 
	{
		if(!(tmp->next)) break;
		tmp = tmp->next;
	}
	tmp->next = p;

	blocker();
}

/*
* unblock
*
* @desc: 	get the proc pcb from the block_q
*
* @param:	pid		pid of the proc pcb to retrieve from the block_q
*
* @output:	p		proc pcb for the matching input pid
*
* @note:	ipc_senders calls this function to find the matching ipc_receiver, and vice versa
*/
pcb_t* unblock(unsigned int pid)
{
	pcb_t *tmp=NULL, *p=NULL;

	/* unblock proc found at the head of the block_q */
	if(block_q->pid == pid)
	{
		p = block_q;
		block_q = block_q->next;
		return p;
	}

	/* find matching ipc proc in body of block_q, 
	*  at this point, the algorithm assumes the block_q has at least 2 proc
	*/
	tmp = block_q;
	while(tmp && tmp->next) 
	{
			
		/* unblock proc found in the body of the block_q */
		if(tmp->next->pid == pid)
		{
			p = tmp->next;
			tmp->next = tmp->next->next;
			return p;
		}

		tmp = tmp->next;
	}

	return NULL;
}

/*
* blocker
*
* @desc: 	count number of proc pcb in the block_q
*
* @output:	cnt		number of proc pcb in the block_q
*/
unsigned int blocker()
{
	unsigned int cnt=0;
	pcb_t *p=block_q;

	while(p)
	{
		cnt++;
		p=p->next;
	}

	return cnt;
}

/*
* puts_ready_q
*
* @desc: 	output all ready queue proc pid to console
*/
void puts_ready_q()
{
	pcb_t *tmp = ready_q;

	while(tmp) 
	{
		kprintf("%d ", tmp->pid);
		tmp=tmp->next;
	}
	kprintf("\n");
}

/*
* puts_block_q
*
* @desc: 	output all block queue proc pid to console
*/
void puts_block_q()
{
	pcb_t *tmp = block_q;

	while(tmp) 
	{
		kprintf("%d ", tmp->pid);
		tmp=tmp->next;
	}
	kprintf("\n");
}
