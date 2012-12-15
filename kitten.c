/*
 * "Kitten" kernel module
 *
 * Mary Kuznetsova <marika952.2@gmail.com>
 */

#include "kitten.h"


static int mode = 0; // открытие: 0 - без контроля, 1 - единичное, 2 - множественное
module_param( mode, int, S_IRUGO );
static int debug = 0;
module_param( debug, int, S_IRUGO );

#define LOG(...) if( debug !=0 ) printk( KERN_INFO __VA_ARGS__ )

static int dev_open = 0;

static int mopen_open( struct inode *n, struct file *f ) {
   LOG( "open - node: %p, file: %p, refcount: %d", n, f, module_refcount( THIS_MODULE ) );
   if( dev_open ) return -EBUSY;
   if( 1 == mode ) dev_open++;
   if( 2 == mode ) {
      f->private_data = kmalloc( LEN_MSG + 1, GFP_KERNEL );
      if( NULL == f->private_data ) return -ENOMEM;
      strcpy( f->private_data, "dynamic: not initialized!" );  // динамический буфер
   }
   return 0;
}
      
static int mopen_release( struct inode *n, struct file *f ) {
   LOG( "close - node: %p, file: %p, refcount: %d", n, f, module_refcount( THIS_MODULE ) );
   if( 1 == mode ) dev_open--;
   if( 2 == mode ) kfree( f->private_data );
   return 0;
}

static char* get_buffer( struct file *f ) {
   static char static_buf[ LEN_MSG + 1 ] = "static: not initialized!"; // статический буфер :
   switch( mode ) {
      case 0:
      case 1:
      default:
         return static_buf;
      case 2:
         return (char*)f->private_data;
   }
}

// чтение из /dev/mopen :
static ssize_t mopen_read( struct file *f, char *buf, size_t count, loff_t *pos ) {
   static int odd = 0;
   char *buf_msg = get_buffer( f );
   LOG( "read - file: %p, read from %p bytes %d; refcount: %d",
        f, buf_msg, count, module_refcount( THIS_MODULE ) );
   if( 0 == odd ) {
      int res = copy_to_user( (void*)buf, buf_msg, strlen( buf_msg ) );
      odd = 1;
      put_user( '\n', buf + strlen( buf_msg ) );
      res = strlen( buf_msg ) + 1;
      LOG( "return bytes :  %d", res );
      return res;
   }
   odd = 0;
   LOG( "return : EOF" );
   return 0;
}

// запись в /dev/mopen :
static ssize_t mopen_write( struct file *f, const char *buf, size_t count, loff_t *pos ) {
   int res, len = count < LEN_MSG ? count : LEN_MSG;
   char *buf_msg = get_buffer( f );
   LOG( "write - file: %p, write to %p bytes %d; refcount: %d",
        f, buf_msg, count, module_refcount( THIS_MODULE ) );
   res = copy_from_user( buf_msg, (void*)buf, len );
   if( '\n' == buf_msg[ len -1 ] ) buf_msg[ len -1 ] = '\0';
   else buf_msg[ len ] = '\0';
   LOG( "put bytes : %d", len );
   return len;
}

static const struct file_operations mopen_fops = {
   .owner  = THIS_MODULE,
   .open =    mopen_open,
   .release = mopen_release,
   .read   =  mopen_read,
   .write  =  mopen_write,
};

static struct miscdevice mopen_dev = {
   MISC_DYNAMIC_MINOR, DEVNAM, &mopen_fops
};

ssize_t proc_node_read( char *buffer, char **start, off_t off,
	                        int count, int *eof, void *data ) {
	// ... в точности то, что и в предыдущем случае ...
    static int offset = 0, i;
	printk( KERN_INFO "read: %d\n", count );
	for( i = 0; offset <= LEN_MSG && '\0' != buf_msg[ offset ]; offset++, i++ )
	   *( buffer + i ) = buf_msg[ offset ];       // buffer не в пространстве пользователя!
	*( buffer + i ) = '\n';                       // дополним переводом строки
	i++;
	if( offset >= LEN_MSG || '\0' == buf_msg[ offset ] ) {
	   offset = 0;
	   *eof = 1;                                 // возвращаем признак EOF
	}
	else *eof = 0;
	printk( KERN_INFO "return bytes: %d\n", i );
	if( *eof != 0 ) printk( KERN_INFO "EOF\n" );
	return i;
};


static int __init mopen_init( void ) {
   int ret = misc_register( &mopen_dev );
   
   struct proc_dir_entry *own_proc_node; 
   own_proc_node = create_proc_entry( NAME_NODE, S_IFREG | S_IRUGO | S_IWUGO, NULL ); 
   
   if( NULL == own_proc_node ) {
	      ret = -ENOMEM;
	      printk( KERN_ERR "can't create /proc/%s\n", NAME_NODE );
	      goto err_node;
   } else {
	      own_proc_node->uid = 0;
	      own_proc_node->gid = 0;
	      own_proc_node->read_proc = proc_node_read;
	      printk( KERN_INFO "module : success!\n");
   }
   if( ret ) 
      printk( KERN_ERR "unable to register %s misc device", DEVNAM );
   else 
      LOG( "installed device /dev/%s\n", DEVNAM );
      err_node:
   return ret;
}

static void __exit mopen_exit( void ) {
   LOG( "released device /dev/%s\n", DEVNAM );
   misc_deregister( &mopen_dev );

   remove_proc_entry( NAME_NODE, NULL );
   printk( KERN_INFO "/proc/%s removed\n", NAME_NODE );
}

module_init( mopen_init );
module_exit( mopen_exit );



