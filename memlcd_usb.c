/*
 *
 */

/* kernel modele (driver) specific */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
/* usb specific */
#include <linux/usb.h>
#include <linux/errno.h>
/* framebuffer specific */
#include <linux/workqueue.h>
#include <linux/mm.h>
#include <linux/fb.h>

#include "memlcd_usb.h"

/*
 * USB device driver specific constants and tables
 */
#define MEMLCD_VID	0xADCA
#define MEMLCD_PID	0x0001
#define MEMLCD_CLASS	0xFF
#define MEMLCD_SUBCLASS	0xFF
#define MEMLCD_PROTO	0xFF

static const struct usb_device_id memlcd_table[] = {
    {USB_DEVICE_AND_INTERFACE_INFO(
	MEMLCD_VID, 
	MEMLCD_PID,
	MEMLCD_CLASS,
	MEMLCD_SUBCLASS,
	MEMLCD_PROTO)},
    { }
};

MODULE_DEVICE_TABLE(usb, memlcd_table);

/*
 * Framebuffer specific constants and tables
 */
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
	struct memlcd_usb_dev	*dev = info->par;
	struct usb_interface	*interface = dev->iface;
	dev_dbg(&interface->dev,"Invalidate region from (%d,%d) to (%d,%d)",
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

static void usb_write_cb(struct urb *urb)
{
	struct memlcd_usb_dev	*dev = urb->context;
	struct usb_interface	*interface = dev->iface;

	if(urb->status) {
		switch(urb->status) {
		case -ENOENT:
			dev_err(&interface->dev,"Bulk write status report -ENOENT\n");
			break;
		case -ECONNRESET:
			dev_err(&interface->dev,"Bulk write status report -ECONNRESET\n");
			break;
		case -ESHUTDOWN:
			dev_err(&interface->dev,"Bulk write status report -ESHUTDOWN\n");
			break;
		default:
		    dev_err(&interface->dev,"Nonzero bulk write status (%d)\n", urb->status);
		}
	};

	usb_free_coherent(urb->dev, urb->transfer_buffer_length,
			urb->transfer_buffer, urb->transfer_dma);
}

static void ls027b7dh01_update(struct fb_info *info, struct list_head *pagelist)
{
	struct memlcd_usb_dev	*dev = info->par;
	struct usb_interface	*interface = dev->iface;
	struct urb *urb = NULL;
	char *buf;
	int retval,i;

	for(i=0; i < LS027B7DH01_HEIGHT; i++) { /* for all lines */
		urb = usb_alloc_urb(0, GFP_KERNEL);
		if(!urb) {
			dev_err(&interface->dev,"Failed to allocate URB!");
			return;
		}
		buf = usb_alloc_coherent(dev->udev, LS027B7DH01_LINE_LEN + 1, 
					GFP_KERNEL, &urb->transfer_dma);
		if(!buf) {
			dev_err(&interface->dev,"Failed to allocate URB data buffer!");
			goto buf_error;
		}

		buf[0] = i; // first is line number, next line data
		memcpy(buf+1, dev->video_memory + (i * LS027B7DH01_LINE_LEN), LS027B7DH01_LINE_LEN);

		// Check for disconnect
		if(!dev->iface) {
			dev_err(&interface->dev,"Transfer aborted - device disconnected!\n");
			goto buf_error;
		}

		usb_fill_bulk_urb(urb, dev->udev, usb_sndbulkpipe(dev->udev, dev->ep), 
				buf, LS027B7DH01_LINE_LEN + 1,
				usb_write_cb, dev);
		urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
		usb_anchor_urb(urb, &dev->submitted);

		retval = usb_submit_urb(urb, GFP_KERNEL);

		if(retval) {
			switch(retval) {
			case -ENOMEM:
				dev_err(&interface->dev,"Bulk write status report -ENOMEM\n");
				break;
			case -EBUSY:
				dev_err(&interface->dev,"Bulk write status report -EBUSY\n");
				break;
			case -ENODEV:
				dev_err(&interface->dev,"Bulk write status report -ENODEV\n");
				break;
			case -ENOENT:
				dev_err(&interface->dev,"Bulk write status report -ENOENT\n");
				break;
			case -ENXIO:
				dev_err(&interface->dev,"Bulk write status report -ENOXIO\n");
				break;
			case -EINVAL:
				dev_err(&interface->dev,"Bulk write status report -EINVAL\n");
				break;
			case -EXDEV:
				dev_err(&interface->dev,"Bulk write status report -EXDEV\n");
				break;
			case -EFBIG:
				dev_err(&interface->dev,"Bulk write status report -EFBIG\n");
				break;
			case -EPIPE:
				dev_err(&interface->dev,"Bulk write status report -EPIPE\n");
				break;
			case -EMSGSIZE:
				dev_err(&interface->dev,"Bulk write status report -EMSGSIZE\n");
				break;
			case -ENOSPC:
				dev_err(&interface->dev,"Bulk write status report -ENOSPC\n");
				break;
			case -ESHUTDOWN:
				dev_err(&interface->dev,"Bulk write status report -ESHUTDOWN\n");
				break;
			case -EPERM:
				dev_err(&interface->dev,"Bulk write status report -EPERM\n");
				break;
			case -EHOSTUNREACH:
				dev_err(&interface->dev,"Bulk write status report -EHOSTUNREACH\n");
				break;
			case -ENOEXEC:
				dev_err(&interface->dev,"Bulk write status report -ENOEXEC\n");
				break;
			default:
				dev_err(&interface->dev,"Failed to submit URB (Code %d)\n", retval);
			}
			goto anchor_error;
		}

		usb_free_urb(urb);
	}
	return;

	/* errors handle */
anchor_error:
	usb_unanchor_urb(urb);
buf_error:
	if(urb) {
		usb_free_coherent(dev->udev, LS027B7DH01_LINE_LEN + 1, buf, urb->transfer_dma);
		usb_free_urb(urb);
	};
	return;	
}

static struct fb_deferred_io ls027b7dh01_defio = {
	.delay	= HZ/10,
	.deferred_io = &ls027b7dh01_update,
};

/*
 * USB device service functions (probe, remove, etc...)
 */

static int memlcd_usb_probe(struct usb_interface *interface,
			    const struct usb_device_id *id)
{
	struct memlcd_usb_dev	*dev;
	struct usb_host_interface	*iface_descr;
	struct usb_endpoint_descriptor	*endpoint;
	int i;
	int retval = -ENOMEM;

	/* allocate private dev info */
	dev = kzalloc(sizeof(struct memlcd_usb_dev), GFP_KERNEL);
	if(!dev) {
		dev_err(&interface->dev, "Out of memory\n");
		goto usb_error;
	}

	/* Fill private info with USB device data */
	dev->udev = usb_get_dev(interface_to_usbdev(interface));
	dev->iface = interface;

	/* Find first out bilk endpoint */
	iface_descr = interface->cur_altsetting;
	for(i = 0; i < iface_descr->desc.bNumEndpoints; ++i) {
		endpoint = &iface_descr->endpoint[i].desc;

		if(!dev->ep && usb_endpoint_is_bulk_out(endpoint))
			dev->ep = endpoint->bEndpointAddress;
	}
	if(!dev->ep) {
		dev_err(&interface->dev, "No bulk out endpoint found!\n");
		goto usb_error;
	}

	init_usb_anchor(&dev->submitted);

	/* Ok, Now allocate framebuffer */
	dev->info = framebuffer_alloc(0, &interface->dev);
	if(dev->info == NULL) {
		dev_err(&interface->dev, "Allocate framebuffer failed!");
		goto usb_error;
	}
	/* allocate video memory */
	dev->video_memory = kzalloc(LS027B7DH01_SCREEN_SIZE, GFP_KERNEL);
	if(dev->video_memory == NULL) {
		dev_err(&interface->dev, "Allocate video memory failed!");
		goto fb_error;
	}

	/* configure framebuffer device */
	dev->info->par = dev;
	dev->info->screen_base = (char __iomem *)dev->video_memory;
	dev->info->screen_size = LS027B7DH01_SCREEN_SIZE;
	dev->info->fbops = &ls027b7dh01_ops;
	dev->info->fix = ls027b7dh01_fix;
	dev->info->fix.smem_start = (unsigned long)dev->video_memory;
	dev->info->fix.smem_len = LS027B7DH01_SCREEN_SIZE;
	dev->info->var = ls027b7dh01_var;
	dev->info->fbdefio = &ls027b7dh01_defio;
	dev->info->pseudo_palette = NULL;
	dev->info->flags = FBINFO_FLAG_DEFAULT;

	/* init deferred io structure */
	fb_deferred_io_init(dev->info);

	/* register framebuffer */
	retval = register_framebuffer(dev->info);
	if(retval < 0) {
		dev_err(&interface->dev,"Register framebuffer failed!");
		goto fb_error;
	}

	/* Nice, let's exit success */
	dev_info(&interface->dev,
			"MemLcdFB usb device attached!\n");
	return 0;

    /* Errors handling */
fb_error:
	if(dev->video_memory)
		kfree(dev->video_memory);
	if(dev->info)
		framebuffer_release(dev->info);
usb_error:
	if(dev)
		kfree(dev);
	return retval;
}

static void memlcd_usb_disconnect(struct usb_interface *interface)
{
	struct memlcd_usb_dev	*dev;

	dev_info(&interface->dev,"Disconnecting MemLcdFB usb device\n");

	dev = usb_get_intfdata(interface);
	usb_set_intfdata(interface, NULL);

	/* deallocate USB device */
	if(dev) {
		/* cleanup defio */
		fb_deferred_io_cleanup(dev->info);

		/* remove framebuffer */
		if(dev->info) {
			dev_info(&interface->dev,"Free framebuffer data");
			unregister_framebuffer(dev->info);
			framebuffer_release(dev->info);
		}

		/* remove video memory */
		if(dev->video_memory) {
			dev_info(&interface->dev,"Free videomemory");
			kfree(dev->video_memory);
		}

		usb_kill_anchored_urbs(&dev->submitted);

		/* remove private data */
		kfree(dev);
	}

	dev_info(&interface->dev,
		"MemLcdFB usb device disconnected!\n");
}

static struct usb_driver memlcd_driver = {
	.name = "memlcd-usb",
	.probe = memlcd_usb_probe,
	.disconnect = memlcd_usb_disconnect,
	.id_table = memlcd_table,
	.supports_autosuspend = 1,
};

module_usb_driver(memlcd_driver);

MODULE_LICENSE("GPL");
