/*
 * This file is part of i915tool
 *
 * Copyright (C) 2012 The ChromiumOS Authors.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "video.h"

int verbose = 0;
static unsigned short addrport, dataport;
struct drm_device *i915;
unsigned short vendor=0x8086, device=0x0116;
struct pci_dev fake = {.vendor_id=0x8086, .device_id = 0x0116};
int dofake = 0;
u8 *bios_image = NULL;
u8 *mmiobase;
u32 mmiophys;
int mmiosize;
size_t bios_image_size;
/* temporary */
unsigned int i915_lvds_downclock = 0;
int i915_vbt_sdvo_panel_type = -1;

#ifdef __APPLE__
u8 *global_map = NULL;
unsigned int mapsize = 0;
unsigned int mapdescsize = 0;
int kfd;

void errx(int eval, const char *fmt, ...)
{
    printf("ERROR: %s\n", fmt);
    
    exit(eval);
}

#define PMEM_ERROR_LOG printf

#include <sys/ioctl.h>

#include "pmem_ioctls.h"

unsigned int get_mmap(uint8_t **mmap, unsigned int *mmap_size, unsigned int *mmap_desc_size, int device_file);

unsigned int get_mmap(uint8_t **mmap, unsigned int *mmap_size, unsigned int *mmap_desc_size, int device_file) {
    int err;
    int status = EXIT_FAILURE;
    
    err = ioctl(device_file, PMEM_IOCTL_GET_MMAP_SIZE, mmap_size);
    if (err != 0) {
        PMEM_ERROR_LOG("Error getting size of memory map");
        goto error;
    }
    err = ioctl(device_file, PMEM_IOCTL_GET_MMAP_DESC_SIZE, mmap_desc_size);
    if (err != 0) {
        PMEM_ERROR_LOG("Error getting size of memory map descriptors");
        goto error;
    }
    
#ifdef DEBUG
    printf("Recieved memory map, size:%d bytes (descriptors: %d)\n", *mmap_size, *mmap_desc_size);
#endif
    
    // Allocate buffer of apropriate size.
    *mmap = (uint8_t *)malloc(*mmap_size);
    if (*mmap == NULL) {
        PMEM_ERROR_LOG("Could not allocate buffer for memory map");
        goto error;
    }
    // Ask the driver to fill it with the physical memory map.
    if ((err = ioctl(device_file, PMEM_IOCTL_GET_MMAP, mmap)) != 0) {
        PMEM_ERROR_LOG("Error getting memory map");
        free(*mmap);
        goto error;
    }
    status = EXIT_SUCCESS;
error:
    return status;
}
#endif

int accessor = 0;

/* */

/* not sure how we want to do this so let's guess */
/* to make it easy, we start at zero and assume 250 hz. */
unsigned long msecs(void)
{
	struct timeval start, now;
	static int first = 0;
	unsigned long j;
	if (! first++)
		gettimeofday(&start, NULL);
	gettimeofday(&now, NULL);
	j = (now.tv_sec - start.tv_sec)*1000 + (now.tv_usec-start.tv_usec)/1000;
	return j;
}

void
mdelay(unsigned long ms)
{
	unsigned long start;
	start = msecs();
	while (msecs() < (start + ms))
		;
}

void
hexdump_pmem(int size);
void
hexdump(u8 *base, int size);

void
hexdump_pmem(int size)
{
    int ptr = 0;
    u8 *ptrdata;
    ptrdata = malloc(size);
    lseek(kfd, mmiophys, SEEK_SET);
    read(kfd, ptrdata, size);
    hexdump(ptrdata, size);
}

void
hexdump(u8 *base, int size)
{
	int i, j;
	for(i = 0; i < size/sizeof(u32); i += 8) {
		printf("%#x: ", i);
		for(j = 0; j < 8; j++)
			printf("%02x", base[i+j]);
		printf("\n");
	}
}
#ifdef __APPLE__
void udelay(int i)
#else
void udelay(int __unused i)
#endif
{
#ifdef __APPLE__
    usleep(i);
#endif
}

unsigned long io_I915_READ(unsigned long addr)
{
	unsigned long val;
	if (dofake)
		return 0xcafebabe;
	outl(addr, addrport);
	val = inl(dataport);
	if (verbose)
		fprintf(stderr, "%s: %lx <- %lx\n", __func__, val, addr);
	return val;
}

u16 io_I915_READ16(unsigned long addr)
{
	u16 val;
	if (dofake)
		return 0xbabe;
	outl(addr, addrport);
	val = inw(dataport);
	if (verbose)
		fprintf(stderr, "%s: %hx <- %lx\n", __func__, val, addr);
	return val;
}

void io_I915_WRITE(unsigned long addr, unsigned long val)
{
	if (dofake)
		return;
	outl(addr, addrport);
	outl(val, dataport);
	if (verbose)
		fprintf(stderr, "%s: %lx -> %lx\n", __func__, val, addr);
}

void io_I915_WRITE16(unsigned long addr, u16 val)
{
	if (dofake)
		return;
	outl(addr, addrport);
	outw(val, dataport);
	if (verbose)
		fprintf(stderr, "%s: %hx -> %lx\n", __func__, val, addr);
}

unsigned long I915_READ(unsigned long addr)
{
	//volatile u32 *ptr = (u32 *)(mmiobase + addr);
	unsigned long val;
    lseek(kfd, mmiophys + addr, SEEK_SET);
	if (dofake)
		return 0xcafebabe;
	//val = *ptr;
    read(kfd, &val, 4);
	if (verbose)
		fprintf(stderr, "%s: %lx <- %lx\n", __func__, val, addr);
	return val;
}

void I915_WRITE(unsigned long addr, unsigned long val)
{
	//volatile u32 *ptr = (u32 *)(mmiobase + addr);
    lseek(kfd, mmiophys + addr, SEEK_SET);
	if (dofake)
		return;
	//*ptr = val;
    write(kfd, &val, 4);
	if (verbose)
		fprintf(stderr, "%s: %lx -> %lx\n", __func__, val, addr);
}

u16 I915_READ16(unsigned long addr)
{
	//volatile u16 *ptr = (u16 *)(mmiobase + addr);
	unsigned long val;
    lseek(kfd, mmiophys + addr, SEEK_SET);
	if (dofake)
		return 0xbabe;
	//val = *ptr;
    read(kfd, &val, 2);
	if (verbose)
		fprintf(stderr, "%s: %lx <- %lx\n", __func__, val, addr);
	return val;
}

void I915_WRITE16(unsigned long addr, u16 val)
{
	//volatile u16 *ptr = (u16 *)(mmiobase + addr);
    lseek(kfd, mmiophys + addr, SEEK_SET);
	if (dofake)
		return;
	//*ptr = val;
    write(kfd, &val, 2);
	if (verbose)
		fprintf(stderr, "%s: %hx -> %lx\n", __func__, val, addr);
}

#define GTT_RETRY 1000
static int gtt_poll(u32 reg, u32 mask, u32 value)
{
        unsigned try = GTT_RETRY;
        u32 data;

        while (try--) {
                if (accessor == 0)
                    data = I915_READ(reg);
                else
                    data = io_I915_READ(reg);
                if ((data & mask) == value){
                    printf("GT init succeeds after %d tries\n", GTT_RETRY-try);

                    return 1;
                }

                udelay(10);
        }

        fprintf(stderr, "GT init timeout\n");
        return 0;
}
void *pci_map_rom(struct pci_dev *dev, size_t *size)
{
	*size = bios_image_size;
	return bios_image;
}

void *pci_unmap_rom(struct pci_dev *dev, void *bios)
{
	return NULL;
}

void *dmi_check_system(unsigned long ignore)
{
	return NULL;
}

void
mapit(void)
{
#ifndef __APPLE__
	int kfd;
	kfd = open("/dev/mem", O_RDWR);
	if (kfd < 0)
		errx(1, "/dev/kmem");
	mmiobase = mmap(NULL, mmiosize, PROT_WRITE|PROT_READ, MAP_SHARED, kfd,
		mmiophys);
	if ((void *)-1 == mmiobase)
		errx(1, "mmap");
#else
    /*mmiobase = map_physical(mmiophys, mmiosize);
	if ((void *)-1 == mmiobase)
		errx(1, "mmap");*/
	kfd = open("/dev/pmem", O_RDWR);
	if (kfd < 0)
		errx(1, "/dev/pmem");
	if (get_mmap(&global_map, &mapsize, &mapdescsize, kfd) != EXIT_SUCCESS) {
		errx(1, "Failed to mmap /dev/pmem");
	}
    mmiobase = (global_map + mmiophys);
#endif
}

void
devinit()
{
	u32 val;
	/* force wake. */
    if (accessor == 0)
    {
        I915_WRITE(0xa18c, 1);
    } else {
        io_I915_WRITE(0xa18c, 1);
    }
	gtt_poll(0x130090, 1, 1);
}

void print_help(void)
{
    printf("Intel Video BIOS Tool V1.0\n");
    printf("Arguments: [-f] [-io] [-vbios vbios.rom]\n");
    printf("\t-f = Use fake ID\n");
    printf("\t-io = Use other IO read (reg based)\n");
    printf("\t-vbios vbios.rom = Read Video BIOS ROM from file\n");
    printf("\t-verbose = Do verbose output\n");
}

int main(int argc, char *argv[])
{
    char *vbiosname = NULL;

    i915 = calloc(1, sizeof(*i915));
	i915->dev_private = calloc(1, sizeof(*i915->dev_private));
 	/* until we do a bit more w/ coccinelle */
	i915->dev_private->dev = i915;

	for(argc--, argv++; argc; argc--, argv++) {
		if (argv[0][0] != '-')
			break;
		else if (!strcmp(argv[0], "-f"))
			dofake++;
        else if (!strcmp(argv[0], "-io"))
            accessor = 1;
        else if (!strcmp(argv[0], "-verbose"))
            verbose = 1;
        else if (!strcmp(argv[0], "-vbios")) {
            vbiosname = malloc(strlen(argv[1]));
            strncpy(vbiosname, argv[1], strlen(argv[1]));
        } else {
            print_help();
            return(0);
        }
	}

    iopl(3);

    if (vbiosname) {
        FILE *fd;

        if (verbose)
            printf("VBIOS file name: %s\n", vbiosname);
        
        /* size it later */
        fd = fopen(vbiosname, "rb");
        fseek(fd, 0, SEEK_END);
        bios_image_size = ftell(fd);
        fseek(fd, 0, SEEK_SET);
        bios_image = malloc(bios_image_size);
        fread(bios_image, 1, bios_image_size, fd);
        fclose(fd);
    }

	if (dofake) {
		i915->pdev = &fake;
		if (!find_idlist(i915, vendor, device))
			errx(1, "can't find fake device in pciidlist");
	} else {
		if (!pci_dev_find(i915))
			errx(1, "No supported VGA device found\n");
	}

	/* get the base address for the mmio indirection registers -- BAR 2 */
	addrport = i915->pdev->base_addr[4] & ~3;
	dataport = addrport + 4;
	printf("Addrport is at %x, dataport at %x\n", addrport, dataport);
	/* get the base of the mmio space */
	mmiophys = i915->pdev->base_addr[0] & ~0xf;
	mmiosize = i915->pdev->size[0];
	printf("phys base is %#x, size %d\n", mmiophys, mmiosize);
    mapit();
	devinit();
	/* we should use ioperm but hey ... it's had troubles */
	intel_setup_bios(i915);
	if (bios_image)
		intel_parse_bios(i915);
}
