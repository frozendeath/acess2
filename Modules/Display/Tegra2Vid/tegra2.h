/*
 * Acess2 NVidia Tegra2 Display Driver
 * - By John Hodge (thePowersGang)
 *
 * tegra2.h
 * - Driver definitions
 */
#ifndef _TEGRA2_DISP_H_
#define _TEGRA2_DISP_H_

#define TEGRA2VID_BASE	0x54200000	// 0x40000 Large (256 KB)

const struct sTegra2_Disp_Mode
{
	Uint16	W,   H;
	Uint16	HFP, VFP;
	Uint16	HS,  VS;
	Uint16	HBP, VBP;
}	caTegra2Vid_Modes[] = {
	// TODO: VESA timings
	{720,  487,  16,33,   63, 33,   59, 133},	// NTSC 2
	{720,  576,  12,33,   63, 33,   69, 193},	// PAL 2 (VFP shown as 2/33, used 33)
	{720,  483,  16, 6,   63,  6,   59,  30},	// 480p
	{1280, 720,  70, 5,  804,  6,  220,  20},	// 720p
	{1920,1080,  44, 4,  884,  5,  148,  36},	// 1080p
	// TODO: Can all but HA/VA be constant and those select the resolution?
};
const int ciTegra2Vid_ModeCount = sizeof(caTegra2Vid_Modes)/sizeof(caTegra2Vid_Modes[0]);

enum eTegra2_Disp_Regs
{
	DC_DISP_DISP_SIGNAL_OPTIONS0_0 = 0x400,
	DC_DISP_DISP_SIGNAL_OPTIONS1_0, // 401
	DC_DISP_DISP_WIN_OPTIONS_0,	// 402
	DC_DISP_MEM_HIGH_PRIORITY_0,	// 403
	DC_DISP_MEM_HIGH_PRIORITY_TIMER_0,	// 404
	DC_DISP_DISP_TIMING_OPTIONS_0,	// 405
	DC_DISP_REF_TO_SYNC_0,  	// 406 (TrimSlice 0x0001 000B)
	DC_DISP_SYNC_WIDTH_0,   	// 407 (TrimSlice 0x0004 003A)
	DC_DISP_BACK_PORCH_0,   	// 408 (TrimSlice 0x0004 003A)
	DC_DISP_DISP_ACTIVE_0,  	// 409 (TrimSlice 0x0300 0400)
	DC_DISP_FRONT_PORCH_0,  	// 40A (TrimSlice 0x0004 003A)

	DC_DISP_H_PULSE0_CONTROL_0,	// 40B

	DC_DISP_DISP_COLOR_CONTROL_0 = 0x430,
	
	DC_WINC_A_COLOR_PALETTE_0 = 0x500,
	DC_WINC_A_PALETTE_COLOR_EXT_0 = 0x600,
	DC_WIN_A_WIN_OPTIONS_0 = 0x700,
	DC_WIN_A_BYTE_SWAP_0,   	// 701
	DC_WIN_A_BUFFER_CONTROL_0,	// 702
	DC_WIN_A_COLOR_DEPTH_0, 	// 703
	DC_WIN_A_POSITION_0,    	// 704
	DC_WIN_A_SIZE_0,        	// 705 (TrimSlice 0x0300 0400)
	DC_WIN_A_PRESCALED_SIZE_0,
	DC_WIN_A_H_INITIAL_DDA_0,
	DC_WIN_A_V_INITIAL_DDA_0,
	DC_WIN_A_DDA_INCREMENT_0,
	DC_WIN_A_LINE_STRIDE_0,
	DC_WIN_A_BUF_STRIDE_0,
	DC_WIN_A_BUFFER_ADDR_MODE_0,
	DC_WIN_A_DV_CONTROL_0,
	DC_WIN_A_BLEND_NOKEY_0,
	
	DC_WINBUF_A_START_ADDR_0 = 0x800,
	DC_WINBUF_A_START_ADDR_NS_0,
	DC_WINBUF_A_ADDR_H_OFFSET_0,
	DC_WINBUF_A_ADDR_H_OFFSET_NS_0,
	DC_WINBUF_A_ADDR_V_OFFSET_0,
	DC_WINBUF_A_ADDR_V_OFFSET_NS_0,
};

#endif

