#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of.h>		/* For DT*/
#include <linux/cdev.h>
#include <linux/fs.h>		/*This is for alloc_chrdev_region */
#include <linux/uaccess.h>	/* Required for the copy to user function */
#include <linux/spi/spi.h>
#include <linux/regmap.h>

static ssize_t myspi_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static ssize_t myspi_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static int myspi_open(struct inode *inode, struct file *filp);

struct mystruct {
	struct cdev spi_cdev;
	struct spi_device *spiDevInStruct;
	u8 *tx_buff;
	u8 *rx_buff;
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
#define CHAR_DEVICE_NAME	"mispi-char" 
#define CHAR_CLASS		"mispi-class" 
dev_t dev_num;
struct class *spi_class;
static struct file_operations spiregmap_fops =
{
	.owner = THIS_MODULE,
	.write = myspi_write,
	.read = myspi_read,
	.open = myspi_open
};
/*------------------------- Character driver related -------------------------*/

/*------------------------------ Regmap related ------------------------------*/
struct regmap *my_regmap;
/*------------------------------ Regmap related ------------------------------*/

/* Used to set file permission in /dev */
static int my_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "DEVMODE=%#o", 0666);
	return 0;
}

/*------------------------------ Regmap related ------------------------------*/
bool drv_writable(struct device *dev, unsigned int reg)
{
	dev_err(dev, "Writable called %d\n", reg);
	return 1;
}

bool drv_readable(struct device *dev, unsigned int reg)
{
	dev_err(dev, "Readable called %d\n", reg);
	return 1;
}
/*------------------------------ Regmap related ------------------------------*/

static int spi_probe(struct spi_device *spi)
{
	int err;
	struct device *dev = &spi->dev;
	struct regmap_config spi_regmap_config;
	dev_t curr_dev;
	struct mystruct *mystr = NULL;
	
	dev_err(dev, "Hello! SPI device probed started");
	
	mystr = devm_kzalloc(dev, sizeof(struct mystruct), GFP_KERNEL);
	if (!mystr) 
	{
		dev_err(dev, "Problem in setting memory for mystruct");
        	return -ENOMEM; 
	}
	
	mystr->spiDevInStruct = spi;
	
	if (!mystr->tx_buff)
    	{
		mystr->tx_buff = devm_kzalloc(dev, bufferSize, GFP_KERNEL);
		if (!mystr->tx_buff) {
			dev_err(dev, "open/ENOMEM\n");
			return -ENOMEM;
		}
	}
	
	if (!mystr->rx_buff)
    	{
		mystr->rx_buff = devm_kzalloc(dev, bufferSize, GFP_KERNEL);
		if (!mystr->rx_buff) {
			dev_err(dev, "open/ENOMEM\n");
			return -ENOMEM;
		}
	}
/*---------------------------- SPI driver related ----------------------------*/
	spi->mode = SPI_MODE_0;
	spi->max_speed_hz = 125000;	/* 125 KHz */
	spi->bits_per_word = 8;
/*---------------------------- SPI driver related ----------------------------*/

/*------------------------------ Regmap related ------------------------------*/
	/* setup the regmap configuration */ 
    	memset(&spi_regmap_config, 0, sizeof(spi_regmap_config));
    	spi_regmap_config.reg_bits = 8;
    	spi_regmap_config.val_bits = 8;
    	spi_regmap_config.writeable_reg = drv_writable;
    	spi_regmap_config.readable_reg = drv_readable;
	
	my_regmap = regmap_init_spi(spi, &spi_regmap_config); 
 
	if (IS_ERR(my_regmap)) 
	{
		err = PTR_ERR(my_regmap);
		dev_err(dev, "Failed to init regmap: %d\n", err);
		return err;
	}
/*------------------------------ Regmap related ------------------------------*/
	
/*------------------------- Character driver related -------------------------*/
	err = alloc_chrdev_region(&dev_num,	/* Major number allocated by kernal */
		0,				/* First Minor numbr we want */
		1,				/* Number of devices we wish to create */
		CHAR_DEVICE_NAME);
						
	if (err)
	{
		dev_err(dev, "Minor number not allocated");
		return err;
	}
 
	 /* Let's create our device's class, visible in /sys/class */
	spi_class = class_create(THIS_MODULE, CHAR_CLASS);
	if (IS_ERR(spi_class))
	{
		dev_err(dev, "Problem in creating CLASS");
		return PTR_ERR(spi_class);
	}
	spi_class->dev_uevent = my_uevent;
    
	cdev_init(&mystr->spi_cdev, &spiregmap_fops);
	mystr->spi_cdev.owner = THIS_MODULE;
	
	/* Device number to use to add cdev to the core */
	curr_dev = MKDEV(MAJOR(dev_num), MINOR(dev_num));
    
	/* Now make the device live for the users to access */ 
    	err = cdev_add(&mystr->spi_cdev, curr_dev, 1);
	if (err)
	{
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
	
	/* Following step is only required because while removing the device
	we need a reference to struct cdev which is embedded in our strcuture */
	spi_set_drvdata(spi, mystr);
	dev_err(dev, "Hello! SPI device probed ENDED");

	return 0;
}

static int spi_remove(struct spi_device *spi)
{
	struct mystruct *mystr = spi_get_drvdata(spi);
	if (mystr == NULL)
	{
		dev_err(&spi->dev, "mystruct not found\n");
		return -ENODEV; /* No such device */
	}
	
	dev_err(&spi->dev, "Remove procedure started for SPI");

/*------------------------------ Regmap related ------------------------------*/
	regmap_exit(my_regmap);
/*------------------------------ Regmap related ------------------------------*/

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
		
	if (mystr == NULL)
	{
		pr_err("mystruct not found\n");
		return -ENODEV; /* No such device */
	}
	
	dev = &mystr->spiDevInStruct->dev;
	
	if (count > bufferSize)
	{
		count = bufferSize;
	}
	
	status = copy_from_user(mystr->tx_buff, buf, count);
	if (status)
	{
		dev_err(dev, "Problem in copying data to Kernel");
		return -EFAULT;
	}
	
	xtfr.tx_buf = mystr->tx_buff;
	xtfr.len = count;
	xtfr.rx_buf = NULL;
	xtfr.cs_change = 1;
	xtfr.speed_hz = 0;	/*Defualt speed will be taken from SPI device */
	
	spi_message_init(&spiMsgStruct);
	spi_message_add_tail(&xtfr, &spiMsgStruct);
	
	status = spi_sync(mystr->spiDevInStruct, &spiMsgStruct);
	if (status == 0)
	{
		status = spiMsgStruct.actual_length;
		dev_err(dev, "Writing to device = %u", status);
	}
	else 
	{
		dev_err(dev, "There was a problem in writing = %u", status);
	}
	return status;
}

static ssize_t myspi_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{	
	unsigned long status;
	struct spi_transfer xtfr;
	struct spi_message spiMsgStruct;
	struct device *dev = NULL;
	struct mystruct *mystr = filp->private_data;
	
	if (mystr == NULL)
	{
		pr_err("mystruct not found\n");
		return -ENODEV; /* No such device */
	}
	
	dev = &mystr->spiDevInStruct->dev;
	
	if(count > bufferSize)
	{
		count = bufferSize;
	}
	
	xtfr.tx_buf = NULL;
	xtfr.len = count;
	xtfr.rx_buf = mystr->rx_buff;
/*		xtfr.cs_change = 1; */
	xtfr.speed_hz = 0;	/*Defualt speed will be taken from SPI device */
	
	spi_message_init(&spiMsgStruct);
	spi_message_add_tail(&xtfr, &spiMsgStruct);
	
	status = spi_sync(mystr->spiDevInStruct, &spiMsgStruct);
	if (status == 0)
	{
		status = spiMsgStruct.actual_length;
		if (copy_to_user(buf, mystr->tx_buff, status))
		{
			dev_err(dev, "Failure in copying to user");
			return -EFAULT;
		}
		dev_err(dev, "Reading from device = %d", status);
	}
	else
	{
		dev_err(dev, "There was a problem in reading = %d", status);
	}
	return count;
}

static int myspi_open(struct inode *inode, struct file *filp)
{
	struct mystruct *mystr = NULL;
	
	pr_err("Opening the device");
	
	mystr = container_of(inode->i_cdev, struct mystruct, spi_cdev);
	if (mystr == NULL)
	{
		pr_err("Container_of did not found any valid data\n");
		return -ENODEV; /* No such device */
	}
	
	if (inode->i_cdev != &mystr->spi_cdev)
	{
        	pr_err("Device open: internal error\n");
        	return -ENODEV; /* No such device */
    	}
	
	filp->private_data = mystr;
	
	pr_err("Device Opening was successful");
	
	return 0;
}

static struct spi_driver mydrv = {
	.id_table = foo_idtable,
	.probe = spi_probe,
	.remove = spi_remove,
	.driver = {
		.name = "simple_SPI_Driver",
        	.of_match_table = of_match_ptr(foobar_of_match),
        	.owner = THIS_MODULE,
    },
};

module_spi_driver(mydrv);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("AUTHOR");
