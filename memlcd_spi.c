/*
 *
 *
 *
 *
 */

#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/spi/spi.h>
#include "memlcd_spi.h"

static struct fb_fix_screeninfo ls027b7dh01_fix = {
	.id = "ls027b7h01",
	.type = FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_MONO10,
	.xpanstep = 0,
	.ypanstep = 0,
	.ywrapstep = 0,
	.line_length = LS027B7DH01_LINE_LEN,
	.accel = FB_ACCEL_NONE,
};

static struct fb_var_screeninfo ls027b7dh01_var = {
        .xres	= LS027B7DH01_WIDTH,
	.yres	= LS027B7DH01_HEIGHT,
	.xres_virtual	= LS027B7DH01_WIDTH,
	.yres_virtual	= LS027B7DH01_HEIGHT,
	.bits_per_pixel = 1,
	.red = { 0, 1, 0 },
	.green = { 0, 1, 0 },
	.blue = { 0, 1, 0 },
	.left_margin = 0,
	.right_margin = 0,
	.upper_margin = 0,
	.lower_margin = 0,
	.vmode = FB_VMODE_NONINTERLACED,
};

static int ls027b7dh01_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	return vm_insert_page(vma, vma->vm_start,
	virt_to_page(info->screen_base));
}

/* Mark dispaly region invalid and need for update */
static void ls027b7dh01_invalidate(struct fb_info *info,
				    int x, int y, int w, int h)
{
	memlcd_priv		*priv = info->par;
	struct spi_device	*spi = priv->spi;
	dev_info(&spi->dev,"Invalidate region from (%d,%d) to (%d,%d)",
		x, y, x+w, y+h);
	schedule_delayed_work(&info->deferred_work, info->fbdefio->delay);
}

static void ls027b7dh01_fillrect(struct fb_info *p, const struct fb_fillrect *rect)
{
	sys_fillrect(p, rect);
	ls027b7dh01_invalidate(p, rect->dx, rect->dy, rect->width, rect->height);
}

static void ls027b7dh01_imageblit(struct fb_info *p, const struct fb_image *image)
{
	sys_imageblit(p, image);
	ls027b7dh01_invalidate(p, image->dx, image->dy, image->width, image->height);
}

static void ls027b7dh01_copyarea(struct fb_info *p, const struct fb_copyarea *area)
{
	sys_copyarea(p, area);
	ls027b7dh01_invalidate(p, area->dx, area->dy, area->width, area->height);
}

static ssize_t ls027b7dh01_write(struct fb_info *p, const char __user *buf, 
				    size_t count, loff_t *ppos)
{
	int retval;
	int begin = *ppos;
	int end = *ppos + count;

	retval = fb_sys_write(p, buf, count, ppos);
	ls027b7dh01_invalidate(p, 
		begin % LS027B7DH01_LINE_LEN, begin / LS027B7DH01_LINE_LEN, 
		end % LS027B7DH01_LINE_LEN, end / LS027B7DH01_LINE_LEN);
	return retval;
}

static struct fb_ops ls027b7dh01_ops = {
        .owner = THIS_MODULE,
	.fb_read = fb_sys_read, /* use system read function */
        .fb_write = ls027b7dh01_write,
	.fb_fillrect = ls027b7dh01_fillrect,
        .fb_copyarea = ls027b7dh01_copyarea,
	.fb_imageblit = ls027b7dh01_imageblit,
	.fb_mmap = ls027b7dh01_mmap,
};

static void ls027b7dh01_update(struct fb_info *info, struct list_head *pagelist)
{
	memlcd_priv		*priv = info->par;
	struct spi_device	*spi = priv->spi;
	struct spi_message	m;
	struct spi_transfer	t;
	memlcd_line_update	line;
	int			i;

	for(i=0; i < LS027B7DH01_HEIGHT; i++) { /* for all lines */
		/* prepare data to transfer */
		line.cmd = 0x80; // FixMe: need mnemonic
		line.addr = i;
		memcpy(line.data, priv->video_memory + i * LS027B7DH01_LINE_LEN, LS027B7DH01_LINE_LEN);
		line.dummy[0] = 0;
		line.dummy[1] = 0;
		/* prepare transfer message */
		spi_message_init(&m);
		t.tx_buf = &line;
		t.len = sizeof(line);
		spi_message_add_tail(&t,&m);
		dev_info(&spi->dev,"Line %d prepared",i);
		/* send data to device */ 
		spi_sync(spi, &m);
		dev_info(&spi->dev,"Line %d sent",i);
	};
}

static struct fb_deferred_io ls027b7dh01_defio = {
	.delay	= HZ/6,
	.deferred_io = &ls027b7dh01_update,
};

static int memlcd_spi_probe(struct spi_device *spi)
{
	memlcd_priv	*priv = NULL;
	int ret = -EINVAL;

	/* allocate private data */
	priv = (memlcd_priv*)kzalloc(sizeof(memlcd_priv),GFP_KERNEL);
	if(priv == NULL) {
		dev_err(&spi->dev, "Allocate private data failed!");
		return -ENOMEM;
	}
	dev_set_drvdata(&spi->dev,priv);
	priv->spi = spi;

	/* configure spi */ 
	spi->bits_per_word = 8;
	spi->max_speed_hz = 2000000;
	ret = spi_setup(spi);
	if (ret < 0) {
		dev_err(&spi->dev, "Configure spi host phase failed!");
		goto free_priv;
	} 

	/* allocate framebuffer */
	priv->info = framebuffer_alloc(0, &spi->dev);
	if(priv->info == NULL) {
		dev_err(&spi->dev, "Allocate framebuffer failed!");
		ret = -ENOMEM;
		goto free_priv;
	}

	/* allocate video memory */
	priv->video_memory = kzalloc(LS027B7DH01_SCREEN_SIZE, GFP_KERNEL);
	if(priv->video_memory == NULL) {
		dev_err(&spi->dev, "Allocate video memory failed!");
		ret = -ENOMEM;
		goto free_fb;
	}

	/* configure framebuffer device */
	priv->info->par = priv;
	priv->info->screen_base = (char __iomem *)priv->video_memory;
	priv->info->screen_size = LS027B7DH01_SCREEN_SIZE;
	priv->info->fbops = &ls027b7dh01_ops;
	priv->info->fix = ls027b7dh01_fix;
	priv->info->var = ls027b7dh01_var;
	priv->info->fbdefio = &ls027b7dh01_defio;
	priv->info->pseudo_palette = NULL;
	priv->info->flags = FBINFO_FLAG_DEFAULT;

	/* init deferred io structure */
	fb_deferred_io_init(priv->info);

	/* register framebuffer */
	ret = register_framebuffer(priv->info);
	if(ret < 0) {
		dev_err(&spi->dev,"Register framebuffer failed!");
		goto free_fb;
	}
	dev_info(&spi->dev,"Framebuffer device registered successfully!");
	return 0;
free_fb:
	kfree(priv->info);
free_priv:
	kfree(priv);
	return ret;
}

static int memlcd_spi_remove(struct spi_device *spi)
{
	memlcd_priv	*priv = dev_get_drvdata(&spi->dev);

	/* cleanup defio */
	fb_deferred_io_cleanup(priv->info);

	/* remove framebuffer */
	if(priv->info) {
		dev_info(&spi->dev,"Free framebuffer data");
		unregister_framebuffer(priv->info);
		framebuffer_release(priv->info);
	}
	/* remove video memory */
	if(priv->video_memory) {
		dev_info(&spi->dev,"Free videomemory");
		kfree(priv->video_memory);
	}
	/* remove private data */
	dev_info(&spi->dev,"Free private data");
	kfree(priv);
	return 0;
}

static struct spi_driver memlcd_spi_driver = {
	.driver = {
		.name           = "memlcdfb-spi",
                .owner          = THIS_MODULE,
        },
        .probe          = memlcd_spi_probe,
        .remove         = memlcd_spi_remove,
};

static int __init memlcd_spi_init(void)
{
        int ret;

        ret = spi_register_driver(&memlcd_spi_driver);
        if (ret < 0) {
                printk(KERN_ERR "Failed to register memlcdfb-spi driver: %d", ret);
                goto out;
        }

out:
        return ret;
}

static void __exit memlcd_spi_exit(void)
{
        spi_unregister_driver(&memlcd_spi_driver);
}

module_init(memlcd_spi_init);
module_exit(memlcd_spi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alex A. Mihaylov <minimumlaw@rambler.ru>");
MODULE_ALIAS("spi:memlcdfb-spi");
MODULE_ALIAS("spi:sharpspifb");
