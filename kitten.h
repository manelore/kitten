#include <linux/module.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>


#define DEVNAM "kitten"
#define LEN_MSG 256
#define NAME_DIR  "kitten"
#define NAME_NODE "kitten_node"

static char buf_msg[ LEN_MSG + 1 ] = "MIAU MIAU!";

MODULE_LICENSE( "GPL" );
MODULE_AUTHOR( "Maria Kuznetsova <marika952.2@gmail.com>" );
MODULE_VERSION( "0.1" );
