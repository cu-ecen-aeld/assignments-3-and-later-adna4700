/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
//#include "aesd-circular-buffer.h"
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/slab.h> //memory allocation and deallocation

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Aditi Nanaware");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
	 /**
     * TODO: handle open
     */

    struct aesd_dev *device;
	PDEBUG("open");
     //char_pt = container_of(inode->i_cdev, struct aesd_dev, cdev);
    //Uses the container_of macro to obtain a pointer to the struct aesd_dev structure associated with the character device file represented by the given inode.
    // The container_of macro takes three arguments: the first argument is a pointer to a member of the structure (in this case, inode->i_cdev)
    //the second argument is the type of the structure (in this case, struct aesd_dev)
    //the third argument is the name of the member in the structure (in this case, cdev).
	device = container_of(inode->i_cdev, struct aesd_dev, cdev);
	filp->private_data = device;
	
	return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");
	/**
     * TODO: handle release
     */
	return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = 0;
	ssize_t entry_offset = 0;
	ssize_t bytes_count_read = 0;
    size_t rem_count=0;
    int result = 0; 
	struct aesd_buffer_entry *buffer_entry = NULL;
	struct aesd_dev *device;
	
	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	printk(KERN_DEBUG "read %zu bytes with offset %lld",count,*f_pos);
	
	device = (struct aesd_dev*) filp->private_data;
	
    /**
     * TODO: handle read
     */

	if(filp == NULL || buf == NULL || f_pos == NULL)
    {
		return -EFAULT;
	}

	if (count == 0){
		return 0;
	}
	
	//lock the device
    //return 0 if the mutex has been acquired 
	if(mutex_lock_interruptible(&device->mutex_lock))
    {
		// If the process is interrupted, the function returns -ERESTARTSYS, indicating that the system call should be restarted
        printk(KERN_DEBUG "Mutex Lock Failed");
        PDEBUG(KERN_ERR "Mutex Lock Failed");
		retval = -ERESTARTSYS;
        return retval;
	}
	
	 //struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
    //   size_t char_offset, size_t *entry_offset_byte_rtn );

	buffer_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&(device->circular_buff), *f_pos, &entry_offset); 
	if(buffer_entry == NULL)
    {
		retval = 0;
        goto exiting;
	}


    /*********/
    // rem_count = (buffer_entry->size - entry_offset);

    // if(rem_count > count)
    // {
    //     bytes_count_read = count;
    // }
    // else
    // {
    //     bytes_count_read = rem_count;
    // }

    // /********/

	//copy data to user the for read
	//bytes_count_read = copy_to_user(buf, (buffer_entry->buffptr + entry_offset), count);
	
    result = copy_to_user(buf, buffer_entry->buffptr , buffer_entry->size);

    if(result)
    {
        retval = -EFAULT;
        goto exiting;
    }
    *f_pos += buffer_entry->size;
    retval =buffer_entry->size;
    
    exiting:
	mutex_unlock(&(device->mutex_lock));
	return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = 0;
    //size_t result_copy = 0;
	const char *new_string_entry = NULL;
	ssize_t data_recv = 0;
	struct aesd_dev *device = (struct aesd_dev*) filp->private_data;
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	printk(KERN_DEBUG "write %zu bytes with offset %lld",count,*f_pos);
	
	//device = (struct aesd_dev*) filp->private_data;
    /**
     * TODO: handle write
     */

	if(filp == NULL || buf == NULL || f_pos == NULL)
    {
		return -EFAULT;
	}
	
	// Check for NULL parameters
	if (count == 0)
    {
		return retval;
	}
	
	if(mutex_lock_interruptible(&(device->mutex_lock)))
    {
        // If the process is interrupted, the function returns -ERESTARTSYS, indicating that the system call should be restarted
		printk(KERN_DEBUG "Mutex Lock Failed");
        PDEBUG(KERN_ERR "Mutex Lock Failed");
		retval = -ERESTARTSYS;
        return retval;
	}
	
	if(device->new_string.size == 0)
    {
		//allocate the new buffer size
        //The GFP_KERNEL flag passed as the second argument to kzalloc indicates that the memory should
        // be allocated from the kernel's normal kernel memory pool
		device->new_string.buffptr = kzalloc(count*sizeof(char), GFP_KERNEL);

		if(device->new_string.buffptr == NULL)
        {
			//means memory not allocated
            printk(KERN_DEBUG "Memory allocation Failed");
            PDEBUG("Memory allocation error");
			goto exiting;
		}
	}

	else
    {
        //reallocation of the buffer
		device->new_string.buffptr = krealloc(device->new_string.buffptr, (device->new_string.size + count)*sizeof(char), GFP_KERNEL);
		if(device->new_string.buffptr == NULL)
        {
			printk(KERN_DEBUG "Memory allocation Failed");
            PDEBUG("Memory realloc failed");
			goto exiting;
		}
	}
	
	//copy data from the user
	data_recv = copy_from_user((void *)(device->new_string.buffptr + device->new_string.size), buf, count);
	
    retval = count - data_recv; //Write bytes
	device->new_string.size += retval;
	
	//add \n in circular buffer
	if(memchr(device->new_string.buffptr, '\n', device->new_string.size))
    {
		new_string_entry = aesd_circular_buffer_add_entry(&device->circular_buff, &device->new_string); 
		if(new_string_entry)
        {
			//free the entry
            kfree(new_string_entry);// !doubt about this
		}
        //Clear entry parameters
		device->new_string.buffptr = NULL;
		device->new_string.size = 0;

	}

    exiting:
	mutex_unlock(&device->mutex_lock);
	return retval;

}
struct file_operations aesd_fops = {
	.owner =    THIS_MODULE,
	.read =     aesd_read,
	.write =    aesd_write,
	.open =     aesd_open,
	.release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
	int err, devno = MKDEV(aesd_major, aesd_minor);

	cdev_init(&dev->cdev, &aesd_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aesd_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_ERR "Error %d adding aesd cdev", err);
	}
	return err;
}



int aesd_init_module(void)
{
	dev_t dev = 0;
	int result;
	result = alloc_chrdev_region(&dev, aesd_minor, 1,
			"aesdchar");
	aesd_major = MAJOR(dev);
	if (result < 0) {
		printk(KERN_WARNING "Can't get major %d\n", aesd_major);
		return result;
	}
	memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */

    //initialize the mutex
	mutex_init(&(aesd_device.mutex_lock));
	aesd_circular_buffer_init(&aesd_device.circular_buff);

	result = aesd_setup_cdev(&aesd_device);

	if( result ) 
    {
		unregister_chrdev_region(dev, 1);
	}
	return result;

}

void aesd_cleanup_module(void)
{
	uint8_t index = 0; 
	struct aesd_buffer_entry *buffer_entry = NULL;
	
	dev_t devno = MKDEV(aesd_major, aesd_minor);
	cdev_del(&aesd_device.cdev);
	kfree(aesd_device.new_string.buffptr);
	
    /**
     * TODO: cleanup AESD specific poritions here as necessary
    */

	AESD_CIRCULAR_BUFFER_FOREACH(buffer_entry, &aesd_device.circular_buff, index)
    {
		if(buffer_entry->buffptr != NULL)
        {
			kfree(buffer_entry->buffptr);
             buffer_entry->size = 0;
		}
	}
    mutex_destroy(&(aesd_device.mutex_lock));
	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);