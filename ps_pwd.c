#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/dirent.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <asm/unistd.h>
#include <asm/errno.h>
#include <asm/uaccess.h>
#define SUCCESS 0
#define DEVICE_NAME "ps_pwd"
#define BUF_LEN 80
#define MAJOR_NUM 40
#define DEBUG  0
static char msg[BUF_LEN];
static char *msg_Ptr = NULL;
wait_queue_head_t wq;
DECLARE_WAIT_QUEUE_HEAD(wq);

/*Driver Open*/
static int 
driver_open(struct inode *i, 
                       struct file *f)
{
#ifdef DEBUG
    printk(KERN_INFO "device_open(%p)\n", f);
#endif
    if(msg_Ptr == NULL) msg_Ptr = msg;

    return SUCCESS;
}

/*Driver Release*/
static int 
driver_release(struct inode *inode, 
                          struct file *file)
{
#ifdef DEBUG
    printk(KERN_INFO "device_release(%p,%p)\n", inode, file);
#endif
    
    return SUCCESS;
}

/*Driver Read*/
static ssize_t 
driver_read(struct file *filep, 
                           char *buffer, 
                           size_t length, 
                           loff_t *offset ) 
{
    /*
     * Number of bytes actually written to the buffer
     */
    int bytes_read = 0;
#ifdef DEBUG
    printk(KERN_INFO "device_read(%p,%p,%d)\n", filep, buffer, length);
#endif
    /*
     * If there is no messages wait for them. 
     * 
     */
    if (*msg_Ptr == 0) interruptible_sleep_on(&wq);
        

    /*
     * Actually put the data into the buffer
     */
    while (length && *msg_Ptr) {
        put_user(*(msg_Ptr++),buffer++);
        length--;
        bytes_read++;
    }
#ifdef DEBUG
    printk(KERN_INFO "Read %d bytes, %d left\n", bytes_read, length);
#endif
    msg_Ptr = msg;
    memset(msg, 0, 80); 
    return bytes_read;
}

/*Driver Write*/
static ssize_t
driver_write(struct file *filep,
             const char *buffer, 
             size_t length, 
             loff_t *offset)
{
    int i;
#ifdef DEBUG
    printk(KERN_INFO "device_write(%p, %s, %d)", filep, buffer, length);
#endif
    for (i=0; i < length && i < BUF_LEN; ++i)
        get_user(msg[i], buffer + i);
        
    msg_Ptr = msg;
    wake_up_interruptible(&wq);
    return i;
}

static struct file_operations fops = {
    NULL,                           //lseek
    .read = driver_read,            //read
    .write = driver_write,          //write
    NULL,                           //readdir
    NULL,                           //select
    NULL,                           //ioctl
    NULL,                           //mmap
    .open = driver_open,            //open
    .release = driver_release,      //release
    NULL,                           //fsync
};
    



/*PS INIT*/
int ps_init(void)
{
    int ret_val;
    printk (KERN_INFO "Loading driver module for device /dev/ps_pwd.\n");
    ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops);
    if (ret_val < 0) {
        printk(KERN_ALERT "%s failed with %d\n",
            "Sorry, registering the character device ", ret_val);
        return ret_val;
    }
    printk (KERN_INFO "%s The major device number is %d.\n",
            "Registration was successful", MAJOR_NUM);
    return SUCCESS;
}

/*PS CLEANUP*/
void ps_cleanup(void)
{
    printk (KERN_INFO "Destroying driver module for device /dev/ps_pwd.\n");
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}


module_init(ps_init);
module_exit(ps_cleanup);

MODULE_LICENSE ("GPL");

MODULE_AUTHOR("Andrea Mambretti");
MODULE_DESCRIPTION("Manage the char device of my privilege separation system to provide the authentication functionality.");

MODULE_SUPPORTED_DEVICE("ps_pwd");
