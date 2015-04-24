static const struct intel_device_info intel_i830_info = {
	.gen = 2, .is_mobile = 1, .cursor_needs_physical = 1,
	.has_overlay = 1, .overlay_needs_physical = 1,
};

static const struct intel_device_info intel_845g_info = {
	.gen = 2,
	.has_overlay = 1, .overlay_needs_physical = 1,
};

static const struct intel_device_info intel_i85x_info = {
	.gen = 2, .is_i85x = 1, .is_mobile = 1,
	.cursor_needs_physical = 1,
	.has_overlay = 1, .overlay_needs_physical = 1,
};

static const struct intel_device_info intel_i865g_info = {
	.gen = 2,
	.has_overlay = 1, .overlay_needs_physical = 1,
};

static const struct intel_device_info intel_i915g_info = {
	.gen = 3, .is_i915g = 1, .cursor_needs_physical = 1,
	.has_overlay = 1, .overlay_needs_physical = 1,
};
static const struct intel_device_info intel_i915gm_info = {
	.gen = 3, .is_mobile = 1,
	.cursor_needs_physical = 1,
	.has_overlay = 1, .overlay_needs_physical = 1,
	.supports_tv = 1,
};
static const struct intel_device_info intel_i945g_info = {
	.gen = 3, .has_hotplug = 1, .cursor_needs_physical = 1,
	.has_overlay = 1, .overlay_needs_physical = 1,
};
static const struct intel_device_info intel_i945gm_info = {
	.gen = 3, .is_i945gm = 1, .is_mobile = 1,
	.has_hotplug = 1, .cursor_needs_physical = 1,
	.has_overlay = 1, .overlay_needs_physical = 1,
	.supports_tv = 1,
};

static const struct intel_device_info intel_i965g_info = {
	.gen = 4, .is_broadwater = 1,
	.has_hotplug = 1,
	.has_overlay = 1,
};

static const struct intel_device_info intel_i965gm_info = {
	.gen = 4, .is_crestline = 1,
	.is_mobile = 1, .has_fbc = 1, .has_hotplug = 1,
	.has_overlay = 1,
	.supports_tv = 1,
};

static const struct intel_device_info intel_g33_info = {
	.gen = 3, .is_g33 = 1,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_overlay = 1,
};

static const struct intel_device_info intel_g45_info = {
	.gen = 4, .is_g4x = 1, .need_gfx_hws = 1,
	.has_pipe_cxsr = 1, .has_hotplug = 1,
	.has_bsd_ring = 1,
};

static const struct intel_device_info intel_gm45_info = {
	.gen = 4, .is_g4x = 1,
	.is_mobile = 1, .need_gfx_hws = 1, .has_fbc = 1,
	.has_pipe_cxsr = 1, .has_hotplug = 1,
	.supports_tv = 1,
	.has_bsd_ring = 1,
};

static const struct intel_device_info intel_pineview_info = {
	.gen = 3, .is_g33 = 1, .is_pineview = 1, .is_mobile = 1,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_overlay = 1,
};

static const struct intel_device_info intel_ironlake_d_info = {
	.gen = 5,
	.need_gfx_hws = 1, .has_pipe_cxsr = 1, .has_hotplug = 1,
	.has_bsd_ring = 1,
};

static const struct intel_device_info intel_ironlake_m_info = {
	.gen = 5, .is_mobile = 1,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_fbc = 1,
	.has_bsd_ring = 1,
};

static const struct intel_device_info intel_sandybridge_d_info = {
	.gen = 6,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_bsd_ring = 1,
	.has_blt_ring = 1,
};

static const struct intel_device_info intel_sandybridge_m_info = {
	.gen = 6, .is_mobile = 1,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_fbc = 1,
	.has_bsd_ring = 1,
	.has_blt_ring = 1,
};

static const struct intel_device_info intel_ivybridge_d_info = {
	.is_ivybridge = 1, .gen = 7,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_bsd_ring = 1,
	.has_blt_ring = 1,
};

static const struct intel_device_info intel_ivybridge_m_info = {
	.is_ivybridge = 1, .gen = 7, .is_mobile = 1,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_fbc = 0,	/* FBC is not enabled on Ivybridge mobile yet */
	.has_bsd_ring = 1,
	.has_blt_ring = 1,
};

static const struct intel_device_info other_info = {
	.is_ivybridge = 0, .gen = 0, .is_mobile = 0,
	.need_gfx_hws = 0, .has_hotplug = 0,
	.has_fbc = 0,	/* FBC is not enabled on Ivybridge mobile yet */
	.has_bsd_ring = 0,
	.has_blt_ring = 0,
};

const struct pci_device_id pciidlist[] = {		/* aka */
    OTHER_VGA_DEVICE(0x1002, 0x6779, &other_info),
	INTEL_VGA_DEVICE(0x3577, &intel_i830_info),		/* I830_M */
	INTEL_VGA_DEVICE(0x2562, &intel_845g_info),		/* 845_G */
	INTEL_VGA_DEVICE(0x3582, &intel_i85x_info),		/* I855_GM */
	INTEL_VGA_DEVICE(0x358e, &intel_i85x_info),
	INTEL_VGA_DEVICE(0x2572, &intel_i865g_info),		/* I865_G */
	INTEL_VGA_DEVICE(0x2582, &intel_i915g_info),		/* I915_G */
	INTEL_VGA_DEVICE(0x258a, &intel_i915g_info),		/* E7221_G */
	INTEL_VGA_DEVICE(0x2592, &intel_i915gm_info),		/* I915_GM */
	INTEL_VGA_DEVICE(0x2772, &intel_i945g_info),		/* I945_G */
	INTEL_VGA_DEVICE(0x27a2, &intel_i945gm_info),		/* I945_GM */
	INTEL_VGA_DEVICE(0x27ae, &intel_i945gm_info),		/* I945_GME */
	INTEL_VGA_DEVICE(0x2972, &intel_i965g_info),		/* I946_GZ */
	INTEL_VGA_DEVICE(0x2982, &intel_i965g_info),		/* G35_G */
	INTEL_VGA_DEVICE(0x2992, &intel_i965g_info),		/* I965_Q */
	INTEL_VGA_DEVICE(0x29a2, &intel_i965g_info),		/* I965_G */
	INTEL_VGA_DEVICE(0x29b2, &intel_g33_info),		/* Q35_G */
	INTEL_VGA_DEVICE(0x29c2, &intel_g33_info),		/* G33_G */
	INTEL_VGA_DEVICE(0x29d2, &intel_g33_info),		/* Q33_G */
	INTEL_VGA_DEVICE(0x2a02, &intel_i965gm_info),		/* I965_GM */
	INTEL_VGA_DEVICE(0x2a12, &intel_i965gm_info),		/* I965_GME */
	INTEL_VGA_DEVICE(0x2a42, &intel_gm45_info),		/* GM45_G */
	INTEL_VGA_DEVICE(0x2e02, &intel_g45_info),		/* IGD_E_G */
	INTEL_VGA_DEVICE(0x2e12, &intel_g45_info),		/* Q45_G */
	INTEL_VGA_DEVICE(0x2e22, &intel_g45_info),		/* G45_G */
	INTEL_VGA_DEVICE(0x2e32, &intel_g45_info),		/* G41_G */
	INTEL_VGA_DEVICE(0x2e42, &intel_g45_info),		/* B43_G */
	INTEL_VGA_DEVICE(0x2e92, &intel_g45_info),		/* B43_G.1 */
	INTEL_VGA_DEVICE(0xa001, &intel_pineview_info),
	INTEL_VGA_DEVICE(0xa011, &intel_pineview_info),
	INTEL_VGA_DEVICE(0x0042, &intel_ironlake_d_info),
	INTEL_VGA_DEVICE(0x0046, &intel_ironlake_m_info),
	INTEL_VGA_DEVICE(0x0102, &intel_sandybridge_d_info),
	INTEL_VGA_DEVICE(0x0112, &intel_sandybridge_d_info),
	INTEL_VGA_DEVICE(0x0122, &intel_sandybridge_d_info),
	INTEL_VGA_DEVICE(0x0106, &intel_sandybridge_m_info),
	INTEL_VGA_DEVICE(0x0116, &intel_sandybridge_m_info),
	INTEL_VGA_DEVICE(0x0126, &intel_sandybridge_m_info),
	INTEL_VGA_DEVICE(0x010A, &intel_sandybridge_d_info),
	INTEL_VGA_DEVICE(0x0156, &intel_ivybridge_m_info), /* GT1 mobile */
	INTEL_VGA_DEVICE(0x0166, &intel_ivybridge_m_info), /* GT2 mobile */
	INTEL_VGA_DEVICE(0x0152, &intel_ivybridge_d_info), /* GT1 desktop */
	INTEL_VGA_DEVICE(0x0162, &intel_ivybridge_d_info), /* GT2 desktop */
	INTEL_VGA_DEVICE(0x015a, &intel_ivybridge_d_info), /* GT1 server */
	{0, 0, 0, 0, 0, 0, 0}
};

const int pciidlistcnt = sizeof(pciidlist) / sizeof(struct pci_device_id);
