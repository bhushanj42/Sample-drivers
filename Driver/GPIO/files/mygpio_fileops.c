#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio/consumer.h>	/* For GPIO Descriptor */
#include <linux/of.h>				/* For DT*/
#include <linux/platform_device.h>  /* For platform devices */
#include <linux/cdev.h>
#include <linux/fs.h>				/*This is for alloc_chrdev_region */
#include <linux/uaccess.h>          /* Required for the copy to user function */

/*-------------- Character driver related --------------*/
static ssize_t set_gpio(struct file *filp, const char __user *buf, size_t count, loff_t *pos);

#define CHAR_DEVICE_NAME 	"migpio-char" 
#define CHAR_CLASS 			"migpio-class" 
dev_t dev_num;
struct class *gpio_class;
struct cdev gpio_cdev;
static struct file_operations gpio_fops =
{
	.write = set_gpio
};
/*-------------- Character driver related --------------*/

/* Used to set file permission in /dev */
static int my_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

/* GPIO_GEN0 - GPIO17 - Pin 11 J8 */
/* GPIO_GEN2 - GPIO27 - Pin 13 J8 */
static struct gpio_desc *red, *green;

static const struct of_device_id gpiod_dt_ids[] = {
	{ .compatible = "my,gpio_sample",},	/*This line says that this driver is compatible with the string that's declared here*/
    { /* sentinel */ } 
};

static int my_pdrv_probe (struct platform_device *pdev)
{

	struct device *dev = &pdev->dev;
	dev_t curr_dev;
	int result;

	pr_err("Hello! device probed started");
	
	red = gpiod_get_index(dev, "myled", 0, GPIOD_OUT_HIGH);
	if (IS_ERR(red)) {
		pr_err("Some ERROR occurred for Pin 17");
	}
	
	green = gpiod_get_index(dev, "myled", 1, GPIOD_OUT_HIGH);
	if (IS_ERR(green)) {
		pr_err("Some ERROR occurred for Pin 27");
	}
	
	gpiod_set_value(red, 1);
	gpiod_set_value(green, 0);
	
	/*-------------- Character driver related --------------*/
	result = alloc_chrdev_region(&dev_num,	/* Major number allocated by kernal */
			0,			/* First Minor numbr we want */
			1,			/* Number of devices we wish to create */
			CHAR_DEVICE_NAME);
						
	if (result) {
		pr_err("Minor number not allocated");
		return result;
	}
 
	 /* Let's create our device's class, visible in /sys/class */
	gpio_class = class_create(THIS_MODULE, CHAR_CLASS);
	if (IS_ERR(gpio_class))
	{
		pr_err("Problem in creating CLASS");
		return PTR_ERR(gpio_class);
	}
	gpio_class->dev_uevent = my_uevent;
    
	cdev_init(&gpio_cdev, &gpio_fops);
	gpio_cdev.owner = THIS_MODULE;
	
	/* Device number to use to add cdev to the core */
	curr_dev = MKDEV(MAJOR(dev_num), MINOR(dev_num));
    
	/* Now make the device live for the users to access */ 
    	result = cdev_add(&gpio_cdev, curr_dev, 1);
	if (result) {
		class_destroy(gpio_class);
		cdev_del(&gpio_cdev);
		pr_err("Char device not added to the system");
		return result;
	}
	
	/* create a device node /dev/migpio-char, with our class used here, 
	devices can also be viewed under /sys/class/eep-class */ 
	device_create(gpio_class,
        	NULL,     /* no parent device */
                curr_dev,
                NULL,     /* no additional data */
                CHAR_DEVICE_NAME);
	/*-------------- Character driver related --------------*/
	
	pr_err("Hello! device probed ENDED");

	return 0;
}

static int my_pdrv_remove(struct platform_device *pdev)
{
	gpiod_set_value(red, 0);
	gpiod_set_value(green, 1);
	pr_err("Remove procedure started");
	
	/*-------------- Character driver related --------------*/
	device_destroy(gpio_class, dev_num);	//removes a device that was created with device_create

	class_destroy(gpio_class);

	cdev_del(&gpio_cdev);
	/*-------------- Character driver related --------------*/
	
	gpiod_put(red); 
	gpiod_put(green);
	pr_err("good bye!");

	return 0;
}

static struct platform_driver mypdrv = {
	.probe      = my_pdrv_probe,
    	.remove     = my_pdrv_remove,
    	.driver     = {
    		.name = "gpio_sample_desc",
        	.of_match_table = of_match_ptr(gpiod_dt_ids),
        	.owner = THIS_MODULE,
    },
};

ssize_t set_gpio(struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{
	pr_err("Writing to device = %c", buf[0]);
	
	switch (buf[0]) {
	case '0':						/* Both LEDs turned off */
		gpiod_set_value(red, 0);
		gpiod_set_value(green, 0);
		break;
	
	case '1':						/* Red LED turned off */
		gpiod_set_value(red, 1);
		break;
	
	case '2':						/* Red LED turned on */
		gpiod_set_value(red, 0);
		break;
	
	case '3':						/* Green LED turned off */
		gpiod_set_value(green, 1);
		break;
	
	case '4':						/* Green LED turned on */
		gpiod_set_value(green, 0);
		break;
	
	case '5':						/* Both LEDs turned on */
		gpiod_set_value(red, 1);
		gpiod_set_value(green, 1);
		break;
	
	default:
		pr_err("Function not implemented");
		break;
	}
	
	return count;
}

module_platform_driver(mypdrv);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("AUTHOR");
