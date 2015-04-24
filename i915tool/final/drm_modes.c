
/**
 * drm_mode_debug_printmodeline - debug print a mode
 * @dev: DRM device
 * @mode: mode to print
 *
 * LOCKING:
 * None.
 *
 * Describe @mode using DRM_DEBUG.
 */
void drm_mode_debug_printmodeline(struct drm_display_mode *mode)
{
	DRM_DEBUG_KMS("Modeline %d:\"%s\" %d %d %d %d %d %d %d %d %d %d "
			"0x%x 0x%x\n",
		mode->base.id, mode->name, mode->vrefresh, mode->clock,
		mode->hdisplay, mode->hsync_start,
		mode->hsync_end, mode->htotal,
		mode->vdisplay, mode->vsync_start,
		mode->vsync_end, mode->vtotal, mode->type, mode->flags);
}

/**
 * drm_mode_set_name - set the name on a mode
 * @mode: name will be set in this mode
 *
 * LOCKING:
 * None.
 *
 * Set the name of @mode to a standard format.
 */
void drm_mode_set_name(struct drm_display_mode *mode)
{
	bool interlaced = !!(mode->flags & DRM_MODE_FLAG_INTERLACE);

	snprintf(mode->name, DRM_DISPLAY_MODE_LEN, "%dx%d%s",
		 mode->hdisplay, mode->vdisplay,
		 interlaced ? "i" : "");
}
