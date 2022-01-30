#include <linux/module.h>
#include <linux/poll.h>

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <asm/current.h>



static int major;
static int irq;
static struct gpio_desc *sr04_trig_gpio;
static struct gpio_desc *sr04_echo_gpio;
static u64 time_ns = 0;
static DECLARE_WAIT_QUEUE_HEAD(sr04_waitqueue);
static int value;
static struct class *sr04_class;

static struct of_device_id sr04_match_table[] = 
{
	{.compatible = "mwl,sr04", },
	{},
};

irqreturn_t sr04_isr(int irq, void *arg)
{
	int value = gpiod_get_value(sr04_echo_gpio);;
	if(value == 1)
	{
		time_ns = ktime_get_ns();
	}
	else
	{
		time_ns = ktime_get_ns() - time_ns;
		wake_up(&sr04_waitqueue);
	}
	
	return IRQ_NONE;
}

static void sr04_start(void)
{
	gpiod_set_value(sr04_trig_gpio, 1);
	udelay(10);
	gpiod_set_value(sr04_trig_gpio, 0);
}


ssize_t sr04_read (struct file *p_file, char __user * buf, size_t size, loff_t *offset)
{
	size_t len = 0;
	len = size < 4 ? size : 4;

	sr04_start();
	wait_event_interruptible(sr04_waitqueue, time_ns);
	copy_to_user(buf, &time_ns, len);
	
	return len;
}

//构造file_ops
struct file_operations sr04_fops = 
{
	.owner = THIS_MODULE,
	.read = sr04_read,
};
	


int sr04_probe(struct platform_device *p_dev)
{
	printk("%s_%d \n", __FUNCTION__, __LINE__);
	//获取GPIO信息
	sr04_trig_gpio = gpiod_get(&p_dev->dev, "trig", GPIOD_OUT_LOW);
	sr04_echo_gpio = gpiod_get(&p_dev->dev, "echo", GPIOD_IN);

	gpiod_set_value(sr04_trig_gpio, 0);
	gpiod_set_value(sr04_echo_gpio, 0);

	//获取中断号
	irq = gpiod_to_irq(sr04_echo_gpio);

	//注册中断
	request_irq(irq, &sr04_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "sr04_irq", NULL);

	//创建设备节点
	device_create(sr04_class, NULL, MKDEV(major, 0), NULL, "dev_sr04");

	return 0;
}


int sr04_remove(struct platform_device *p_dev)
{
	printk("%s_%d \n", __FUNCTION__, __LINE__);
	device_destroy(sr04_class, MKDEV(major, 0));
	free_irq(irq, NULL);
	gpiod_put(sr04_trig_gpio);
	gpiod_put(sr04_echo_gpio);

	return 0;
}


//构造platform_driver
struct platform_driver sr04_platform_drv =
{
	.probe = sr04_probe,
	.remove = sr04_remove,
	.driver = {
		.name = "sr04_drv",
		.of_match_table = sr04_match_table,
	},
};




//入口函数
static int sr04_init(void)
{
	printk("%s_%d \n", __FUNCTION__, __LINE__);
	

	//注册file_ops	
	major = register_chrdev(0, "sr04_chrdev", &sr04_fops);

	sr04_class = class_create(THIS_MODULE, "sr04_class");
	if (IS_ERR(sr04_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "sr04");
		return PTR_ERR(sr04_class);
	}

	//注册platfrom_driver
	platform_driver_register(&sr04_platform_drv);
	return 0;
}


//出口函数
static void sr04_exit(void)
{
	printk("%s_%d \n", __FUNCTION__, __LINE__);
	platform_driver_unregister(&sr04_platform_drv);
	class_destroy(sr04_class);
	unregister_chrdev(major, "sr04_chrdev");
}


module_init(sr04_init);
module_exit(sr04_exit);
MODULE_LICENSE("GPL");

