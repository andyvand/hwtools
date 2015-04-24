/*
 * Copyright © 2006-2007 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *	Eric Anholt <eric@anholt.net>
 */

#include <linux/module.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vgaarb.h>
#include "drmP.h"
#include "intel_drv.h"
#include "i915_drm.h"
#include "i915_drv.h"
#include "i915_trace.h"
#include "drm_dp_helper.h"

#include "drm_crtc_helper.h"

#define HAS_eDP (intel_pipe_has_type(crtc, INTEL_OUTPUT_EDP))

typedef struct {
    /* given values */
    int n;
    int m1, m2;
    int p1, p2;
    /* derived values */
    int	dot;
    int	vco;
    int	m;
    int	p;
} intel_clock_t;

typedef struct {
    int	min, max;
} intel_range_t;

typedef struct {
    int	dot_limit;
    int	p2_slow, p2_fast;
} intel_p2_t;

#define INTEL_P2_NUM		      2
typedef struct intel_limit intel_limit_t;

/* FDI */
#define IRONLAKE_FDI_FREQ		2700000 /* in kHz for mode->clock */

static bool
intel_find_pll_g4x_dp( intel_limit_t *, struct drm_crtc *crtc,
		      int target, int refclk, intel_clock_t *best_clock);

static inline u32 /* units of 100MHz */
intel_fdi_link_freq(struct drm_device *dev)
{
	if (IS_GEN5(dev)) {
		struct drm_i915_private *dev_priv = dev->dev_private;
		return (I915_READ(FDI_PLL_BIOS_0) & FDI_PLL_FB_CLOCK_MASK) + 2;
	} else
		return 27;
}

#define INTELPllInvalid(s)   do { /* DRM_DEBUG(s); */ return false; } while (0)

/* DisplayPort has only two frequencies, 162MHz and 270MHz */
static bool
intel_find_pll_g4x_dp( intel_limit_t *limit, struct drm_crtc *crtc,
		      int target, int refclk, intel_clock_t *best_clock)
{
	intel_clock_t clock;
	if (target < 200000) {
		clock.p1 = 2;
		clock.p2 = 10;
		clock.n = 2;
		clock.m1 = 23;
		clock.m2 = 8;
	} else {
		clock.p1 = 1;
		clock.p2 = 10;
		clock.n = 1;
		clock.m1 = 14;
		clock.m2 = 2;
	}
	clock.m = 5 * (clock.m1 + 2) + (clock.m2 + 2);
	clock.p = (clock.p1 * clock.p2);
	clock.dot = 96000 * clock.m / (clock.n + 2) / clock.p;
	clock.vco = 0;
	memcpy(best_clock, &clock, sizeof(intel_clock_t));
	return true;
}

/**
 * intel_wait_for_vblank - wait for vblank on a given pipe
 * @dev: drm device
 * @pipe: pipe to wait for
 *
 * Wait for vblank to occur on a given pipe.  Needed for various bits of
 * mode setting code.
 */
void intel_wait_for_vblank(struct drm_device *dev, int pipe)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int pipestat_reg = PIPESTAT(pipe);

	/* Clear existing vblank status. Note this will clear any other
	 * sticky status fields as well.
	 *
	 * This races with i915_driver_irq_handler() with the result
	 * that either function could miss a vblank event.  Here it is not
	 * fatal, as we will either wait upon the next vblank interrupt or
	 * timeout.  Generally speaking intel_wait_for_vblank() is only
	 * called during modeset at which time the GPU should be idle and
	 * should *not* be performing page flips and thus not waiting on
	 * vblanks...
	 * Currently, the result of us stealing a vblank from the irq
	 * handler is that a single frame will be skipped during swapbuffers.
	 */
	I915_WRITE(pipestat_reg,
		   I915_READ(pipestat_reg) | PIPE_VBLANK_INTERRUPT_STATUS);

	/* Wait for vblank interrupt bit to set */
	if (wait_for(I915_READ(pipestat_reg) &
		     PIPE_VBLANK_INTERRUPT_STATUS,
		     50))
		DRM_DEBUG_KMS("vblank wait timed out\n");
}

/*
 * intel_wait_for_pipe_off - wait for pipe to turn off
 * @dev: drm device
 * @pipe: pipe to wait for
 *
 * After disabling a pipe, we can't wait for vblank in the usual way,
 * spinning on the vblank interrupt status bit, since we won't actually
 * see an interrupt when the pipe is disabled.
 *
 * On Gen4 and above:
 *   wait for the pipe register state bit to turn off
 *
 * Otherwise:
 *   wait for the display line value to settle (it usually
 *   ends up stopping at the start of the next frame).
 *
 */
void intel_wait_for_pipe_off(struct drm_device *dev, int pipe)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (INTEL_INFO(dev)->gen >= 4) {
		int reg = PIPECONF(pipe);

		/* Wait for the Pipe State to go off */
		if (wait_for((I915_READ(reg) & I965_PIPECONF_ACTIVE) == 0,
			     100))
			DRM_DEBUG_KMS("pipe_off wait timed out\n");
	} else {
		u32 last_line;
		int reg = PIPEDSL(pipe);
		unsigned long timeout = jiffies + msecs_to_jiffies(100);

		/* Wait for the display line to settle */
		do {
			last_line = I915_READ(reg) & DSL_LINEMASK;
			mdelay(5);
		} while (((I915_READ(reg) & DSL_LINEMASK) != last_line) &&
			 time_after(timeout, jiffies));
		if (time_after(jiffies, timeout))
			DRM_DEBUG_KMS("pipe_off wait timed out\n");
	}
}

static  char *state_string(bool enabled)
{
	return enabled ? "on" : "off";
}

/* Only for pre-ILK configs */
static void assert_pll(struct drm_i915_private *dev_priv,
		       enum pipe pipe, bool state)
{
	int reg;
	u32 val;
	bool cur_state;

	reg = DPLL(pipe);
	val = I915_READ(reg);
	cur_state = !!(val & DPLL_VCO_ENABLE);
	WARN(cur_state != state,
	     "PLL state assertion failure (expected %s, current %s)\n",
	     state_string(state), state_string(cur_state));
}
#define assert_pll_enabled(d, p) assert_pll(d, p, true)
#define assert_pll_disabled(d, p) assert_pll(d, p, false)

/* For ILK+ */
static void assert_pch_pll(struct drm_i915_private *dev_priv,
			   enum pipe pipe, bool state)
{
	int reg;
	u32 val;
	bool cur_state;

	reg = PCH_DPLL(pipe);
	val = I915_READ(reg);
	cur_state = !!(val & DPLL_VCO_ENABLE);
	WARN(cur_state != state,
	     "PCH PLL state assertion failure (expected %s, current %s)\n",
	     state_string(state), state_string(cur_state));
}
#define assert_pch_pll_enabled(d, p) assert_pch_pll(d, p, true)
#define assert_pch_pll_disabled(d, p) assert_pch_pll(d, p, false)

static void assert_fdi_tx(struct drm_i915_private *dev_priv,
			  enum pipe pipe, bool state)
{
	int reg;
	u32 val;
	bool cur_state;

	reg = FDI_TX_CTL(pipe);
	val = I915_READ(reg);
	cur_state = !!(val & FDI_TX_ENABLE);
	WARN(cur_state != state,
	     "FDI TX state assertion failure (expected %s, current %s)\n",
	     state_string(state), state_string(cur_state));
}
#define assert_fdi_tx_enabled(d, p) assert_fdi_tx(d, p, true)
#define assert_fdi_tx_disabled(d, p) assert_fdi_tx(d, p, false)

static void assert_fdi_rx(struct drm_i915_private *dev_priv,
			  enum pipe pipe, bool state)
{
	int reg;
	u32 val;
	bool cur_state;

	reg = FDI_RX_CTL(pipe);
	val = I915_READ(reg);
	cur_state = !!(val & FDI_RX_ENABLE);
	WARN(cur_state != state,
	     "FDI RX state assertion failure (expected %s, current %s)\n",
	     state_string(state), state_string(cur_state));
}
#define assert_fdi_rx_enabled(d, p) assert_fdi_rx(d, p, true)
#define assert_fdi_rx_disabled(d, p) assert_fdi_rx(d, p, false)

static void assert_fdi_tx_pll_enabled(struct drm_i915_private *dev_priv,
				      enum pipe pipe)
{
	int reg;
	u32 val;

	/* ILK FDI PLL is always enabled */
	if (dev_priv->info->gen == 5)
		return;

	reg = FDI_TX_CTL(pipe);
	val = I915_READ(reg);
	WARN(!(val & FDI_TX_PLL_ENABLE), "FDI TX PLL assertion failure, should be active but is disabled\n");
}

static void assert_fdi_rx_pll_enabled(struct drm_i915_private *dev_priv,
				      enum pipe pipe)
{
	int reg;
	u32 val;

	reg = FDI_RX_CTL(pipe);
	val = I915_READ(reg);
	WARN(!(val & FDI_RX_PLL_ENABLE), "FDI RX PLL assertion failure, should be active but is disabled\n");
}

static void assert_panel_unlocked(struct drm_i915_private *dev_priv,
				  enum pipe pipe)
{
	int pp_reg, lvds_reg;
	u32 val;
	enum pipe panel_pipe = PIPE_A;
	bool locked = locked;

	if (HAS_PCH_SPLIT(dev_priv->dev)) {
		pp_reg = PCH_PP_CONTROL;
		lvds_reg = PCH_LVDS;
	} else {
		pp_reg = PP_CONTROL;
		lvds_reg = LVDS;
	}

	val = I915_READ(pp_reg);
	if (!(val & PANEL_POWER_ON) ||
	    ((val & PANEL_UNLOCK_REGS) == PANEL_UNLOCK_REGS))
		locked = false;

	if (I915_READ(lvds_reg) & LVDS_PIPEB_SELECT)
		panel_pipe = PIPE_B;

	WARN(panel_pipe == pipe && locked,
	     "panel assertion failure, pipe %c regs locked\n",
	     pipe_name(pipe));
}

static void assert_pipe(struct drm_i915_private *dev_priv,
			enum pipe pipe, bool state)
{
	int reg;
	u32 val;
	bool cur_state;

	reg = PIPECONF(pipe);
	val = I915_READ(reg);
	cur_state = !!(val & PIPECONF_ENABLE);
	WARN(cur_state != state,
	     "pipe %c assertion failure (expected %s, current %s)\n",
	     pipe_name(pipe), state_string(state), state_string(cur_state));
}
#define assert_pipe_enabled(d, p) assert_pipe(d, p, true)
#define assert_pipe_disabled(d, p) assert_pipe(d, p, false)

static void assert_plane_enabled(struct drm_i915_private *dev_priv,
				 enum plane plane)
{
	int reg;
	u32 val;

	reg = DSPCNTR(plane);
	val = I915_READ(reg);
	WARN(!(val & DISPLAY_PLANE_ENABLE),
	     "plane %c assertion failure, should be active but is disabled\n",
	     plane_name(plane));
}

static void assert_planes_disabled(struct drm_i915_private *dev_priv,
				   enum pipe pipe)
{
	int reg, i;
	u32 val;
	int cur_pipe;

	/* Planes are fixed to pipes on ILK+ */
	if (HAS_PCH_SPLIT(dev_priv->dev))
		return;

	/* Need to check both planes against the pipe */
	for (i = 0; i < 2; i++) {
		reg = DSPCNTR(i);
		val = I915_READ(reg);
		cur_pipe = (val & DISPPLANE_SEL_PIPE_MASK) >>
			DISPPLANE_SEL_PIPE_SHIFT;
		WARN((val & DISPLAY_PLANE_ENABLE) && pipe == cur_pipe,
		     "plane %c assertion failure, should be off on pipe %c but is still active\n",
		     plane_name(i), pipe_name(pipe));
	}
}

static void assert_pch_refclk_enabled(struct drm_i915_private *dev_priv)
{
	u32 val;
	bool enabled;

	val = I915_READ(PCH_DREF_CONTROL);
	enabled = !!(val & (DREF_SSC_SOURCE_MASK | DREF_NONSPREAD_SOURCE_MASK |
			    DREF_SUPERSPREAD_SOURCE_MASK));
	WARN(!enabled, "PCH refclk assertion failure, should be active but is disabled\n");
}

static void assert_transcoder_disabled(struct drm_i915_private *dev_priv,
				       enum pipe pipe)
{
	int reg;
	u32 val;
	bool enabled;

	reg = TRANSCONF(pipe);
	val = I915_READ(reg);
	enabled = !!(val & TRANS_ENABLE);
	WARN(enabled,
	     "transcoder assertion failed, should be off on pipe %c but is still active\n",
	     pipe_name(pipe));
}

static void assert_pch_dp_disabled(struct drm_i915_private *dev_priv,
				   enum pipe pipe, int reg)
{
	u32 val = I915_READ(reg);
	WARN(DP_PIPE_ENABLED(val, pipe),
	     "PCH DP (0x%08x) enabled on transcoder %c, should be disabled\n",
	     reg, pipe_name(pipe));
}

static void assert_pch_hdmi_disabled(struct drm_i915_private *dev_priv,
				     enum pipe pipe, int reg)
{
	u32 val = I915_READ(reg);
	WARN(HDMI_PIPE_ENABLED(val, pipe),
	     "PCH DP (0x%08x) enabled on transcoder %c, should be disabled\n",
	     reg, pipe_name(pipe));
}

static void assert_pch_ports_disabled(struct drm_i915_private *dev_priv,
				      enum pipe pipe)
{
	int reg;
	u32 val;

	assert_pch_dp_disabled(dev_priv, pipe, PCH_DP_B);
	assert_pch_dp_disabled(dev_priv, pipe, PCH_DP_C);
	assert_pch_dp_disabled(dev_priv, pipe, PCH_DP_D);

	reg = PCH_ADPA;
	val = I915_READ(reg);
	WARN(ADPA_PIPE_ENABLED(val, pipe),
	     "PCH VGA enabled on transcoder %c, should be disabled\n",
	     pipe_name(pipe));

	reg = PCH_LVDS;
	val = I915_READ(reg);
	WARN(LVDS_PIPE_ENABLED(val, pipe),
	     "PCH LVDS enabled on transcoder %c, should be disabled\n",
	     pipe_name(pipe));

	assert_pch_hdmi_disabled(dev_priv, pipe, HDMIB);
	assert_pch_hdmi_disabled(dev_priv, pipe, HDMIC);
	assert_pch_hdmi_disabled(dev_priv, pipe, HDMID);
}

/**
 * intel_enable_pch_pll - enable PCH PLL
 * @dev_priv: i915 private structure
 * @pipe: pipe PLL to enable
 *
 * The PCH PLL needs to be enabled before the PCH transcoder, since it
 * drives the transcoder clock.
 */
static void intel_enable_pch_pll(struct drm_i915_private *dev_priv,
				 enum pipe pipe)
{
	int reg;
	u32 val;

	/* PCH only available on ILK+ */
	BUG_ON(dev_priv->info->gen < 5);

	/* PCH refclock must be enabled first */
	assert_pch_refclk_enabled(dev_priv);

	reg = PCH_DPLL(pipe);
	val = I915_READ(reg);
	val |= DPLL_VCO_ENABLE;
	I915_WRITE(reg, val);
	POSTING_READ(reg);
	udelay(200);
}

static void intel_disable_pch_pll(struct drm_i915_private *dev_priv,
				  enum pipe pipe)
{
	int reg;
	u32 val;

	/* PCH only available on ILK+ */
	BUG_ON(dev_priv->info->gen < 5);

	/* Make sure transcoder isn't still depending on us */
	assert_transcoder_disabled(dev_priv, pipe);

	reg = PCH_DPLL(pipe);
	val = I915_READ(reg);
	val &= ~DPLL_VCO_ENABLE;
	I915_WRITE(reg, val);
	POSTING_READ(reg);
	udelay(200);
}

static void intel_enable_transcoder(struct drm_i915_private *dev_priv,
				    enum pipe pipe)
{
	int reg;
	u32 val;

	/* PCH only available on ILK+ */
	BUG_ON(dev_priv->info->gen < 5);

	/* Make sure PCH DPLL is enabled */
	assert_pch_pll_enabled(dev_priv, pipe);

	/* FDI must be feeding us bits for PCH ports */
	assert_fdi_tx_enabled(dev_priv, pipe);
	assert_fdi_rx_enabled(dev_priv, pipe);

	reg = TRANSCONF(pipe);
	val = I915_READ(reg);
	/*
	 * make the BPC in transcoder be consistent with
	 * that in pipeconf reg.
	 */
	val &= ~PIPE_BPC_MASK;
	val |= I915_READ(PIPECONF(pipe)) & PIPE_BPC_MASK;
	I915_WRITE(reg, val | TRANS_ENABLE);
	if (wait_for(I915_READ(reg) & TRANS_STATE_ENABLE, 100))
		DRM_ERROR("failed to enable transcoder %d\n", pipe);
}

static void intel_disable_transcoder(struct drm_i915_private *dev_priv,
				     enum pipe pipe)
{
	int reg;
	u32 val;

	/* FDI relies on the transcoder */
	assert_fdi_tx_disabled(dev_priv, pipe);
	assert_fdi_rx_disabled(dev_priv, pipe);

	/* Ports must be off as well */
	assert_pch_ports_disabled(dev_priv, pipe);

	reg = TRANSCONF(pipe);
	val = I915_READ(reg);
	val &= ~TRANS_ENABLE;
	I915_WRITE(reg, val);
	/* wait for PCH transcoder off, transcoder state */
	if (wait_for((I915_READ(reg) & TRANS_STATE_ENABLE) == 0, 50))
		DRM_ERROR("failed to disable transcoder\n");
}

/**
 * intel_enable_pipe - enable a pipe, asserting requirements
 * @dev_priv: i915 private structure
 * @pipe: pipe to enable
 * @pch_port: on ILK+, is this pipe driving a PCH port or not
 *
 * Enable @pipe, making sure that various hardware specific requirements
 * are met, if applicable, e.g. PLL enabled, LVDS pairs enabled, etc.
 *
 * @pipe should be %PIPE_A or %PIPE_B.
 *
 * Will wait until the pipe is actually running (i.e. first vblank) before
 * returning.
 */
static void intel_enable_pipe(struct drm_i915_private *dev_priv, enum pipe pipe,
			      bool pch_port)
{
	int reg;
	u32 val;

	/*
	 * A pipe without a PLL won't actually be able to drive bits from
	 * a plane.  On ILK+ the pipe PLLs are integrated, so we don't
	 * need the check.
	 */
	if (!HAS_PCH_SPLIT(dev_priv->dev))
		assert_pll_enabled(dev_priv, pipe);
	else {
		if (pch_port) {
			/* if driving the PCH, we need FDI enabled */
			assert_fdi_rx_pll_enabled(dev_priv, pipe);
			assert_fdi_tx_pll_enabled(dev_priv, pipe);
		}
		/* FIXME: assert CPU port conditions for SNB+ */
	}

	reg = PIPECONF(pipe);
	val = I915_READ(reg);
	if (val & PIPECONF_ENABLE)
		return;

	I915_WRITE(reg, val | PIPECONF_ENABLE);
	intel_wait_for_vblank(dev_priv->dev, pipe);
}

/**
 * intel_disable_pipe - disable a pipe, asserting requirements
 * @dev_priv: i915 private structure
 * @pipe: pipe to disable
 *
 * Disable @pipe, making sure that various hardware specific requirements
 * are met, if applicable, e.g. plane disabled, panel fitter off, etc.
 *
 * @pipe should be %PIPE_A or %PIPE_B.
 *
 * Will wait until the pipe has shut down before returning.
 */
static void intel_disable_pipe(struct drm_i915_private *dev_priv,
			       enum pipe pipe)
{
	int reg;
	u32 val;

	/*
	 * Make sure planes won't keep trying to pump pixels to us,
	 * or we might hang the display.
	 */
	assert_planes_disabled(dev_priv, pipe);

	/* Don't disable pipe A or pipe A PLLs if needed */
	if (pipe == PIPE_A && (dev_priv->quirks & QUIRK_PIPEA_FORCE))
		return;

	reg = PIPECONF(pipe);
	val = I915_READ(reg);
	if ((val & PIPECONF_ENABLE) == 0)
		return;

	I915_WRITE(reg, val & ~PIPECONF_ENABLE);
	intel_wait_for_pipe_off(dev_priv->dev, pipe);
}

/**
 * intel_enable_plane - enable a display plane on a given pipe
 * @dev_priv: i915 private structure
 * @plane: plane to enable
 * @pipe: pipe being fed
 *
 * Enable @plane on @pipe, making sure that @pipe is running first.
 */
static void intel_enable_plane(struct drm_i915_private *dev_priv,
			       enum plane plane, enum pipe pipe)
{
	int reg;
	u32 val;

	/* If the pipe isn't enabled, we can't pump pixels and may hang */
	assert_pipe_enabled(dev_priv, pipe);

	reg = DSPCNTR(plane);
	val = I915_READ(reg);
	if (val & DISPLAY_PLANE_ENABLE)
		return;

	I915_WRITE(reg, val | DISPLAY_PLANE_ENABLE);
	intel_wait_for_vblank(dev_priv->dev, pipe);
}

/*
 * Plane regs are double buffered, going from enabled->disabled needs a
 * trigger in order to latch.  The display address reg provides this.
 */
static void intel_flush_display_plane(struct drm_i915_private *dev_priv,
				      enum plane plane)
{
	u32 reg = DSPADDR(plane);
	I915_WRITE(reg, I915_READ(reg));
}

/**
 * intel_disable_plane - disable a display plane
 * @dev_priv: i915 private structure
 * @plane: plane to disable
 * @pipe: pipe consuming the data
 *
 * Disable @plane; should be an independent operation.
 */
static void intel_disable_plane(struct drm_i915_private *dev_priv,
				enum plane plane, enum pipe pipe)
{
	int reg;
	u32 val;

	reg = DSPCNTR(plane);
	val = I915_READ(reg);
	if ((val & DISPLAY_PLANE_ENABLE) == 0)
		return;

	I915_WRITE(reg, val & ~DISPLAY_PLANE_ENABLE);
	intel_flush_display_plane(dev_priv, plane);
	intel_wait_for_vblank(dev_priv->dev, pipe);
}

static void disable_pch_dp(struct drm_i915_private *dev_priv,
			   enum pipe pipe, int reg)
{
	u32 val = I915_READ(reg);
	if (DP_PIPE_ENABLED(val, pipe))
		I915_WRITE(reg, val & ~DP_PORT_EN);
}

static void disable_pch_hdmi(struct drm_i915_private *dev_priv,
			     enum pipe pipe, int reg)
{
	u32 val = I915_READ(reg);
	if (HDMI_PIPE_ENABLED(val, pipe))
		I915_WRITE(reg, val & ~PORT_ENABLE);
}

/* Disable any ports connected to this transcoder */
static void intel_disable_pch_ports(struct drm_i915_private *dev_priv,
				    enum pipe pipe)
{
	u32 reg, val;

	val = I915_READ(PCH_PP_CONTROL);
	I915_WRITE(PCH_PP_CONTROL, val | PANEL_UNLOCK_REGS);

	disable_pch_dp(dev_priv, pipe, PCH_DP_B);
	disable_pch_dp(dev_priv, pipe, PCH_DP_C);
	disable_pch_dp(dev_priv, pipe, PCH_DP_D);

	reg = PCH_ADPA;
	val = I915_READ(reg);
	if (ADPA_PIPE_ENABLED(val, pipe))
		I915_WRITE(reg, val & ~ADPA_DAC_ENABLE);

	reg = PCH_LVDS;
	val = I915_READ(reg);
	if (LVDS_PIPE_ENABLED(val, pipe)) {
		I915_WRITE(reg, val & ~LVDS_PORT_EN);
		POSTING_READ(reg);
		udelay(100);
	}

	disable_pch_hdmi(dev_priv, pipe, HDMIB);
	disable_pch_hdmi(dev_priv, pipe, HDMIC);
	disable_pch_hdmi(dev_priv, pipe, HDMID);
}

static bool i8xx_fbc_enabled(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	return I915_READ(FBC_CONTROL) & FBC_CTL_EN;
}

static bool g4x_fbc_enabled(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	return I915_READ(DPFC_CONTROL) & DPFC_CTL_EN;
}

static bool ironlake_fbc_enabled(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	return I915_READ(ILK_DPFC_CONTROL) & DPFC_CTL_EN;
}

static  int snb_b_fdi_train_param [] = {
	FDI_LINK_TRAIN_400MV_0DB_SNB_B,
	FDI_LINK_TRAIN_400MV_6DB_SNB_B,
	FDI_LINK_TRAIN_600MV_3_5DB_SNB_B,
	FDI_LINK_TRAIN_800MV_0DB_SNB_B,
};

static int i945_get_display_clock_speed(struct drm_device *dev)
{
	return 400000;
}

static int i915_get_display_clock_speed(struct drm_device *dev)
{
	return 333000;
}

static int i9xx_misc_get_display_clock_speed(struct drm_device *dev)
{
	return 200000;
}

static int i915gm_get_display_clock_speed(struct drm_device *dev)
{
	u16 gcfgc = 0;

	pci_read_config_word(dev->pdev, GCFGC, &gcfgc);

	if (gcfgc & GC_LOW_FREQUENCY_ENABLE)
		return 133000;
	else {
		switch (gcfgc & GC_DISPLAY_CLOCK_MASK) {
		case GC_DISPLAY_CLOCK_333_MHZ:
			return 333000;
		default:
		case GC_DISPLAY_CLOCK_190_200_MHZ:
			return 190000;
		}
	}
}

static int i865_get_display_clock_speed(struct drm_device *dev)
{
	return 266000;
}

static int i855_get_display_clock_speed(struct drm_device *dev)
{
	u16 hpllcc = 0;
	/* Assume that the hardware is in the high speed state.  This
	 * should be the default.
	 */
	switch (hpllcc & GC_CLOCK_CONTROL_MASK) {
	case GC_CLOCK_133_200:
	case GC_CLOCK_100_200:
		return 200000;
	case GC_CLOCK_166_250:
		return 250000;
	case GC_CLOCK_100_133:
		return 133000;
	}

	/* Shouldn't happen */
	return 0;
}

static int i830_get_display_clock_speed(struct drm_device *dev)
{
	return 133000;
}

struct fdi_m_n {
	u32        tu;
	u32        gmch_m;
	u32        gmch_n;
	u32        link_m;
	u32        link_n;
};

static void
fdi_reduce_ratio(u32 *num, u32 *den)
{
	while (*num > 0xffffff || *den > 0xffffff) {
		*num >>= 1;
		*den >>= 1;
	}
}

static void
ironlake_compute_m_n(int bits_per_pixel, int nlanes, int pixel_clock,
		     int link_clock, struct fdi_m_n *m_n)
{
	m_n->tu = 64; /* default size */

	/* BUG_ON(pixel_clock > INT_MAX / 36); */
	m_n->gmch_m = bits_per_pixel * pixel_clock;
	m_n->gmch_n = link_clock * nlanes * 8;
	fdi_reduce_ratio(&m_n->gmch_m, &m_n->gmch_n);

	m_n->link_m = pixel_clock;
	m_n->link_n = link_clock;
	fdi_reduce_ratio(&m_n->link_m, &m_n->link_n);
}


struct intel_watermark_params {
	unsigned long fifo_size;
	unsigned long max_wm;
	unsigned long default_wm;
	unsigned long guard_size;
	unsigned long cacheline_size;
};

/* Pineview has different values for various configs */
static  struct intel_watermark_params pineview_display_wm = {
	PINEVIEW_DISPLAY_FIFO,
	PINEVIEW_MAX_WM,
	PINEVIEW_DFT_WM,
	PINEVIEW_GUARD_WM,
	PINEVIEW_FIFO_LINE_SIZE
};
static  struct intel_watermark_params pineview_display_hplloff_wm = {
	PINEVIEW_DISPLAY_FIFO,
	PINEVIEW_MAX_WM,
	PINEVIEW_DFT_HPLLOFF_WM,
	PINEVIEW_GUARD_WM,
	PINEVIEW_FIFO_LINE_SIZE
};
static  struct intel_watermark_params pineview_cursor_wm = {
	PINEVIEW_CURSOR_FIFO,
	PINEVIEW_CURSOR_MAX_WM,
	PINEVIEW_CURSOR_DFT_WM,
	PINEVIEW_CURSOR_GUARD_WM,
	PINEVIEW_FIFO_LINE_SIZE,
};
static  struct intel_watermark_params pineview_cursor_hplloff_wm = {
	PINEVIEW_CURSOR_FIFO,
	PINEVIEW_CURSOR_MAX_WM,
	PINEVIEW_CURSOR_DFT_WM,
	PINEVIEW_CURSOR_GUARD_WM,
	PINEVIEW_FIFO_LINE_SIZE
};
static  struct intel_watermark_params g4x_wm_info = {
	G4X_FIFO_SIZE,
	G4X_MAX_WM,
	G4X_MAX_WM,
	2,
	G4X_FIFO_LINE_SIZE,
};
static  struct intel_watermark_params g4x_cursor_wm_info = {
	I965_CURSOR_FIFO,
	I965_CURSOR_MAX_WM,
	I965_CURSOR_DFT_WM,
	2,
	G4X_FIFO_LINE_SIZE,
};
static  struct intel_watermark_params i965_cursor_wm_info = {
	I965_CURSOR_FIFO,
	I965_CURSOR_MAX_WM,
	I965_CURSOR_DFT_WM,
	2,
	I915_FIFO_LINE_SIZE,
};
static  struct intel_watermark_params i945_wm_info = {
	I945_FIFO_SIZE,
	I915_MAX_WM,
	1,
	2,
	I915_FIFO_LINE_SIZE
};
static  struct intel_watermark_params i915_wm_info = {
	I915_FIFO_SIZE,
	I915_MAX_WM,
	1,
	2,
	I915_FIFO_LINE_SIZE
};
static  struct intel_watermark_params i855_wm_info = {
	I855GM_FIFO_SIZE,
	I915_MAX_WM,
	1,
	2,
	I830_FIFO_LINE_SIZE
};
static  struct intel_watermark_params i830_wm_info = {
	I830_FIFO_SIZE,
	I915_MAX_WM,
	1,
	2,
	I830_FIFO_LINE_SIZE
};

static  struct intel_watermark_params ironlake_display_wm_info = {
	ILK_DISPLAY_FIFO,
	ILK_DISPLAY_MAXWM,
	ILK_DISPLAY_DFTWM,
	2,
	ILK_FIFO_LINE_SIZE
};
static  struct intel_watermark_params ironlake_cursor_wm_info = {
	ILK_CURSOR_FIFO,
	ILK_CURSOR_MAXWM,
	ILK_CURSOR_DFTWM,
	2,
	ILK_FIFO_LINE_SIZE
};
static  struct intel_watermark_params ironlake_display_srwm_info = {
	ILK_DISPLAY_SR_FIFO,
	ILK_DISPLAY_MAX_SRWM,
	ILK_DISPLAY_DFT_SRWM,
	2,
	ILK_FIFO_LINE_SIZE
};
static  struct intel_watermark_params ironlake_cursor_srwm_info = {
	ILK_CURSOR_SR_FIFO,
	ILK_CURSOR_MAX_SRWM,
	ILK_CURSOR_DFT_SRWM,
	2,
	ILK_FIFO_LINE_SIZE
};

static  struct intel_watermark_params sandybridge_display_wm_info = {
	SNB_DISPLAY_FIFO,
	SNB_DISPLAY_MAXWM,
	SNB_DISPLAY_DFTWM,
	2,
	SNB_FIFO_LINE_SIZE
};
static  struct intel_watermark_params sandybridge_cursor_wm_info = {
	SNB_CURSOR_FIFO,
	SNB_CURSOR_MAXWM,
	SNB_CURSOR_DFTWM,
	2,
	SNB_FIFO_LINE_SIZE
};
static  struct intel_watermark_params sandybridge_display_srwm_info = {
	SNB_DISPLAY_SR_FIFO,
	SNB_DISPLAY_MAX_SRWM,
	SNB_DISPLAY_DFT_SRWM,
	2,
	SNB_FIFO_LINE_SIZE
};
static  struct intel_watermark_params sandybridge_cursor_srwm_info = {
	SNB_CURSOR_SR_FIFO,
	SNB_CURSOR_MAX_SRWM,
	SNB_CURSOR_DFT_SRWM,
	2,
	SNB_FIFO_LINE_SIZE
};


/**
 * intel_calculate_wm - calculate watermark level
 * @clock_in_khz: pixel clock
 * @wm: chip FIFO params
 * @pixel_size: display pixel size
 * @latency_ns: memory latency for the platform
 *
 * Calculate the watermark level (the level at which the display plane will
 * start fetching from memory again).  Each chip has a different display
 * FIFO size and allocation, so the caller needs to figure that out and pass
 * in the correct intel_watermark_params structure.
 *
 * As the pixel clock runs, the FIFO will be drained at a rate that depends
 * on the pixel size.  When it reaches the watermark level, it'll start
 * fetching FIFO line sized based chunks from memory until the FIFO fills
 * past the watermark point.  If the FIFO drains completely, a FIFO underrun
 * will occur, and a display engine hang could result.
 */
static unsigned long intel_calculate_wm(unsigned long clock_in_khz,
					 struct intel_watermark_params *wm,
					int fifo_size,
					int pixel_size,
					unsigned long latency_ns)
{
	long entries_required, wm_size;

	/*
	 * Note: we need to make sure we don't overflow for various clock &
	 * latency values.
	 * clocks go from a few thousand to several hundred thousand.
	 * latency is usually a few thousand
	 */
	entries_required = ((clock_in_khz / 1000) * pixel_size * latency_ns) /
		1000;
	entries_required = DIV_ROUND_UP(entries_required, wm->cacheline_size);

	DRM_DEBUG_KMS("FIFO entries required for mode: %ld\n", entries_required);

	wm_size = fifo_size - (entries_required + wm->guard_size);

	DRM_DEBUG_KMS("FIFO watermark level: %ld\n", wm_size);

	/* Don't promote wm_size to unsigned... */
	if (wm_size > (long)wm->max_wm)
		wm_size = wm->max_wm;
	if (wm_size <= 0)
		wm_size = wm->default_wm;
	return wm_size;
}

struct cxsr_latency {
	int is_desktop;
	int is_ddr3;
	unsigned long fsb_freq;
	unsigned long mem_freq;
	unsigned long display_sr;
	unsigned long display_hpll_disable;
	unsigned long cursor_sr;
	unsigned long cursor_hpll_disable;
};

static  struct cxsr_latency cxsr_latency_table[] = {
	{1, 0, 800, 400, 3382, 33382, 3983, 33983},    /* DDR2-400 SC */
	{1, 0, 800, 667, 3354, 33354, 3807, 33807},    /* DDR2-667 SC */
	{1, 0, 800, 800, 3347, 33347, 3763, 33763},    /* DDR2-800 SC */
	{1, 1, 800, 667, 6420, 36420, 6873, 36873},    /* DDR3-667 SC */
	{1, 1, 800, 800, 5902, 35902, 6318, 36318},    /* DDR3-800 SC */

	{1, 0, 667, 400, 3400, 33400, 4021, 34021},    /* DDR2-400 SC */
	{1, 0, 667, 667, 3372, 33372, 3845, 33845},    /* DDR2-667 SC */
	{1, 0, 667, 800, 3386, 33386, 3822, 33822},    /* DDR2-800 SC */
	{1, 1, 667, 667, 6438, 36438, 6911, 36911},    /* DDR3-667 SC */
	{1, 1, 667, 800, 5941, 35941, 6377, 36377},    /* DDR3-800 SC */

	{1, 0, 400, 400, 3472, 33472, 4173, 34173},    /* DDR2-400 SC */
	{1, 0, 400, 667, 3443, 33443, 3996, 33996},    /* DDR2-667 SC */
	{1, 0, 400, 800, 3430, 33430, 3946, 33946},    /* DDR2-800 SC */
	{1, 1, 400, 667, 6509, 36509, 7062, 37062},    /* DDR3-667 SC */
	{1, 1, 400, 800, 5985, 35985, 6501, 36501},    /* DDR3-800 SC */

	{0, 0, 800, 400, 3438, 33438, 4065, 34065},    /* DDR2-400 SC */
	{0, 0, 800, 667, 3410, 33410, 3889, 33889},    /* DDR2-667 SC */
	{0, 0, 800, 800, 3403, 33403, 3845, 33845},    /* DDR2-800 SC */
	{0, 1, 800, 667, 6476, 36476, 6955, 36955},    /* DDR3-667 SC */
	{0, 1, 800, 800, 5958, 35958, 6400, 36400},    /* DDR3-800 SC */

	{0, 0, 667, 400, 3456, 33456, 4103, 34106},    /* DDR2-400 SC */
	{0, 0, 667, 667, 3428, 33428, 3927, 33927},    /* DDR2-667 SC */
	{0, 0, 667, 800, 3443, 33443, 3905, 33905},    /* DDR2-800 SC */
	{0, 1, 667, 667, 6494, 36494, 6993, 36993},    /* DDR3-667 SC */
	{0, 1, 667, 800, 5998, 35998, 6460, 36460},    /* DDR3-800 SC */

	{0, 0, 400, 400, 3528, 33528, 4255, 34255},    /* DDR2-400 SC */
	{0, 0, 400, 667, 3500, 33500, 4079, 34079},    /* DDR2-667 SC */
	{0, 0, 400, 800, 3487, 33487, 4029, 34029},    /* DDR2-800 SC */
	{0, 1, 400, 667, 6566, 36566, 7145, 37145},    /* DDR3-667 SC */
	{0, 1, 400, 800, 6042, 36042, 6584, 36584},    /* DDR3-800 SC */
};

static  struct cxsr_latency *intel_get_cxsr_latency(int is_desktop,
							 int is_ddr3,
							 int fsb,
							 int mem)
{
	 struct cxsr_latency *latency;
	int i;

	if (fsb == 0 || mem == 0)
		return NULL;

	for (i = 0; i < ARRAY_SIZE(cxsr_latency_table); i++) {
		latency = &cxsr_latency_table[i];
		if (is_desktop == latency->is_desktop &&
		    is_ddr3 == latency->is_ddr3 &&
		    fsb == latency->fsb_freq && mem == latency->mem_freq)
			return latency;
	}

	DRM_DEBUG_KMS("Unknown FSB/MEM found, disable CxSR\n");

	return NULL;
}

/*
 * Latency for FIFO fetches is dependent on several factors:
 *   - memory configuration (speed, channels)
 *   - chipset
 *   - current MCH state
 * It can be fairly high in some situations, so here we assume a fairly
 * pessimal value.  It's a tradeoff between extra memory fetches (if we
 * set this value too high, the FIFO will fetch frequently to stay full)
 * and power consumption (set it too low to save power and we might see
 * FIFO underruns and display "flicker").
 *
 * A value of 5us seems to be a good balance; safe for very low end
 * platforms but not overly aggressive on lower latency configs.
 */
static  int latency_ns = 5000;

static int i9xx_get_fifo_size(struct drm_device *dev, int plane)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	uint32_t dsparb = I915_READ(DSPARB);
	int size;

	size = dsparb & 0x7f;
	if (plane)
		size = ((dsparb >> DSPARB_CSTART_SHIFT) & 0x7f) - size;

	DRM_DEBUG_KMS("FIFO size - (0x%08x) %s: %d\n", dsparb,
		      plane ? "B" : "A", size);

	return size;
}

static int i85x_get_fifo_size(struct drm_device *dev, int plane)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	uint32_t dsparb = I915_READ(DSPARB);
	int size;

	size = dsparb & 0x1ff;
	if (plane)
		size = ((dsparb >> DSPARB_BEND_SHIFT) & 0x1ff) - size;
	size >>= 1; /* Convert to cachelines */

	DRM_DEBUG_KMS("FIFO size - (0x%08x) %s: %d\n", dsparb,
		      plane ? "B" : "A", size);

	return size;
}

static int i845_get_fifo_size(struct drm_device *dev, int plane)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	uint32_t dsparb = I915_READ(DSPARB);
	int size;

	size = dsparb & 0x7f;
	size >>= 2; /* Convert to cachelines */

	DRM_DEBUG_KMS("FIFO size - (0x%08x) %s: %d\n", dsparb,
		      plane ? "B" : "A",
		      size);

	return size;
}

static int i830_get_fifo_size(struct drm_device *dev, int plane)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	uint32_t dsparb = I915_READ(DSPARB);
	int size;

	size = dsparb & 0x7f;
	size >>= 1; /* Convert to cachelines */

	DRM_DEBUG_KMS("FIFO size - (0x%08x) %s: %d\n", dsparb,
		      plane ? "B" : "A", size);

	return size;
}

/*
 * Check the wm result.
 *
 * If any calculated watermark values is larger than the maximum value that
 * can be programmed into the associated watermark register, that watermark
 * must be disabled.
 */
static bool g4x_check_srwm(struct drm_device *dev,
			   int display_wm, int cursor_wm,
			    struct intel_watermark_params *display,
			    struct intel_watermark_params *cursor)
{
	DRM_DEBUG_KMS("SR watermark: display plane %d, cursor %d\n",
		      display_wm, cursor_wm);

	if (display_wm > display->max_wm) {
		DRM_DEBUG_KMS("display watermark is too large(%d/%ld), disabling\n",
			      display_wm, display->max_wm);
		return false;
	}

	if (cursor_wm > cursor->max_wm) {
		DRM_DEBUG_KMS("cursor watermark is too large(%d/%ld), disabling\n",
			      cursor_wm, cursor->max_wm);
		return false;
	}

	if (!(display_wm || cursor_wm)) {
		DRM_DEBUG_KMS("SR latency is 0, disabling\n");
		return false;
	}

	return true;
}

#define single_plane_enabled(mask) is_power_of_2(mask)

#define ILK_LP0_PLANE_LATENCY		700
#define ILK_LP0_CURSOR_LATENCY		1300

/*
 * Check the wm result.
 *
 * If any calculated watermark values is larger than the maximum value that
 * can be programmed into the associated watermark register, that watermark
 * must be disabled.
 */
static bool ironlake_check_srwm(struct drm_device *dev, int level,
				int fbc_wm, int display_wm, int cursor_wm,
				 struct intel_watermark_params *display,
				 struct intel_watermark_params *cursor)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	DRM_DEBUG_KMS("watermark %d: display plane %d, fbc lines %d,"
		      " cursor %d\n", level, display_wm, fbc_wm, cursor_wm);

	if (fbc_wm > SNB_FBC_MAX_SRWM) {
		DRM_DEBUG_KMS("fbc watermark(%d) is too large(%d), disabling wm%d+\n",
			      fbc_wm, SNB_FBC_MAX_SRWM, level);

		/* fbc has it's own way to disable FBC WM */
		I915_WRITE(DISP_ARB_CTL,
			   I915_READ(DISP_ARB_CTL) | DISP_FBC_WM_DIS);
		return false;
	}

	if (display_wm > display->max_wm) {
		DRM_DEBUG_KMS("display watermark(%d) is too large(%d), disabling wm%d+\n",
			      display_wm, SNB_DISPLAY_MAX_SRWM, level);
		return false;
	}

	if (cursor_wm > cursor->max_wm) {
		DRM_DEBUG_KMS("cursor watermark(%d) is too large(%d), disabling wm%d+\n",
			      cursor_wm, SNB_CURSOR_MAX_SRWM, level);
		return false;
	}

	if (!(fbc_wm || display_wm || cursor_wm)) {
		DRM_DEBUG_KMS("latency %d is 0, disabling wm%d+\n", level, level);
		return false;
	}

	return true;
}

/**
 * Get a pipe with a simple mode set on it for doing load-based monitor
 * detection.
 *
 * It will be up to the load-detect code to adjust the pipe as appropriate for
 * its requirements.  The pipe will be connected to no other encoders.
 *
 * Currently this code will only succeed if there is a pipe with no encoders
 * configured for it.  In the future, it could choose to temporarily disable
 * some outputs to free up a pipe for its use.
 *
 * \return crtc, or NULL if no pipes are available.
 */

/* VESA 640x480x72Hz mode to set on the pipe */
static struct drm_display_mode load_detect_mode = {
	DRM_MODE("640x480", DRM_MODE_TYPE_DEFAULT, 31500, 640, 664,
		 704, 832, 0, 480, 489, 491, 520, 0, DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
};

static u32
intel_framebuffer_pitch_for_width(int width, int bpp)
{
	u32 pitch = DIV_ROUND_UP(width * bpp, 8);
	return ALIGN(pitch, 64);
}

#define GPU_IDLE_TIMEOUT 500 /* ms */

#define CRTC_IDLE_TIMEOUT 1000 /* ms */

static void intel_sanitize_modesetting(struct drm_device *dev,
				       int pipe, int plane)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 reg, val;

	if (HAS_PCH_SPLIT(dev))
		return;

	/* Who knows what state these registers were left in by the BIOS or
	 * grub?
	 *
	 * If we leave the registers in a conflicting state (e.g. with the
	 * display plane reading from the other pipe than the one we intend
	 * to use) then when we attempt to teardown the active mode, we will
	 * not disable the pipes and planes in the correct order -- leaving
	 * a plane reading from a disabled pipe and possibly leading to
	 * undefined behaviour.
	 */

	reg = DSPCNTR(plane);
	val = I915_READ(reg);

	if ((val & DISPLAY_PLANE_ENABLE) == 0)
		return;
	if (!!(val & DISPPLANE_SEL_PIPE_MASK) == pipe)
		return;

	/* This display plane is active and attached to the other CPU pipe. */
	pipe = !pipe;

	/* Disable the plane and wait for it to stop reading from the pipe. */
	intel_disable_plane(dev_priv, plane, pipe);
	intel_disable_pipe(dev_priv, pipe);
}

static bool has_edp_a(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (!IS_MOBILE(dev))
		return false;

	if ((I915_READ(DP_A) & DP_DETECTED) == 0)
		return false;

	if (IS_GEN5(dev) &&
	    (I915_READ(ILK_DISPLAY_CHICKEN_FUSES) & ILK_eDP_A_DISABLE))
		return false;

	return true;
}

bool ironlake_set_drps(struct drm_device *dev, u8 val)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u16 rgvswctl;

	rgvswctl = I915_READ16(MEMSWCTL);
	if (rgvswctl & MEMCTL_CMD_STS) {
		DRM_DEBUG("gpu busy, RCS change rejected\n");
		return false; /* still busy with another command */
	}

	rgvswctl = (MEMCTL_CMD_CHFREQ << MEMCTL_CMD_SHIFT) |
		(val << MEMCTL_FREQ_SHIFT) | MEMCTL_SFCAVM;
	I915_WRITE16(MEMSWCTL, rgvswctl);
	POSTING_READ16(MEMSWCTL);

	rgvswctl |= MEMCTL_CMD_STS;
	I915_WRITE16(MEMSWCTL, rgvswctl);

	return true;
}

static unsigned long intel_pxfreq(u32 vidfreq)
{
	unsigned long freq;
	int div = (vidfreq & 0x3f0000) >> 16;
	int post = (vidfreq & 0x3000) >> 12;
	int pre = (vidfreq & 0x7);

	if (!pre)
		return 0;

	freq = ((div * 133333) / ((1<<post) * pre));

	return freq;
}

struct intel_quirk {
	int device;
	int subsystem_vendor;
	int subsystem_device;
	void (*hook)(struct drm_device *dev);
};

/*
 * set vga decode state - true == enable VGA decode
 */
int intel_modeset_vga_set_state(struct drm_device *dev, bool state)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u16 gmch_ctrl;

	pci_read_config_word(dev_priv->bridge_dev, INTEL_GMCH_CTRL, &gmch_ctrl);
	if (state)
		gmch_ctrl &= ~INTEL_GMCH_VGA_DISABLE;
	else
		gmch_ctrl |= INTEL_GMCH_VGA_DISABLE;
	pci_write_config_word(dev_priv->bridge_dev, INTEL_GMCH_CTRL, gmch_ctrl);
	return 0;
}

#ifdef CONFIG_DEBUG_FS
#include <linux/seq_file.h>

struct intel_display_error_state {
	struct intel_cursor_error_state {
		u32 control;
		u32 position;
		u32 base;
		u32 size;
	} cursor[2];

	struct intel_pipe_error_state {
		u32 conf;
		u32 source;

		u32 htotal;
		u32 hblank;
		u32 hsync;
		u32 vtotal;
		u32 vblank;
		u32 vsync;
	} pipe[2];

	struct intel_plane_error_state {
		u32 control;
		u32 stride;
		u32 size;
		u32 pos;
		u32 addr;
		u32 surface;
		u32 tile_offset;
	} plane[2];
};

struct intel_display_error_state *
intel_display_capture_error_state(struct drm_device *dev)
{
        drm_i915_private_t *dev_priv = dev->dev_private;
	struct intel_display_error_state *error;
	int i;

	error = kmalloc(sizeof(*error), GFP_ATOMIC);
	if (error == NULL)
		return NULL;

	for (i = 0; i < 2; i++) {
		error->cursor[i].control = I915_READ(CURCNTR(i));
		error->cursor[i].position = I915_READ(CURPOS(i));
		error->cursor[i].base = I915_READ(CURBASE(i));

		error->plane[i].control = I915_READ(DSPCNTR(i));
		error->plane[i].stride = I915_READ(DSPSTRIDE(i));
		error->plane[i].size = I915_READ(DSPSIZE(i));
		error->plane[i].pos= I915_READ(DSPPOS(i));
		error->plane[i].addr = I915_READ(DSPADDR(i));
		if (INTEL_INFO(dev)->gen >= 4) {
			error->plane[i].surface = I915_READ(DSPSURF(i));
			error->plane[i].tile_offset = I915_READ(DSPTILEOFF(i));
		}

		error->pipe[i].conf = I915_READ(PIPECONF(i));
		error->pipe[i].source = I915_READ(PIPESRC(i));
		error->pipe[i].htotal = I915_READ(HTOTAL(i));
		error->pipe[i].hblank = I915_READ(HBLANK(i));
		error->pipe[i].hsync = I915_READ(HSYNC(i));
		error->pipe[i].vtotal = I915_READ(VTOTAL(i));
		error->pipe[i].vblank = I915_READ(VBLANK(i));
		error->pipe[i].vsync = I915_READ(VSYNC(i));
	}

	return error;
}

void
intel_display_print_error_state(struct seq_file *m,
				struct drm_device *dev,
				struct intel_display_error_state *error)
{
	int i;

	for (i = 0; i < 2; i++) {
		seq_printf(m, "Pipe [%d]:\n", i);
		seq_printf(m, "  CONF: %08x\n", error->pipe[i].conf);
		seq_printf(m, "  SRC: %08x\n", error->pipe[i].source);
		seq_printf(m, "  HTOTAL: %08x\n", error->pipe[i].htotal);
		seq_printf(m, "  HBLANK: %08x\n", error->pipe[i].hblank);
		seq_printf(m, "  HSYNC: %08x\n", error->pipe[i].hsync);
		seq_printf(m, "  VTOTAL: %08x\n", error->pipe[i].vtotal);
		seq_printf(m, "  VBLANK: %08x\n", error->pipe[i].vblank);
		seq_printf(m, "  VSYNC: %08x\n", error->pipe[i].vsync);

		seq_printf(m, "Plane [%d]:\n", i);
		seq_printf(m, "  CNTR: %08x\n", error->plane[i].control);
		seq_printf(m, "  STRIDE: %08x\n", error->plane[i].stride);
		seq_printf(m, "  SIZE: %08x\n", error->plane[i].size);
		seq_printf(m, "  POS: %08x\n", error->plane[i].pos);
		seq_printf(m, "  ADDR: %08x\n", error->plane[i].addr);
		if (INTEL_INFO(dev)->gen >= 4) {
			seq_printf(m, "  SURF: %08x\n", error->plane[i].surface);
			seq_printf(m, "  TILEOFF: %08x\n", error->plane[i].tile_offset);
		}

		seq_printf(m, "Cursor [%d]:\n", i);
		seq_printf(m, "  CNTR: %08x\n", error->cursor[i].control);
		seq_printf(m, "  POS: %08x\n", error->cursor[i].position);
		seq_printf(m, "  BASE: %08x\n", error->cursor[i].base);
	}
}
#endif
