#include<linux/linkage.h>
#include<linux/sched.h>
#include<linux/unistd.h>
#include<linux/kernel.h>
#include<linux/syscalls.h>
struct proc_segs {
	unsigned long mssv;
	unsigned long pcount;
	unsigned long long run_delay;
	unsigned long long last_arrival;
	unsigned long long last_queued;
};

asmlinkage long sys_procsched(int pid,struct proc_segs * info){
	struct task_struct * task;
	for_each_process(task){
		if((int)task->pid == pid){
			info->mssv = 1712572;
			info->pcount = task->sched_info.pcount;
			info->run_delay = task->sched_info.run_delay;
			info->last_arrival = task->sched_info.last_arrival;
			info->last_queued = task->sched_info.last_queued;
		return 0;
		}
	}
	return 1;
}
