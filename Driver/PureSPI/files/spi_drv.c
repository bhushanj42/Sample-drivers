#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of.h>		/* For DT*/
#include <linux/cdev.h>
#include <linux/fs.h>		/*This is for alloc_chrdev_region */
#include <linux/uaccess.h>	/* Required for the copy to user function */
#include <linux/spi/spi.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/gpio/consumer.h>	/* For GPIO Descriptor */

static ssize_t myspi_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static ssize_t myspi_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static int myspi_open(struct inode *inode, struct file *filp);

struct mystruct {
	struct cdev spi_cdev;
	struct spi_device *spiDevInStruct;
	u8 *tx_buff;
	u8 *rx_buff;
	struct mutex buf_lock;
};
static unsigned bufferSize = 8;	/* Bytes */

/*---------------------------- SPI driver related ----------------------------*/
#define ID_FOR_FOO_DEVICE  0

static struct spi_device_id foo_idtable[] = {
   { "foo", ID_FOR_FOO_DEVICE },
   { /* sentinel */ }
};
MODULE_DEVICE_TABLE(spi, foo_idtable);

static const struct of_device_id foobar_of_match[] = {
	{ .compatible = "my,spi_sample" },
	{ /* sentinel */ }
}; 
MODULE_DEVICE_TABLE(of, foobar_of_match);
/*---------------------------- SPI driver related ----------------------------*/

/*------------------------- Character driver related -------------------------*/
#define CHAR_DEVICE_NAME	"purespi-char" 
#define CHAR_CLASS		"purespi-class" 
dev_t dev_num;
struct class *spi_class;
static struct file_operations purespi_fops =
{
	.owner = THIS_MODULE,
	.write = myspi_write,
	.read = myspi_read,
	.open = myspi_open,
};
/*------------------------- Character driver related -------------------------*/

/* Used to set file permission in /dev */
static int my_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "DEVMODE=%#o", 0666);
	return 0;
}

static int spi_probe(struct spi_device *spi)
{
	int err;
	struct device *dev = &spi->dev;
	dev_t curr_dev;
	struct mystruct *mystr = NULL;
	
	mystr = devm_kzalloc(dev, sizeof(struct mystruct), GFP_KERNEL);
	if (!mystr) {
		dev_err(dev, "Problem in setting memory for mystruct");
        	return -ENOMEM; 
	}
	
	/* Following step is only required because while removing the device
	we need a reference to struct cdev which is embedded in our strcuture */
	spi_set_drvdata(spi, mystr);
	
	mystr->spiDevInStruct = spi;
	mutex_init(&mystr->buf_lock);
/*---------------------------- SPI driver related ----------------------------*/
	spi->mode = SPI_MODE_0;
	spi->max_speed_hz = 122000;	/* 125 KHz */
	spi->bits_per_word = 8;
	spi->cs_gpio = 1;
	err = spi_setup(spi);
	if(err < 0) {
		dev_err(dev, "Problem in spi_setup");
		return err;
	}
/*---------------------------- SPI driver related ----------------------------*/
/*------------------------- Character driver related -------------------------*/
	err = alloc_chrdev_region(&dev_num,	/* Major number allocated by kernal */
		0,				/* First Minor numbr we want */
		1,				/* Number of devices we wish to create */
		CHAR_DEVICE_NAME);
						
	if (err) {
		dev_err(dev, "Minor number not allocated");
		return err;
	}
 
	 /* Let's create our device's class, visible in /sys/class */
	spi_class = class_create(THIS_MODULE, CHAR_CLASS);
	if (IS_ERR(spi_class)) {
		dev_err(dev, "Problem in creating CLASS");
		return PTR_ERR(spi_class);
	}
	spi_class->dev_uevent = my_uevent;
    
	cdev_init(&mystr->spi_cdev, &purespi_fops);
	mystr->spi_cdev.owner = THIS_MODULE;
	
	/* Device number to use to add cdev to the core */
	curr_dev = MKDEV(MAJOR(dev_num), MINOR(dev_num));
    
	/* Now make the device live for the users to access */ 
    	err = cdev_add(&mystr->spi_cdev, curr_dev, 1);
	if (err) {
		class_destroy(spi_class);
		cdev_del(&mystr->spi_cdev);
		dev_err(dev, "Char device not added to the system");
		return err;
	}
	
	/* create a device node /dev/migpio-char, with our class used here, 
	devices can also be viewed under /sys/class/eep-class */ 
	device_create(spi_class,
        	NULL,     /* no parent device */
               curr_dev,
               NULL,     /* no additional data */
               CHAR_DEVICE_NAME);
/*------------------------- Character driver related -------------------------*/
	dev_err(dev, "Hello! SPI device probed ENDED");

	return 0;
}

static int spi_remove(struct spi_device *spi)
{
	struct mystruct *mystr = spi_get_drvdata(spi);
	if (mystr == NULL) {
		dev_err(&spi->dev, "mystruct not found\n");
		return -ENODEV; /* No such device */
	}

	mutex_destroy(&mystr->buf_lock);
/*------------------------- Character driver related -------------------------*/
	device_destroy(spi_class, dev_num);

	class_destroy(spi_class);

	cdev_del(&mystr->spi_cdev);
/*------------------------- Character driver related -------------------------*/
	dev_err(&spi->dev, "good bye!");
	
	return 0;
}

static ssize_t myspi_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	unsigned long status;
	struct spi_transfer xtfr;
	struct spi_message spiMsgStruct;
	struct mystruct *mystr = filp->private_data;
	struct device *dev = NULL;

	if (mystr == NULL) {
		pr_err("mystruct not found\n");
		return -ENODEV; /* No such device */
	}
	
	dev = &mystr->spiDevInStruct->dev;
	if (count > bufferSize) {
		dev_err(dev, "Writing size overload = %zu", count);
		count = bufferSize;
	}

	mutex_lock(&mystr->buf_lock);

	status = copy_from_user(mystr->tx_buff, buf, count);

	if (status) {
		dev_err(dev, "Problem in copying data to Kernel");
		return -EFAULT;
	}
	
	xtfr.tx_buf = mystr->tx_buff;
	xtfr.len = count;
	xtfr.rx_buf = NULL;
	
	spi_message_init(&spiMsgStruct);
	spi_message_add_tail(&xtfr, &spiMsgStruct);
	
	status = spi_sync(mystr->spiDevInStruct, &spiMsgStruct);
	if (status == 0) {
		status = spiMsgStruct.actual_length;
		dev_err(dev, "Writing to device = %lu", status);
	}
	else {
		dev_err(dev, "There was a problem in writing = %lu", status);
	}
	mutex_unlock(&mystr->buf_lock);
	return status;
}

static ssize_t myspi_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{	
	unsigned long status;
	struct spi_transfer xtfr;
	struct spi_message spiMsgStruct;
	struct device *dev = NULL;
	struct mystruct *mystr = filp->private_data;
	
	if (mystr == NULL) {
		pr_err("mystruct not found\n");
		return -ENODEV; /* No such device */
	}
	
	dev = &mystr->spiDevInStruct->dev;
	
	if(count > bufferSize) {
		dev_err(dev, "Reading size overload = %zu", count);
		count = bufferSize;
	}
	
	mutex_lock(&mystr->buf_lock);
	
	xtfr.tx_buf = NULL;
	xtfr.len = count;
	xtfr.rx_buf = mystr->rx_buff;
	
	spi_message_init(&spiMsgStruct);
	spi_message_add_tail(&xtfr, &spiMsgStruct);
	
	status = spi_sync(mystr->spiDevInStruct, &spiMsgStruct);
	if (status == 0) {
		status = spiMsgStruct.actual_length;
		if (copy_to_user(buf, mystr->rx_buff, status)) {
			dev_err(dev, "Failure in copying to user");
			return -EFAULT;
		}
		dev_err(dev, "Reading from device = %lu", status);
	}
	else {
		dev_err(dev, "There was a problem in reading = %lu", status);
	}
	mutex_unlock(&mystr->buf_lock);
	return count;
}

static int myspi_open(struct inode *inode, struct file *filp)
{
	struct mystruct *mystr = NULL;
	struct device *dev = NULL;
	
	mystr = container_of(inode->i_cdev, struct mystruct, spi_cdev);
	if (mystr == NULL) {
		pr_err("Container_of did not find any valid data\n");
		return -ENODEV; /* No such device */
	}
	
	if (inode->i_cdev != &mystr->spi_cdev) {
        	pr_err("Device open: internal error\n");
        	return -ENODEV; /* No such device */
    	}
	
	filp->private_data = mystr;
	
	dev = &mystr->spiDevInStruct->dev;
	
	if (!mystr->tx_buff) {
		mystr->tx_buff = devm_kzalloc(dev, bufferSize, GFP_KERNEL);
		if (!mystr->tx_buff) {
			dev_err(dev, "open/ENOMEM\n");
			return -ENOMEM;
		}
	}
	
	if (!mystr->rx_buff) {
		mystr->rx_buff = devm_kzalloc(dev, bufferSize, GFP_KERNEL);
		if (!mystr->rx_buff) {
			dev_err(dev, "open/ENOMEM\n");
			return -ENOMEM;
		}
	}
	dev_err(dev, "Device Opening was successful");
	return 0;
}

static struct spi_driver mydrv = {
	.id_table = foo_idtable,
	.probe = spi_probe,
	.remove = spi_remove,
	.driver = {
		.name = "Pure_SPI_Driver",
        	.of_match_table = of_match_ptr(foobar_of_match),
        	.owner = THIS_MODULE,
	},
};

module_spi_driver(mydrv);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("AUTHOR");
