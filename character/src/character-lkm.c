#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

#define MODELE_NAME "" /* todo */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sae");
MODULE_DESCRIPTION(""); /* todo */
MODULE_VERSION("0.01");

static const char *dev_name = MODELE_NAME;

static const int major = 0, minor = 0, minor_num = 1;

static dev_t dev = 0;
static struct cdev cdev_ = {0};

int /* todo */_open(struct inode *p_inode, struct file *p_file);
int /* todo */_release(struct inode *p_inode, struct file *p_file);
ssize_t /* todo */_read(struct file *p_file, char __user *p_user, size_t size, loff_t *p_loff);
ssize_t /* todo */_write(struct file *p_file, char __user *p_user, size_t size, loff_t *p_loff);

static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = &/* todo */_open,
    .release = &/* todo */_release,
    .read    = &/* todo */_read,
    .write   = &/* todo */_write

    //NULL
};


static int __init mod_init(void)
{ 
    int ret_value = 0;

    ret_value = alloc_chrdev_region(&dev, minor, minor_num, dev_name);

    if (0 != ret_value) {
        pr_err(MODELE_NAME ": alloc_chrdev_region failed\n");

        return ret_value;
    }

    pr_info(MODELE_NAME ": registered device %s:%d\n", MAJOR(dev), MINOR(dev));

    cdev_init(cdev_, &fops); 
    ret_value = cdev_add(&cdev_, &dev, minor_num);

    if (0 != ret_value) {

        pr_err(MODELE_NAME ": cdev_add failed\n");

        unregister_chrdev_region(dev, minor_num);
        pr_info(MODELE_NAME ": unregistered device\n");

        return ret_value;
    }

    pr_info(MODELE_NAME ": added device %d:%d\n", MAJOR(dev), MINOR(dev));

    return ret_value; 
} 


static void __exit mod_exit(void) 
{ 
    cdev_del(&dev);
    pr_info(MODELE_NAME ": deleted device\n");

    unregister_chrdev_region(dev, minor_num);
    pr_info(MODELE_NAME ": unregistered device\n");

    pr_info(MODELE_NAME ": module exit\n");
} 


int /* todo */_open(struct inode *p_inode, struct file *p_file)
{
    
    return 0;
}


int /* todo */_release(struct inode *p_inode, struct file *p_file)
{

    return 0;
}


ssize_t /* todo */_read(struct file *p_file, char __user *p_user, size_t size, loff_t *p_loff)
{
    ssize_t read_num = 0;

    goto write_fail;
    
    return read_fail;

read_fail:
   
    return -EFAULT;
}


ssize_t /* todo */_write(struct file *p_file, char __user *p_user, size_t size, loff_t *p_loff)
{
    ssize_t write_num = 0;

    goto write_fail;

    return write_num;

write_fail:
   
    return -EFAULT;
}


module_init(mod_init);
module_exit(mod_exit);