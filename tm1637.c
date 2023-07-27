#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <asm-generic/uaccess.h>
#include <linux/spinlock.h>
#include <linux/ioctl.h>


// IOCTL
#define tm_IOC_MAGIC 'M'
#define DISPLAY_NUMBER _IOW(tm_IOC_MAGIC,0,int)
#define SET_BRIGHTNESS _IOW(tm_IOC_MAGIC,1,int)
#define SHOW_DOT _IOW(tm_IOC_MAGIC,2,int)
#define ON_OFF _IOW(tm_IOC_MAGIC,3,int)





#define DEVICE_MAJOR 227
#define DEVICE_NAME "tm1637"
#define CLKK	EXYNOS4_GPX0(0)
#define DIO	EXYNOS4_GPX0(2)

#define SEG_A   0b00000001
#define SEG_B   0b00000010
#define SEG_C   0b00000100
#define SEG_D   0b00001000
#define SEG_E   0b00010000
#define SEG_F   0b00100000
#define SEG_G   0b01000000

//
//      A
//     ---
//  F |   | B
//     -G-
//  E |   | C
//     ---
//      D


#define TM1637_I2C_COMM1    0x40
#define TM1637_I2C_COMM2    0xC0
#define TM1637_I2C_COMM3    0x80



uint8_t m_pinClk;
uint8_t m_pinDIO;
uint8_t m_brightness=4;
uint8_t m_dot=0;


const uint8_t digitToSegment[] = {
 // XGFEDCBA
  0b00111111,    // 0
  0b00000110,    // 1
  0b01011011,    // 2
  0b01001111,    // 3
  0b01100110,    // 4
  0b01101101,    // 5
  0b01111101,    // 6
  0b00000111,    // 7
  0b01111111,    // 8
  0b01101111,    // 9
  0b01110111,    // A
  0b01111100,    // B
  0b00111001,    // C
  0b01000111,    // D
  0b01111001,    // E
  0b01110001     // F
  };



unsigned char buf[6];
uint8_t digits[4],i=0;


//adjust the delay time here
void bitdelay(void){
	udelay(10);
}

void start(void)
{
	gpio_direction_output(DIO,0);
  	bitdelay();
}

void stop(void)
{
	gpio_direction_output(DIO,0);
  	bitdelay();
	gpio_direction_output(CLKK,1);
  	bitdelay();
	gpio_direction_output(DIO,1);
  	bitdelay();
}

uint8_t encodeDigit(uint8_t digit)
{
	return digitToSegment[digit & 0x0f];
}


void setBrightness(uint8_t brightness, bool on)
{
	m_brightness = (brightness & 0x7) | (on? 0x08 : 0x00);
	//printk("Setting brightness : %x\n",m_brightness);
}

bool writeByte(uint8_t b)
{
  uint8_t data = b, ack;
	
  //printk("Start writting data!!!\n");
  // 8 Data Bits
  for(i = 0; i < 8; i++) {
    // CLK low
    gpio_direction_output(CLKK,0);
  	udelay(5);

	// Set data bit
    if (data & 0x01)
      gpio_direction_output(DIO,1);
    else
      gpio_direction_output(DIO,0);

 	udelay(5);

	// CLK high
    gpio_direction_output(CLKK,1);
  	bitdelay();
    data = data >> 1;
  }
	//printk("Data writting finished\n Wait for ACK\n");
  // Wait for acknowledge
  // CLK to zero
  gpio_direction_output(CLKK,0);
  gpio_direction_input(DIO);
  bitdelay();

  // CLK to high
  gpio_direction_output(CLKK,1);
  bitdelay();
  
  ack = gpio_get_value(DIO);
  if (ack == 0){
    gpio_direction_output(DIO,0);
	//printk("Read a succcess ACK!!!\n");
  	}

  bitdelay();
  gpio_direction_output(CLKK,0);
  bitdelay();

  return ack;
}

void showDot(bool on)
{	
	m_dot = on;
    if(on)
		digits[1] |= 0b10000000;
	else
		digits[1] &= 0b01111111;
}

void setSegments(const uint8_t segments[], uint8_t length, uint8_t pos)
{
	uint8_t k=0;
    // Write COMM1
    //printk("Write Command 1\n");
	start();
	writeByte(TM1637_I2C_COMM1);
	stop();

	// Write COMM2 + first digit address
	//printk("Write Command 2\n");
	start();
	writeByte(TM1637_I2C_COMM2 + (pos & 0x03));

	// Write the data bytes
	//printk("Write the data bytes\n");
	for (k=0; k < length; k++)
	  writeByte(segments[k]);

	stop();

	// Write COMM3 + brightness
	//printk("Write Command 3\n");
	start();
	writeByte(TM1637_I2C_COMM3 + (m_brightness & 0x0f));
	stop();
}


static ssize_t tm1637_read(struct file *file, char* buffer, size_t size, loff_t *off)
{
	
	printk("read in kernel, start setting numbers\n");
	
	for(i=0;i<4;i++)
		digits[i] = encodeDigit(i);
	
	setBrightness(4, 1);
	setSegments(digits, 4, 0);

	printk("numbers display (0123) on!\n");
    return 0;
}

static int tm1637_open(struct inode *inode, struct file *file)
{
    printk("open in kernel\n");
	
	printk("Start setting numbers\n");
	
	for(i=0;i<4;i++)
		digits[i] = encodeDigit(0);

	showDot(1);
	setBrightness(7, 1);
	setSegments(digits, 4, 0);

	printk("numbers display (00:00) on!\n");
    return 0;
}

long tm1637_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	int num,j;
	
	switch(cmd)
	{
		case DISPLAY_NUMBER:
			
			for(j=3;j>=0;j--){
				num = arg%10;
				digits[j] = encodeDigit(num);
				arg /= 10;
				}
			showDot(m_dot);
			setSegments(digits, 4, 0);
			//printk("Ioctl display setting complete!!\n");
			break;
		case SET_BRIGHTNESS:
			setBrightness(arg, 1);
			start();
			writeByte(TM1637_I2C_COMM3 + (m_brightness & 0x0f));
			stop();
			//printk("Ioctl setting brightness complete!!\n");
			break;
		case SHOW_DOT:
			showDot(arg);
			setSegments(digits, 4, 0);
			/*
			if(arg)
				printk("Ioctl setting dot display, ON!!\n");
			else
				printk("Ioctl setting dot display, OFF!!\n");
			*/
			break;
			
		case ON_OFF:
			
			if(arg) m_brightness |=0x08;
			else      m_brightness &=0xf7;
			start();
			writeByte(TM1637_I2C_COMM3 + (m_brightness & 0x0f));
			stop();
			//printk("Display setting  complete!!\n");
			break;
		default:
			printk("Unknown ioctl command!\n");
			break;
	
	}
	return 0;
}
static int tm1637_release(struct inode *inode, struct file *file)
{	
	//Set display off
	setBrightness(4, 0);
	
	// Write COMM3 + brightness
	//printk("Write Command 3\n");
	start();
	writeByte(TM1637_I2C_COMM3 + (m_brightness & 0x0f));
	stop();
	printk("LED display off!!!\n");
    printk("tm1637 release\n");
    return 0;
}

static struct file_operations tm1637_dev_fops={
    owner		            :	THIS_MODULE,
    open	              	:	tm1637_open,
	read		            :	tm1637_read,
	release	            	:	tm1637_release,
	unlocked_ioctl          :    tm1637_ioctl,
};
static struct class *tm1637_class;

static int __init tm1637_dev_init(void) 
{
	/*
 	 * Register device
 	 */
	int	ret;

	ret = register_chrdev(DEVICE_MAJOR, DEVICE_NAME, &tm1637_dev_fops);
	if (ret < 0) {
		printk(KERN_INFO "%s: registering device %s with major %d failed with %d\n",
		       __func__, DEVICE_NAME, DEVICE_MAJOR, DEVICE_MAJOR );
		return(ret);
	}
	printk("tm1637 driver register success!\n");
	
	tm1637_class = class_create(THIS_MODULE, "tm1637");
        if (IS_ERR(tm1637_class))
	{
		printk(KERN_WARNING "Can't make node %d\n", DEVICE_MAJOR);
                return PTR_ERR(tm1637_class);
	}

    device_create(tm1637_class, NULL, MKDEV(DEVICE_MAJOR, 0), NULL, DEVICE_NAME);
        
	printk("tm1637 driver make node success!\n");
	
	// Reserve gpios
	if( gpio_request( CLKK, DEVICE_NAME ) < 0 )	// request pin 2
	{
		printk( KERN_INFO "%s: %s unable to get CLKK gpio\n", DEVICE_NAME, __func__ );
		ret = -EBUSY;
		return(ret);
	}

	if( gpio_request( DIO, DEVICE_NAME ) < 0 )	// request pin 6
	{
		printk( KERN_INFO "%s: %s unable to get DIO gpio\n", DEVICE_NAME, __func__ );
		ret = -EBUSY;
		return(ret);
	}
	
	// Set gpios directions
	if( gpio_direction_output( CLKK, 1) < 0 )	// Set pin 2 as output with default value 1
	{
		printk( KERN_INFO "%s: %s unable to set CLKK gpio as output\n", DEVICE_NAME, __func__ );
		ret = -EBUSY;
		return(ret);
	}


	if( gpio_direction_output( DIO, 1) < 0 )	// Set pin 4 as output with default value 1
	{
		printk( KERN_INFO "%s: %s unable to set DIO gpio as output\n", DEVICE_NAME, __func__ );
		ret = -EBUSY;
		return(ret);
	}

    return 0;
}

static void __exit tm1637_dev_exit(void)
{
    gpio_free(CLKK);
	gpio_free(DIO);
    unregister_chrdev(DEVICE_MAJOR, DEVICE_NAME);
}

module_init(tm1637_dev_init);
module_exit(tm1637_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SAM");
