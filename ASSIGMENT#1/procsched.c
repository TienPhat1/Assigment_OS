#include "procsched.h"
#include<linux/kernel.h>
#include<sys/syscall.h>

long procsched(pid_t pid, struct proc_segs *info){
	long sysvalue;
	unsigned long INFO[5];
	sysvalue = syscall(546, pid, INFO);
	info->mssv = INFO[0];
	info->pcount = INFO[1];
	info->run_delay = INFO[2];
	info->last_arrival = INFO[3];
	info->last_queued = INFO[4];
	return 0;
}
