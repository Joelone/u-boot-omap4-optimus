#include <common.h>
#include <watchdog.h>
#include <command.h>
#include <image.h>
#include <malloc.h>
#include <zlib.h>
#include <bzlib.h>
#include <environment.h>
#include <fastboot.h>
#include <asm/byteorder.h>
#include <mmc.h>
#include <asm/io.h>

 
 extern int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

#if (CONFIG_COMMANDS & CFG_CMD_DATE) || defined(CONFIG_TIMESTAMP)
#include <rtc.h>
#endif

#ifdef CFG_HUSH_PARSER
#include <hush.h>
#endif

#ifdef CONFIG_SHOW_BOOT_PROGRESS
# include <status_led.h>
# define SHOW_BOOT_PROGRESS(arg)	show_boot_progress(arg)
#else
# define SHOW_BOOT_PROGRESS(arg)
#endif

#ifdef CFG_INIT_RAM_LOCK
#include <asm/cache.h>
#endif

#ifdef CONFIG_LOGBUFFER
#include <logbuff.h>
#endif

#ifdef CONFIG_HAS_DATAFLASH
#include <dataflash.h>
#endif

#include <bootimg.h>

#if defined(CONFIG_HW_WATCHDOG) || defined(CONFIG_WATCHDOG)
# define CHUNKSZ (64 * 1024)
#endif

int  gunzip (void *, int, unsigned char *, unsigned long *);

static void *zalloc(void *, unsigned, unsigned);
static void zfree(void *, void *, unsigned);

#if (CONFIG_COMMANDS & CFG_CMD_IMI)
static int image_info (unsigned long addr);
#endif

#if (CONFIG_COMMANDS & CFG_CMD_IMLS)
#include <flash.h>
extern flash_info_t flash_info[CFG_MAX_FLASH_BANKS]; 
static int do_imls (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
#endif

static void print_type (image_header_t *hdr);

#ifdef __I386__
image_header_t *fake_header(image_header_t *hdr, void *ptr, int size);
#endif

#define DIE_ID_REG_BASE		(OMAP44XX_L4_IO_BASE + 0x2000)
#define DIE_ID_REG_OFFSET		0x200

typedef void boot_os_Fcn (cmd_tbl_t *cmdtp, int flag,
			  int	argc, char *argv[],
			  ulong	addr,		
			  ulong	*len_ptr,	
			  int	verify);	

#ifdef	DEBUG
extern int do_bdinfo ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
#endif

#ifdef CONFIG_PPC
static boot_os_Fcn do_bootm_linux;
#else
extern boot_os_Fcn do_bootm_linux;
#endif
#ifdef CONFIG_SILENT_CONSOLE
static void fixup_silent_linux (void);
#endif
static boot_os_Fcn do_bootm_netbsd;
static boot_os_Fcn do_bootm_rtems;
#if (CONFIG_COMMANDS & CFG_CMD_ELF)
static boot_os_Fcn do_bootm_vxworks;
static boot_os_Fcn do_bootm_qnxelf;
int do_bootvx ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[] );
int do_bootelf (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[] );
#endif 
#if defined(CONFIG_ARTOS) && defined(CONFIG_PPC)
static boot_os_Fcn do_bootm_artos;
#endif
#ifdef CONFIG_LYNXKDI
static boot_os_Fcn do_bootm_lynxkdi;
extern void lynxkdi_boot( image_header_t * );
#endif

image_header_t header;

ulong load_addr = CFG_LOAD_ADDR;		

#ifndef CFG_BOOTM_LEN
#define CFG_BOOTM_LEN	0x800000	
#endif

#define POLYNOMIAL  0x04c11db7L
static u64 crc_table[256];

void gen_crc_table()
{
    register int i, j;
    register u64 crc_accum;

    for(i=0; i<256; i++)
    {
        crc_accum = ((u32) i << 24);
        for(j=0; j<8; j++)
        {
            if(crc_accum & 0x80000000L)
                crc_accum = (crc_accum << 1) ^ POLYNOMIAL;
            else
                crc_accum = (crc_accum << 1);
        }
        crc_table[i] = crc_accum;
    }
}

static u32 update_crc(u32 crc_accum, unsigned char *data_blk_ptr, u32 data_blk_size)
{
    u32 i, j;
    for(j=0; j<data_blk_size; j++)
    {
        i = ((u32)(crc_accum >> 24) ^ *data_blk_ptr++) & 0xff;
        crc_accum = (crc_accum << 8) ^ crc_table[i];
    }
    return crc_accum;
}

int do_bootm (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	iflag;
	ulong	addr;
	ulong	data, len, checksum;
	u32 crc;
	ulong  *len_ptr;
	uint	unc_len = CFG_BOOTM_LEN;
	int	i, verify;
	char	*name, *s;
	int	(*appl)(int, char *[]);
	image_header_t *hdr = &header;
	char buff[128];

	s = getenv ("verify");
	verify = (s && (*s == 'n')) ? 0 : 1;

	if (argc < 2) {
		addr = load_addr;
	} else {
		addr = simple_strtoul(argv[1], NULL, 16);
	}

	SHOW_BOOT_PROGRESS (1);

#ifdef CONFIG_HAS_DATAFLASH
	if (addr_dataflash(addr)){
		read_dataflash(addr, sizeof(image_header_t), (char *)&header);
	} else
#endif
	memmove (&header, (char *)addr, sizeof(image_header_t));

	if (ntohl(hdr->ih_magic) != IH_MAGIC) {
#ifdef __I386__	
		if (fake_header(hdr, (void*)addr, -1) != NULL) {
			
			addr -= sizeof(image_header_t);

			verify = 0;
		} else
#endif	
	    {
		puts ("Bad Magic Number\n");
		SHOW_BOOT_PROGRESS (-1);
		return 1;
	    }
	}
	SHOW_BOOT_PROGRESS (2);

	data = (ulong)&header;
	len  = sizeof(image_header_t);

	checksum = ntohl(hdr->ih_hcrc);
	hdr->ih_hcrc = 0;

    gen_crc_table();
    crc = update_crc( 0, (unsigned char *)0x81000000, ntohl(hdr->ih_size) );
	sprintf(buff,"%s CRC=0x%x ", getenv("bootargs"), crc);
	setenv("bootargs", buff);
	puts (buff);
	puts ("\n");

	if (crc32 (0, (unsigned char *)data, len) != checksum) {
		puts ("Bad Header Checksum\n");
		SHOW_BOOT_PROGRESS (-2);
		return 1;
	}
	SHOW_BOOT_PROGRESS (3);

	print_image_hdr ((image_header_t *)addr);

	data = addr + sizeof(image_header_t);
	len  = ntohl(hdr->ih_size);

#ifdef CONFIG_HAS_DATAFLASH
	if (addr_dataflash(addr)){
		read_dataflash(data, len, (char *)CFG_LOAD_ADDR);
		data = CFG_LOAD_ADDR;
	}
#endif

	if (verify) {
		puts ("   Verifying Checksum ... ");

		puts ("OK\n");
	}
	SHOW_BOOT_PROGRESS (4);

	len_ptr = (ulong *)data;

#if defined(__PPC__)
	if (hdr->ih_arch != IH_CPU_PPC)
#elif defined(__ARM__)
	if (hdr->ih_arch != IH_CPU_ARM)
#elif defined(__I386__)
	if (hdr->ih_arch != IH_CPU_I386)
#elif defined(__mips__)
	if (hdr->ih_arch != IH_CPU_MIPS)
#elif defined(__nios__)
	if (hdr->ih_arch != IH_CPU_NIOS)
#elif defined(__M68K__)
	if (hdr->ih_arch != IH_CPU_M68K)
#elif defined(__microblaze__)
	if (hdr->ih_arch != IH_CPU_MICROBLAZE)
#elif defined(__nios2__)
	if (hdr->ih_arch != IH_CPU_NIOS2)
#else
# error Unknown CPU type
#endif
	{
		printf ("Unsupported Architecture 0x%x\n", hdr->ih_arch);
		SHOW_BOOT_PROGRESS (-4);
		return 1;
	}
	SHOW_BOOT_PROGRESS (5);

	switch (hdr->ih_type) {
	case IH_TYPE_STANDALONE:
		name = "Standalone Application";
		
		if (argc > 2) {
			hdr->ih_load = simple_strtoul(argv[2], NULL, 16);
		}
		break;
	case IH_TYPE_KERNEL:
		name = "Kernel Image";
		break;
	case IH_TYPE_MULTI:
		name = "Multi-File Image";
		len  = ntohl(len_ptr[0]);
		
		data += 8; 
		for (i=1; len_ptr[i]; ++i)
			data += 4;
		break;
	default: printf ("Wrong Image Type for %s command\n", cmdtp->name);
		SHOW_BOOT_PROGRESS (-5);
		return 1;
	}
	SHOW_BOOT_PROGRESS (6);

	iflag = disable_interrupts();

#ifdef CONFIG_AMIGAONEG3SE

	icache_disable();
	invalidate_l1_instruction_cache();
	flush_data_cache();
	dcache_disable();
#endif

	switch (hdr->ih_comp) {
	case IH_COMP_NONE:
		if(ntohl(hdr->ih_load) == addr) {
			printf ("   XIP %s ... ", name);
		} else {
#if defined(CONFIG_HW_WATCHDOG) || defined(CONFIG_WATCHDOG)
			size_t l = len;
			void *to = (void *)ntohl(hdr->ih_load);
			void *from = (void *)data;

			printf ("   Loading %s ... ", name);

			while (l > 0) {
				size_t tail = (l > CHUNKSZ) ? CHUNKSZ : l;
				WATCHDOG_RESET();
				memmove (to, from, tail);
				to += tail;
				from += tail;
				l -= tail;
			}
#else	
			memmove ((void *) ntohl(hdr->ih_load), (uchar *)data, len);
#endif	
		}
		break;
	case IH_COMP_GZIP:
		printf ("   Uncompressing %s ... ", name);
		if (gunzip ((void *)ntohl(hdr->ih_load), unc_len,
			    (uchar *)data, &len) != 0) {
			puts ("GUNZIP ERROR - must RESET board to recover\n");
			SHOW_BOOT_PROGRESS (-6);
			do_reset (cmdtp, flag, argc, argv);
		}
		break;
#ifdef CONFIG_BZIP2
	case IH_COMP_BZIP2:
		printf ("   Uncompressing %s ... ", name);

		i = BZ2_bzBuffToBuffDecompress ((char*)ntohl(hdr->ih_load),
						&unc_len, (char *)data, len,
						CFG_MALLOC_LEN < (4096 * 1024), 0);
		if (i != BZ_OK) {
			printf ("BUNZIP2 ERROR %d - must RESET board to recover\n", i);
			SHOW_BOOT_PROGRESS (-6);
			udelay(100000);
			do_reset (cmdtp, flag, argc, argv);
		}
		break;
#endif 
	default:
		if (iflag)
			enable_interrupts();
		printf ("Unimplemented compression type %d\n", hdr->ih_comp);
		SHOW_BOOT_PROGRESS (-7);
		return 1;
	}
	puts ("OK\n");
	SHOW_BOOT_PROGRESS (7);

	switch (hdr->ih_type) {
	case IH_TYPE_STANDALONE:
		if (iflag)
			enable_interrupts();

		if (((s = getenv("autostart")) != NULL) && (strcmp(s,"no") == 0)) {
			char buf[32];
			sprintf(buf, "%lX", len);
			setenv("filesize", buf);
			return 0;
		}
		appl = (int (*)(int, char *[]))ntohl(hdr->ih_ep);
		(*appl)(argc-1, &argv[1]);
		return 0;
	case IH_TYPE_KERNEL:
	case IH_TYPE_MULTI:
		
		break;
	default:
		if (iflag)
			enable_interrupts();
		printf ("Can't boot image type %d\n", hdr->ih_type);
		SHOW_BOOT_PROGRESS (-8);
		return 1;
	}
	SHOW_BOOT_PROGRESS (8);

	switch (hdr->ih_os) {
	default:			
	case IH_OS_LINUX:
#ifdef CONFIG_SILENT_CONSOLE
	    fixup_silent_linux();
#endif
	    do_bootm_linux  (cmdtp, flag, argc, argv,
			     addr, len_ptr, verify);
	    break;
	case IH_OS_NETBSD:
	    do_bootm_netbsd (cmdtp, flag, argc, argv,
			     addr, len_ptr, verify);
	    break;

#ifdef CONFIG_LYNXKDI
	case IH_OS_LYNXOS:
	    do_bootm_lynxkdi (cmdtp, flag, argc, argv,
			     addr, len_ptr, verify);
	    break;
#endif

	case IH_OS_RTEMS:
	    do_bootm_rtems (cmdtp, flag, argc, argv,
			     addr, len_ptr, verify);
	    break;

#if (CONFIG_COMMANDS & CFG_CMD_ELF)
	case IH_OS_VXWORKS:
	    do_bootm_vxworks (cmdtp, flag, argc, argv,
			      addr, len_ptr, verify);
	    break;
	case IH_OS_QNX:
	    do_bootm_qnxelf (cmdtp, flag, argc, argv,
			      addr, len_ptr, verify);
	    break;
#endif 
#ifdef CONFIG_ARTOS
	case IH_OS_ARTOS:
	    do_bootm_artos  (cmdtp, flag, argc, argv,
			     addr, len_ptr, verify);
	    break;
#endif
	}

	SHOW_BOOT_PROGRESS (-9);
#ifdef DEBUG
	puts ("\n## Control returned to monitor - resetting...\n");
	do_reset (cmdtp, flag, argc, argv);
#endif
	return 1;
}

U_BOOT_CMD(
 	bootm,	CFG_MAXARGS,	1,	do_bootm,
 	"bootm   - boot application image from memory\n",
 	"[addr [arg ...]]\n    - boot application image stored in memory\n"
 	"\tpassing arguments 'arg ...'; when booting a Linux kernel,\n"
 	"\t'arg' can be the address of an initrd image\n"
);

#ifdef CONFIG_SILENT_CONSOLE
static void
fixup_silent_linux ()
{
	DECLARE_GLOBAL_DATA_PTR;
	char buf[256], *start, *end;
	char *cmdline = getenv ("bootargs");

	if (!(gd->flags & GD_FLG_SILENT))
		return;

	debug ("before silent fix-up: %s\n", cmdline);
	if (cmdline) {
		if ((start = strstr (cmdline, "console=")) != NULL) {
			end = strchr (start, ' ');
			strncpy (buf, cmdline, (start - cmdline + 8));
			if (end)
				strcpy (buf + (start - cmdline + 8), end);
			else
				buf[start - cmdline + 8] = '\0';
		} else {
			strcpy (buf, cmdline);
			strcat (buf, " console=");
		}
	} else {
		strcpy (buf, "console=");
	}

	setenv ("bootargs", buf);
	debug ("after silent fix-up: %s\n", buf);
}
#endif 

#ifdef CONFIG_PPC
static void
do_bootm_linux (cmd_tbl_t *cmdtp, int flag,
		int	argc, char *argv[],
		ulong	addr,
		ulong	*len_ptr,
		int	verify)
{
	DECLARE_GLOBAL_DATA_PTR;

	ulong	sp;
	ulong	len, checksum;
	ulong	initrd_start, initrd_end;
	ulong	cmd_start, cmd_end;
	ulong	initrd_high;
	ulong	data;
	int	initrd_copy_to_ram = 1;
	char    *cmdline;
	char	*s;
	bd_t	*kbd;
	void	(*kernel)(bd_t *, ulong, ulong, ulong, ulong);
	image_header_t *hdr = &header;

	if ((s = getenv ("initrd_high")) != NULL) {

		initrd_high = simple_strtoul(s, NULL, 16);
		if (initrd_high == ~0)
			initrd_copy_to_ram = 0;
	} else {	
		initrd_high = ~0;
	}

#ifdef CONFIG_LOGBUFFER
	kbd=gd->bd;
	
	if (initrd_high < (kbd->bi_memsize-LOGBUFF_LEN-LOGBUFF_OVERHEAD))
		initrd_high = kbd->bi_memsize-LOGBUFF_LEN-LOGBUFF_OVERHEAD;
	debug ("## Logbuffer at 0x%08lX ", kbd->bi_memsize-LOGBUFF_LEN);
#endif

	asm( "mr %0,1": "=r"(sp) : );

	debug ("## Current stack ends at 0x%08lX ", sp);

	sp -= 2048;		
	if (sp > CFG_BOOTMAPSZ)
		sp = CFG_BOOTMAPSZ;
	sp &= ~0xF;

	debug ("=> set upper limit to 0x%08lX\n", sp);

	cmdline = (char *)((sp - CFG_BARGSIZE) & ~0xF);
	kbd = (bd_t *)(((ulong)cmdline - sizeof(bd_t)) & ~0xF);

	if ((s = getenv("bootargs")) == NULL)
		s = "";

	strcpy (cmdline, s);

	cmd_start    = (ulong)&cmdline[0];
	cmd_end      = cmd_start + strlen(cmdline);

	*kbd = *(gd->bd);

#ifdef	DEBUG
	printf ("## cmdline at 0x%08lX ... 0x%08lX\n", cmd_start, cmd_end);

	do_bdinfo (NULL, 0, 0, NULL);
#endif

	if ((s = getenv ("clocks_in_mhz")) != NULL) {
		
		kbd->bi_intfreq /= 1000000L;
		kbd->bi_busfreq /= 1000000L;
#if defined(CONFIG_MPC8220)
	kbd->bi_inpfreq /= 1000000L;
	kbd->bi_pcifreq /= 1000000L;
	kbd->bi_pevfreq /= 1000000L;
	kbd->bi_flbfreq /= 1000000L;
	kbd->bi_vcofreq /= 1000000L;
#endif
#if defined(CONFIG_8260) || defined(CONFIG_MPC8560)
		kbd->bi_cpmfreq /= 1000000L;
		kbd->bi_brgfreq /= 1000000L;
		kbd->bi_sccfreq /= 1000000L;
		kbd->bi_vco     /= 1000000L;
#endif 
#if defined(CONFIG_MPC5xxx)
		kbd->bi_ipbfreq /= 1000000L;
		kbd->bi_pcifreq /= 1000000L;
#endif 
	}

	kernel = (void (*)(bd_t *, ulong, ulong, ulong, ulong))hdr->ih_ep;

	if (argc >= 3) {
		SHOW_BOOT_PROGRESS (9);

		addr = simple_strtoul(argv[2], NULL, 16);

		printf ("## Loading RAMDisk Image at %08lx ...\n", addr);

		memmove (&header, (char *)addr, sizeof(image_header_t));

		if (hdr->ih_magic  != IH_MAGIC) {
			puts ("Bad Magic Number\n");
			SHOW_BOOT_PROGRESS (-10);
			do_reset (cmdtp, flag, argc, argv);
		}

		data = (ulong)&header;
		len  = sizeof(image_header_t);

		checksum = hdr->ih_hcrc;
		hdr->ih_hcrc = 0;

		if (crc32 (0, (char *)data, len) != checksum) {
			puts ("Bad Header Checksum\n");
			SHOW_BOOT_PROGRESS (-11);
			do_reset (cmdtp, flag, argc, argv);
		}

		SHOW_BOOT_PROGRESS (10);

		print_image_hdr (hdr);

		data = addr + sizeof(image_header_t);
		len  = hdr->ih_size;

		if (verify) {
			ulong csum = 0;
#if defined(CONFIG_HW_WATCHDOG) || defined(CONFIG_WATCHDOG)
			ulong cdata = data, edata = cdata + len;
#endif	

			puts ("   Verifying Checksum ... ");
#if 0
#if defined(CONFIG_HW_WATCHDOG) || defined(CONFIG_WATCHDOG)

			while (cdata < edata) {
				ulong chunk = edata - cdata;

				if (chunk > CHUNKSZ)
					chunk = CHUNKSZ;
				csum = crc32 (csum, (char *)cdata, chunk);
				cdata += chunk;

				WATCHDOG_RESET();
			}
#else	
			csum = crc32 (0, (char *)data, len);
#endif	

			if (csum != hdr->ih_dcrc) {
				puts ("Bad Data CRC\n");
				SHOW_BOOT_PROGRESS (-12);
				do_reset (cmdtp, flag, argc, argv);
			}
#endif
			puts ("OK\n");
		}

		SHOW_BOOT_PROGRESS (11);

		if ((hdr->ih_os   != IH_OS_LINUX)	||
		    (hdr->ih_arch != IH_CPU_PPC)	||
		    (hdr->ih_type != IH_TYPE_RAMDISK)	) {
			puts ("No Linux PPC Ramdisk Image\n");
			SHOW_BOOT_PROGRESS (-13);
			do_reset (cmdtp, flag, argc, argv);
		}

	} else if ((hdr->ih_type==IH_TYPE_MULTI) && (len_ptr[1])) {
		u_long tail    = ntohl(len_ptr[0]) % 4;
		int i;

		SHOW_BOOT_PROGRESS (13);

		data = (ulong)(&len_ptr[2]);
		
		for (i=1; len_ptr[i]; ++i)
			data += 4;
		
		data += ntohl(len_ptr[0]);
		if (tail) {
			data += 4 - tail;
		}

		len   = ntohl(len_ptr[1]);

	} else {

		SHOW_BOOT_PROGRESS (14);

		len = data = 0;
	}

	if (!data) {
		debug ("No initrd\n");
	}

	if (data) {
	    if (!initrd_copy_to_ram) {	
		initrd_start = data;
		initrd_end = initrd_start + len;
	    } else {
		initrd_start  = (ulong)kbd - len;
		initrd_start &= ~(4096 - 1);	

		if (initrd_high) {
			ulong nsp;

			asm( "mr %0,1": "=r"(nsp) : );
			nsp -= 2048;		
			nsp &= ~0xF;
			if (nsp > initrd_high)	
				nsp = initrd_high;
			nsp -= len;
			nsp &= ~(4096 - 1);	
			if (nsp >= sp)
				initrd_start = nsp;
		}

		SHOW_BOOT_PROGRESS (12);

		debug ("## initrd at 0x%08lX ... 0x%08lX (len=%ld=0x%lX)\n",
			data, data + len - 1, len, len);

		initrd_end    = initrd_start + len;
		printf ("   Loading Ramdisk to %08lx, end %08lx ... ",
			initrd_start, initrd_end);
#if defined(CONFIG_HW_WATCHDOG) || defined(CONFIG_WATCHDOG)
		{
			size_t l = len;
			void *to = (void *)initrd_start;
			void *from = (void *)data;

			while (l > 0) {
				size_t tail = (l > CHUNKSZ) ? CHUNKSZ : l;
				WATCHDOG_RESET();
				memmove (to, from, tail);
				to += tail;
				from += tail;
				l -= tail;
			}
		}
#else	
		memmove ((void *)initrd_start, (void *)data, len);
#endif	
		puts ("OK\n");
	    }
	} else {
		initrd_start = 0;
		initrd_end = 0;
	}

	debug ("## Transferring control to Linux (at address %08lx) ...\n",
		(ulong)kernel);

	SHOW_BOOT_PROGRESS (15);

#if defined(CFG_INIT_RAM_LOCK) && !defined(CONFIG_E500)
	unlock_ram_in_cache();
#endif

	(*kernel) (kbd, initrd_start, initrd_end, cmd_start, cmd_end);
}
#endif 

static void
do_bootm_netbsd (cmd_tbl_t *cmdtp, int flag,
		int	argc, char *argv[],
		ulong	addr,
		ulong	*len_ptr,
		int	verify)
{
	DECLARE_GLOBAL_DATA_PTR;

	image_header_t *hdr = &header;

	void	(*loader)(bd_t *, image_header_t *, char *, char *);
	image_header_t *img_addr;
	char     *consdev;
	char     *cmdline;

	img_addr = 0;
	if ((hdr->ih_type==IH_TYPE_MULTI) && (len_ptr[1]))
		img_addr = (image_header_t *) addr;

	consdev = "";
#if   defined (CONFIG_8xx_CONS_SMC1)
	consdev = "smc1";
#elif defined (CONFIG_8xx_CONS_SMC2)
	consdev = "smc2";
#elif defined (CONFIG_8xx_CONS_SCC2)
	consdev = "scc2";
#elif defined (CONFIG_8xx_CONS_SCC3)
	consdev = "scc3";
#endif

	if (argc > 2) {
		ulong len;
		int   i;

		for (i=2, len=0 ; i<argc ; i+=1)
			len += strlen (argv[i]) + 1;
		cmdline = malloc (len);

		for (i=2, len=0 ; i<argc ; i+=1) {
			if (i > 2)
				cmdline[len++] = ' ';
			strcpy (&cmdline[len], argv[i]);
			len += strlen (argv[i]);
		}
	} else if ((cmdline = getenv("bootargs")) == NULL) {
		cmdline = "";
	}

	loader = (void (*)(bd_t *, image_header_t *, char *, char *)) hdr->ih_ep;

	printf ("## Transferring control to NetBSD stage-2 loader (at address %08lx) ...\n",
		(ulong)loader);

	SHOW_BOOT_PROGRESS (15);

	(*loader) (gd->bd, img_addr, consdev, cmdline);
}

#if defined(CONFIG_ARTOS) && defined(CONFIG_PPC)

extern uchar (*env_get_char)(int);

static void
do_bootm_artos (cmd_tbl_t *cmdtp, int flag,
		int	argc, char *argv[],
		ulong	addr,
		ulong	*len_ptr,
		int	verify)
{
	DECLARE_GLOBAL_DATA_PTR;
	ulong top;
	char *s, *cmdline;
	char **fwenv, **ss;
	int i, j, nxt, len, envno, envsz;
	bd_t *kbd;
	void (*entry)(bd_t *bd, char *cmdline, char **fwenv, ulong top);
	image_header_t *hdr = &header;

#ifdef CONFIG_PPC
	
	asm volatile ("mr %0,1" : "=r"(top) );
#endif
	debug ("## Current stack ends at 0x%08lX ", top);

	top -= 2048;		
	if (top > CFG_BOOTMAPSZ)
		top = CFG_BOOTMAPSZ;
	top &= ~0xF;

	debug ("=> set upper limit to 0x%08lX\n", top);

	if ((s = getenv("abootargs")) == NULL && (s = getenv("bootargs")) == NULL)
		s = "";

	len = strlen(s);
	top = (top - (len + 1)) & ~0xF;
	cmdline = (char *)top;
	debug ("## cmdline at 0x%08lX ", top);
	strcpy(cmdline, s);

	top = (top - sizeof(bd_t)) & ~0xF;
	debug ("## bd at 0x%08lX ", top);
	kbd = (bd_t *)top;
	memcpy(kbd, gd->bd, sizeof(bd_t));

	envno = 0;
	envsz = 0;
	for (i = 0; env_get_char(i) != '\0'; i = nxt + 1) {
		for (nxt = i; env_get_char(nxt) != '\0'; ++nxt)
			;
		envno++;
		envsz += (nxt - i) + 1;	
	}
	envno++;	
	debug ("## %u envvars total size %u ", envno, envsz);

	top = (top - sizeof(char **)*envno) & ~0xF;
	fwenv = (char **)top;
	debug ("## fwenv at 0x%08lX ", top);

	top = (top - envsz) & ~0xF;
	s = (char *)top;
	ss = fwenv;

	for (i = 0; env_get_char(i) != '\0'; i = nxt + 1) {
		for (nxt = i; env_get_char(nxt) != '\0'; ++nxt)
			;
		*ss++ = s;
		for (j = i; j < nxt; ++j)
			*s++ = env_get_char(j);
		*s++ = '\0';
	}
	*ss++ = NULL;	

	entry = (void (*)(bd_t *, char *, char **, ulong))ntohl(hdr->ih_ep);
	(*entry)(kbd, cmdline, fwenv, top);
}
#endif

#if (CONFIG_COMMANDS & CFG_CMD_BOOTD)
int do_bootd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int rcode = 0;
#ifndef CFG_HUSH_PARSER
	if (run_command (getenv ("bootcmd"), flag) < 0) rcode = 1;
#else
	if (parse_string_outer(getenv("bootcmd"),
		FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP) != 0 ) rcode = 1;
#endif
	return rcode;
}

U_BOOT_CMD(
 	boot,	1,	1,	do_bootd,
 	"boot    - boot default, i.e., run 'bootcmd'\n",
	NULL
);

U_BOOT_CMD(
 	bootd, 1,	1,	do_bootd,
 	"bootd   - boot default, i.e., run 'bootcmd'\n",
	NULL
);

#endif

#if (CONFIG_COMMANDS & CFG_CMD_IMI)
int do_iminfo ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int	arg;
	ulong	addr;
	int     rcode=0;

	if (argc < 2) {
		return image_info (load_addr);
	}

	for (arg=1; arg <argc; ++arg) {
		addr = simple_strtoul(argv[arg], NULL, 16);
		if (image_info (addr) != 0) rcode = 1;
	}
	return rcode;
}

static int image_info (ulong addr)
{
	ulong	data, len, checksum;
	image_header_t *hdr = &header;

	printf ("\n## Checking Image at %08lx ...\n", addr);

	memmove (&header, (char *)addr, sizeof(image_header_t));

	if (ntohl(hdr->ih_magic) != IH_MAGIC) {
		puts ("   Bad Magic Number\n");
		return 1;
	}

	data = (ulong)&header;
	len  = sizeof(image_header_t);

	checksum = ntohl(hdr->ih_hcrc);
	hdr->ih_hcrc = 0;

	if (crc32 (0, (unsigned char *)data, len) != checksum) {
		puts ("   Bad Header Checksum\n");
		return 1;
	}

	print_image_hdr ((image_header_t *)addr);

	data = addr + sizeof(image_header_t);
	len  = ntohl(hdr->ih_size);

	puts ("   Verifying Checksum ... ");

	puts ("OK\n");
	return 0;
}

U_BOOT_CMD(
	iminfo,	CFG_MAXARGS,	1,	do_iminfo,
	"iminfo  - print header information for application image\n",
	"addr [addr ...]\n"
	"    - print header information for application image starting at\n"
	"      address 'addr' in memory; this includes verification of the\n"
	"      image contents (magic number, header and payload checksums)\n"
);

#endif	

#if (CONFIG_COMMANDS & CFG_CMD_IMLS)

int do_imls (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	flash_info_t *info;
	int i, j;
	image_header_t *hdr;
	ulong data, len, checksum;

	for (i=0, info=&flash_info[0]; i<CFG_MAX_FLASH_BANKS; ++i, ++info) {
		if (info->flash_id == FLASH_UNKNOWN)
			goto next_bank;
		for (j=0; j<CFG_MAX_FLASH_SECT; ++j) {

			if (!(hdr=(image_header_t *)info->start[j]) ||
			    (ntohl(hdr->ih_magic) != IH_MAGIC))
				goto next_sector;

			memmove (&header, (char *)hdr, sizeof(image_header_t));

			checksum = ntohl(header.ih_hcrc);
			header.ih_hcrc = 0;

			if (crc32 (0, (unsigned char *)&header, sizeof(image_header_t))
			    != checksum)
				goto next_sector;

			printf ("Image at %08lX:\n", (ulong)hdr);
			print_image_hdr( hdr );

			data = (ulong)hdr + sizeof(image_header_t);
			len  = ntohl(hdr->ih_size);

			puts ("   Verifying Checksum ... ");

			puts ("OK\n");
next_sector:		;
		}
next_bank:	;
	}

	return (0);
}

U_BOOT_CMD(
	imls,	1,		1,	do_imls,
	"imls    - list all images found in flash\n",
	"\n"
	"    - Prints information about all images found at sector\n"
	"      boundaries in flash.\n"
);
#endif	

void
print_image_hdr (image_header_t *hdr)
{
#if (CONFIG_COMMANDS & CFG_CMD_DATE) || defined(CONFIG_TIMESTAMP)
	time_t timestamp = (time_t)ntohl(hdr->ih_time);
	struct rtc_time tm;
#endif

	printf ("   Image Name:   %.*s\n", IH_NMLEN, hdr->ih_name);
#if (CONFIG_COMMANDS & CFG_CMD_DATE) || defined(CONFIG_TIMESTAMP)
	to_tm (timestamp, &tm);
	printf ("   Created:      %4d-%02d-%02d  %2d:%02d:%02d UTC\n",
		tm.tm_year, tm.tm_mon, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
#endif	
	puts ("   Image Type:   "); print_type(hdr);
	printf ("\n   Data Size:    %d Bytes = ", ntohl(hdr->ih_size));
	print_size (ntohl(hdr->ih_size), "\n");
	printf ("   Load Address: %08x\n"
		"   Entry Point:  %08x\n",
		 ntohl(hdr->ih_load), ntohl(hdr->ih_ep));

	printf ("   Data CRC Checksum:   0x%x\n", ntohl(hdr->ih_dcrc));

	if (hdr->ih_type == IH_TYPE_MULTI) {
		int i;
		ulong len;
		ulong *len_ptr = (ulong *)((ulong)hdr + sizeof(image_header_t));

		puts ("   Contents:\n");
		for (i=0; (len = ntohl(*len_ptr)); ++i, ++len_ptr) {
			printf ("   Image %d: %8ld Bytes = ", i, len);
			print_size (len, "\n");
		}
	}
}

static void
print_type (image_header_t *hdr)
{
	char *os, *arch, *type, *comp;

	switch (hdr->ih_os) {
	case IH_OS_INVALID:	os = "Invalid OS";		break;
	case IH_OS_NETBSD:	os = "NetBSD";			break;
	case IH_OS_LINUX:	os = "Linux";			break;
	case IH_OS_VXWORKS:	os = "VxWorks";			break;
	case IH_OS_QNX:		os = "QNX";			break;
	case IH_OS_U_BOOT:	os = "U-Boot";			break;
	case IH_OS_RTEMS:	os = "RTEMS";			break;
#ifdef CONFIG_ARTOS
	case IH_OS_ARTOS:	os = "ARTOS";			break;
#endif
#ifdef CONFIG_LYNXKDI
	case IH_OS_LYNXOS:	os = "LynxOS";			break;
#endif
	default:		os = "Unknown OS";		break;
	}

	switch (hdr->ih_arch) {
	case IH_CPU_INVALID:	arch = "Invalid CPU";		break;
	case IH_CPU_ALPHA:	arch = "Alpha";			break;
	case IH_CPU_ARM:	arch = "ARM";			break;
	case IH_CPU_I386:	arch = "Intel x86";		break;
	case IH_CPU_IA64:	arch = "IA64";			break;
	case IH_CPU_MIPS:	arch = "MIPS";			break;
	case IH_CPU_MIPS64:	arch = "MIPS 64 Bit";		break;
	case IH_CPU_PPC:	arch = "PowerPC";		break;
	case IH_CPU_S390:	arch = "IBM S390";		break;
	case IH_CPU_SH:		arch = "SuperH";		break;
	case IH_CPU_SPARC:	arch = "SPARC";			break;
	case IH_CPU_SPARC64:	arch = "SPARC 64 Bit";		break;
	case IH_CPU_M68K:	arch = "M68K"; 			break;
	case IH_CPU_MICROBLAZE:	arch = "Microblaze"; 		break;
	default:		arch = "Unknown Architecture";	break;
	}

	switch (hdr->ih_type) {
	case IH_TYPE_INVALID:	type = "Invalid Image";		break;
	case IH_TYPE_STANDALONE:type = "Standalone Program";	break;
	case IH_TYPE_KERNEL:	type = "Kernel Image";		break;
	case IH_TYPE_RAMDISK:	type = "RAMDisk Image";		break;
	case IH_TYPE_MULTI:	type = "Multi-File Image";	break;
	case IH_TYPE_FIRMWARE:	type = "Firmware";		break;
	case IH_TYPE_SCRIPT:	type = "Script";		break;
	default:		type = "Unknown Image";		break;
	}

	switch (hdr->ih_comp) {
	case IH_COMP_NONE:	comp = "uncompressed";		break;
	case IH_COMP_GZIP:	comp = "gzip compressed";	break;
	case IH_COMP_BZIP2:	comp = "bzip2 compressed";	break;
	default:		comp = "unknown compression";	break;
	}

	printf ("%s %s %s (%s)", arch, os, type, comp);
}

#define	ZALLOC_ALIGNMENT	16

static void *zalloc(void *x, unsigned items, unsigned size)
{
	void *p;

	size *= items;
	size = (size + ZALLOC_ALIGNMENT - 1) & ~(ZALLOC_ALIGNMENT - 1);

	p = malloc (size);

	return (p);
}

static void zfree(void *x, void *addr, unsigned nb)
{
	free (addr);
}

#define HEAD_CRC	2
#define EXTRA_FIELD	4
#define ORIG_NAME	8
#define COMMENT		0x10
#define RESERVED	0xe0

#define DEFLATED	8

int gunzip(void *dst, int dstlen, unsigned char *src, unsigned long *lenp)
{
	z_stream s;
	int r, i, flags;

	i = 10;
	flags = src[3];
	if (src[2] != DEFLATED || (flags & RESERVED) != 0) {
		puts ("Error: Bad gzipped data\n");
		return (-1);
	}
	if ((flags & EXTRA_FIELD) != 0)
		i = 12 + src[10] + (src[11] << 8);
	if ((flags & ORIG_NAME) != 0)
		while (src[i++] != 0)
			;
	if ((flags & COMMENT) != 0)
		while (src[i++] != 0)
			;
	if ((flags & HEAD_CRC) != 0)
		i += 2;
	if (i >= *lenp) {
		puts ("Error: gunzip out of data in header\n");
		return (-1);
	}

	s.zalloc = zalloc;
	s.zfree = zfree;
#if defined(CONFIG_HW_WATCHDOG) || defined(CONFIG_WATCHDOG)
	s.outcb = (cb_func)WATCHDOG_RESET;
#else
	s.outcb = Z_NULL;
#endif	

	r = inflateInit2(&s, -MAX_WBITS);
	if (r != Z_OK) {
		printf ("Error: inflateInit2() returned %d\n", r);
		return (-1);
	}
	s.next_in = src + i;
	s.avail_in = *lenp - i;
	s.next_out = dst;
	s.avail_out = dstlen;
	r = inflate(&s, Z_FINISH);
	if (r != Z_OK && r != Z_STREAM_END) {
		printf ("Error: inflate() returned %d\n", r);
		return (-1);
	}
	*lenp = s.next_out - (unsigned char *) dst;
	inflateEnd(&s);

	return (0);
}

#ifdef CONFIG_BZIP2
void bz_internal_error(int errcode)
{
	printf ("BZIP2 internal error %d\n", errcode);
}
#endif 

static void
do_bootm_rtems (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[],
		ulong addr, ulong *len_ptr, int verify)
{
	DECLARE_GLOBAL_DATA_PTR;
	image_header_t *hdr = &header;
	void	(*entry_point)(bd_t *);

	entry_point = (void (*)(bd_t *)) hdr->ih_ep;

	printf ("## Transferring control to RTEMS (at address %08lx) ...\n",
		(ulong)entry_point);

	SHOW_BOOT_PROGRESS (15);

	(*entry_point ) ( gd->bd );
}

#if (CONFIG_COMMANDS & CFG_CMD_ELF)
static void
do_bootm_vxworks (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[],
		  ulong addr, ulong *len_ptr, int verify)
{
	image_header_t *hdr = &header;
	char str[80];

	sprintf(str, "%x", hdr->ih_ep); 
	setenv("loadaddr", str);
	do_bootvx(cmdtp, 0, 0, NULL);
}

static void
do_bootm_qnxelf (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[],
		 ulong addr, ulong *len_ptr, int verify)
{
	image_header_t *hdr = &header;
	char *local_args[2];
	char str[16];

	sprintf(str, "%x", hdr->ih_ep); 
	local_args[0] = argv[0];
	local_args[1] = str;	
	do_bootelf(cmdtp, 0, 2, local_args);
}
#endif 

#ifdef CONFIG_LYNXKDI
static void
do_bootm_lynxkdi (cmd_tbl_t *cmdtp, int flag,
		 int	argc, char *argv[],
		 ulong	addr,
		 ulong	*len_ptr,
		 int	verify)
{
	lynxkdi_boot( &header );
}

#endif 

  

void
bootimg_print_image_hdr (boot_img_hdr *hdr)
{
#ifdef DEBUG
	int i;
	printf ("   Image magic:   %s\n", hdr->magic);

	printf ("   kernel_size:   0x%x\n", hdr->kernel_size);
	printf ("   kernel_addr:   0x%x\n", hdr->kernel_addr);

	printf ("   rdisk_size:   0x%x\n", hdr->ramdisk_size);
	printf ("   rdisk_addr:   0x%x\n", hdr->ramdisk_addr);

	printf ("   second_size:   0x%x\n", hdr->second_size);
	printf ("   second_addr:   0x%x\n", hdr->second_addr);

	printf ("   tags_addr:   0x%x\n", hdr->tags_addr);
	printf ("   page_size:   0x%x\n", hdr->page_size);

	printf ("   name:      %s\n", hdr->name);
	printf ("   cmdline:   %s%x\n", hdr->cmdline);

	for (i=0;i<8;i++)
		printf ("   id[%d]:   0x%x\n", i, hdr->id[i]);
#endif
}

static unsigned char boothdr[512];

#define ALIGN(n,pagesz) ((n + (pagesz - 1)) & (~(pagesz - 1)))

int do_booti (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned addr;
	char *ptn = "boot";
	int mmcc = -1;
	boot_img_hdr *hdr = (void*) boothdr;
	static char device_serial[38];
	unsigned int val[4] = { 0 };
	unsigned int reg = 0;


	if (argc < 2)
		return -1;

	if (!strcmp(argv[1],"mmc0")) {
		mmcc = 0;
	} else if (!strcmp(argv[1],"mmc1")) {
		mmcc = 1;
	} else {
		addr = simple_strtoul(argv[1], NULL, 16);
	}

	if (argc > 2)
		ptn = argv[2];

	if (mmcc != -1) {
#if (CONFIG_MMC)
		struct fastboot_ptentry *pte;
		unsigned sector;

		pte = fastboot_flash_find_ptn(ptn);
		if (!pte) {
			printf("booti: cannot find '%s' partition\n", ptn);
			goto fail;
		}
		if (mmc_init(mmcc)) {
			printf("mmc%d init failed\n", mmcc);
			goto fail;
		}
		if (mmc_read(mmcc, pte->start, (void*) hdr, 512) != 1) {
			printf("booti: mmc failed to read bootimg header\n");
			goto fail;
		}
		if (memcmp(hdr->magic, BOOT_MAGIC, 8)) {
			printf("booti: bad boot image magic\n");
			goto fail;
		}

		sector = pte->start + (hdr->page_size / 512);
		if (mmc_read(mmcc, sector, (void*) hdr->kernel_addr,
			     hdr->kernel_size) != 1) {
			printf("booti: failed to read kernel\n");
			goto fail;
		}

		sector += ALIGN(hdr->kernel_size, hdr->page_size) / 512;
		if (mmc_read(mmcc, sector, (void*) hdr->ramdisk_addr,
			     hdr->ramdisk_size) != 1) {
			printf("booti: failed to read ramdisk\n");
			goto fail;
		}
#else
		return -1;
#endif
	} else {
		unsigned kaddr, raddr;

		memcpy(hdr, (void*) addr, sizeof(*hdr));

		if (memcmp(hdr->magic, BOOT_MAGIC, 8)) {
			printf("booti: bad boot image magic\n");
			return 1;
		}

		bootimg_print_image_hdr(hdr);

		kaddr = addr + hdr->page_size;
		raddr = kaddr + ALIGN(hdr->kernel_size, hdr->page_size);

		memmove((void*) hdr->kernel_addr, kaddr, hdr->kernel_size);
		memmove((void*) hdr->ramdisk_addr, raddr, hdr->ramdisk_size);
	}

	printf("kernel   @ %08x (%d)\n", hdr->kernel_addr, hdr->kernel_size);
	printf("ramdisk  @ %08x (%d)\n", hdr->ramdisk_addr, hdr->ramdisk_size);

#if (CONFIG_OMAP4_ANDROID_CMD_LINE)
	char serial_str[128];
	unsigned serial_len;

	reg = DIE_ID_REG_BASE + DIE_ID_REG_OFFSET;
	val[0] = __raw_readl(reg);
	val[1] = __raw_readl(reg + 0x8);
	val[2] = __raw_readl(reg + 0xC);
	val[3] = __raw_readl(reg + 0x10);

	serial_len = sprintf(serial_str, " androidboot.serialno=%08X%08X",
			val[1], val[0]);

	if(sizeof(hdr->cmdline) >= (serial_len + strlen(hdr->cmdline) + 1))
		strcat(hdr->cmdline, serial_str);
#endif

	do_booti_linux(hdr);

	puts ("booti: Control returned to monitor - resetting...\n");
	do_reset (cmdtp, flag, argc, argv);
	return 1;

fail:
	return do_fastboot(NULL, 0, 0, NULL);
}

U_BOOT_CMD(
	booti,	3,	1,	do_booti,
	"booti   - boot android bootimg from memory\n",
	"<addr>\n    - boot application image stored in memory\n"
	"\t'addr' should be the address of boot image which is zImage+ramdisk.img\n"
);
