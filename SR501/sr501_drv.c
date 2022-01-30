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
#include <asm/current.h>



static int major;
static int irq;
static struct gpio_desc *sr501_gpio;
static DECLARE_WAIT_QUEUE_HEAD(sr501_waitqueue);
static int has_data;
static int value;
static struct class *sr501_class;


static struct of_device_id sr501_match_table[] = 
{
	{.compatible = "mwl,sr501", },
	{},
};

irqreturn_t sr501_isr(int irq, void *arg)
{
	value = gpiod_get_value(sr501_gpio);
	value |= irq << 4;
	printk("%s_%d, irq %d, value %d\n", __FUNCTION__, __LINE__, irq, gpiod_get_value(sr501_gpio));
	has_data = 1;
	wake_up(&sr501_waitqueue);
	return IRQ_NONE;
}


ssize_t sr501_read (struct file *p_file, char __user * buf, size_t size, loff_t *offset)
{

	wait_event_interruptible(sr501_waitqueue, has_data);
	copy_to_user(buf, &value, 4);
	has_data = 0;
	
	return 4;
}

//构造file_ops
struct file_operations sr501_fops = 
{
	.owner = THIS_MODULE,
	.read = sr501_read,
};
	


int sr501_probe(struct platform_device *p_dev)
{
	printk("%s_%d \n", __FUNCTION__, __LINE__);
	//获取GPIO信息
	sr501_gpio = gpiod_get(&p_dev->dev, NULL, GPIOD_IN);


	//获取中断号
	irq = gpiod_to_irq(sr501_gpio);

	//注册中断
	request_irq(irq, &sr501_isr, IRQF_TRIGGER_RISING, "sr501_irq", NULL);

	//创建设备节点
	device_create(sr501_class, NULL, MKDEV(major, 0), NULL, "dev_sr501");

	return 0;
}


int sr501_remove(struct platform_device *p_dev)
{
	printk("%s_%d \n", __FUNCTION__, __LINE__);
	device_destroy(sr501_class, MKDEV(major, 0));
	free_irq(irq, NULL);
	gpiod_put(sr501_gpio);
	return 0;
}


//构造platform_driver
struct platform_driver sr501_platform_drv =
{
	.probe = sr501_probe,
	.remove = sr501_remove,
	.driver = {
		.name = "sr501_drv",
		.of_match_table = sr501_match_table,
	},
};




//入口函数
static int sr501_init(void)
{
	printk("%s_%d \n", __FUNCTION__, __LINE__);
	

	//注册file_ops	
	major = register_chrdev(0, "sr501_chrdev", &sr501_fops);

	sr501_class = class_create(THIS_MODULE, "sr501_class");
	if (IS_ERR(sr501_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "sr501");
		return PTR_ERR(sr501_class);
	}

	//注册platfrom_driver
	platform_driver_register(&sr501_platform_drv);
	return 0;
}


//出口函数
static void sr501_exit(void)
{
	printk("%s_%d \n", __FUNCTION__, __LINE__);
	class_destroy(sr501_class);
	unregister_chrdev(major, "sr501_chrdev");
	platform_driver_unregister(&sr501_platform_drv);
}


module_init(sr501_init);
module_exit(sr501_exit);
MODULE_LICENSE("GPL");

