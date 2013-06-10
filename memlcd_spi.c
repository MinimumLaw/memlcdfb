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
	.visual = FB_VISUAL_MONO01,
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
	.red = {0,1,0},
	.green = {0,1,0},
	.blue = {0,1,0},
	.transp={0,0,0},
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
	dev_dbg(&spi->dev,"Invalidate region from (%d,%d) to (%d,%d)",
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

static const unsigned char msb2lsb[] = 
{
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

static void ls027b7dh01_update(struct fb_info *info, struct list_head *pagelist)
{
	memlcd_priv		*priv = info->par;
	struct spi_device	*spi = priv->spi;
	memlcd_line_update	line;
	int			i,j;

	for(i=0; i < LS027B7DH01_HEIGHT; i++) { /* for all lines */
		/* prepare data to transfer */
		line.cmd = 0x80; // FixMe: need mnemonic
		line.addr = msb2lsb[i];
		for(j=0;j<LS027B7DH01_SPI_LINE_LEN;j++)
			line.data[j] = msb2lsb[priv->video_memory[i * LS027B7DH01_LINE_LEN + j]];
		//memcpy(line.data, priv->video_memory + i * LS027B7DH01_LINE_LEN, LS027B7DH01_SPI_LINE_LEN);
		line.dummy[0] = 0;
		line.dummy[1] = 0;
		/* send data to device */
		spi_write(spi, &line, sizeof(line));
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
	priv->info->fix.smem_start = (unsigned long)priv->video_memory;
	priv->info->fix.smem_len = LS027B7DH01_SCREEN_SIZE;
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
