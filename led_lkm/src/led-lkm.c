/**
 * \file
 * \brief   A linux led lkm.
 * \authors Shimko A.
 * \version 0.1
 * \date    04.06.22
 *
 * \todo 1. A normal file description block.
 * 
 */

/** \defgroup The module header. @{ */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>

#if (0 != defined(LED_INPUT_ARG)) /**< A terminal arguments case. */
#include <linux/fs.h>
#include <linux/cdev.h>
#else                     /**< A DTS data case. */
#include <linux/mod_devicetable.h>
#include <linux/property.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/proc_fs.h>
#include <linux/gpio/consumer.h>
#endif
/** }@ */


/** A success status definition. */
#if (0 == defined(SUCCESS))
#define SUCCESS               0
#endif


/** Number of registered devices. */
#define LED_REG_DEVICE_NUM    1U

#define DRIVER_NAME           "led-lkm"
#define DRIVER_CLASS          "lkm"
#define MODULE_LOG_NAME       "led-lkm: "


#if (0 != defined(LED_INPUT_ARG))    /**< A terminal arguments case. */
/** \defgroup The module structs. @{ */
static dev_t device_num_s;
static struct class *p_class_s;
static struct cdev device_s;
/** }@ */
#endif

/**
 * If the module has to start via the terminal instead .dts file \
 * the lED_INPUT_ARG define have to set by "make input_arg" \
 * command. It lets to start tje module with input arguments.
 *
 * \example sudo insmod led_lkm.ko pin_num=4
 *
 * \defgroup The LED pin number definition.
 * @{
 */
#if  (0 != defined(LED_INPUT_ARG))    /**< A terminal arguments case. */
static unsigned int pin_num = 4;
module_param(pin_num, uint, S_IRUSR | S_IRGRP | S_IROTH);   /**< S_IRUSR | S_IRGRP | S_IROTH allows sudo cat /sys/module/led-lkm command. */
MODULE_PARM_DESC(pin_num, "A LED pin number.");
#else                                 /**< A DTS data case. */
static struct gpio_desc *pin_num = NULL;
static struct of_device_id ids[] = {
	{
		.compatible = "led-lkm,dev",
	}, { }
};

MODULE_DEVICE_TABLE(of, ids);

static struct proc_dir_entry *proc_file;
#endif
/** }@ */


/**
 * Sends data to the device from userspace.
 * \param[in] p_file_s     File to write into.
 * \param[in] buffer       User space buffer.
 * \param[in] lenght       Length of the buffer
 * \param[in] offset       Present position pointer.
 *
 * return Number of bytes successfully written or -EINVAL if it failed.
 */
static ssize_t _write(struct file *p_file_s, const char *p_buffer, size_t lenght, loff_t *p_offset) 
{
    char value = 0;

    enum states {
        clear_e = '0',
        set_e = '1',
        line_e = '\n'
    };

    int to_copy = 0, not_copied = 0, delta = 0;
    const int clear_c = 0, set_c = 1;

    to_copy = min(lenght, sizeof(value));

    not_copied = copy_from_user(&value, p_buffer, to_copy);

    /** The setting LED proccess. */
    switch (value) {
        case clear_e:
            gpio_set_value(pin_num, clear_c);
            break;
        case set_e:
            gpio_set_value(pin_num, set_c);
            break;
        case line_e:
            asm("NOP");
            break;
        default:
            pr_err(MODULE_LOG_NAME "Invalid input.\n");
            break;
    }

    delta = to_copy - not_copied;

    return delta;
}


/**
 * Opens the module for access from userspace.
 * \param[in] p_inod_s    The major and minor numbers information.
 * \param[in] p_file_s    File to write into.
 *
 * return Always SUCCESS.
 */
static int _open(struct inode *p_inod_s, struct file *p_file_s)
{
    pr_info(MODULE_LOG_NAME "Open was called\n");
    return SUCCESS;
}


/**
 * Closes the module for access from userspace.
 * \param[in] p_inod_s    The major and minor numbers information.
 * \param[in] p_file_s    File to write into.
 *
 * return Always SUCCESS.
 */
static int _close(struct inode *p_inod_s, struct file *p_file_s)
{
    pr_info(MODULE_LOG_NAME "Close was called.\n");
    return SUCCESS;
}


/** \brief Fops structs.*/
static struct file_operations fops_s = {
    .owner = THIS_MODULE,
    .open = _open,
    .release = _close,
    .write = _write
};


#if (0 == defined(LED_INPUT_ARG))    /**< A terminal arguments case. */
static int dt_probe(struct platform_device *p_platform_dev_s)
{
    const char *p_label = NULL;
    int value = 0, ret = 0;

    struct device *p_dev_s = &p_platform_dev_s->dev;

    if(SUCCESS != device_property_present(p_dev_s, "label")) {
        pr_err(MODULE_LOG_NAME "Device property 'label' not found.\n");
        return -EPERM;
    }

    if(SUCCESS != device_property_present(p_dev_s, "value")) {
        pr_err(MODULE_LOG_NAME "Device property 'value' not found.\n");
        return -EPERM;
    }

    if(SUCCESS != device_property_present(p_dev_s, "led-gpio")) {
        pr_err(MODULE_LOG_NAME "Device property 'led-gpio' not found.\n");
            return -EPERM;
    }

    ret = device_property_read_string(p_dev_s, "label", &p_label);
    if(SUCCESS != ret) {
        pr_err(MODULE_LOG_NAME "Could not read 'label'.\n");
        return -EPERM;
    }

    ret = device_property_read_u32(p_dev_s, "value", &value);
    if(SUCCESS != ret) {
        pr_err(MODULE_LOG_NAME "Could not read the value.\n");
        return -EPERM;
    }

    pin_num = gpiod_get(p_dev_s, "led", GPIOD_OUT_LOW);
    if(IS_ERR(pin_num)) {
        pr_err(MODULE_LOG_NAME "Could not setup the LED GPIO.\n");
        return -1 * IS_ERR(pin_num);
    }

    proc_file = proc_create("led-lkm", 0666, NULL, &fops_s);
    if(NULL == proc_file) {
        pr_err(MODULE_LOG_NAME "Error creating /proc/led-lkm.\n");
        gpiod_put(pin_num);
        return -ENOMEM;
    }

    return SUCCESS;
}


/**
 * /brief Unloading the driver.
 */
static int dt_remove(struct platform_device *pdev) {
	printk("dt_gpio - Now I am in the remove function\n");
	gpiod_put(pin_num);
	proc_remove(proc_file);
	return 0;
}


static struct platform_driver my_driver = {
        .probe = dt_probe,
        .remove = dt_remove,
        .driver = {
                .name = "led_driver",
                .of_match_table = ids
        }
};
#endif


/**
 * /brief Inits the module.
 *
 * return SUCCESS or -EINVAL.
 */
static int __init lkm_init(void)
{

#if (0 != defined(LED_INPUT_ARG))
    const int output_dir_value_c = 0;

    struct device *p_device_status_s = NULL;

    /** The device allocation. */
    if (SUCCESS !=  alloc_chrdev_region(&device_num_s, 0, 1, DRIVER_NAME)) {
        pr_err(MODULE_LOG_NAME "The allocatin failed.\n");
        return -ENOMEM;
    }

    pr_info(MODULE_LOG_NAME "The device allocation successed. Major: %d, Minor: %d.\n", device_num_s >> 20, device_num_s && 0xfffff);

    /** The device class creation. */
    p_class_s = class_create(THIS_MODULE, DRIVER_CLASS);
    if (IS_ERR(p_class_s)) {
        pr_err(MODULE_LOG_NAME "The device class creation failed.\n");
        goto class_error;
    }

    /** The device file creation. */
    p_device_status_s = device_create(p_class_s, NULL, device_num_s, NULL, DRIVER_NAME);
    if (IS_ERR(p_device_status_s)) {
        pr_err(MODULE_LOG_NAME "The device file creation failed.\n");
        goto file_error;
    }

    /** The device file initialization. */
    cdev_init(&device_s, &fops_s);

    /** The device registration to the kernel. */
    if (SUCCESS != cdev_add(&device_s, device_num_s, LED_REG_DEVICE_NUM)) {
        pr_err(MODULE_LOG_NAME "The device registration to kernel failed.\n");
        goto add_error;
    }

    /** A GPIO initialization. */
    if (SUCCESS != (gpio_request(pin_num, "led-gpio"))) {
        pr_err(MODULE_LOG_NAME "The GPIO %d allocation failed.\n", pin_num);
        goto add_error;
    }

    /** The GPIO direction setting. */
    if (SUCCESS != (gpio_direction_output(pin_num, output_dir_value_c))) {
        pr_err(MODULE_LOG_NAME "The GPIO %d setting to output failed.\n", pin_num);
        goto gpio_error;
    }

    pr_info(MODULE_LOG_NAME "The module initializing successed");
#else
   printk("dt_gpio - Loading the driver...\n");
	if(platform_driver_register(&my_driver)) {
		printk("dt_gpio - Error! Could not load driver\n");
		return -1;
	}
#endif


    return SUCCESS;

#if (0 != defined(LED_INPUT_ARG))
gpio_error:
    gpio_free(pin_num);
add_error:
    device_destroy(p_class_s, device_num_s);
file_error:
    class_destroy(p_class_s);
class_error:
    unregister_chrdev_region(device_num_s, LED_REG_DEVICE_NUM);

    return -EPERM;
#endif
}


/**
 * Exits the module.
 *
 */
static void __exit lkm_exit(void)
{
#if (0 != defined(LED_INPUT_ARG))
    const int gpio_exit_value_c = 0;

    gpio_set_value(pin_num, gpio_exit_value_c);
    gpio_free(pin_num);
    cdev_del(&device_s);
    device_destroy(p_class_s, device_num_s);
    class_destroy(p_class_s);
    unregister_chrdev_region(device_num_s, LED_REG_DEVICE_NUM);
    pr_info(MODULE_LOG_NAME "The module exit successed\n");
#else
   printk("dt_gpio - Unload driver");
	platform_driver_unregister(&my_driver);
#endif
}


/** \defgroup The module description. @{ */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sae");
MODULE_DESCRIPTION("A linux led lkm.");
MODULE_VERSION("0.01");
/** }@ */


/** \defgroup The module initialization. @{ */
module_init(lkm_init);
module_exit(lkm_exit);
/** }@ */
