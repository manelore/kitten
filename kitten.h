#include <linux/module.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/kernel.h>

#define DEVNAM "kitten"
#define LEN_MSG 256
#define NAME_DIR  "kitten"
#define NAME_NODE "kitten"

char buf_msg[ LEN_MSG + 1 ] = "MIAU MIAU!";

int sleepy = 6;
int hungry = 4;
int zzz = 0;

MODULE_LICENSE( "GPL" );
MODULE_AUTHOR( "Maria Kuznetsova <marika952.2@gmail.com>" );
MODULE_VERSION( "0.1" );

