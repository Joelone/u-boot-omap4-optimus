#include <common.h>
#include <asm-arm/arch-omap4/bits.h>
#include <asm-arm/io.h>
#include <asm-arm/arch-omap4/gpio.h>
#include <asm-arm/arch-omap4/cpu.h>
#include <asm-arm/errno.h>
#include <linux/mtd/compat.h>
#include "p2_dss.h"

#ifndef CONFIG_FBCON
#include "font8x8.h"
#include "font8x16.h"
#endif
extern char const lglogo[];
extern char const web_download[];
extern char const web_download_eng[];

#define DEBUG_DSI_VIDEO_MODE

#undef DEBUG

#define LCD_XRES		480
#define LCD_YRES		800

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

   	
#define DSS_BASE                0x58000000
   	
#define DISPC_BASE              0x58001000

#define DSS_SZ_REGS				0x00000200

#define DSI_CLOCK_POLARITY  0   
#define DSI_DATA1_POLARITY  0   
#define DSI_DATA2_POLARITY  0   
#define DSI_CLOCK_LANE      1   
#define DSI_DATA1_LANE      3   
#define DSI_DATA2_LANE      2   
#define DSI_CTRL_PIXEL_SIZE 24  
#define DSI_DIV_REGN        20
#define DSI_DIV_REGM        110
#define DSI_DIV_REGM3       3
#define DSI_DIV_REGM4       10
#define DSI_DIV_LCK_DIV     2
#define DSI_DIV_PCK_DIV     4
#define DSI_DIV_LP_CLK_DIV 12

#if 1	
enum omap_dsi_mode {
	OMAP_DSI_MODE_CMD = 0,
	OMAP_DSI_MODE_VIDEO = 1,
};
#endif

#define DSI_BASE		0x58004000
#define DSI2_BASE		0x58005000
#define CONFIG_OMAP2_DSS_MIN_FCK_PER_PCK 0

static int dss_state = OMAP_DSS_DISPLAY_DISABLED;

struct dsi_reg { u16 idx; };

#define DSI_REG(idx)		((const struct dsi_reg) { idx })

#define DSI_SZ_REGS		SZ_1K

#define DSI_REVISION			DSI_REG(0x0000)
#define DSI_SYSCONFIG			DSI_REG(0x0010)
#define DSI_SYSSTATUS			DSI_REG(0x0014)
#define DSI_IRQSTATUS			DSI_REG(0x0018)
#define DSI_IRQENABLE			DSI_REG(0x001C)
#define DSI_CTRL					DSI_REG(0x0040)
#define DSI_GNQ					DSI_REG(0x0044)
#define DSI_COMPLEXIO_CFG1		DSI_REG(0x0048)
#define DSI_COMPLEXIO_IRQ_STATUS	DSI_REG(0x004C)
#define DSI_COMPLEXIO_IRQ_ENABLE	DSI_REG(0x0050)
#define DSI_CLK_CTRL			DSI_REG(0x0054)
#define DSI_TIMING1				DSI_REG(0x0058)
#define DSI_TIMING2				DSI_REG(0x005C)
#define DSI_VM_TIMING1			DSI_REG(0x0060)
#define DSI_VM_TIMING2			DSI_REG(0x0064)
#define DSI_VM_TIMING3			DSI_REG(0x0068)
#define DSI_CLK_TIMING			DSI_REG(0x006C)
#define DSI_TX_FIFO_VC_SIZE		DSI_REG(0x0070)
#define DSI_RX_FIFO_VC_SIZE		DSI_REG(0x0074)
#define DSI_COMPLEXIO_CFG2		DSI_REG(0x0078)
#define DSI_RX_FIFO_VC_FULLNESS		DSI_REG(0x007C)
#define DSI_VM_TIMING4			DSI_REG(0x0080)
#define DSI_TX_FIFO_VC_EMPTINESS	DSI_REG(0x0084)
#define DSI_VM_TIMING5			DSI_REG(0x0088)
#define DSI_VM_TIMING6			DSI_REG(0x008C)
#define DSI_VM_TIMING7			DSI_REG(0x0090)
#define DSI_STOPCLK_TIMING		DSI_REG(0x0094)
#ifdef CONFIG_ARCH_OMAP4
#define DSI_CTRL2			DSI_REG(0x0098)
#define DSI_VM_TIMING8			DSI_REG(0x009C)
#define DSI_TE_HSYNC_WIDTH(n)		DSI_REG(0x00A0 + (n * 0xC))
#define DSI_TE_VSYNC_WIDTH(n)		DSI_REG(0x00A4 + (n * 0xC))
#define DSI_TE_HSYNC_NUMBER(n)		DSI_REG(0x00A8 + (n * 0xC))
#endif
#define DSI_VC_CTRL(n)			DSI_REG(0x0100 + (n * 0x20))
#define DSI_VC_TE(n)			DSI_REG(0x0104 + (n * 0x20))
#define DSI_VC_LONG_PACKET_HEADER(n)	DSI_REG(0x0108 + (n * 0x20))
#define DSI_VC_LONG_PACKET_PAYLOAD(n)	DSI_REG(0x010C + (n * 0x20))
#define DSI_VC_SHORT_PACKET_HEADER(n)	DSI_REG(0x0110 + (n * 0x20))
#define DSI_VC_IRQSTATUS(n)		DSI_REG(0x0118 + (n * 0x20))
#define DSI_VC_IRQENABLE(n)		DSI_REG(0x011C + (n * 0x20))

#define DSI_DSIPHY_CFG0			DSI_REG(0x200 + 0x0000)
#define DSI_DSIPHY_CFG1			DSI_REG(0x200 + 0x0004)
#define DSI_DSIPHY_CFG2			DSI_REG(0x200 + 0x0008)
#define DSI_DSIPHY_CFG5			DSI_REG(0x200 + 0x0014)

#define DSI_DSIPHY_CFG12		DSI_REG(0x200 + 0x0030)
#define DSI_DSIPHY_CFG14		DSI_REG(0x200 + 0x0038)
#define DSI_DSIPHY_CFG8			DSI_REG(0x200 + 0x0020)
#define DSI_DSIPHY_CFG9			DSI_REG(0x200 + 0x0024)

#define DSI_PLL_CONTROL			DSI_REG(0x300 + 0x0000)
#define DSI_PLL_STATUS			DSI_REG(0x300 + 0x0004)
#define DSI_PLL_GO			DSI_REG(0x300 + 0x0008)
#define DSI_PLL_CONFIGURATION1		DSI_REG(0x300 + 0x000C)
#define DSI_PLL_CONFIGURATION2		DSI_REG(0x300 + 0x0010)
#ifdef CONFIG_ARCH_OMAP4
#define DSI_PLL_CONFIGURATION3		DSI_REG(0x300 + 0x0014)
#define DSI_SSC_CONFIGURATION1		DSI_REG(0x300 + 0x0018)
#define DSI_SSC_CONFIGURATION2		DSI_REG(0x300 + 0x001C)
#define DSI_SSC_CONFIGURATION4		DSI_REG(0x300 + 0x0020)
#endif

#define REG_GET(no, idx, start, end) \
	FLD_GET(dsi_read_reg(no, idx), start, end)

#define REG_FLD_MOD(no, idx, val, start, end) \
	dsi_write_reg(no, idx, FLD_MOD(dsi_read_reg(no, idx), val, start, end))

#define DSICLKCTRLLPCLKDIVISOR    		1<<0| 1<<1| 1<<2 | 1<<3 | 1<<4 | 1<<5 | 1<<6 | 1<<7 | 1<<8 | 1<<9 | 1<<10 | 1<<11 | 1<<12

 
#define DSI_DT_DCS_SHORT_WRITE_0	0x05
#define DSI_DT_DCS_SHORT_WRITE_1	0x15
#define DSI_DT_DCS_READ			0x06
#define DSI_DT_SET_MAX_RET_PKG_SIZE	0x37
#define DSI_DT_NULL_PACKET		0x09
#define DSI_DT_DCS_LONG_WRITE		0x39

#define DSI_DT_RX_ACK_WITH_ERR		0x02
#define DSI_DT_RX_DCS_LONG_READ		0x1c
#define DSI_DT_RX_SHORT_READ_1		0x21
#define DSI_DT_RX_SHORT_READ_2		0x22

#define DSI_PACKED_PIXEL_STREAM_16	0x0e
#define DSI_PACKED_PIXEL_STREAM_18	0x1e
#define DSI_PIXEL_STREAM_3BYTE_18	0x2e
#define DSI_PACKED_PIXEL_STREAM_24	0x3e

#if defined (CONFIG_LGE_P2) 
#define DSI_VC_CMD 0
#define DSI_VC_VIDEO 0
#else
#define DSI_VC_CMD 0
#define DSI_VC_VIDEO 1
#endif

#if CONFIG_ARCH_OMAP4
#define FINT_MAX 2500000
#define FINT_MIN 750000
#define REGN 8
#define REGM 12
#define REGM3 5
#else

#define FINT_MAX 2500000
#define FINT_MIN 500000
#define REGN 7
#define REGM 11
#define REGM3 4
#endif
#define REGN_MAX (1 << REGN)
#define REGM_MAX ((1 << REGM) - 1)
#define REGM3_MAX (1 << REGM3)
#define REGM4 REGM3
#define REGM4_MAX (1 << REGM4)

#define LP_DIV_MAX ((1 << 13) - 1)

extern void volatile  *dss_base;
extern void volatile  *dispc_base;
void volatile  *gpio_base;
void volatile  *dsi_base;
#ifdef CONFIG_ARCH_OMAP4
void volatile  *dsi2_base;
#endif

enum fifo_size {
	DSI_FIFO_SIZE_0		= 0,
	DSI_FIFO_SIZE_32	= 1,
	DSI_FIFO_SIZE_64	= 2,
	DSI_FIFO_SIZE_96	= 3,
	DSI_FIFO_SIZE_128	= 4,
};

enum dsi_vc_mode {
	DSI_VC_MODE_L4 = 0,
	DSI_VC_MODE_VP,
};

#define abs(x) ({				\
		long __x = (x);			\
		(__x < 0) ? -__x : __x;		\
	})

#define cpu_is_omap7xx()		0
#define cpu_is_omap15xx()		0
#define cpu_is_omap16xx()		0
#define cpu_is_omap24xx()		0
#define cpu_is_omap242x()		0
#define cpu_is_omap243x()		0
#define cpu_is_omap34xx()		0
#define cpu_is_omap343x()		0
#define cpu_is_omap3630()		0
#define cpu_is_omap44xx()		1
#define cpu_is_omap443x()		1

#define OMAP1_IO_OFFSET		0x01000000	
#define OMAP2_L4_IO_OFFSET		0xb2000000

#define OMAP1_IO_ADDRESS(pa)	IOMEM((pa) - OMAP1_IO_OFFSET)
#define OMAP2_L4_IO_ADDRESS(pa)	IOMEM((pa) + OMAP2_L4_IO_OFFSET) 

#define cpu_class_is_omap1()	(cpu_is_omap7xx() || cpu_is_omap15xx() || \
				cpu_is_omap16xx())
#define cpu_class_is_omap2()	(cpu_is_omap24xx() || cpu_is_omap34xx() || \
				cpu_is_omap44xx())

static u32 cosmo_config = (OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_ONOFF | OMAP_DSS_LCD_RF);
static int cosmo_acbi = 0; 
static int cosmo_acb = 0; 

static struct omap_video_timings cosmo_panel_timings = {	
	.x_res          = LCD_XRES,
	.y_res          = LCD_YRES,
	.pixel_clock    = 26583,
	.hfp            = 14, 
	.hsw            = 51, 
	.hbp            = 19, 
	.vfp            = 0, 
	.vsw            = 8, 
	.vbp            = 22, 
};  

struct dsi_update_region {
	bool dirty;
	u16 x, y, w, h; 
};

static struct dsi_struct
{
	void volatile	*base;

	struct dsi_clock_info current_cinfo;

	struct {
		enum dsi_vc_mode mode;
		enum fifo_size fifo_size;
		int dest_per;	
	} vc[4];
 
	unsigned pll_locked; 

 	bool framedone_received;
	struct dsi_update_region update_region;
	struct dsi_update_region active_update_region;
 
	enum omap_dss_update_mode user_update_mode;
	enum omap_dss_update_mode update_mode;
	enum omap_dsi_mode dsi_mode;
	bool te_enabled;
	bool use_ext_te;

 
	unsigned long cache_req_pck;
	unsigned long cache_clk_freq;
	struct dsi_clock_info cache_cinfo;

	u32		errors;
	
 	int debug_read;
	int debug_write;
} dsi_2;

static inline void dsi_write_reg(enum dsi lcd_ix,
	const struct dsi_reg idx, u32 val)
{
	(lcd_ix == dsi1) ? __raw_writel(val, DSI_BASE + idx.idx) :
		__raw_writel(val, DSI2_BASE + idx.idx);
}

static inline u32 dsi_read_reg(enum dsi lcd_ix, const struct dsi_reg idx)
{
	if (lcd_ix == dsi1)
		return __raw_readl(DSI_BASE + idx.idx);
	else
		return __raw_readl(DSI2_BASE + idx.idx);
}

static inline void dss_write_reg(const struct dss_reg idx, u32 val)
{
	__raw_writel(val, DSS_BASE + idx.idx);
}

static inline u32 dss_read_reg(const struct dss_reg idx)
{
	return __raw_readl(DSS_BASE + idx.idx);
}

static inline int wait_for_bit_change_delay(enum dsi lcd_ix,
		const struct dsi_reg idx, int bitnum, int value, int delay)
{
	int t = 100000;

	while (REG_GET(lcd_ix, idx, bitnum, bitnum) != value) {
		udelay(delay);
		if (--t == 0)
			return !value;
	}

	return value;
}

static inline int wait_for_bit_change(enum dsi lcd_ix,
	const struct dsi_reg idx, int bitnum, int value)
{
	int t = 100000;

	while (REG_GET(lcd_ix, idx, bitnum, bitnum) != value) {
		if (--t == 0)
			return !value;
	}

	return value;
}

unsigned long dss_clk_get_rate(enum dss_clock clk)
{
	switch (clk) {
		case DSS_CLK_ICK:
			return 166000000;
		case DSS_CLK_FCK1:
			return 153600000;
		case DSS_CLK_FCK2:
			return 0;
		case DSS_CLK_54M:
			return 54000000;
		case DSS_CLK_96M:
			return 96000000;
		}

	return 0;

}
 
static inline int dsi_if_enable(enum dsi lcd_ix, bool enable)
{
	printf("dsi_if_enable(%d, %d)\n", lcd_ix, enable);

	enable = enable ? 1 : 0;
	REG_FLD_MOD(lcd_ix, DSI_CTRL, enable, 0, 0); 

	if (wait_for_bit_change(lcd_ix, DSI_CTRL, 0, enable) != enable) {
			printf("Failed to set dsi_if_enable to %d\n", enable);
			return -EIO;
	}

	return 0;
}

unsigned long dsi_get_dsi1_pll_rate(enum dsi lcd_ix)
{
	return dsi_2.current_cinfo.dsi1_pll_fclk;
}

static unsigned long dsi_get_dsi2_pll_rate(enum dsi lcd_ix)
{
	return dsi_2.current_cinfo.dsi2_pll_fclk;
}

static unsigned long dsi_get_txbyteclkhs(enum dsi lcd_ix)
{
	return dsi_2.current_cinfo.clkin4ddr / 16;
}

int dss_get_dsi_clk_source(void)
{
	return FLD_GET(dss_read_reg(DSS_CONTROL), 1, 1);
}

static unsigned long dsi_fclk_rate(enum dsi lcd_ix)
{
	unsigned long r;

	if (dss_get_dsi_clk_source() == 2) {				
		
		r = dss_clk_get_rate(DSS_CLK_FCK1);
	} else {
		
		r = dsi_get_dsi2_pll_rate(lcd_ix);
	}

	return r;
}

static int dsi_set_lp_clk_divisor(void)
{
	unsigned long dsi_fclk;
	unsigned lp_clk_div;
	unsigned long lp_clk;
	enum dsi lcd_ix;
	lcd_ix = dsi2;

	lp_clk_div = DSI_DIV_LP_CLK_DIV;

	if (lp_clk_div == 0 || lp_clk_div > LP_DIV_MAX)
		return -EINVAL;

	dsi_fclk = dsi_fclk_rate(lcd_ix);

	lp_clk = dsi_fclk / 2 / lp_clk_div;

	printf("LP_CLK_DIV %u, LP_CLK %lu\n", lp_clk_div, lp_clk);
	dsi_2.current_cinfo.lp_clk = lp_clk;
	dsi_2.current_cinfo.lp_clk_div = lp_clk_div;

	REG_FLD_MOD(lcd_ix, DSI_CLK_CTRL, lp_clk_div,
			12, 0);			

	REG_FLD_MOD(lcd_ix, DSI_CLK_CTRL, 0,
			21, 21);		

	return 0;
}

enum dsi_pll_power_state {
	DSI_PLL_POWER_OFF	= 0x0,
	DSI_PLL_POWER_ON_HSCLK	= 0x1,
	DSI_PLL_POWER_ON_ALL	= 0x2,
	DSI_PLL_POWER_ON_DIV	= 0x3,
};

static int dsi_pll_power(enum dsi lcd_ix, enum dsi_pll_power_state state)
{
	int t = 0;

	REG_FLD_MOD(lcd_ix, DSI_CLK_CTRL, state, 31, 30);	

	while (FLD_GET(dsi_read_reg(lcd_ix, DSI_CLK_CTRL), 29, 28) != state) {
		udelay(1);
		if (t++ > 1000) {
			printf("Failed to set DSI PLL power mode to %d\n",
					state);
			return -ENODEV;
		}
	}

	return 0;
}

int dsi_calc_clock_rates(struct dsi_clock_info *cinfo)
{
	if (cinfo->regn == 0 || cinfo->regn > REGN_MAX)
		return -EINVAL;

	if (cinfo->regm == 0 || cinfo->regm > REGM_MAX)
		return -EINVAL;

	if (cinfo->regm3 > REGM3_MAX)
		return -EINVAL;

	if (cinfo->regm4 > REGM4_MAX)
		return -EINVAL;

	if (cinfo->use_dss2_sys_clk) {

		if(cpu_is_omap44xx())
			cinfo->clkin = 38400000;
		else
			cinfo->clkin = 26000000;
		cinfo->highfreq = 0;
	} else if (cinfo->use_dss2_fck) {
		
		cinfo->clkin = 38400000;

		cinfo->highfreq = 0;
	} else {
		cinfo->clkin = dispc_pclk_rate(OMAP_DSS_CHANNEL_LCD);

		if (cinfo->clkin < 32000000)
			cinfo->highfreq = 0;
		else
			cinfo->highfreq = 1;
	}
	printf("here cinfo->clkin =%x\n",cinfo->clkin);

	cinfo->fint = cinfo->clkin / (cinfo->regn * (cinfo->highfreq ? 2 : 1));

	if (cinfo->fint > FINT_MAX || cinfo->fint < FINT_MIN)
		return -EINVAL;

	cinfo->clkin4ddr = 2 * cinfo->regm * cinfo->fint;

	if (cinfo->clkin4ddr > 1800 * 1000 * 1000)
		return -EINVAL;

	if (cinfo->regm3 > 0)
		cinfo->dsi1_pll_fclk = cinfo->clkin4ddr / cinfo->regm3;
	else
		cinfo->dsi1_pll_fclk = 0;

	if (cinfo->regm4 > 0)
		cinfo->dsi2_pll_fclk = cinfo->clkin4ddr / cinfo->regm4;
	else
		cinfo->dsi2_pll_fclk = 0;

	return 0;
}

int dsi_pll_calc_clock_div_pck(enum dsi lcd_ix, bool is_tft,
		unsigned long req_pck,	struct dsi_clock_info *dsi_cinfo,
		struct dispc_clock_info *dispc_cinfo)
{
	struct dsi_clock_info cur, best;
	struct dispc_clock_info best_dispc;
	int min_fck_per_pck;
	int match = 0;
	unsigned long dss_clk_fck2;

	dss_clk_fck2 = dss_clk_get_rate(DSS_CLK_FCK2);

	if (req_pck == dsi_2.cache_req_pck &&
			dsi_2.cache_cinfo.clkin == dss_clk_fck2) {
		printf("DSI clock info found from cache\n");
		*dsi_cinfo = dsi_2.cache_cinfo;
		dispc_find_clk_divs(is_tft, req_pck, dsi_cinfo->dsi1_pll_fclk,
				dispc_cinfo);
		return 0;
	}

	min_fck_per_pck = CONFIG_OMAP2_DSS_MIN_FCK_PER_PCK;

	if (min_fck_per_pck &&
		req_pck * min_fck_per_pck > DISPC_MAX_FCK) {
		printf("Requested pixel clock not possible with the current "
				"OMAP2_DSS_MIN_FCK_PER_PCK setting. Turning "
				"the constraint off.\n");
		min_fck_per_pck = 0;
	}

	printf("dsi_pll_calc\n");

retry:
	memset(&best, 0, sizeof(best));
	memset(&best_dispc, 0, sizeof(best_dispc));

	memset(&cur, 0, sizeof(cur));
	cur.clkin = dss_clk_fck2;
	cur.use_dss2_fck = 1;
	cur.highfreq = 0;

	for (cur.regn = 1; cur.regn < REGN_MAX; ++cur.regn) {
		if (cur.highfreq == 0)
			cur.fint = cur.clkin / cur.regn;
		else
			cur.fint = cur.clkin / (2 * cur.regn);

		if (cur.fint > FINT_MAX || cur.fint < FINT_MIN)
			continue;

		for (cur.regm = 1; cur.regm < REGM_MAX; ++cur.regm) {
			unsigned long a, b;

			a = 2 * cur.regm * (cur.clkin/1000);
			b = cur.regn * (cur.highfreq + 1);
			cur.clkin4ddr = a / b * 1000;

			if (cur.clkin4ddr > 1800 * 1000 * 1000)
				break;

			for (cur.regm3 = 1; cur.regm3 < REGM3_MAX;
					++cur.regm3) {
				struct dispc_clock_info cur_dispc;
				cur.dsi1_pll_fclk = cur.clkin4ddr / cur.regm3;

				if (cur.dsi1_pll_fclk  < req_pck)
					break;

				if (cur.dsi1_pll_fclk > DISPC_MAX_FCK)
					continue;

				if (min_fck_per_pck &&
					cur.dsi1_pll_fclk <
						req_pck * min_fck_per_pck)
					continue;

				match = 1;

				dispc_find_clk_divs(is_tft, req_pck,
						cur.dsi1_pll_fclk,
						&cur_dispc);

				if (abs(cur_dispc.pck - req_pck) <
						abs(best_dispc.pck - req_pck)) {
					best = cur;
					best_dispc = cur_dispc;

					if (cur_dispc.pck == req_pck)
						goto found;
				}
			}
		}
	}
found:
	if (!match) {
		if (min_fck_per_pck) {
			printf("Could not find suitable clock settings.\n"
					"Turning FCK/PCK constraint off and"
					"trying again.\n");
			min_fck_per_pck = 0;
			goto retry;
		}

		printf("Could not find suitable clock settings.\n");

		return -EINVAL;
	}

	best.regm4 = best.clkin4ddr / 48000000;
	if (best.regm4 > REGM4_MAX)
		best.regm4 = REGM4_MAX;
	else if (best.regm4 == 0)
		best.regm4 = 1;
	best.dsi2_pll_fclk = best.clkin4ddr / best.regm4;

	if (dsi_cinfo)
		*dsi_cinfo = best;
	if (dispc_cinfo)
		*dispc_cinfo = best_dispc;

	dsi_2.cache_req_pck = req_pck;
	dsi_2.cache_clk_freq = 0;
	dsi_2.cache_cinfo = best;

	return 0;
}

int dsi_pll_set_clock_div(enum dsi lcd_ix, struct dsi_clock_info *cinfo)
{
	int r = 0;
	u32 l;
	int f = 0;
	dsi_2.current_cinfo.fint = cinfo->fint;
	dsi_2.current_cinfo.clkin4ddr = cinfo->clkin4ddr;
	dsi_2.current_cinfo.dsi1_pll_fclk = cinfo->dsi1_pll_fclk;
	dsi_2.current_cinfo.dsi2_pll_fclk = cinfo->dsi2_pll_fclk;

	dsi_2.current_cinfo.regn = cinfo->regn;
	dsi_2.current_cinfo.regm = cinfo->regm;
	dsi_2.current_cinfo.regm3 = cinfo->regm3;
	dsi_2.current_cinfo.regm4 = cinfo->regm4;

	printf("DSI Fint %ld\n", cinfo->fint);

	printf("clkin (%s) rate %ld, highfreq %d\n",
			cinfo->use_dss2_fck ? "dss2_fck" : "pclkfree",
			cinfo->clkin,
			cinfo->highfreq);

	printf("CLKIN4DDR = 2 * %d / %d * %lu / %d = %lu\n",
			cinfo->regm,
			cinfo->regn,
			cinfo->clkin,
			cinfo->highfreq + 1,
			cinfo->clkin4ddr);

	printf("Data rate on 1 DSI lane %ld Mbps\n",
			cinfo->clkin4ddr / 1000 / 1000 / 2);

	printf("Clock lane freq %ld Hz\n", cinfo->clkin4ddr / 4);

	printf("regm3 = %d, dsi1_pll_fclk = %lu\n",
			cinfo->regm3, cinfo->dsi1_pll_fclk);
	printf("regm4 = %d, dsi2_pll_fclk = %lu\n",
			cinfo->regm4, cinfo->dsi2_pll_fclk);

	REG_FLD_MOD(lcd_ix, DSI_PLL_CONTROL, 0, 0,
			0); 

	l = dsi_read_reg(lcd_ix, DSI_PLL_CONFIGURATION1);
	l = FLD_MOD(l, 1, 0, 0);		
	l = FLD_MOD(l, cinfo->regn - 1, REGN, 1);	
	
	l = FLD_MOD(l, cinfo->regm, REGN + REGM, REGN + 1);
	l = FLD_MOD(l, cinfo->regm3 > 0 ? cinfo->regm3 - 1 : 0,
		REGN + REGM + REGM3, REGN + REGM + 1);	
	
	l = FLD_MOD(l, cinfo->regm4 > 0 ? cinfo->regm4 - 1 : 0,
		REGN + REGM + REGM3 + REGM4, REGN + REGM + REGM3 + 1);
	dsi_write_reg(lcd_ix, DSI_PLL_CONFIGURATION1, l);

	BUG_ON(cinfo->fint < FINT_MIN || cinfo->fint > FINT_MAX);

	l = dsi_read_reg(lcd_ix, DSI_PLL_CONFIGURATION2);
	
	l = FLD_MOD(l, cinfo->use_dss2_fck ? 0 : 1,
			11, 11);		
	l = FLD_MOD(l, cinfo->highfreq,
			12, 12);		
	l = FLD_MOD(l, 1, 13, 13);		
	l = FLD_MOD(l, 0, 14, 14);		
	l = FLD_MOD(l, 1, 20, 20);		
	if (cpu_is_omap44xx())
		l = FLD_MOD(l, 3, 22, 21);	
	dsi_write_reg(lcd_ix, DSI_PLL_CONFIGURATION2, l);

	REG_FLD_MOD(lcd_ix, DSI_PLL_GO, 1, 0, 0); 

	if (wait_for_bit_change(lcd_ix, DSI_PLL_GO, 0, 0) != 0) {
		printf("dsi pll go bit not going down.\n");
		r = -EIO;
		goto err;
	}
	
	if (wait_for_bit_change(lcd_ix, DSI_PLL_STATUS, 1, 1) != 1) {
		printf("cannot lock PLL\n");
		r = -EIO;
		goto err;
	}

	dsi_2.pll_locked = 1;

	l = dsi_read_reg(lcd_ix, DSI_PLL_CONFIGURATION2);
	l = FLD_MOD(l, 0, 0, 0);	
	l = FLD_MOD(l, 0, 5, 5);	
	l = FLD_MOD(l, 0, 6, 6);	
	if (cpu_is_omap34xx())
		l = FLD_MOD(l, 0, 7, 7);
	l = FLD_MOD(l, 0, 8, 8);	
	l = FLD_MOD(l, 0, 10, 9);	
	l = FLD_MOD(l, 1, 13, 13);	
	l = FLD_MOD(l, 1, 14, 14);	
	l = FLD_MOD(l, 0, 15, 15);	
	l = FLD_MOD(l, 1, 16, 16);	
	l = FLD_MOD(l, 0, 17, 17);	
	l = FLD_MOD(l, 1, 18, 18);	
	l = FLD_MOD(l, 0, 19, 19);	
	l = FLD_MOD(l, 0, 20, 20);	
	if (cpu_is_omap44xx()) {
		l = FLD_MOD(l, 0, 25, 25);	
		l = FLD_MOD(l, 0, 26, 26);	
	}
	dsi_write_reg(lcd_ix, DSI_PLL_CONFIGURATION2, l);

	printf("PLL config done\n");
err:
	return r;
}

int dsi_pll_init(enum dsi lcd_ix, bool enable_hsclk, bool enable_hsdiv)
{
	int r = 0;
	enum dsi_pll_power_state pwstate;
	struct dsi_struct *p_dsi;
	
	printf("PLL init\n");

	REG_FLD_MOD(lcd_ix, DSI_CLK_CTRL, 1, 14, 14);
	 

		dispc_pck_free_enable(1);

	if (wait_for_bit_change(lcd_ix, DSI_PLL_STATUS, 0, 1) != 1) {
		puts("PLL not coming out of reset.\n");
		r = -ENODEV;
		goto err1;
	}

	dispc_pck_free_enable(1);

	pwstate = DSI_PLL_POWER_ON_ALL;

	r = dsi_pll_power(lcd_ix, pwstate);

	if (r)
		goto err1;

	printf("PLL init done\n");

	return 0;

err1:
#if 0
	if (!cpu_is_omap44xx())
		regulator_disable(p_dsi->vdds_dsi_reg);
#endif
err0:
	return r;
}

enum dsi_complexio_power_state {
	DSI_COMPLEXIO_POWER_OFF		= 0x0,
	DSI_COMPLEXIO_POWER_ON		= 0x1,
	DSI_COMPLEXIO_POWER_ULPS	= 0x2,
};

static int dsi_complexio_power(enum dsi lcd_ix,
		enum dsi_complexio_power_state state)
{
	int t = 0;

	REG_FLD_MOD(lcd_ix, DSI_COMPLEXIO_CFG1, state, 28, 27);

if (cpu_is_omap44xx())
	
	REG_FLD_MOD(lcd_ix, DSI_COMPLEXIO_CFG1, 1, 30, 30);

	while (FLD_GET(dsi_read_reg(lcd_ix, DSI_COMPLEXIO_CFG1),
			26, 25) != state) {
		udelay(1);
		if (t++ > 10000) {
			printf("failed to set complexio power state to "
					"%d\n", state);
			return -ENODEV;
		}
	}

	return 0;
}

static void dsi_complexio_config(void)
{
	u32 r;
	enum dsi lcd_ix;

	int clk_lane   = DSI_CLOCK_LANE;
	int data1_lane = DSI_DATA1_LANE;
	int data2_lane = DSI_DATA2_LANE;
	int clk_pol    = DSI_CLOCK_POLARITY;
	int data1_pol  = DSI_DATA1_POLARITY;
	int data2_pol  = DSI_DATA2_POLARITY;

	lcd_ix = dsi2;
	r = dsi_read_reg(lcd_ix, DSI_COMPLEXIO_CFG1);
	r = FLD_MOD(r, clk_lane, 2, 0);
	r = FLD_MOD(r, clk_pol, 3, 3);
	r = FLD_MOD(r, data1_lane, 6, 4);
	r = FLD_MOD(r, data1_pol, 7, 7);
	r = FLD_MOD(r, data2_lane, 10, 8);
	r = FLD_MOD(r, data2_pol, 11, 11);
	dsi_write_reg(lcd_ix, DSI_COMPLEXIO_CFG1, r);

}

static inline unsigned ns2ddr(enum dsi lcd_ix, unsigned ns)
{
	unsigned long ddr_clk;
	
	ddr_clk = dsi_2.current_cinfo.clkin4ddr / 4;
	return (ns * (ddr_clk/1000/1000) + 999) / 1000;
}

static inline unsigned ddr2ns(enum dsi lcd_ix, unsigned ddr)
{
	unsigned long ddr_clk;

	ddr_clk = dsi_2.current_cinfo.clkin4ddr / 4;
	return ddr * 1000 * 1000 / (ddr_clk / 1000);
}

static void dsi_complexio_timings(enum dsi lcd_ix)
{
	u32 r;
	u32 ths_prepare, ths_prepare_ths_zero, ths_trail, ths_exit;
	u32 tlpx_half, tclk_trail, tclk_zero;
	u32 tclk_prepare;
	unsigned int val;

	ths_prepare = ns2ddr(lcd_ix, 70) + 2;

	ths_prepare_ths_zero = ns2ddr(lcd_ix, 175) + 2;

	ths_trail = ns2ddr(lcd_ix, 60) + 5;

	ths_exit = ns2ddr(lcd_ix, 145);

	tlpx_half = ns2ddr(lcd_ix, 25);

	tclk_trail = ns2ddr(lcd_ix, 60) + 2;

	tclk_prepare = ns2ddr(lcd_ix, 65);

	tclk_zero = ns2ddr(lcd_ix, 265);

	r = dsi_read_reg(lcd_ix, DSI_DSIPHY_CFG0);
	r = FLD_MOD(r, ths_prepare, 31, 24);
	r = FLD_MOD(r, ths_prepare_ths_zero, 23, 16);
	r = FLD_MOD(r, ths_trail, 15, 8);
	r = FLD_MOD(r, ths_exit, 7, 0);
	dsi_write_reg(lcd_ix, DSI_DSIPHY_CFG0, r);

	r = dsi_read_reg(lcd_ix, DSI_DSIPHY_CFG1);
	r = FLD_MOD(r, tlpx_half, 22, 16);
	r = FLD_MOD(r, tclk_trail, 15, 8);
	r = FLD_MOD(r, tclk_zero, 7, 0);
	dsi_write_reg(lcd_ix, DSI_DSIPHY_CFG1, r);

	r = dsi_read_reg(lcd_ix, DSI_DSIPHY_CFG2);
	r = FLD_MOD(r, tclk_prepare, 7, 0);
	dsi_write_reg(lcd_ix, DSI_DSIPHY_CFG2, r);
}

static int dsi_complexio_init(enum dsi lcd_ix)
{	
	int r = 0;

	void volatile *phymux_base = NULL;

	unsigned int dsimux = 0xFFFFFFFF;

	printf("dsi_complexio_init\n");

	if (cpu_is_omap44xx()) {

		*((volatile unsigned int *)0x4A100618) |= 0xffffffff;

__raw_readl(0x4A100618);

		REG_FLD_MOD(lcd_ix, DSI_CLK_CTRL, 1, 13, 13);	
		REG_FLD_MOD(lcd_ix, DSI_CLK_CTRL, 1, 18, 18);	 
		REG_FLD_MOD(lcd_ix, DSI_CLK_CTRL, 1, 14, 14);
	}

	dsi_read_reg(lcd_ix, DSI_DSIPHY_CFG5);

	if (wait_for_bit_change(lcd_ix, DSI_DSIPHY_CFG5,
			30, 1) != 1) {
		printf("ComplexIO PHY not coming out of reset.\n");
		r = -ENODEV;
		goto err;
	}

	dsi_complexio_config();
	r = dsi_complexio_power(lcd_ix, DSI_COMPLEXIO_POWER_ON);
	if (r)
		goto err;

	if (wait_for_bit_change(lcd_ix, DSI_COMPLEXIO_CFG1,
			29, 1) != 1) {
		printf("ComplexIO not coming out of reset.\n");

		r = -ENODEV;

		goto err;

	}

	dsi_complexio_timings(lcd_ix);

	dsi_if_enable(lcd_ix, 1);

	dsi_if_enable(lcd_ix, 0);

	REG_FLD_MOD(lcd_ix, DSI_CLK_CTRL, 1,

			20, 20); 

	dsi_if_enable(lcd_ix, 1);

	dsi_if_enable(lcd_ix, 0);

	printf("CIO init done\n");

err:

	return r;

}
 
static int _dsi_wait_reset(enum dsi lcd_ix)

{

	int i = 0;

	while (REG_GET(lcd_ix, DSI_SYSSTATUS, 0, 0) == 0) {

		if (i++ > 50) {

			return -ENODEV;

		}

		udelay(1);

	}

	return 0;

}

 

static void dsi_config_tx_fifo(enum dsi lcd_ix, enum fifo_size size1,

		enum fifo_size size2, enum fifo_size size3,

		enum fifo_size size4)

{

	u32 r = 0;

	int add = 0;

	int i;

	dsi_2.vc[0].fifo_size = size1;

	dsi_2.vc[1].fifo_size = size2;

	dsi_2.vc[2].fifo_size = size3;

	dsi_2.vc[3].fifo_size = size4;

	for (i = 0; i < 4; i++) {

		u8 v;

		int size = dsi_2.vc[i].fifo_size;

		if (add + size > 4) {

			printf("Illegal FIFO configuration\n");

			BUG();

		}

		v = FLD_VAL(add, 2, 0) | FLD_VAL(size, 7, 4);

		r |= v << (8 * i);

		add += size;

	}

	dsi_write_reg(lcd_ix, DSI_TX_FIFO_VC_SIZE, r);

}

static void dsi_config_rx_fifo(enum dsi lcd_ix, enum fifo_size size1,

		enum fifo_size size2, enum fifo_size size3,

		enum fifo_size size4)

{

	u32 r = 0;

	int add = 0;

	int i;

	dsi_2.vc[0].fifo_size = size1;

	dsi_2.vc[1].fifo_size = size2;

	dsi_2.vc[2].fifo_size = size3;

	dsi_2.vc[3].fifo_size = size4;

	for (i = 0; i < 4; i++) {

		u8 v;

		int size = dsi_2.vc[i].fifo_size;

		if (add + size > 4) {

			printf("Illegal FIFO configuration\n");

			BUG();

		}

		v = FLD_VAL(add, 2, 0) | FLD_VAL(size, 7, 4);

		r |= v << (8 * i);

		add += size;

	}

		dsi_write_reg(lcd_ix, DSI_RX_FIFO_VC_SIZE, r);

}

static int dsi_force_tx_stop_mode_io(enum dsi lcd_ix)

{
	u32 r;

	r = dsi_read_reg(lcd_ix, DSI_TIMING1);

	r = FLD_MOD(r, 1, 15, 15);	

	dsi_write_reg(lcd_ix, DSI_TIMING1, r);

	if (wait_for_bit_change(lcd_ix, DSI_TIMING1, 15, 0) != 0) {

		printf("TX_STOP bit not going down\n");

		return -EIO;

	}

	return 0;

}

 static int dsi_vc_enable(enum dsi lcd_ix, int channel, bool enable)

{	

	if (dsi_2.update_mode != OMAP_DSS_UPDATE_AUTO)

		printf("dsi_vc_enable channel %d, enable %d\n",

				channel, enable);

	enable = enable ? 1 : 0;

	REG_FLD_MOD(lcd_ix, DSI_VC_CTRL(channel), enable, 0, 0);

	if (wait_for_bit_change(lcd_ix, DSI_VC_CTRL(channel),

			0, enable) != enable) {

			printf("Failed to set dsi_vc_enable to %d\n", enable);

			return -EIO;

	}

	return 0;

}

static void dsi_vc_initial_config(enum dsi lcd_ix, int channel)

{

	u32 r;

	printf("%d", channel);

	r = dsi_read_reg(lcd_ix, DSI_VC_CTRL(channel));

	if (FLD_GET(r, 15, 15)) 
		printf("VC(%d) busy when trying to configure it!\n",
				channel);

	r = FLD_MOD(r, 0, 1, 1); 
	r = FLD_MOD(r, 0, 2, 2); 
	r = FLD_MOD(r, 0, 3, 3); 
	r = FLD_MOD(r, 0, 4, 4); 
	r = FLD_MOD(r, 1, 7, 7); 
	r = FLD_MOD(r, 1, 8, 8); 
	r = FLD_MOD(r, 0, 9, 9); 
	if (cpu_is_omap44xx()) {
		r = FLD_MOD(r, 3, 11, 10);	
		r = FLD_MOD(r, 1, 12, 12);	
		}
	r = FLD_MOD(r, 4, 29, 27); 
	if (!cpu_is_omap44xx())
	r = FLD_MOD(r, 4, 23, 21); 

	dsi_write_reg(lcd_ix, DSI_VC_CTRL(channel), r);
	dsi_2.vc[channel].mode = DSI_VC_MODE_L4;
}

static void dsi_vc_initial_config_vp(enum dsi lcd_ix, int channel, enum omap_dsi_mode dsi_mode)
{
	u32 r;

	printf("%d", channel);

	r = dsi_read_reg(lcd_ix, DSI_VC_CTRL(channel));
	r = FLD_MOD(r, 1, 1, 1); 
	r = FLD_MOD(r, 0, 2, 2); 
	r = FLD_MOD(r, 0, 3, 3); 
	r = FLD_MOD(r, (OMAP_DSI_MODE_CMD == dsi_mode) ? 0 : 1, 4, 4); 
	r = FLD_MOD(r, 1, 7, 7); 
	r = FLD_MOD(r, 1, 8, 8); 
	r = FLD_MOD(r, 1, 9, 9); 
	r = FLD_MOD(r, 1, 12, 12);	
	r = FLD_MOD(r, 4, 29, 27); 
	r = FLD_MOD(r, 4, 23, 21); 
	r = FLD_MOD(r, (OMAP_DSI_MODE_CMD == dsi_mode) ? 1: 0, 30, 30);	
	r = FLD_MOD(r, 0, 31, 31);	
	dsi_write_reg(lcd_ix, DSI_VC_CTRL(channel), r);
}

static void dsi_vc_config_l4(enum dsi lcd_ix, int channel)
{	
	if (dsi_2.vc[channel].mode == DSI_VC_MODE_L4)
		return;

	printf("%d", channel);

	dsi_vc_enable(lcd_ix, channel, 0);

	if (REG_GET(lcd_ix, DSI_VC_CTRL(channel), 15, 15)) 
		printf("vc(%d) busy when trying to config for L4\n", channel);
#if defined (CONFIG_LGE_P2) 
	dsi_vc_initial_config(lcd_ix, channel);
#else
	REG_FLD_MOD(lcd_ix, DSI_VC_CTRL(channel), 0, 1, 1); 
#endif
	dsi_vc_enable(lcd_ix, channel, 1);

	dsi_2.vc[channel].mode = DSI_VC_MODE_L4;
}

static void dsi_vc_config_vp(enum dsi lcd_ix, int channel)
{
	
#ifdef CONFIG_ARCH_OMAP4
	if (dsi_2.vc[channel].mode == DSI_VC_MODE_VP)
			return;

	printf("%d", channel);

	dsi_vc_enable(lcd_ix, channel, 0);

	if (REG_GET(lcd_ix, DSI_VC_CTRL(channel), 15, 15)) 
			printf("vc(%d) busy when trying to config for VP\n",
			channel);
#if defined (CONFIG_LGE_P2) 
	dsi_vc_initial_config_vp(lcd_ix, channel, OMAP_DSI_MODE_VIDEO);
#else
	REG_FLD_MOD(lcd_ix, DSI_VC_CTRL(channel), 1,
			1, 1); 
#endif
	dsi_vc_enable(lcd_ix, channel, 1);

	dsi_2.vc[channel].mode = DSI_VC_MODE_VP;
#endif
}

 
static void dsi_vc_flush_long_data(enum dsi lcd_ix, int channel)
{
	while (REG_GET(lcd_ix, DSI_VC_CTRL(channel), 20, 20)) {
		u32 val;
		val = dsi_read_reg(lcd_ix, DSI_VC_SHORT_PACKET_HEADER(channel));
		printf("\t\tb1 %#02x b2 %#02x b3 %#02x b4 %#02x\n",
				(val >> 0) & 0xff,
				(val >> 8) & 0xff,
				(val >> 16) & 0xff,
				(val >> 24) & 0xff);
	}
}

static void dsi_show_rx_ack_with_err(u16 err)
{
#if 0
	printk("\tACK with ERROR (%#x):\n", err);
	if (err & (1 << 0))
		printk("\t\tSoT Error\n");
	if (err & (1 << 1))
		printk("\t\tSoT Sync Error\n");
	if (err & (1 << 2))
		printk("\t\tEoT Sync Error\n");
	if (err & (1 << 3))
		printk("\t\tEscape Mode Entry Command Error\n");
	if (err & (1 << 4))
		printk("\t\tLP Transmit Sync Error\n");
	if (err & (1 << 5))
		printk("\t\tHS Receive Timeout Error\n");
	if (err & (1 << 6))
		printk("\t\tFalse Control Error\n");
	if (err & (1 << 7))
		printk("\t\t(reserved7)\n");
	if (err & (1 << 8))
		printk("\t\tECC Error, single-bit (corrected)\n");
	if (err & (1 << 9))
		printk("\t\tECC Error, multi-bit (not corrected)\n");
	if (err & (1 << 10))
		printk("\t\tChecksum Error\n");
	if (err & (1 << 11))
		printk("\t\tData type not recognized\n");
	if (err & (1 << 12))
		printk("\t\tInvalid VC ID\n");
	if (err & (1 << 13))
		printk("\t\tInvalid Transmission Length\n");
	if (err & (1 << 14))
		printk("\t\t(reserved14)\n");
	if (err & (1 << 15))
		printk("\t\tDSI Protocol Violation\n");
#endif	
}

static u16 dsi_vc_flush_receive_data(enum dsi lcd_ix, int channel)
{	
	
	while (REG_GET(lcd_ix, DSI_VC_CTRL(channel), 20, 20)) {
		u32 val;
		u8 dt;
		val = dsi_read_reg(lcd_ix, DSI_VC_SHORT_PACKET_HEADER(channel));
		printf("\trawval %#08x\n", val);
		dt = FLD_GET(val, 5, 0);
		if (dt == DSI_DT_RX_ACK_WITH_ERR) {
			u16 err = FLD_GET(val, 23, 8);
			if (!cpu_is_omap44xx())
				dsi_show_rx_ack_with_err(err);
		} else if (dt == DSI_DT_RX_SHORT_READ_1) {
			printf("\tDCS short response, 1 byte: %#x\n",
					FLD_GET(val, 23, 8));
		} else if (dt == DSI_DT_RX_SHORT_READ_2) {
			printf("\tDCS short response, 2 byte: %#x\n",
					FLD_GET(val, 23, 8));
		} else if (dt == DSI_DT_RX_DCS_LONG_READ) {
			printf("\tDCS long response, len %d\n",
					FLD_GET(val, 23, 8));
			dsi_vc_flush_long_data(lcd_ix, channel);
		} else {
			printf("\tunknown datatype 0x%02x\n", dt);
		}
	}
	return 0;
}
 

static inline void dsi_vc_write_long_header(enum dsi lcd_ix, int channel,
	u8 data_type, u16 len, u8 ecc)
{
	u32 val;
	u8 data_id;

	ecc = 0; 

	if (cpu_is_omap44xx())
	data_id = data_type | dsi_2.vc[channel].dest_per << 6;
	else
		data_id = data_type | channel << 6;

	val = FLD_VAL(data_id, 7, 0) | FLD_VAL(len, 23, 8) |
		FLD_VAL(ecc, 31, 24);

	dsi_write_reg(lcd_ix, DSI_VC_LONG_PACKET_HEADER(channel), val);
}

static inline void dsi_vc_write_long_payload(enum dsi lcd_ix, int channel,
		u8 b1, u8 b2, u8 b3, u8 b4)
{
	u32 val;

	val = b4 << 24 | b3 << 16 | b2 << 8  | b1 << 0;
	dsi_write_reg(lcd_ix, DSI_VC_LONG_PACKET_PAYLOAD(channel), val);
}

static int dsi_vc_send_long(enum dsi lcd_ix,
	int channel, u8 data_type, u8 *data, u16 len, u8 ecc)
{
	int i;
	u8 *p;
	int r = 0;
	u8 b1, b2, b3, b4;

	if (dsi_2.debug_write)
		printf("dsi_vc_send_long, %d bytes\n", len);

	if (dsi_2.vc[channel].fifo_size * 32 * 4 < len + 4) {
		printf("unable to send long packet: packet too long. ch = %d , len =%d, fifo = %d\n",channel,len,dsi_2.vc[channel].fifo_size);
		return -EINVAL;
	}

	dsi_vc_config_l4(lcd_ix, channel);

	dsi_vc_write_long_header(lcd_ix, channel, data_type, len, ecc);

	p = data;
	for (i = 0; i < len >> 2; i++) {
		if (dsi_2.debug_write)
			printf("\tsending full packet %d\n", i);

		b1 = *p++;
		b2 = *p++;
		b3 = *p++;
		b4 = *p++;

		mdelay(2+1);
		dsi_vc_write_long_payload(lcd_ix, channel, b1, b2, b3, b4);
	}

	i = len % 4;
	if (i) {
		b1 = 0; b2 = 0; b3 = 0;

		if (dsi_2.debug_write)
			printf("\tsending remainder bytes %d\n", i);

		switch (i) {
		case 3:
			b1 = *p++;
			b2 = *p++;
			b3 = *p++;
			break;
		case 2:
			b1 = *p++;
			b2 = *p++;
			break;
		case 1:
			b1 = *p++;
			break;
		}

		dsi_vc_write_long_payload(lcd_ix, channel, b1, b2, b3, 0);
	}

	return r;
}

int send_short_packet(enum dsi lcd_ix, u8 data_type, u8 vc, u8 data0,
 u8 data1, bool mode, bool ecc)
{
	u32 val, header = 0, count = 10000;

	dsi_vc_enable(lcd_ix, vc, 0);
	
	val = dsi_read_reg(lcd_ix, DSI_VC_CTRL(vc));
	if (mode == 1) {
		val = val | (1<<9);
		}
	else if (mode == 0) {
		val = val & ~(1<<9);
		}
	dsi_write_reg(lcd_ix, DSI_VC_CTRL(vc), val);

	dsi_vc_enable(lcd_ix, vc, 1);
	
	header = (0<<24)|
			(data1<<16)|
			(data0<<8)|
			(0<<6) |
			(data_type<<0);
	dsi_write_reg(lcd_ix, DSI_VC_SHORT_PACKET_HEADER(0), header);

	printf("Header = 0x%x", header);

	do {
		val = dsi_read_reg(lcd_ix, DSI_VC_IRQSTATUS(vc));
	} while ((!(val & 0x00000004)) && --count);
	if (count) {
		printf("Short packet  success!!! \n\r");

		dsi_write_reg(lcd_ix, DSI_VC_IRQSTATUS(vc), 0x00000004);
		return 0;
		}
	else	{
		printf("Failed to send Short packet !!! \n\r");
		return -1;
		}
}

static int dsi_vc_send_short(enum dsi lcd_ix, int channel, u8 data_type,
	u16 data, u8 ecc)
{	
	u32 r;
	u8 data_id;
	u32 val, u, count;

	dsi_vc_config_l4(lcd_ix, channel);

	if (cpu_is_omap44xx()) {
		if (FLD_GET(dsi_read_reg(lcd_ix, DSI_VC_CTRL(channel)),
			16, 16)) {
			printf("ERROR FIFO FULL, aborting transfer\n");
			return -EINVAL;
		}

		data_id = data_type | dsi_2.vc[channel].dest_per << 6;
	} else {
		data_id = data_type | 0 << 6;
	}

	r = (data_id << 0) | (data << 8) | (0 << 16) | (ecc << 24);

 
	dsi_write_reg(lcd_ix, DSI_VC_SHORT_PACKET_HEADER(channel), r);
	
	return 0;
}

int dsi_vc_dcs_write_nosync(enum dsi lcd_ix, int channel, u8 *data, int len)
{	
	int r = 0;

	BUG_ON(len == 0);

	if (len == 1) {
		r = dsi_vc_send_short(lcd_ix, channel, DSI_DT_DCS_SHORT_WRITE_0,
				data[0], 0);
	} else if (len == 2) {
		r = dsi_vc_send_short(lcd_ix, channel, DSI_DT_DCS_SHORT_WRITE_1,
				data[0] | (data[1] << 8), 0);		
	} else {
		
		r = dsi_vc_send_long(lcd_ix, channel, DSI_DT_DCS_LONG_WRITE,
				data, len, 0);
	}

	return r;
}

int dsi_vc_generic_write_short(enum dsi ix, int channel, u8 *data, int len)
{
	int r;
	if( len == 1) {
		r = dsi_vc_send_short(ix, channel, 0x23, data[0], 0);
	} else {
		r = dsi_vc_send_short(ix, channel, 0x23, data[0] | (data[1] << 8), 0);
	}
	
	return r;
}
EXPORT_SYMBOL(dsi_vc_generic_write_short);

int dsi_vc_dcs_write(enum dsi ix, int channel,
	u8 *data, int len)
{

	int r;

	r = dsi_vc_dcs_write_nosync(ix, channel, data, len);
	if (r)
		goto err;
	
	if (REG_GET(ix, DSI_VC_CTRL(channel), 20, 20)) {	
		printf("rx fifo not empty after write, dumping data:\n");
		dsi_vc_flush_receive_data(ix, channel);
		r = -EIO;
		printf("###  rx fifo not empty after write, dumping data. ###\n");
		goto err;
	}

	return 0;
err:

	return r;
}
static void dsi_set_lp_rx_timeout(enum dsi lcd_ix, unsigned long ns)
{
	u32 r;
	unsigned x4 = 0, x16 = 0;
	unsigned long fck = 0;
	unsigned long ticks = 0;
	lcd_ix = dsi2;

	fck = dsi_fclk_rate(lcd_ix);
	ticks = (fck / 1000 / 1000) * ns / 1000;
	x4 = 0;
	x16 = 0;

	if (ticks > 0x1fff) {
		ticks = (fck / 1000 / 1000) * ns / 1000 / 4;
		x4 = 1;
		x16 = 0;
	}

	if (ticks > 0x1fff) {
		ticks = (fck / 1000 / 1000) * ns / 1000 / 16;
		x4 = 0;
		x16 = 1;
	}

	if (ticks > 0x1fff) {
		ticks = (fck / 1000 / 1000) * ns / 1000 / (4 * 16);
		x4 = 1;
		x16 = 1;
	}

	if (ticks > 0x1fff) {
		printf("LP_TX_TO over limit, setting it to max\n");
		ticks = 0x1fff;
		x4 = 1;
		x16 = 1;
	}

	r = dsi_read_reg(lcd_ix, DSI_TIMING2);
	r = FLD_MOD(r, 1, 15, 15);	
	r = FLD_MOD(r, x16, 14, 14);	
	r = FLD_MOD(r, x4, 13, 13);	
	r = FLD_MOD(r, ticks, 12, 0);	
	dsi_write_reg(lcd_ix, DSI_TIMING2, r);

	printf("LP_RX_TO %lu ns (%#lx ticks%s%s)\n",
		(ticks * (x16 ? 16 : 1) * (x4 ? 4 : 1) * 1000) /
		(fck / 1000 / 1000),
		ticks, x4 ? " x4" : "", x16 ? " x16" : "");
}

static void dsi_set_ta_timeout(enum dsi lcd_ix, unsigned long ns)
{
	u32 r;
	unsigned x8 = 0, x16 = 0;
	unsigned long fck = 0;
	unsigned long ticks = 0;

	fck = dsi_fclk_rate(lcd_ix);
	ticks = (fck / 1000 / 1000) * ns / 1000;
	x8 = 0;
	x16 = 0;

	if (ticks > 0x1fff) {
		ticks = (fck / 1000 / 1000) * ns / 1000 / 8;
		x8 = 1;
		x16 = 0;
	}

	if (ticks > 0x1fff) {
		ticks = (fck / 1000 / 1000) * ns / 1000 / 16;
		x8 = 0;
		x16 = 1;
	}

	if (ticks > 0x1fff) {
		ticks = (fck / 1000 / 1000) * ns / 1000 / (8 * 16);
		x8 = 1;
		x16 = 1;
	}

	if (ticks > 0x1fff) {
		printf("TA_TO over limit, setting it to max\n");
		ticks = 0x1fff;
		x8 = 1;
		x16 = 1;
	}

	r = dsi_read_reg(lcd_ix, DSI_TIMING1);

	r = FLD_MOD(r, 1, 31, 31);		
	
	r = FLD_MOD(r, 0, 31, 31);		
	r = FLD_MOD(r, x16, 30, 30);	
	r = FLD_MOD(r, x8, 29, 29);		
	r = FLD_MOD(r, ticks, 28, 16);	

	dsi_write_reg(lcd_ix, DSI_TIMING1, r);

	printf("TA_TO %lu ns (%#lx ticks%s%s)\n",
		(ticks * (x16 ? 16 : 1) * (x8 ? 8 : 1) * 1000) /
		(fck / 1000 / 1000),
		ticks, x8 ? " x8" : "", x16 ? " x16" : "");
}

static void dsi_set_stop_state_counter(enum dsi lcd_ix, unsigned long ns)
{
	u32 r;
	unsigned x4 = 0, x16 = 0;
	unsigned long fck = 0;
	unsigned long ticks = 0;

	fck = dsi_fclk_rate(lcd_ix);
	ticks = (fck / 1000 / 1000) * ns / 1000;
	x4 = 0;
	x16 = 0;

	if (ticks > 0x1fff) {
		ticks = (fck / 1000 / 1000) * ns / 1000 / 4;
		x4 = 1;
		x16 = 0;
	}

	if (ticks > 0x1fff) {
		ticks = (fck / 1000 / 1000) * ns / 1000 / 16;
		x4 = 0;
		x16 = 1;
	}

	if (ticks > 0x1fff) {
		ticks = (fck / 1000 / 1000) * ns / 1000 / (4 * 16);
		x4 = 1;
		x16 = 1;
	}

	if (ticks > 0x1fff) {
		printf("STOP_STATE_COUNTER_IO over limit, "
				"setting it to max\n");
		ticks = 0x1fff;
		x4 = 1;
		x16 = 1;
	}

	r = dsi_read_reg(lcd_ix, DSI_TIMING1);
	r = FLD_MOD(r, 1, 15, 15);		
	r = FLD_MOD(r, x16, 14, 14);	
	r = FLD_MOD(r, x4, 13, 13);		
	r = FLD_MOD(r, ticks, 12, 0);	
	dsi_write_reg(lcd_ix, DSI_TIMING1, r);

	printf("STOP_STATE_COUNTER %lu ns (%#lx ticks%s%s)\n",
		(ticks * (x16 ? 16 : 1) * (x4 ? 4 : 1) * 1000) /
		(fck / 1000 / 1000),
		ticks, x4 ? " x4" : "", x16 ? " x16" : "");
}

static void dsi_set_hs_tx_timeout(enum dsi lcd_ix, unsigned long ns)
{
	u32 r;
	unsigned x4 = 0, x16 = 0;
	unsigned long fck = 0;
	unsigned long ticks = 0;
	lcd_ix = dsi2;

	fck = dsi_get_txbyteclkhs(lcd_ix);
	ticks = (fck / 1000 / 1000) * ns / 1000;
	x4 = 0;
	x16 = 0;

	if (ticks > 0x1fff) {
		ticks = (fck / 1000 / 1000) * ns / 1000 / 4;
		x4 = 1;
		x16 = 0;
	}

	if (ticks > 0x1fff) {
		ticks = (fck / 1000 / 1000) * ns / 1000 / 16;
		x4 = 0;
		x16 = 1;
	}

	if (ticks > 0x1fff) {
		ticks = (fck / 1000 / 1000) * ns / 1000 / (4 * 16);
		x4 = 1;
		x16 = 1;
	}

	if (ticks > 0x1fff) {
		printf("HS_TX_TO over limit, setting it to max\n");
		ticks = 0x1fff;
		x4 = 1;
		x16 = 1;
	}

	r = dsi_read_reg(lcd_ix, DSI_TIMING2);
	r = FLD_MOD(r, 1, 31, 31);	
	r = FLD_MOD(r, x16, 30, 30);	
	r = FLD_MOD(r, x4, 29, 29);	
	r = FLD_MOD(r, ticks, 28, 16);	
	dsi_write_reg(lcd_ix, DSI_TIMING2, r);

	printf("HS_TX_TO %lu ns (%#lx ticks%s%s)\n",
		(ticks * (x16 ? 16 : 1) * (x4 ? 4 : 1) * 1000) /
		(fck / 1000 / 1000),
		ticks, x4 ? " x4" : "", x16 ? " x16" : "");
}

static int dsi_proto_config(void)
{
	u32 r;
	int buswidth = 0;
	enum dsi lcd_ix;

	enum omap_dsi_mode dsi_mode = OMAP_DSI_MODE_VIDEO;
	
	lcd_ix = dsi2;

	if (cpu_is_omap44xx()) {
#if defined (CONFIG_LGE_P2) 
		enum fifo_size size1 = DSI_FIFO_SIZE_32;
		enum fifo_size size2 = DSI_FIFO_SIZE_32;
#else
		enum fifo_size size1 =  (OMAP_DSI_MODE_VIDEO == dsi_mode) ? ( (DSI_VC_VIDEO == 0) ? DSI_FIFO_SIZE_0 : DSI_FIFO_SIZE_64) : DSI_FIFO_SIZE_64;
		enum fifo_size size2 = (OMAP_DSI_MODE_VIDEO == dsi_mode) ? ( (DSI_VC_VIDEO == 1) ? DSI_FIFO_SIZE_0 : DSI_FIFO_SIZE_64) : DSI_FIFO_SIZE_64;
#endif
		dsi_config_tx_fifo(lcd_ix, size1,
			size2,
			DSI_FIFO_SIZE_0,
			DSI_FIFO_SIZE_0);

		dsi_config_rx_fifo(lcd_ix, size1,
			size2,
			DSI_FIFO_SIZE_0,
			DSI_FIFO_SIZE_0);
	}

	dsi_set_stop_state_counter(lcd_ix, 1000);
	dsi_set_ta_timeout(lcd_ix, 6400000);
	dsi_set_lp_rx_timeout(lcd_ix, 48000);
	dsi_set_hs_tx_timeout(lcd_ix, 1000000);

	switch (DSI_CTRL_PIXEL_SIZE) {
	case 16:
		buswidth = 0;
		break;
	case 18:
		buswidth = 1;
		break;
	case 24:
		buswidth = 2;
		break;
	default:
		BUG();
	}

	r = dsi_read_reg(lcd_ix, DSI_CTRL);
	r = FLD_MOD(r, (cpu_is_omap44xx()) ? 0 : 1,
			1, 1);				
	r = FLD_MOD(r, (cpu_is_omap44xx()) ? 0 : 1,
			2, 2);				
	r = FLD_MOD(r, 1, 3, 3);	
	r = FLD_MOD(r, 1, 4, 4);	
	r = FLD_MOD(r, buswidth, 7, 6); 
	r = FLD_MOD(r, 0, 8, 8);	
	r = FLD_MOD(r, 1, 14, 14);      
	
	if(OMAP_DSI_MODE_VIDEO == dsi_mode)
	{
		
		r = FLD_MOD(r, 2, 13, 12);      
		r = FLD_MOD(r, (cpu_is_omap44xx()) ? 1 : 0,
			9, 9); 							
		r = FLD_MOD(r, 1, 15, 15);      
		r = FLD_MOD(r, 0, 16, 16);      
		r = FLD_MOD(r, 1, 17, 17);      
		r = FLD_MOD(r, 0, 18, 18);	
		r = FLD_MOD(r, 0, 20, 20);	
		r = FLD_MOD(r, 0, 21, 21);	
		r = FLD_MOD(r, 0, 22, 22);	
		r = FLD_MOD(r, 0, 23, 23);      		
		r = FLD_MOD(r, (cpu_is_omap44xx()) ? 1 : 0,
			11, 11);					
		r = FLD_MOD(r, (cpu_is_omap44xx()) ? 1 : 0,
			10, 10);			
		
		r = FLD_MOD(r, 0,19, 19);					
	}
	else
	{
		r = FLD_MOD(r, 2, 13, 12);      
		r = FLD_MOD(r, (cpu_is_omap44xx()) ? 0 : 1,
			19, 19);							
	}	
	
	if (!cpu_is_omap44xx()) {
		r = FLD_MOD(r, 1, 24, 24);	
		
		r = FLD_MOD(r, 0, 25, 25);
	}
	dsi_write_reg(lcd_ix, DSI_CTRL, r);

	dsi_vc_initial_config(lcd_ix, DSI_VC_CMD);
#if ! defined (CONFIG_LGE_P2) 
	if (cpu_is_omap44xx())
		dsi_vc_initial_config_vp(lcd_ix, DSI_VC_VIDEO, dsi_mode);
#endif
	
	dsi_2.vc[0].dest_per = 0;
	dsi_2.vc[1].dest_per = 0;
	dsi_2.vc[2].dest_per = 0;
	dsi_2.vc[3].dest_per = 0;

	return 0;
}

static void dsi_proto_timings(void)
{
	unsigned tlpx, tclk_zero, tclk_prepare, tclk_trail;
	unsigned tclk_pre, tclk_post;
	unsigned ths_prepare, ths_prepare_ths_zero, ths_zero;
	unsigned ths_trail, ths_exit;
	unsigned ddr_clk_pre, ddr_clk_post;
	unsigned enter_hs_mode_lat, exit_hs_mode_lat;
	unsigned ths_eot;
	u32 r;
	enum dsi lcd_ix;
	lcd_ix = dsi2;

	r = dsi_read_reg(lcd_ix, DSI_DSIPHY_CFG0);
	ths_prepare = FLD_GET(r, 31, 24);
	ths_prepare_ths_zero = FLD_GET(r, 23, 16);
	ths_zero = ths_prepare_ths_zero - ths_prepare;
	ths_trail = FLD_GET(r, 15, 8);
	ths_exit = FLD_GET(r, 7, 0);

	r = dsi_read_reg(lcd_ix, DSI_DSIPHY_CFG1);
	tlpx = FLD_GET(r, 22, 16) * 2;
	tclk_trail = FLD_GET(r, 15, 8);
	tclk_zero = FLD_GET(r, 7, 0);

	r = dsi_read_reg(lcd_ix, DSI_DSIPHY_CFG2);
	tclk_prepare = FLD_GET(r, 7, 0);

	tclk_pre = 20;
	if(cpu_is_omap44xx())
		tclk_pre = 8;
	
	tclk_post = ns2ddr(lcd_ix, 60) + 26;

	if (DSI_DATA1_LANE != 0 && DSI_DATA2_LANE != 0)
		ths_eot = 2;
	else
		ths_eot = 4;

	if(OMAP_DSI_MODE_VIDEO == dsi_2.dsi_mode)
		ths_eot = 0;

	printf("tclk_post=%d, tclk_trail=%d\n", tclk_post, tclk_trail);
	ddr_clk_pre = DIV_ROUND_UP(tclk_pre + tlpx + tclk_zero + tclk_prepare,
			4);
	ddr_clk_post = DIV_ROUND_UP(tclk_post + tclk_trail, 4) + ths_eot;

	BUG_ON(ddr_clk_pre == 0 || ddr_clk_pre > 255);
	BUG_ON(ddr_clk_post == 0 || ddr_clk_post > 255);

	r = dsi_read_reg(lcd_ix, DSI_CLK_TIMING);
	r = FLD_MOD(r, ddr_clk_pre, 15, 8);
	r = FLD_MOD(r, ddr_clk_post, 7, 0);
	dsi_write_reg(lcd_ix, DSI_CLK_TIMING, r);

	enter_hs_mode_lat = 1 + DIV_ROUND_UP(tlpx, 4) +
		DIV_ROUND_UP(ths_prepare, 4) +
		DIV_ROUND_UP(ths_zero + 3, 4);

	exit_hs_mode_lat = DIV_ROUND_UP(ths_trail + ths_exit, 4) + 1 + ths_eot;

	r = FLD_VAL(enter_hs_mode_lat, 31, 16) |
		FLD_VAL(exit_hs_mode_lat, 15, 0);
	dsi_write_reg(lcd_ix, DSI_VM_TIMING7, r);

}

#define DSI_DECL_VARS \
	int __dsi_cb = 0; u32 __dsi_cv = 0;

#define DSI_FLUSH(no, ch) \
	if (__dsi_cb > 0) { \
		 \
		dsi_write_reg(no, DSI_VC_LONG_PACKET_PAYLOAD(ch), __dsi_cv); \
		__dsi_cb = __dsi_cv = 0; \
	}

#define DSI_PUSH(no, ch, data) \
	do { \
		__dsi_cv |= (data) << (__dsi_cb * 8); \
		 \
		if (++__dsi_cb > 3) \
			DSI_FLUSH(no, ch); \
	} while (0)

static void dsi_set_update_region(enum dsi lcd_ix,
	u16 x, u16 y, 	u16 w, u16 h)
{
	if (dsi_2.update_region.dirty) {
		dsi_2.update_region.x = min(x, dsi_2.update_region.x);
		dsi_2.update_region.y = min(y, dsi_2.update_region.y);
		dsi_2.update_region.w = max(w, dsi_2.update_region.w);
		dsi_2.update_region.h = max(h, dsi_2.update_region.h);
	} else {
		dsi_2.update_region.x = x;
		dsi_2.update_region.y = y;
		dsi_2.update_region.w = w;
		dsi_2.update_region.h = h;
	}

	dsi_2.update_region.dirty = 1;
}

static int dsi_configure_dsi_clocks(void)
{
	struct dsi_clock_info cinfo;
	int r;
	enum dsi lcd_ix;
	lcd_ix = dsi2;

		cinfo.use_dss2_fck = 1;

	cinfo.regn  = 20;  

	cinfo.regm  = 177; 

	cinfo.regm3 = 5; 
	cinfo.regm4 = 7; 

	r = dsi_calc_clock_rates(&cinfo);
	if (r)
		return r;

	r = dsi_pll_set_clock_div(lcd_ix, &cinfo);
	if (r) {
		printf("Failed to set dsi clocks\n");
		return r;
	}

	return 0;
}
static int dsi_configure_dispc_clocks(void)
{
	struct dispc_clock_info dispc_cinfo;
	int r;
	unsigned long long fck;
	enum dsi lcd_ix;

	lcd_ix = dsi2;
	
	fck = dsi_get_dsi1_pll_rate(lcd_ix);

	dispc_cinfo.lck_div = 1;
	dispc_cinfo.pck_div = 5;

	r = dispc_calc_clock_rates(fck, &dispc_cinfo);
	if (r) {
		printf("Failed to calc dispc clocks\n");
		return r;
	}

	r = dispc_set_clock_div(OMAP_DSS_CHANNEL_LCD2, &dispc_cinfo);
	if (r) {
		printf("Failed to set dispc clocks\n");
		return r;
	}

	return 0;
}

void dsi_enable_video_mode(void)
{
	u8 pixel_type;
	u16 n_bytes;
	enum dsi lcd_ix;
	
	lcd_ix = dsi2;
	dsi_if_enable(lcd_ix, 0);

	switch (DSI_CTRL_PIXEL_SIZE) {
	case 16:
		pixel_type = DSI_PACKED_PIXEL_STREAM_16;
		n_bytes = 480 * 2;
		break;
	case 18:
		pixel_type = DSI_PACKED_PIXEL_STREAM_18;
		n_bytes = (480 * 18) / 8;
		break;
	case 24:
	default:
		pixel_type = DSI_PACKED_PIXEL_STREAM_24;
		n_bytes = 480 * 3;
		break;	
	}
#if defined (CONFIG_LGE_P2) 
	dsi_vc_config_vp(lcd_ix, DSI_VC_VIDEO);
	dsi_vc_write_long_header(lcd_ix, DSI_VC_VIDEO, pixel_type, n_bytes, 0);
#else
	dsi_vc_enable(lcd_ix, DSI_VC_CMD, 0); 
	dsi_vc_write_long_header(lcd_ix, DSI_VC_VIDEO, pixel_type, n_bytes, 0);		
	dsi_vc_enable(lcd_ix, DSI_VC_VIDEO, 1);
#endif
	dsi_if_enable(lcd_ix, 1);	
}

static void dsi_set_video_mode_timings(void)
{
	u32 r;
	u16 window_sync;
	u16 vm_tl;
	u16 n_lanes;
	enum dsi lcd_ix;	
	lcd_ix = dsi2;

	window_sync = 4;
	r = FLD_VAL(101, 11, 0) | FLD_VAL(20, 23, 12) | FLD_VAL(58, 31, 24);
	dsi_write_reg(lcd_ix, DSI_VM_TIMING1, r);

	r = FLD_VAL(22, 7, 0) | FLD_VAL(1, 15, 8) | FLD_VAL(8, 23, 16) | FLD_VAL(window_sync, 27, 24);
	dsi_write_reg(lcd_ix, DSI_VM_TIMING2, r);

	n_lanes = 2;	
	vm_tl = ((24) *
		(480 +
		 51 + 
		 19 +
		 14) )/ (8 * n_lanes);	

	r = FLD_VAL(800, 15, 0) | FLD_VAL(vm_tl, 31, 16);
	dsi_write_reg(lcd_ix, DSI_VM_TIMING3, r);
} 

int dsi2_init(void)
{
	u32 rev,l;
	int r;
	enum dsi lcd_ix = dsi2;

	dsi_2.errors = 0;

	dsi_2.update_mode = OMAP_DSS_UPDATE_DISABLED;
	dsi_2.user_update_mode = OMAP_DSS_UPDATE_DISABLED;
	dsi_2.dsi_mode = OMAP_DSI_MODE_VIDEO;	 
	
#if 0
	rev = dsi_read_reg(lcd_ix, DSI_REVISION);
	printf("OMAP DSI2 rev %d.%d\n",
	FLD_GET(rev, 7, 4), FLD_GET(rev, 3, 0));
#endif

	printf("dsi_display_enable\n");
	
	if (cpu_is_omap44xx())
		*((volatile unsigned int*) 0x4A307100)	|=	0x00030007;  

	REG_FLD_MOD(dsi2, DSI_SYSCONFIG, 1, 1, 1);
	r = _dsi_wait_reset(dsi2);
	if (r)
		return r;

	dispc_set_control2_reg();
	dispc_set_lcd_timings(OMAP_DSS_CHANNEL_LCD2, &cosmo_panel_timings); 
	dispc_set_lcd_size(OMAP_DSS_CHANNEL_LCD2, 480, 801);
	dispc_set_pol_freq(cosmo_config, cosmo_acbi, cosmo_acb);

	r = dsi_pll_init(lcd_ix, 1, 1);
	if (r)
		return r;

	r = dsi_configure_dsi_clocks();
	if (r)
		return r;

	r = dss_read_reg(DSS_CONTROL);	
	r = FLD_MOD(r, 1, 10, 10);	
	r = FLD_MOD(r, 1, 12, 12);	
	dss_write_reg(DSS_CONTROL, r);
	
	puts("PLL OK\n");

	r = dsi_configure_dispc_clocks();
	if (r)
		return r;

	printk("dsi_configure_dispc_clocks is ok\n");

	r = dsi_complexio_init(lcd_ix);
	if (r)
		return r;
	printk("dsi_complexio_init is ok\n");

	dsi_proto_timings();
	dsi_set_lp_clk_divisor();

	if (OMAP_DSI_MODE_VIDEO == dsi_2.dsi_mode) {
		dsi_set_video_mode_timings();		
	}
	
	r = dsi_proto_config();
	if (r)
		return r;
	printk("dsi_proto_config is ok\n");

	dsi_vc_enable(lcd_ix, DSI_VC_CMD, 1);
	dsi_if_enable(lcd_ix, 1);
	dsi_force_tx_stop_mode_io(lcd_ix);

#ifdef CONFIG_ARCH_OMAP4
	
	dsi_write_reg(lcd_ix, DSI_DSIPHY_CFG12, 0x58);

	l = dsi_read_reg(lcd_ix, DSI_DSIPHY_CFG14);
	l = FLD_MOD(l, 1, 31, 31);
	l = FLD_MOD(l, 0x54, 30, 23);
	l = FLD_MOD(l, 1, 19, 19);
	l = FLD_MOD(l, 1, 18, 18);
	l = FLD_MOD(l, 7, 17, 14);
	l = FLD_MOD(l, 1, 11, 11);
	dsi_write_reg(lcd_ix, DSI_DSIPHY_CFG14, l);

	l = dsi_read_reg(lcd_ix, DSI_DSIPHY_CFG8);
	l = FLD_MOD(l, 1, 11, 11);
	l = FLD_MOD(l, 0x10, 10, 6);
	l = FLD_MOD(l, 1, 5, 5);
	l = FLD_MOD(l, 0xE, 3, 0);
	dsi_write_reg(lcd_ix, DSI_DSIPHY_CFG8, l);
#endif

	dsi_2.use_ext_te = 1;

	dss_state = OMAP_DSS_DISPLAY_ACTIVE;

	return 0;
}

void panel_logo_draw(unsigned short *src_ptr)
{
	unsigned int* dest_ptr = (int *)0x87000000;
	unsigned short size;
	unsigned short value;
	int length = LCD_XRES*LCD_YRES;
	unsigned char red, green, blue;

	memset(dest_ptr, 0x00000000, 4 * LCD_XRES * LCD_YRES);
	while (length > 0) {
		size = *src_ptr;
		src_ptr++;
		value = *src_ptr;
		src_ptr++;
		while(size--) {
			red = ((value >> 11) & 0xFF) << 3;
			green = ((value >> 5) & 0xFF) << 2;
			blue = (value & 0xFF) << 3;
			*dest_ptr = (0xFF << 24) | (red << 16) | (green << 8) | blue;
			dest_ptr++;
			length --;
		}
	}
}


void cosmo_panel_emergency_logo_draw()
{
#ifdef CONFIG_COSMO_SU760
	unsigned short* src_ptr = (unsigned short*)web_download;
#else
	unsigned short* src_ptr = (unsigned short*)web_download_eng;
#endif
	panel_logo_draw(src_ptr);
}

void cosmo_panel_logo_draw()
{
	unsigned short* src_ptr = (unsigned short*)lglogo;
	panel_logo_draw(src_ptr);
#if 0
	u32 *ptr;
	u32 y;
	u32 x;
	
	long pos = 0;	
	u32 val = lglogo[pos];
	u32 count = (val&0xFF000000)>>24;
	u32 color = val&0x00FFFFFF;
	
	ptr = (int *)0x87000000;
	memset(ptr,0x00000000,4*LCD_XRES*LCD_YRES);
	ptr += COSMO_LOGO_POS_Y*LCD_XRES+COSMO_LOGO_POS_X;
	for(y = 0 ; y < COSMO_LOGO_HEIGHT ; y++)
	{
		for(x = 0 ; x < COSMO_LOGO_WIDTH ; x++)
		{
			if(count ==0)
			{
				pos++;
				val = lglogo[pos];
				count = (val&0xFF000000)>>24;
				color = val&0x00FFFFFF;
			}
			memcpy(ptr,&color,4);
			ptr++;
			count--;
		}
		ptr += (LCD_XRES-COSMO_LOGO_WIDTH);
	}
#endif
}

#ifndef CONFIG_FBCON
void cosmo_draw_char(u32* pFB,int x, int y, char ch)
{
	int font_y,font_x;

	pFB += y*LCD_XRES+x;

	char bit = 0;
	char* font_bit = NULL;

	font_bit = fontdata + FONT_HEIGHT*ch;
	for(font_y = 0 ; font_y < FONT_HEIGHT; font_y++)
	{	
		for(font_x = 0 ; font_x < FONT_WIDTH; font_x++)
		{
			bit = 1<<FONT_WIDTH - 1 - font_x;
			if(bit&(*font_bit))
				*pFB = FONT_COLOR;

			pFB++;
		}
		pFB += LCD_XRES-FONT_WIDTH;
		font_bit++;
	}
}

void cosmo_draw_text(u32* pFB, int line, char* str,int str_len)
{	
	int x;
	for(x=0;x<str_len;x++)
	{
		if(x>53)
			break;

		if(str[x] == 0x00)
			break;

		cosmo_draw_char(pFB,x*FONT_WGAP,line*FONT_HGAP,str[x]);
	}
}

void cosmo_panel_ap_trap_draw()
{
	static int toggle = 0;
	static int ap_trap_draw_init = 0;
	u32 *ptr;
	u32 y;
	u32 x;	

	ptr = (int *)0x87000000;
	if(ap_trap_draw_init == 0)
	{
		memset(ptr,0xFF,4*LCD_XRES*LCD_YRES);

		cosmo_draw_text(ptr,2,"  AP is reset!!",15);
		cosmo_draw_text(ptr,3,"  Please check the saved log",28);
		cosmo_draw_text(ptr,5,"  log file : /data/panic.txt",28);
		cosmo_draw_text(ptr,76,"       For continue booting, press this button -->",52);
		ap_trap_draw_init = 1;
	}
	else
	{
		switch(toggle)
		{
			case 0:				
				memset(ptr+76*FONT_HGAP*LCD_XRES,0xFF,FONT_HGAP*LCD_XRES*4);
				cosmo_draw_text(ptr,76,"       For continue booting, press this button -->",52);
				break;
			case 1:
			case 2:
				break;
			case 3:
				memset(ptr+76*FONT_HGAP*LCD_XRES,0xFF,FONT_HGAP*LCD_XRES*4);
				cosmo_draw_text(ptr,76,"       For continue booting, press this button-->",51);
				break;
			case 4:
				break;
			case 5:
				toggle =0;
				return;
				break;
		}
		toggle++;
	}
}

void cosmo_panel_cp_trap_draw()
{
	static int toggle = 0;
	static int cp_trap_draw_init = 0;

	u32 *ptr;
	u32 y;
	u32 x;

	ptr = (int *)0x87000000;
	if(cp_trap_draw_init == 0)
	{
		memset(ptr,0xFF,4*LCD_XRES*LCD_YRES);

		cosmo_draw_text(ptr,2,"  CP is reset!!",15);
		
		cosmo_draw_text(ptr,3,"  Please check the core dump files", 34);
		cosmo_draw_text(ptr,4,"  /mnt/sdcard/coredump/", 23);
		cosmo_draw_text(ptr,5,"  You can access the .cd files via UMS", 38);		
		
		cosmo_draw_text(ptr,76,"       For continue booting, press this button -->",52);
		cp_trap_draw_init = 1;
	}
	else
	{
		switch(toggle)
		{
			case 0:				
				memset(ptr+76*FONT_HGAP*LCD_XRES,0xFF,FONT_HGAP*LCD_XRES*4);
				cosmo_draw_text(ptr,76,"       For continue booting, press this button -->",52);
				break;
			case 1:
			case 2:
				break;
			case 3:
				memset(ptr+76*FONT_HGAP*LCD_XRES,0xFF,FONT_HGAP*LCD_XRES*4);
				cosmo_draw_text(ptr,76,"       For continue booting, press this button-->",51);
				break;
			case 4:
				break;
			case 5:
				toggle =0;
				return;
				break;
		}
		toggle++;
	}
}

void cosmo_draw_download_char(u32* pFB,int x, int y, char ch,int mode)
{
	int FONT_WIDTH_Download =  8;
	int FONT_HEIGHT_Download =  16;
	unsigned int FONT_COLOR_Download = 0x0000FF00;

	int font_y,font_x;
	int i=0;

	pFB += y*LCD_XRES+x;

	char bit = 0;
	char* font_bit = NULL;

	if(mode == 0 || mode ==2)
	{
		FONT_WIDTH_Download =  8;
		FONT_HEIGHT_Download =  16;
		FONT_COLOR_Download = 0x0000FF00;
		font_bit = fontdata_8x16+ FONT_HEIGHT_Download*ch;

	}
	else
	{
#ifdef CONFIG_COSMO_SU760
		FONT_WIDTH_Download =  8;
		FONT_HEIGHT_Download =  16;
		FONT_COLOR_Download = 0x00FF0000;
		font_bit = fontdata_8x16+ FONT_HEIGHT_Download*ch;
#else
		FONT_WIDTH_Download =  8;
		FONT_HEIGHT_Download =  8;
		FONT_COLOR_Download = 0x00FF0000;
		font_bit = fontdata+ FONT_HEIGHT_Download*ch;
#endif	
	}
	
	for(font_y = 0 ; font_y < FONT_HEIGHT_Download; font_y++)
	{
		for(font_x = 0 ; font_x < FONT_WIDTH_Download; font_x++)
		{
			bit = 1<<FONT_WIDTH_Download - 1 - font_x;
			if(bit&(*font_bit))
				*pFB = FONT_COLOR_Download;

			pFB++;
		}
	
		pFB += LCD_XRES-FONT_WIDTH_Download;
		font_bit++;	
	}
}

void cosmo_draw_dl_text(u32* pFB, int line, char* str,int str_len,int mode)
{	
	int x;
	for(x=0;x<str_len;x++)
	{
		if(x>53)
			break;

		if(str[x] == 0x00)
			break;

		cosmo_draw_download_char(pFB,x*20,line*20,str[x],mode);
	}
}
#endif

void cosmo_panel_download_logo_draw(int mode)
{
	u32 *ptr;
	
	ptr = (int *)0x87000000;

#ifdef CONFIG_FBCON
	if(mode == 0) {
		memset(ptr, 0x00, 4*LCD_XRES*LCD_YRES);

		fbcon_puts("  S/W Upgrade");
		fbcon_puts("  Please wait");
		fbcon_puts("  while upgrading...");
	} else if(mode == 2) {
		fbcon_puts(" Connect the USB cable");
		fbcon_puts(" for SW updating..");
	} else {
		memset(ptr, 0xFF, 4*LCD_XRES*LCD_YRES);
		fbcon_puts("Factory Download");
	}
#else
	if(mode == 0)
	{
		memset(ptr,0x00,4*LCD_XRES*LCD_YRES);

		cosmo_draw_dl_text(ptr,5,"  S/W Upgrade", strlen("  S/W Upgrade"),mode);
		cosmo_draw_dl_text(ptr,10,"  Please wait", strlen("  Please wait"),mode);
		cosmo_draw_dl_text(ptr,13,"  while upgrading...", strlen("  while upgrading..."),mode);
	}
	else if(mode == 2)
	{

		cosmo_draw_dl_text(ptr,5," Connect the USB cable", strlen(" Connect the USB cable"),mode);
		cosmo_draw_dl_text(ptr,8," for SW updating..", strlen(" for SW updating.."),mode);
	}
	
	else
	{
		memset(ptr,0xFF,4*480*800);				
		cosmo_draw_dl_text(ptr,20,"Factory Download",52 ,mode);
	}
#endif
}
