/*
 *
 *
 *
 *
 */

#include <linux/module.h>
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

static struct fb_ops ls027b7dh01_ops = {
        .owner = THIS_MODULE,
	.fb_read = fb_sys_read,
        .fb_write = fb_sys_write,
	.fb_fillrect = sys_fillrect,
        .fb_copyarea = sys_copyarea,
	.fb_imageblit = sys_imageblit,
	.fb_mmap = ls027b7dh01_mmap,
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
	priv->info->screen_base = (char __iomem *)priv->video_memory;
	priv->info->screen_size = LS027B7DH01_SCREEN_SIZE;
	priv->info->fbops = &ls027b7dh01_ops;
	priv->info->fix = ls027b7dh01_fix;
	priv->info->var = ls027b7dh01_var;
	priv->info->pseudo_palette = NULL;
	priv->info->par = NULL;
	priv->info->flags = FBINFO_FLAG_DEFAULT;

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
