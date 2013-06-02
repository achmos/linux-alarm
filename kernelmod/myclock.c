#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/moduleparam.h>
#include <linux/time.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <asm/siginfo.h>
#include <linux/slab.h>

#define SIG_TEST 44

static int setclock;

//signal object
struct siginfo info;

//head of our linked list
static LIST_HEAD(time_list);

//spinlock for synchronization
static DEFINE_SPINLOCK(myLock);

#define CACHE_NAME "slab_struct"

//our slab allocator's cache
static struct kmem_cache *timer_cache;

//structure that will be used by slab allocator
typedef struct timing_struct {
	struct timer_list myTimer;	
	int time;
	struct list_head list;
} timing_struct;

//pointer to userspace program's task struct
static	struct task_struct *t;

//timer callback function
void my_timer_callback(unsigned long data)
{
	unsigned long flags;
	int	ret, time = 0, pending;
	struct timing_struct* tObj;

	printk("myclock_kobj: my_timer_callback called, send signal to pid %d\n",t->pid);
	
	//critical section starts here (using a spinlock)
	spin_lock_irqsave(&myLock, flags);
	
	//traverse the list
	list_for_each_entry(tObj, &time_list, list) {
		pending = timer_pending(&(tObj->myTimer));
		if (!pending) {
			//get all info and deallocated the timer
			time = tObj->time;
			list_del(&(tObj->list));
			kmem_cache_free(timer_cache,tObj);
			printk("myclock: timer DEALLOCATED!\n");
			break;
		}
	}
	
	//now send the signal
	memset(&info,0,sizeof(struct siginfo));
	info.si_signo=SIG_TEST;
	info.si_code=SI_QUEUE;
	info.si_int= time;
	ret=send_sig_info(SIG_TEST,&info,t);
	if(ret < 0) {
		printk("myclock_kobj: error sending signal\n");
	}
	
	spin_unlock_irqrestore(&myLock, flags);
	//critical section finished!
}

/*---------------------------------------------------
* Code for the myclock entry in the /sys filesystem: 
* ---------------------------------------------------
*/

//--------------------------------------------
// myclock sys psuedo file function
//--------------------------------------------
static ssize_t myclock_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	printk("myclock_kobj: myclock_show called!\n");		
	return sprintf(buf, "%d %d\n", (int)CURRENT_TIME.tv_sec, (int)CURRENT_TIME.tv_nsec/1000);
}

static ssize_t myclock_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	printk("myclock_kobj myclock_store called!\n");
	return count;
}

//--------------------------------------------
// setclock sys psuedo file function
//--------------------------------------------
static ssize_t setclock_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	int	ret;
	struct timing_struct* tObj;
	
	printk("myclock_kobj (setclock_show): pid= %d, setclock = %d\n",current->pid, setclock);
	t = current;
	printk("myclock_kobj: starting timer to fire in %d seconds\n",setclock);
	
	tObj = kmem_cache_alloc(timer_cache, GFP_KERNEL);
	
	
	if (!tObj) {
		printk("myclock_kobj: Error in making timing struct!\n");
	} else {
		setup_timer(&(tObj->myTimer), my_timer_callback,0);
		tObj->time = setclock;
		list_add(&(tObj->list),&time_list);
	
		ret = mod_timer(&(tObj->myTimer),jiffies+msecs_to_jiffies(setclock*1000));
		if (ret) printk ("myclock_kobj:error in setting timer\n");
		else printk ("myclock_kobj:Timer set!!\n");
	}
	return sprintf(buf, "%d\n", setclock);

}

static ssize_t setclock_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	sscanf(buf, "%du", &setclock);
	printk("myclock_kobj (setclock_store): pid= %d, setclock = %d\n",current->pid, setclock);
	return count;
}
//--------------------------------------------
//Set all the attributes for the kobject
//--------------------------------------------
static struct kobj_attribute myclock2_attribute =
	__ATTR(myclock2, 0666, myclock_show, myclock_store);
	
static struct kobj_attribute setclock_attribute =
	__ATTR(setclock, 0666, setclock_show, setclock_store);
	
static struct attribute *attrs[] = {
	&myclock2_attribute.attr,
	&setclock_attribute.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static struct kobject *myclock_kobj;

/*---------------------------------------------------
* Code for the myclock entry in the /proc filesystem: 
* ---------------------------------------------------
*/
#define proc_file_name "myclock"

//proc file structure
struct proc_dir_entry * myproc_file;

//read function for specific proc file
int readfunc(char * buffer, char** buffer_location, 
			off_t offset, int buffer_length, int *eof, void *data) 
{
	int ret; 
	
	printk(KERN_ALERT "assign6 myclock readfunc called!\n");
	
	if (offset > 0) {
		ret = 0;
	} else {
		ret = sprintf(buffer,"%d %d\n", (int)CURRENT_TIME.tv_sec, (int)CURRENT_TIME.tv_nsec/1000);
	}
	return (ret);
}

/*-------------------------------
* Module init and exit functions: 
* -------------------------------
*/
//Module Initialize Function
static int myclockInitFunc(void) {
	int retval;
	
	/*-------------------------------
	* setup the proc entry....
	* -------------------------------*/
	printk(KERN_ALERT "Starting assign6 myclock module.\n");
	printk(KERN_ALERT "%d %d\n",(int)CURRENT_TIME.tv_sec, (int)CURRENT_TIME.tv_nsec/1000);
	
	//create our new proc file
	myproc_file = create_proc_entry(proc_file_name, 0644,NULL);
	
	//if allocation and creation failed
	if (myproc_file == NULL) {
		remove_proc_entry(proc_file_name,NULL);
		printk(KERN_ALERT "Error: could not create myclock proc file!");
		return -ENOMEM;
	}
	
	//set all the fields for our new proc file
	myproc_file->read_proc = readfunc;
	myproc_file->mode = S_IFREG | S_IRUGO;
	myproc_file->uid = 0;
	myproc_file->gid = 0;
	myproc_file->size = 0;
	
	printk(KERN_ALERT "/proc/myclock file created!\n");
	
	/*-------------------------------
	* setup the sys entry....
	* -------------------------------*/
	printk(KERN_ALERT "myclock_kobj: setting up kobject!\n");
	
	myclock_kobj = kobject_create_and_add("myclock2", kernel_kobj);
	if (!myclock_kobj)
		return -ENOMEM;

	/* Create the files associated with this kobject */
	retval = sysfs_create_group(myclock_kobj, &attr_group);
	if (retval)
		kobject_put(myclock_kobj);

	printk(KERN_ALERT "myclock_kobj: Initialized kobject\n");
	
	timer_cache = kmem_cache_create(CACHE_NAME, sizeof(timing_struct), 0, 0, NULL);
	printk(KERN_ALERT "slab allocator setup!\n");
	
	return retval;
}

//Module Exit and cleanup Function
static void myclockExitFunc(void) {
	printk(KERN_ALERT "Ending assign7 myclock module.\n");
	remove_proc_entry(proc_file_name,NULL);
	printk(KERN_ALERT "/proc/myclock removed!\n");
	kobject_put(myclock_kobj);
	printk(KERN_ALERT "myclock_kobj: uninstalled!\n");
	kmem_cache_destroy(timer_cache);
	printk(KERN_ALERT "myclock: cache destroyed!!\n");
	printk(KERN_ALERT "Myclock module unloaded!\n");
}

//declare init and exit functions for module
module_init(myclockInitFunc);
module_exit(myclockExitFunc);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("RAMIN_T");