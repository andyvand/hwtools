/* cocci issues ;-( */
#ifndef VIDEO_H
#define VIDEO_H 1
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <pci/pci.h>
#include <sys/time.h>

#ifndef __APPLE__
#include <sys/io.h>
#include <linux/types.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>

#include "DirectHW.h"

#ifndef __u8
#define __u8 unsigned char
#endif

#ifndef __u16
#define __u16 unsigned short
#endif

#ifndef __u32
#define __u32 unsigned int
#endif

#ifndef __u64
#define __u64 unsigned long long
#endif

#ifndef __s8
#define __s8 char
#endif

#ifndef __s16
#define __s16 short
#endif

#ifndef __s32
#define __s32 long
#endif

#ifndef __s64
#define __s64 long long
#endif

extern void errx(int eval, const char *fmt, ...);
extern void udelay(int i);
#endif

/* stuff we can't get coccinelle to do yet */
#define __iomem
#define __read_mostly
#define __always_unused
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#define DRM_DEBUG_KMS printf
#define CONFIG_DRM_I915_KMS 1
#define module_init(x);
#define module_exit(x);

#define MODULE_AUTHOR(x)
#define MODULE_DESCRIPTION(x)
#define MODULE_LICENSE(a)
#define MODULE_DEVICE_TABLE(a, b)

/* constants that will never change from linux/vga.h */
/* Legacy VGA regions */
#define VGA_RSRC_NONE          0x00
#define VGA_RSRC_LEGACY_IO     0x01
#define VGA_RSRC_LEGACY_MEM    0x02
#define VGA_RSRC_LEGACY_MASK   (VGA_RSRC_LEGACY_IO | VGA_RSRC_LEGACY_MEM)
/* Non-legacy access */
#define VGA_RSRC_NORMAL_IO     0x04
#define VGA_RSRC_NORMAL_MEM    0x08


/* define in pci.h! */
#include <pci/pci.h>
/* idiocy. how many names to we need for a type? */
#ifdef __APPLE__
#define u8 __u8
#define u16 __u16
#define u32 __u32
#define u64 __u64
#else
typedef u32 uint32_t;
typedef u64 uint64_t;
#endif
/* WTF */
typedef int bool;
enum {false = 0, true};

/* we define our own. The kernel one is too full of stuff. */
struct mode_config {
	int num_fb;
	int num_connector;
	int num_crtc;
	int num_encoder;
	int min_width, min_height, max_width, max_height;
};

struct drm_device {
	struct pci_dev *pdev;
	u8 *bios_bin;
    int bios_bin_len;
	struct drm_i915_private *dev_private;
	struct mode_config mode_config;
};

/* we're willing to define our own here because it's relatively unchanging */
#define PCI_ANY_ID (~0)

struct pci_device_id {
        u32 vendor, device;           /* Vendor and device ID or PCI_ANY_ID*/
        u32 subvendor, subdevice;     /* Subsystem ID's or PCI_ANY_ID */
        u32 class, class_mask;        /* (class,subclass,prog-if) triplet */
        unsigned long driver_data;     /* Data private to the driver  */
};

#define DRIVERDATA_SET(x) (unsigned long)(x)

#define OTHER_VGA_DEVICE(ven, id, info) \
    { ven, id, PCI_ANY_ID, PCI_ANY_ID, 0x030000, 0xFF0000, DRIVERDATA_SET(info) }

#define INTEL_VGA_DEVICE(id, info) \
    { 0x8086, id, PCI_ANY_ID, PCI_ANY_ID, 0x030000, 0xFF0000, DRIVERDATA_SET(info) }

#define wait_for(condition, time) (sleep(1+time/50)  && (!condition))

/* random crap from kernel.h.
 * Kernel.h is a catch-all for all kinds of junk and it's
 * not worth using coccinelle (yet) to pull it apart. Maybe later.
 * And, yes, gcc still does not have nelem!
 */
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define __ALIGN_KERNEL(x, a)		__ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_KERNEL_MASK(x, mask)	(((x) + (mask)) & ~(mask))
#define ALIGN(x, a)		__ALIGN_KERNEL((x), (a))
#define __ALIGN_MASK(x, mask)	__ALIGN_KERNEL_MASK((x), (mask))
#define PTR_ALIGN(p, a)		((typeof(p))ALIGN((unsigned long)(p), (a)))
#define IS_ALIGNED(x, a)		(((x) & ((typeof(x))(a) - 1)) == 0)


/* temporary. */
extern void *dmi_check_system(unsigned long);

#include "final/drm_dp_helper.h"
#include "final/i915_reg.h"
#include "final/i915_drv.h"
#include "final/drm_mode.h"
#include "final/drm_crtc.h"

extern unsigned long io_I915_READ(unsigned long addr);
extern void io_I915_WRITE(unsigned long addr, unsigned long val);
extern u16 io_I915_READ16(unsigned long addr);
extern void io_I915_WRITE16(unsigned long addr, u16 val);

extern unsigned long I915_READ(unsigned long addr);
extern void I915_WRITE(unsigned long addr, unsigned long val);
extern u16 I915_READ16(unsigned long addr);
extern void I915_WRITE16(unsigned long addr, u16 val);
extern unsigned long msecs(void);
extern void mdelay(unsigned long ms);

/* these should be the same. */
#define POSTING_READ I915_READ
#define POSTING_READ16 I915_READ16

extern int pci_dev_find(struct drm_device *dev);
extern void *pci_map_rom(struct pci_dev *dev, size_t *size);
extern void *pci_unmap_rom(struct pci_dev *dev, void *p);
extern unsigned int i915_lvds_downclock;
extern int i915_vbt_sdvo_panel_type;
extern unsigned long lvds_do_not_use_alternate_frequency;
extern int find_idlist(struct drm_device *dev, u16 vendor, u16 device);
extern void pci_read_config_word(struct pci_dev *dev, unsigned long offset, u16 *val);
extern void pci_write_config_word(struct pci_dev *dev, unsigned long offset, u16 val);

#endif /* VIDEO_H */
