/* i915_drv.h -- Private header for the I915 driver -*- linux-c -*-
 */
/*
 *
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _I915_DRV_H_
#define _I915_DRV_H_

#include "i915_reg.h"
#include "intel_bios.h"
#include "intel_ringbuffer.h"

/* General customization:
 */

#define DRIVER_AUTHOR		"Tungsten Graphics, Inc."

#define DRIVER_NAME		"i915"
#define DRIVER_DESC		"Intel Graphics"
#define DRIVER_DATE		"20080730"

enum pipe {
	PIPE_A = 0,
	PIPE_B,
	PIPE_C,
	I915_MAX_PIPES
};
#define pipe_name(p) ((p) + 'A')

enum plane {
	PLANE_A = 0,
	PLANE_B,
	PLANE_C,
};
#define plane_name(p) ((p) + 'A')

#define I915_GEM_GPU_DOMAINS	(~(I915_GEM_DOMAIN_CPU | I915_GEM_DOMAIN_GTT))

#define for_each_pipe(p) for ((p) = 0; (p) < dev_priv->num_pipe; (p)++)

/* Interface history:
 *
 * 1.1: Original.
 * 1.2: Add Power Management
 * 1.3: Add vblank support
 * 1.4: Fix cmdbuffer path, add heap destroy
 * 1.5: Add vblank pipe configuration
 * 1.6: - New ioctl for scheduling buffer swaps on vertical blank
 *      - Support vertical blank on secondary display pipe
 */
#define DRIVER_MAJOR		1
#define DRIVER_MINOR		6
#define DRIVER_PATCHLEVEL	0

#define WATCH_COHERENCY	0
#define WATCH_LISTS	0

#define I915_GEM_PHYS_CURSOR_0 1
#define I915_GEM_PHYS_CURSOR_1 2
#define I915_GEM_PHYS_OVERLAY_REGS 3
#define I915_MAX_PHYS_OBJECT (I915_GEM_PHYS_OVERLAY_REGS)

struct opregion_header;
struct opregion_acpi;
struct opregion_swsci;
struct opregion_asle;

struct intel_opregion {
	struct opregion_header *header;
	struct opregion_acpi *acpi;
	struct opregion_swsci *swsci;
	struct opregion_asle *asle;
	void *vbt;
	u32 __iomem *lid_state;
};
#define OPREGION_SIZE            (8*1024)

struct intel_overlay;
struct intel_overlay_error_state;
#define I915_FENCE_REG_NONE -1

struct sdvo_device_mapping {
	u8 initialized;
	u8 dvo_port;
	u8 slave_addr;
	u8 dvo_wiring;
	u8 i2c_pin;
	u8 i2c_speed;
	u8 ddc_pin;
};

struct intel_display_error_state;

struct intel_device_info {
	u8 gen;
	u8 is_mobile : 1;
	u8 is_i85x : 1;
	u8 is_i915g : 1;
	u8 is_i945gm : 1;
	u8 is_g33 : 1;
	u8 need_gfx_hws : 1;
	u8 is_g4x : 1;
	u8 is_pineview : 1;
	u8 is_broadwater : 1;
	u8 is_crestline : 1;
	u8 is_ivybridge : 1;
	u8 has_fbc : 1;
	u8 has_pipe_cxsr : 1;
	u8 has_hotplug : 1;
	u8 cursor_needs_physical : 1;
	u8 has_overlay : 1;
	u8 overlay_needs_physical : 1;
	u8 supports_tv : 1;
	u8 has_bsd_ring : 1;
	u8 has_blt_ring : 1;
};

enum no_fbc_reason {
	FBC_NO_OUTPUT, /* no outputs enabled to compress */
	FBC_STOLEN_TOO_SMALL, /* not enough space to hold compressed buffers */
	FBC_UNSUPPORTED_MODE, /* interlace or doublescanned mode */
	FBC_MODE_TOO_LARGE, /* mode too large for compression */
	FBC_BAD_PLANE, /* fbc not supported on plane */
	FBC_NOT_TILED, /* buffer not tiled */
	FBC_MULTIPLE_PIPES, /* more than one pipe active */
	FBC_MODULE_PARAM,
};

enum intel_pch {
	PCH_IBX,	/* Ibexpeak PCH */
	PCH_CPT,	/* Cougarpoint PCH */
};

#define QUIRK_PIPEA_FORCE (1<<0)
#define QUIRK_LVDS_SSC_DISABLE (1<<1)

struct intel_fbdev;

typedef struct drm_i915_private {
	struct drm_device *dev;

	const struct intel_device_info *info;

	int has_gem;
	int relative_constants_mode;

	void __iomem *regs;


	struct pci_dev *bridge_dev;
	uint32_t next_seqno;

	uint32_t counter;

	unsigned int cpp;
	int back_offset;
	int front_offset;
	int current_page;
	int page_flipping;
	/** Cached value of IMR to avoid reads in updating the bitfield */
	u32 pipestat[2];
	u32 irq_mask;
	u32 gt_irq_mask;
	u32 pch_irq_mask;

	u32 hotplug_supported_mask;

	int tex_lru_log_granularity;
	int allow_batchbuffer;
	struct mem_block *agp_heap;
	unsigned int sr01, adpa, ppcr, dvob, dvoc, lvds;
	int vblank_pipe;
	int num_pipe;

	/* For hangcheck timer */
#define DRM_I915_HANGCHECK_PERIOD 1500 /* in ms */
	int hangcheck_count;
	uint32_t last_acthd;
	uint32_t last_instdone;
	uint32_t last_instdone1;

	unsigned long cfb_size;
	unsigned long cfb_pitch;
	unsigned long cfb_offset;
	int cfb_fence;
	int cfb_plane;
	int cfb_y;

	struct intel_opregion opregion;

	/* overlay */
	struct intel_overlay *overlay;

	/* LVDS info */
	int backlight_level;  /* restore backlight to this value */
	bool backlight_enabled;
	struct drm_display_mode *panel_fixed_mode;
	struct drm_display_mode *lfp_lvds_vbt_mode; /* if any */
	struct drm_display_mode *sdvo_lvds_vbt_mode; /* if any */

	/* Feature bits from the VBIOS */
	unsigned int int_tv_support:1;
	unsigned int lvds_dither:1;
	unsigned int lvds_vbt:1;
	unsigned int int_crt_support:1;
	unsigned int lvds_use_ssc:1;
	int lvds_ssc_freq;
	struct {
		int rate;
		int lanes;
		int preemphasis;
		int vswing;

		bool initialized;
		bool support;
		int bpp;
		struct edp_power_seq pps;
	} edp;
	bool no_aux_handshake;


	int crt_ddc_pin;/* assume 965 */
	int fence_reg_start; /* 4 if userland hasn't ioctl'd us yet */
	int num_fence_regs; /* 8 on pre-965, 16 otherwise */

	unsigned int fsb_freq, mem_freq, is_ddr3;

	struct drm_i915_error_state *first_error;
	struct workqueue_struct *wq;

	/* Display functions */

	/* PCH chipset type */
	enum intel_pch pch_type;

	unsigned long quirks;

	/* Register state */
	bool modeset_on_lid;
	u8 saveLBB;
	u32 saveDSPACNTR;
	u32 saveDSPBCNTR;
	u32 saveDSPARB;
	u32 saveHWS;
	u32 savePIPEACONF;
	u32 savePIPEBCONF;
	u32 savePIPEASRC;
	u32 savePIPEBSRC;
	u32 saveFPA0;
	u32 saveFPA1;
	u32 saveDPLL_A;
	u32 saveDPLL_A_MD;
	u32 saveHTOTAL_A;
	u32 saveHBLANK_A;
	u32 saveHSYNC_A;
	u32 saveVTOTAL_A;
	u32 saveVBLANK_A;
	u32 saveVSYNC_A;
	u32 saveBCLRPAT_A;
	u32 saveTRANSACONF;
	u32 saveTRANS_HTOTAL_A;
	u32 saveTRANS_HBLANK_A;
	u32 saveTRANS_HSYNC_A;
	u32 saveTRANS_VTOTAL_A;
	u32 saveTRANS_VBLANK_A;
	u32 saveTRANS_VSYNC_A;
	u32 savePIPEASTAT;
	u32 saveDSPASTRIDE;
	u32 saveDSPASIZE;
	u32 saveDSPAPOS;
	u32 saveDSPAADDR;
	u32 saveDSPASURF;
	u32 saveDSPATILEOFF;
	u32 savePFIT_PGM_RATIOS;
	u32 saveBLC_HIST_CTL;
	u32 saveBLC_PWM_CTL;
	u32 saveBLC_PWM_CTL2;
	u32 saveBLC_CPU_PWM_CTL;
	u32 saveBLC_CPU_PWM_CTL2;
	u32 saveFPB0;
	u32 saveFPB1;
	u32 saveDPLL_B;
	u32 saveDPLL_B_MD;
	u32 saveHTOTAL_B;
	u32 saveHBLANK_B;
	u32 saveHSYNC_B;
	u32 saveVTOTAL_B;
	u32 saveVBLANK_B;
	u32 saveVSYNC_B;
	u32 saveBCLRPAT_B;
	u32 saveTRANSBCONF;
	u32 saveTRANS_HTOTAL_B;
	u32 saveTRANS_HBLANK_B;
	u32 saveTRANS_HSYNC_B;
	u32 saveTRANS_VTOTAL_B;
	u32 saveTRANS_VBLANK_B;
	u32 saveTRANS_VSYNC_B;
	u32 savePIPEBSTAT;
	u32 saveDSPBSTRIDE;
	u32 saveDSPBSIZE;
	u32 saveDSPBPOS;
	u32 saveDSPBADDR;
	u32 saveDSPBSURF;
	u32 saveDSPBTILEOFF;
	u32 saveVGA0;
	u32 saveVGA1;
	u32 saveVGA_PD;
	u32 saveVGACNTRL;
	u32 saveADPA;
	u32 saveLVDS;
	u32 savePP_ON_DELAYS;
	u32 savePP_OFF_DELAYS;
	u32 saveDVOA;
	u32 saveDVOB;
	u32 saveDVOC;
	u32 savePP_ON;
	u32 savePP_OFF;
	u32 savePP_CONTROL;
	u32 savePP_DIVISOR;
	u32 savePFIT_CONTROL;
	u32 save_palette_a[256];
	u32 save_palette_b[256];
	u32 saveDPFC_CB_BASE;
	u32 saveFBC_CFB_BASE;
	u32 saveFBC_LL_BASE;
	u32 saveFBC_CONTROL;
	u32 saveFBC_CONTROL2;
	u32 saveIER;
	u32 saveIIR;
	u32 saveIMR;
	u32 saveDEIER;
	u32 saveDEIMR;
	u32 saveGTIER;
	u32 saveGTIMR;
	u32 saveFDI_RXA_IMR;
	u32 saveFDI_RXB_IMR;
	u32 saveCACHE_MODE_0;
	u32 saveMI_ARB_STATE;
	u32 saveSWF0[16];
	u32 saveSWF1[16];
	u32 saveSWF2[3];
	u8 saveMSR;
	u8 saveSR[8];
	u8 saveGR[25];
	u8 saveAR_INDEX;
	u8 saveAR[21];
	u8 saveDACMASK;
	u8 saveCR[37];
	uint64_t saveFENCE[16];
	u32 saveCURACNTR;
	u32 saveCURAPOS;
	u32 saveCURABASE;
	u32 saveCURBCNTR;
	u32 saveCURBPOS;
	u32 saveCURBBASE;
	u32 saveCURSIZE;
	u32 saveDP_B;
	u32 saveDP_C;
	u32 saveDP_D;
	u32 savePIPEA_GMCH_DATA_M;
	u32 savePIPEB_GMCH_DATA_M;
	u32 savePIPEA_GMCH_DATA_N;
	u32 savePIPEB_GMCH_DATA_N;
	u32 savePIPEA_DP_LINK_M;
	u32 savePIPEB_DP_LINK_M;
	u32 savePIPEA_DP_LINK_N;
	u32 savePIPEB_DP_LINK_N;
	u32 saveFDI_RXA_CTL;
	u32 saveFDI_TXA_CTL;
	u32 saveFDI_RXB_CTL;
	u32 saveFDI_TXB_CTL;
	u32 savePFA_CTL_1;
	u32 savePFB_CTL_1;
	u32 savePFA_WIN_SZ;
	u32 savePFB_WIN_SZ;
	u32 savePFA_WIN_POS;
	u32 savePFB_WIN_POS;
	u32 savePCH_DREF_CONTROL;
	u32 saveDISP_ARB_CTL;
	u32 savePIPEA_DATA_M1;
	u32 savePIPEA_DATA_N1;
	u32 savePIPEA_LINK_M1;
	u32 savePIPEA_LINK_N1;
	u32 savePIPEB_DATA_M1;
	u32 savePIPEB_DATA_N1;
	u32 savePIPEB_LINK_M1;
	u32 savePIPEB_LINK_N1;
	u32 saveMCHBAR_RENDER_STANDBY;
	u32 savePCH_PORT_HOTPLUG;

	struct sdvo_device_mapping sdvo_mappings[2];
	/* indicate whether the LVDS_BORDER should be enabled or not */
	unsigned int lvds_border_bits;
	/* Panel fitter placement and size for Ironlake+ */
	u32 pch_pf_pos, pch_pf_size;
	int panel_t3, panel_t12;

	struct drm_crtc *plane_to_crtc_mapping[2];
	struct drm_crtc *pipe_to_crtc_mapping[2];
	bool flip_pending_is_done;

	/* Reclocking support */
	bool render_reclock_avail;
	bool lvds_downclock_avail;
	/* indicates the reduced downclock for LVDS*/
	int lvds_downclock;
	bool busy;
	u16 orig_clock;
	int child_dev_num;
	struct child_device_config *child_dev;
	struct drm_connector *int_lvds_connector;

	bool mchbar_need_disable;

	u32 pm_iir;

	u8 cur_delay;
	u8 min_delay;
	u8 max_delay;
	u8 fmax;
	u8 fstart;

	u64 last_count1;
	unsigned long last_time1;
	u64 last_count2;
	struct timespec last_time2;
	unsigned long gfx_power;
	int c_m;
	int r_t;
	u8 corr;

	enum no_fbc_reason no_fbc_reason;

	struct drm_mm_node *compressed_fb;
	struct drm_mm_node *compressed_llb;

	unsigned long last_gpu_reset;

	/* list of fbdev register on this device */
	struct intel_fbdev *fbdev;

	struct drm_property *broadcast_rgb_property;
	struct drm_property *force_audio_property;
} drm_i915_private_t;

enum i915_cache_level {
	I915_CACHE_NONE,
	I915_CACHE_LLC,
	I915_CACHE_LLC_MLC, /* gen6+ */
};

#define to_intel_bo(x) container_of(x, struct drm_i915_gem_object, base)

#define INTEL_INFO(dev)	(((struct drm_i915_private *) (dev)->dev_private)->info)

#define IS_I830(dev)		((dev)->pci_device == 0x3577)
#define IS_845G(dev)		((dev)->pci_device == 0x2562)
#define IS_I85X(dev)		(INTEL_INFO(dev)->is_i85x)
#define IS_I865G(dev)		((dev)->pci_device == 0x2572)
#define IS_I915G(dev)		(INTEL_INFO(dev)->is_i915g)
#define IS_I915GM(dev)		((dev)->pci_device == 0x2592)
#define IS_I945G(dev)		((dev)->pci_device == 0x2772)
#define IS_I945GM(dev)		(INTEL_INFO(dev)->is_i945gm)
#define IS_BROADWATER(dev)	(INTEL_INFO(dev)->is_broadwater)
#define IS_CRESTLINE(dev)	(INTEL_INFO(dev)->is_crestline)
#define IS_GM45(dev)		((dev)->pci_device == 0x2A42)
#define IS_G4X(dev)		(INTEL_INFO(dev)->is_g4x)
#define IS_PINEVIEW_G(dev)	((dev)->pci_device == 0xa001)
#define IS_PINEVIEW_M(dev)	((dev)->pci_device == 0xa011)
#define IS_PINEVIEW(dev)	(INTEL_INFO(dev)->is_pineview)
#define IS_G33(dev)		(INTEL_INFO(dev)->is_g33)
#define IS_IRONLAKE_D(dev)	((dev)->pci_device == 0x0042)
#define IS_IRONLAKE_M(dev)	((dev)->pci_device == 0x0046)
#define IS_IVYBRIDGE(dev)	(INTEL_INFO(dev)->is_ivybridge)
#define IS_MOBILE(dev)		(INTEL_INFO(dev)->is_mobile)

/*
 * The genX designation typically refers to the render engine, so render
 * capability related checks should use IS_GEN, while display and other checks
 * have their own (e.g. HAS_PCH_SPLIT for ILK+ display, IS_foo for particular
 * chips, etc.).
 */
#define IS_GEN2(dev)	(INTEL_INFO(dev)->gen == 2)
#define IS_GEN3(dev)	(INTEL_INFO(dev)->gen == 3)
#define IS_GEN4(dev)	(INTEL_INFO(dev)->gen == 4)
#define IS_GEN5(dev)	(INTEL_INFO(dev)->gen == 5)
#define IS_GEN6(dev)	(INTEL_INFO(dev)->gen == 6)
#define IS_GEN7(dev)	(INTEL_INFO(dev)->gen == 7)

#define HAS_BSD(dev)            (INTEL_INFO(dev)->has_bsd_ring)
#define HAS_BLT(dev)            (INTEL_INFO(dev)->has_blt_ring)
#define I915_NEED_GFX_HWS(dev)	(INTEL_INFO(dev)->need_gfx_hws)

#define HAS_OVERLAY(dev)		(INTEL_INFO(dev)->has_overlay)
#define OVERLAY_NEEDS_PHYSICAL(dev)	(INTEL_INFO(dev)->overlay_needs_physical)

/* With the 945 and later, Y tiling got adjusted so that it was 32 128-byte
 * rows, which changed the alignment requirements and fence programming.
 */
#define HAS_128_BYTE_Y_TILING(dev) (!IS_GEN2(dev) && !(IS_I915G(dev) || \
						      IS_I915GM(dev)))
#define SUPPORTS_DIGITAL_OUTPUTS(dev)	(!IS_GEN2(dev) && !IS_PINEVIEW(dev))
#define SUPPORTS_INTEGRATED_HDMI(dev)	(IS_G4X(dev) || IS_GEN5(dev))
#define SUPPORTS_INTEGRATED_DP(dev)	(IS_G4X(dev) || IS_GEN5(dev))
#define SUPPORTS_EDP(dev)		(IS_IRONLAKE_M(dev))
#define SUPPORTS_TV(dev)		(INTEL_INFO(dev)->supports_tv)
#define I915_HAS_HOTPLUG(dev)		 (INTEL_INFO(dev)->has_hotplug)
/* dsparb controlled by hw only */
#define DSPARB_HWCONTROL(dev) (IS_G4X(dev) || IS_IRONLAKE(dev))

#define HAS_FW_BLC(dev) (INTEL_INFO(dev)->gen > 2)
#define HAS_PIPE_CXSR(dev) (INTEL_INFO(dev)->has_pipe_cxsr)
#define I915_HAS_FBC(dev) (INTEL_INFO(dev)->has_fbc)

#define HAS_PCH_SPLIT(dev) (IS_GEN5(dev) || IS_GEN6(dev) || IS_IVYBRIDGE(dev))
#define HAS_PIPE_CONTROL(dev) (INTEL_INFO(dev)->gen >= 5)

#define INTEL_PCH_TYPE(dev) (((struct drm_i915_private *)(dev)->dev_private)->pch_type)
#define HAS_PCH_CPT(dev) (INTEL_PCH_TYPE(dev) == PCH_CPT)
#define HAS_PCH_IBX(dev) (INTEL_PCH_TYPE(dev) == PCH_IBX)



#endif
