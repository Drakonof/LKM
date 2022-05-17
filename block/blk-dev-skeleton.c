#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/blk-mq.h>
#include <linux/hdreg.h>

#define MODULE_NAME "blkdev_skeleton"

#define __SUCCESS 0

static const char *c_p_blkdev_name = MODULE_NAME;  

static int major = 0;

/* Just internal representation of the our block device
 * can hold any useful data */
typedef struct blkdev_s {
    sector_t capacity;
    u8 *data;   /* Data buffer to emulate real storage device */
    struct blk_mq_tag_set tag_set; //?
    struct request_queue *queue;
    struct gendisk *gdisk;
} blkdev_t;

/* Device instance */
static blkdev_t *blkdev = NULL;

static int blkdev_open(struct block_device *dev, fmode_t mode)
{
    pr_info(MODULE_NAME ": blkdev_open\n");
    return __SUCCESS;
}

static void blkdev_release(struct gendisk *gdisk, fmode_t mode)
{
    pr_info(MODULE_NAME ": blkdev_release\n");
}

int blkdev_ioctl(struct block_device *bdev, fmode_t mode, unsigned cmd, unsigned long arg)
{
    pr_info(MODULE_NAME ": blkdev_ioctl 0x%08x\n", cmd);

    return -ENOTTY;
}

/* Serve requests */
static int request_data(struct request *rq, unsigned int *nr_bytes)
{
    int ret = 0;

    struct bio_vec bvec;
    struct req_iterator iter;

    blkdev_t *dev = rq->q->queuedata;
    // loff_t - "длинное смещение", 64 бита даже на 32-х разрядных платформах
    // blk_rq_pos - the current sector
    loff_t pos = blk_rq_pos(rq) << SECTOR_SHIFT; // << SECTOR_SHIFT ??
    loff_t blkdev_size = (loff_t)(dev->capacity << SECTOR_SHIFT);

    memset(&bvec, 0, sizeof(struct bio_vec));
    memset(&iter, 0, sizeof(struct req_iterator));

    pr_info(MODULE_NAME ": request start from sector %lld  pos = %lld  blkdev_size = %lld\n", 
            blk_rq_pos(rq), pos, blkdev_size);

    /* Iterate over all requests segments */
    rq_for_each_segment(bvec, rq, iter) {
        unsigned long bv_len = bvec.bv_len;

        /* Get pointer to the data */
        void* bv_buf = page_address(bvec.bv_page) + bvec.bv_offset;

        /* Simple check that we are not out of the memory bounds */
        if ((pos + bv_len) > blkdev_size) {
            bv_len = (unsigned long)(blkdev_size - pos);
        }

        if (WRITE == rq_data_dir(rq)) {
            /* Copy data to the buffer in to required position to dev->data + pos from bv_buf*/
            memcpy(dev->data + pos, bv_buf, bv_len);
        } else {
            /* Read data from the buffer's position */
            memcpy(bv_buf, dev->data + pos, bv_len);
        }

        /* Increment counters */
        pos += bv_len;
        *nr_bytes += bv_len;
    }

    return ret;
}

/* queue callback function */
static blk_status_t queue_rq(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data* bd)
{
    unsigned int nr_bytes = 0;
    blk_status_t status = BLK_STS_OK;
    struct request *rq = bd->rq;

    /* Start request serving procedure */
    blk_mq_start_request(rq);

    if (0 != request_data(rq, &nr_bytes)) {
        status = BLK_STS_IOERR;
    }

    /* Notify kernel about processed nr_bytes */
    if (0 != blk_update_request(rq, status, nr_bytes)) {
        /* Shouldn't fail */
        BUG();
    }

    /* Stop request serving procedure */
    __blk_mq_end_request(rq, status);

    return status;
}

static struct blk_mq_ops mq_ops = {
    .queue_rq = queue_rq,
};

/* Set block device file I/O */
static struct block_device_operations blkdev_ops = {
    .owner = THIS_MODULE,
    .open = blkdev_open,
    .release = blkdev_release,
    .ioctl = blkdev_ioctl
};

static int __init blkdev_init(void)
{
    /* Register new block device and get the device major number */
    major = register_blkdev(major, c_p_blkdev_name);

    blkdev = kmalloc(sizeof (blkdev_t), GFP_KERNEL);

    if (NULL == blkdev) {
        pr_err(MODULE_NAME ": blkdev kmalloc in blkdev_init failed\n");
        unregister_blkdev(major, c_p_blkdev_name);

        return -ENOMEM;
    }

    blkdev->capacity = (100 * PAGE_SIZE); /* nsectors * __SECTOR_SIZE; */
    /* Allocate corresponding data buffer */
    blkdev->data = kmalloc(blkdev->capacity, GFP_KERNEL);

    if (NULL == blkdev->data) {
        pr_err(MODULE_NAME ": blkdev->data kmalloc in blkdev_init failed\n");
        unregister_blkdev(major, c_p_blkdev_name);
        kfree(blkdev);

        return -ENOMEM;
    }

    pr_info(MODULE_NAME ": initializing queue\n");

    blkdev->queue = blk_mq_init_sq_queue(&blkdev->tag_set, &mq_ops, 128, BLK_MQ_F_SHOULD_MERGE);

    if (blkdev->queue == NULL) {
        pr_err(MODULE_NAME ": blk_mq_init_sq_queue failed\n");
        kfree(blkdev->data);

        unregister_blkdev(major, c_p_blkdev_name);
        kfree(blkdev);

        return -ENOMEM;
    }

    /* Set driver's structure as user data of the queue */
    blkdev->queue->queuedata = blkdev;

    /* Allocate new disk */
    blkdev->gdisk = alloc_disk(1);

    if (0 == blkdev->gdisk)
    {
        pr_err(MODULE_NAME ": alloc_disk failure\n");
        blk_cleanup_queue(blkdev->queue);
        unregister_blkdev(major, c_p_blkdev_name);
        kfree(blkdev);
        return -ENOMEM;
    }

    /* Set all required flags and data */
    blkdev->gdisk->major = major;
    blkdev->gdisk->first_minor = 0;
    blkdev->gdisk->fops = &blkdev_ops;
    blkdev->gdisk->private_data = blkdev;
    blkdev->gdisk->queue = blkdev->queue;
    blkdev->gdisk->flags = GENHD_FL_NO_PART_SCAN;

    /* Set device name as it will be represented in /dev */
    strncpy(blkdev->gdisk->disk_name, c_p_blkdev_name, strlen(c_p_blkdev_name));

    pr_info(MODULE_NAME ": adding disk %s\n", blkdev->gdisk->disk_name);

    /* Set device capacity */
    set_capacity(blkdev->gdisk, blkdev->capacity);

    /* Notify kernel about new disk device */
    add_disk(blkdev->gdisk);

    return 0;
}

static void __exit blkdev_exit(void)
{
    /* Don't forget to cleanup everything */
    if (blkdev->gdisk) {
        del_gendisk(blkdev->gdisk);
        put_disk(blkdev->gdisk);
    }

    if (blkdev->queue) {
        blk_cleanup_queue(blkdev->queue);
    }

    kfree(blkdev->data);

    unregister_blkdev(major, c_p_blkdev_name);
    kfree(blkdev);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sae");
MODULE_DESCRIPTION("blkdev skeleton");
MODULE_VERSION("0.01");

module_init(blkdev_init);
module_exit(blkdev_exit);