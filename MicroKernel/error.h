/*	error.h for ulios
	作者：孙亮
	功能：定义错误代码
	最后修改日期：2009-05-29
*/

#ifndef	_ERROR_H_
#define	_ERROR_H_

#define NO_ERROR					0	/*无错*/

/*内核资源错误*/
#define KERR_OUT_OF_KNLMEM			-1	/*内核内存不足*/
#define KERR_OUT_OF_PHYMEM			-2	/*物理内存不足*/
#define KERR_OUT_OF_LINEADDR		-3	/*线性地址空间不足*/

/*进程线程错误*/
#define KERR_INVALID_PROCID			-4	/*非法进程ID*/
#define KERR_PROC_NOT_EXIST			-5	/*进程不存在*/
#define KERR_PROC_NOT_ENOUGH		-6	/*进程管理结构不足*/
#define KERR_PROC_KILL_SELF			-7	/*杀死进程自身*/
#define KERR_INVALID_THEDID			-8	/*非法线程ID*/
#define KERR_THED_NOT_EXIST			-9	/*线程不存在*/
#define KERR_THED_NOT_ENOUGH		-10	/*线程管理结构不足*/
#define KERR_THED_KILL_SELF			-11	/*杀死线程自身*/

/*内核注册表错误*/
#define KERR_INVALID_KPTNUN			-12	/*非法内核端口号*/
#define KERR_KPT_ALREADY_REGISTERED	-13	/*内核端口已被注册*/
#define KERR_KPT_NOT_REGISTERED		-14	/*内核端口未被注册*/
#define KERR_INVALID_IRQNUM			-15	/*非法IRQ号*/
#define KERR_IRQ_ALREADY_REGISTERED	-16	/*IRQ已被注册*/
#define KERR_IRQ_NOT_REGISTERED		-17	/*IRQ未被注册*/
#define KERR_CURPROC_NOT_REGISTRANT	-18	/*当前进程不是注册者*/

/*消息错误*/
#define KERR_INVALID_USERMSG_ATTR	-19	/*非法用户消息属性*/
#define KERR_MSG_NOT_ENOUGH			-20	/*消息结构不足*/
#define KERR_MSG_QUEUE_FULL			-21	/*消息队列满*/
#define KERR_MSG_QUEUE_EMPTY		-22	/*消息队列空*/

/*地址映射错误*/
#define KERR_MAPSIZE_IS_ZERO		-23	/*映射长度为0*/
#define KERR_MAPSIZE_TOO_LONG		-24	/*映射长度太大*/
#define KERR_PROC_SELF_MAPED		-25	/*映射进程自身*/
#define KERR_PAGE_ALREADY_MAPED		-26	/*目标进程页面已经被映射*/
#define KERR_ILLEGAL_PHYADDR_MAPED	-27	/*映射不允许的物理地址*/
#define KERR_ADDRARGS_NOT_FOUND		-28	/*地址参数未找到*/

/*程序执行错误*/
#define KERR_OUT_OF_TIME			-29	/*超时错误*/
#define KERR_ACCESS_ILLEGAL_ADDR	-30	/*访问非法地址*/
#define KERR_WRITE_RDONLY_ADDR		-31	/*写只读地址*/
#define KERR_THED_EXCEPTION			-32	/*线程执行异常*/
#define KERR_THED_KILLED			-33	/*线程被杀死*/

/*调用错误*/
#define KERR_INVALID_APINUM			-34	/*非法系统调用号*/
#define KERR_ARGS_TOO_LONG			-35	/*参数字串超长*/
#define KERR_INVALID_MEMARGS_ADDR	-36	/*非法内存参数地址*/
#define KERR_NO_DRIVER_PRIVILEGE	-37	/*没有执行驱动功能的特权*/

#endif
