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

static ktime_t tout;
static struct kt_data {
struct hrtimer timer;
ktime_t        period;
int            numb;
} *data;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
static int ktfun( struct hrtimer *var ) {
#else
static enum hrtimer_restart ktfun( struct hrtimer *var ) {
#endif
    ktime_t now = var->base->get_time();      // текущее время в типе ktime_t
    hrtimer_forward( var, now, tout );
    if (zzz) {
        sleepy++; hungry++; data->numb++; zzz--;
        strcpy(buf_msg, "I'm sleeping! Z-Z-Z...");  
    } else if (sleepy < 5 && hungry < 4) {
        strcpy(buf_msg, "I WANT TO SLEEP AND EAT!");  
    } else if (sleepy < 5) {
        strcpy(buf_msg, "I WANT TO SLEEP!");  
    } else if (hungry < 4) {
        strcpy(buf_msg, "I WANT TO EAT!");  
    } else {
        strcpy(buf_msg, "MIAU! MIAU!");  
    }
    
    data->numb = hungry + sleepy - 2; 
    if (sleepy <= 0) {
        data->numb = 0;
        printk( KERN_ERR "KITTEN DEAD!");
        //panic("MIAAAAAAAAAAAAAAAAAAU!");
    } else sleepy--;

    if (hungry <= 0) {
        data->numb = 0;
        printk( KERN_ERR "KITTEN DEAD!");
        //panic("MIAAAAAAAAAAAAAAAAAAU!");
    } else hungry--;
    printk(KERN_INFO "total %d: hungry %d - sleepy %d", data->numb, hungry, sleepy);

    return data->numb-- > 0 ? HRTIMER_RESTART : HRTIMER_NORESTART;
}

int timer_init(int ticks) {
    enum hrtimer_mode mode;
    #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
    mode = HRTIMER_REL;
    #else
    mode = HRTIMER_MODE_REL;
    #endif
    tout = ktime_set(10, 0 );      /* 1 sec. + 0 nsec. */
    data = kmalloc( sizeof(*data), GFP_KERNEL );
    data->period = tout;
    hrtimer_init( &data->timer, CLOCK_REALTIME, mode );
    data->timer.function = ktfun;
    data->numb = ticks;
    hrtimer_start( &data->timer, data->period, mode );
    return 0;
}

void timer_stop( void ) {
    hrtimer_cancel( &data->timer );
    kfree( data );
    return;
}


static int dev_open = 0;

static int kitten_open( struct inode *n, struct file *f ) {
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
      
static int kitten_release( struct inode *n, struct file *f ) {
   LOG( "close - node: %p, file: %p, refcount: %d", n, f, module_refcount( THIS_MODULE ) );
   if( 1 == mode ) dev_open--;
   if( 2 == mode ) kfree( f->private_data );
   return 0;
}

static char* get_buffer( struct file *f ) {
   static char static_buf[ LEN_MSG + 1 ] = "MIAU!"; // статический буфер :
   switch( mode ) {
      case 0:
      case 1:
      default:
         return static_buf;
      case 2:
         return (char*)f->private_data;
   }
}

// чтение из /dev/kitten :
static ssize_t kitten_read( struct file *f, char *buf, size_t count, loff_t *pos ) {
   static int odd = 0;
   char *buf_msg = get_buffer( f );
   if( 0 == odd ) {
      int res = copy_to_user( (void*)buf, buf_msg, strlen( buf_msg ) );
      odd = 1;
      put_user( '\n', buf + strlen( buf_msg ) );
      res = strlen( buf_msg ) + 1;
      return res;
   }
   odd = 0;
   return 0;
}

// запись в /dev/kitten :
static ssize_t kitten_write( struct file *f, const char *buf, size_t count, loff_t *pos ) {
   if (zzz) return 0;
   int res, len = count < LEN_MSG ? count : LEN_MSG;
   char *buf_msg = get_buffer( f );
   res = copy_from_user( buf_msg, (void*)buf, len );
   if( '\n' == buf_msg[ len -1 ] ) buf_msg[ len -1 ] = '\0';
   else buf_msg[ len ] = '\0';
   if (!strcmp(buf_msg, "sleep")) {
        printk(KERN_INFO "Z-Z-Z");
        zzz = 5;
        sleepy += 5;
   } else {
        hungry += 4;
   }
   return len;
}

static const struct file_operations kitten_fops = {
   .owner  = THIS_MODULE,
   .open =    kitten_open,
   .release = kitten_release,
   .read   =  kitten_read,
   .write  =  kitten_write,
};

static struct miscdevice kitten_dev = {
   MISC_DYNAMIC_MINOR, DEVNAM, &kitten_fops
};

ssize_t proc_node_read( char *buffer, char **start, off_t off,
	                        int count, int *eof, void *data ) {
    static int offset = 0, i;
	printk( KERN_INFO "read: %d\n", count );
	for( i = 0; offset <= LEN_MSG && '\0' != buf_msg[ offset ]; offset++, i++ )
	   *( buffer + i ) = buf_msg[ offset ];       // buffer не в пространстве пользователя!
	*( buffer + i ) = '\n';                       
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


static int __init kitten_init( void ) {
   int ret = misc_register( &kitten_dev );
   
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
          timer_init(10);
   }
   if( ret ) 
      printk( KERN_ERR "unable to register %s misc device", DEVNAM );
   else 
      LOG( "installed device /dev/%s\n", DEVNAM );
      err_node:
   return ret;
}

static void __exit kitten_exit( void ) {
   timer_stop();
   LOG( "released device /dev/%s\n", DEVNAM );
   misc_deregister( &kitten_dev );

   remove_proc_entry( NAME_NODE, NULL );
   printk( KERN_INFO "/proc/%s removed\n", NAME_NODE );
}

module_init( kitten_init );
module_exit( kitten_exit );



