/*
 * "Kitten" kernel module
 *
 * Mary Kuznetsova <marika952.2@gmail.com>
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>

#define LEN_MSG 256

static ssize_t kitten_read(struct file *file, char *buf,
        size_t count, loff_t *ppos) {
    char *hi_str = "MIAU\n";
    int len = strlen(hi_str);
    if (count < len)
        return -EINVAL;
    if (*ppos != 0)
        return 0;
    if (copy_to_user(buf, hi_str, len))
        return -EINVAL;
    *ppos = len;
    return len;
}

static ssize_t kitten_write(struct file *f, const char *buf, 
                            size_t count, loff_t *pos ) 
{ 
   int res, len = count < LEN_MSG ? count : LEN_MSG; 
   //char *buf_msg = (char*)f->private_data; 
   printk(KERN_INFO "niam-niam-niam"); 
   //res = copy_from_user( buf_msg, (void*)buf, len ); 
   //if( '\n' == buf_msg[ len -1 ] ) buf_msg[ len -1 ] = '\0'; 
   //else buf_msg[ len ] = '\0'; 
    //LOG( "put bytes : %d", len ); 
   return len; 
}

static const struct file_operations kitten_fops = {
    .owner  = THIS_MODULE,
    .read   = kitten_read,
    .write  = kitten_write,
};

static struct miscdevice kitten_dev = {
    MISC_DYNAMIC_MINOR,
    "kitten",
    &kitten_fops
};

static int __init kitten_init(void)
{
    int ret;
    ret = misc_register(&kitten_dev);
    if (ret)
        printk(KERN_ERR "Unable to register \"kitten\" misc device.\n");
    else
        printk(KERN_INFO "MIAU\n");
    return ret;
}
module_init(kitten_init);

static void __exit kitten_exit(void)
{
    misc_deregister(&kitten_dev);
    printk(KERN_INFO "kitten being unloaded.\n");
}
module_exit(kitten_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mary Kuznetsova <marika952.2@gmail.com");
MODULE_DESCRIPTION("\"Kitten\" kernel module");
MODULE_VERSION("0.1");
