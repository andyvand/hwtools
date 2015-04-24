objects=main.o pci.o final/intel_bios.o final/drm_modes.o final/i915_drv.o DirectHW.o
source=main.c pci.c final/intel_bios.c final/drm_modes.c final/i915_drv.c DirectHW.c

.c.o:
	gcc -include video.h -c $< -o $@

all: $(objects) probe video

video: $(source) final/intel_display.o
	gcc -g -o video $(objects) final/intel_display.o -lpci -framework IOKit

probe: $(source)
	gcc -g -o probe $(objects) -lpci -framework IOKit

clean:
	rm -f *.o video probe

moreclean:  clean
	rm final/* per-file-changes/* tmp/*

superclean: moreclean
	rm inputs/*
