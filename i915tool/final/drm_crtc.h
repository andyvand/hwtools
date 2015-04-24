/*
 * Copyright © 2006 Keith Packard
 * Copyright © 2007-2008 Dave Airlie
 * Copyright © 2007-2008 Intel Corporation
 *   Jesse Barnes <jesse.barnes@intel.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __DRM_CRTC_H__
#define __DRM_CRTC_H__

struct drm_device;
struct drm_mode_set;
struct drm_framebuffer;


#define DRM_MODE_OBJECT_CRTC 0xcccccccc
#define DRM_MODE_OBJECT_CONNECTOR 0xc0c0c0c0
#define DRM_MODE_OBJECT_ENCODER 0xe0e0e0e0
#define DRM_MODE_OBJECT_MODE 0xdededede
#define DRM_MODE_OBJECT_PROPERTY 0xb0b0b0b0
#define DRM_MODE_OBJECT_FB 0xfbfbfbfb
#define DRM_MODE_OBJECT_BLOB 0xbbbbbbbb

struct drm_mode_object {
	uint32_t id;
	uint32_t type;
};

/*
 * Note on terminology:  here, for brevity and convenience, we refer to connector
 * control chips as 'CRTCs'.  They can control any type of connector, VGA, LVDS,
 * DVI, etc.  And 'screen' refers to the whole of the visible display, which
 * may span multiple monitors (and therefore multiple CRTC and connector
 * structures).
 */

enum drm_mode_status {
    MODE_OK	= 0,	/* Mode OK */
    MODE_HSYNC,		/* hsync out of range */
    MODE_VSYNC,		/* vsync out of range */
    MODE_H_ILLEGAL,	/* mode has illegal horizontal timings */
    MODE_V_ILLEGAL,	/* mode has illegal horizontal timings */
    MODE_BAD_WIDTH,	/* requires an unsupported linepitch */
    MODE_NOMODE,	/* no mode with a matching name */
    MODE_NO_INTERLACE,	/* interlaced mode not supported */
    MODE_NO_DBLESCAN,	/* doublescan mode not supported */
    MODE_NO_VSCAN,	/* multiscan mode not supported */
    MODE_MEM,		/* insufficient video memory */
    MODE_VIRTUAL_X,	/* mode width too large for specified virtual size */
    MODE_VIRTUAL_Y,	/* mode height too large for specified virtual size */
    MODE_MEM_VIRT,	/* insufficient video memory given virtual size */
    MODE_NOCLOCK,	/* no fixed clock available */
    MODE_CLOCK_HIGH,	/* clock required is too high */
    MODE_CLOCK_LOW,	/* clock required is too low */
    MODE_CLOCK_RANGE,	/* clock/mode isn't in a ClockRange */
    MODE_BAD_HVALUE,	/* horizontal timing was out of range */
    MODE_BAD_VVALUE,	/* vertical timing was out of range */
    MODE_BAD_VSCAN,	/* VScan value out of range */
    MODE_HSYNC_NARROW,	/* horizontal sync too narrow */
    MODE_HSYNC_WIDE,	/* horizontal sync too wide */
    MODE_HBLANK_NARROW,	/* horizontal blanking too narrow */
    MODE_HBLANK_WIDE,	/* horizontal blanking too wide */
    MODE_VSYNC_NARROW,	/* vertical sync too narrow */
    MODE_VSYNC_WIDE,	/* vertical sync too wide */
    MODE_VBLANK_NARROW,	/* vertical blanking too narrow */
    MODE_VBLANK_WIDE,	/* vertical blanking too wide */
    MODE_PANEL,         /* exceeds panel dimensions */
    MODE_INTERLACE_WIDTH, /* width too large for interlaced mode */
    MODE_ONE_WIDTH,     /* only one width is supported */
    MODE_ONE_HEIGHT,    /* only one height is supported */
    MODE_ONE_SIZE,      /* only one resolution is supported */
    MODE_NO_REDUCED,    /* monitor doesn't accept reduced blanking */
    MODE_UNVERIFIED = -3, /* mode needs to reverified */
    MODE_BAD = -2,	/* unspecified reason */
    MODE_ERROR	= -1	/* error condition */
};

#define DRM_MODE_TYPE_CLOCK_CRTC_C (DRM_MODE_TYPE_CLOCK_C | \
				    DRM_MODE_TYPE_CRTC_C)

#define DRM_MODE(nm, t, c, hd, hss, hse, ht, hsk, vd, vss, vse, vt, vs, f) \
	.name = nm, .status = 0, .type = (t), .clock = (c), \
	.hdisplay = (hd), .hsync_start = (hss), .hsync_end = (hse), \
	.htotal = (ht), .hskew = (hsk), .vdisplay = (vd), \
	.vsync_start = (vss), .vsync_end = (vse), .vtotal = (vt), \
	.vscan = (vs), .flags = (f), .vrefresh = 0

#define CRTC_INTERLACE_HALVE_V 0x1 /* halve V values for interlacing */

struct drm_display_mode {
	/* Header */
	struct drm_mode_object base;

	char name[DRM_DISPLAY_MODE_LEN];

	int connector_count;
	enum drm_mode_status status;
	int type;

	/* Proposed mode values */
	int clock;		/* in kHz */
	int hdisplay;
	int hsync_start;
	int hsync_end;
	int htotal;
	int hskew;
	int vdisplay;
	int vsync_start;
	int vsync_end;
	int vtotal;
	int vscan;
	unsigned int flags;

	/* Addressable image size (may be 0 for projectors, etc.) */
	int width_mm;
	int height_mm;

	/* Actual mode we give to hw */
	int clock_index;
	int synth_clock;
	int crtc_hdisplay;
	int crtc_hblank_start;
	int crtc_hblank_end;
	int crtc_hsync_start;
	int crtc_hsync_end;
	int crtc_htotal;
	int crtc_hskew;
	int crtc_vdisplay;
	int crtc_vblank_start;
	int crtc_vblank_end;
	int crtc_vsync_start;
	int crtc_vsync_end;
	int crtc_vtotal;
	int crtc_hadjusted;
	int crtc_vadjusted;

	/* Driver private mode info */
	int private_size;
	int *private;
	int private_flags;

	int vrefresh;		/* in Hz */
	int hsync;		/* in kHz */
};

enum drm_connector_status {
	connector_status_connected = 1,
	connector_status_disconnected = 2,
	connector_status_unknown = 3,
};

enum subpixel_order {
	SubPixelUnknown = 0,
	SubPixelHorizontalRGB,
	SubPixelHorizontalBGR,
	SubPixelVerticalRGB,
	SubPixelVerticalBGR,
	SubPixelNone,
};

#define DRM_COLOR_FORMAT_RGB444		(1<<0)
#define DRM_COLOR_FORMAT_YCRCB444	(1<<1)
#define DRM_COLOR_FORMAT_YCRCB422	(1<<2)
/*
 * Describes a given display (e.g. CRT or flat panel) and its limitations.
 */
struct drm_display_info {
	char name[DRM_DISPLAY_INFO_LEN];

	/* Physical size */
        unsigned int width_mm;
	unsigned int height_mm;

	/* Clock limits FIXME: storage format */
	unsigned int min_vfreq, max_vfreq;
	unsigned int min_hfreq, max_hfreq;
	unsigned int pixel_clock;
	unsigned int bpc;

	enum subpixel_order subpixel_order;
	u32 color_formats;

	char *raw_edid; /* if any */
};

struct drm_framebuffer {
	struct drm_device *dev;
	struct drm_mode_object base;
	const struct drm_framebuffer_funcs *funcs;
	unsigned int pitch;
	unsigned int width;
	unsigned int height;
	/* depth can be 15 or 16 */
	unsigned int depth;
	int bits_per_pixel;
	int flags;
	/* if you are using the helper */
	void *helper_private;
};

struct drm_property_blob {
	struct drm_mode_object base;
	unsigned int length;
	void *data;
};

struct drm_property_enum {
	uint64_t value;
	char name[DRM_PROP_NAME_LEN];
};

struct drm_property {
	struct drm_mode_object base;
	uint32_t flags;
	char name[DRM_PROP_NAME_LEN];
	uint32_t num_values;
	uint64_t *values;
};

struct drm_crtc;
struct drm_connector;
struct drm_encoder;
struct drm_pending_vblank_event;

#define DRM_CONNECTOR_MAX_UMODES 16
#define DRM_CONNECTOR_MAX_PROPERTY 16
#define DRM_CONNECTOR_LEN 32
#define DRM_CONNECTOR_MAX_ENCODER 2

enum drm_connector_force {
	DRM_FORCE_UNSPECIFIED,
	DRM_FORCE_OFF,
	DRM_FORCE_ON,         /* force on analog part normally */
	DRM_FORCE_ON_DIGITAL, /* for DVI-I use digital connector */
};

/* should we poll this connector for connects and disconnects */
/* hot plug detectable */
#define DRM_CONNECTOR_POLL_HPD (1 << 0)
/* poll for connections */
#define DRM_CONNECTOR_POLL_CONNECT (1 << 1)
/* can cleanly poll for disconnections without flickering the screen */
/* DACs should rarely do this without a lot of testing */
#define DRM_CONNECTOR_POLL_DISCONNECT (1 << 2)

/**
 * struct drm_mode_set
 *
 * Represents a single crtc the connectors that it drives with what mode
 * and from which framebuffer it scans out from.
 *
 * This is used to set modes.
 */
struct drm_mode_set {
	struct drm_framebuffer *fb;
	struct drm_crtc *crtc;
	struct drm_display_mode *mode;

	uint32_t x;
	uint32_t y;

	struct drm_connector **connectors;
	size_t num_connectors;
};

struct drm_mode_group {
	uint32_t num_crtcs;
	uint32_t num_encoders;
	uint32_t num_connectors;

	/* list of object IDs for this group */
	uint32_t *id_list;
};

#define obj_to_crtc(x) container_of(x, struct drm_crtc, base)
#define obj_to_connector(x) container_of(x, struct drm_connector, base)
#define obj_to_encoder(x) container_of(x, struct drm_encoder, base)
#define obj_to_mode(x) container_of(x, struct drm_display_mode, base)
#define obj_to_fb(x) container_of(x, struct drm_framebuffer, base)
#define obj_to_property(x) container_of(x, struct drm_property, base)
#define obj_to_blob(x) container_of(x, struct drm_property_blob, base)


#endif /* __DRM_CRTC_H__ */
