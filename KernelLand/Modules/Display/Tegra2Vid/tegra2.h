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
	{1024, 768,  58, 4,   58,  4,   58,   4},	// 1024x768 (reset), RtS=11,4
	// TV Timings
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
	DC_CMD_STATE_CONTROL_0 = 0x041,
	DC_CMD_DISPLAY_WINDOW_HEADER_0,	// 042
	DC_CMD_REG_ACT_CONTROL_0,	// 043

	DC_COM_CRC_CONTROL_0 = 0x300,
	DC_COM_CRC_CHECKSUM_0,  	// 301
	DC_COM_PIN_OUTPUT_ENABLE0_0,	// 302
	DC_COM_PIN_OUTPUT_ENABLE1_0,	// 303
	DC_COM_PIN_OUTPUT_ENABLE2_0,	// 304
	DC_COM_PIN_OUTPUT_ENABLE3_0,	// 305
	DC_COM_PIN_OUTPUT_POLARITY0_0,	// 306
	DC_COM_PIN_OUTPUT_POLARITY1_0,	// 307
	DC_COM_PIN_OUTPUT_POLARITY2_0,	// 308
	DC_COM_PIN_OUTPUT_POLARITY3_0,	// 309
	DC_COM_PIN_OUTPUT_DATA0_0,	// 30A
	DC_COM_PIN_OUTPUT_DATA1_0,	// 30B
	DC_COM_PIN_OUTPUT_DATA2_0,	// 30C
	DC_COM_PIN_OUTPUT_DATA3_0,	// 30D
	DC_COM_PIN_INPUT_ENABLE0_0,	// 30E
	DC_COM_PIN_INPUT_ENABLE1_0,	// 30F
	DC_COM_PIN_INPUT_ENABLE2_0,	// 310
	DC_COM_PIN_INPUT_ENABLE3_0,	// 311
	DC_COM_PIN_INPUT_DATA0_0,	// 312
	DC_COM_PIN_INPUT_DATA1_0,	// 313
	DC_COM_PIN_OUTPUT_SELECT0_0,	// 314
	DC_COM_PIN_OUTPUT_SELECT1_0,	// 315
	DC_COM_PIN_OUTPUT_SELECT2_0,	// 316
	DC_COM_PIN_OUTPUT_SELECT3_0,	// 317
	DC_COM_PIN_OUTPUT_SELECT4_0,	// 318
	DC_COM_PIN_OUTPUT_SELECT5_0,	// 319
	DC_COM_PIN_OUTPUT_SELECT6_0,	// 31A
	DC_COM_PIN_MISC_CONTROL_0,	// 31B
	// TODO: Complete

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
	DC_DISP_H_PULSE0_POSITION_A_0,	// 40C
	DC_DISP_H_PULSE0_POSITION_B_0,	// 40D
	DC_DISP_H_PULSE0_POSITION_C_0,	// 40E
	DC_DISP_H_PULSE0_POSITION_D_0,	// 40F
	DC_DISP_H_PULSE1_CONTROL_0,	// 410
	DC_DISP_H_PULSE1_POSITION_A_0,	// 411
	DC_DISP_H_PULSE1_POSITION_B_0,	// 412
	DC_DISP_H_PULSE1_POSITION_C_0,	// 413
	DC_DISP_H_PULSE1_POSITION_D_0,	// 414
	DC_DISP_H_PULSE2_CONTROL_0,	// 415
	DC_DISP_H_PULSE2_POSITION_A_0,	// 416
	DC_DISP_H_PULSE2_POSITION_B_0,	// 417
	DC_DISP_H_PULSE2_POSITION_C_0,	// 418
	DC_DISP_H_PULSE2_POSITION_D_0,	// 419
	DC_DISP_V_PULSE0_CONTROL_0,	// 41A
	DC_DISP_V_PULSE0_POSITION_A_0,	// 41B
	DC_DISP_V_PULSE0_POSITION_B_0,	// 41C
	DC_DISP_V_PULSE0_POSITION_C_0,	// 41D
	DC_DISP_V_PULSE1_CONTROL_0,	// 41E
	DC_DISP_V_PULSE1_POSITION_A_0,	// 41F
	DC_DISP_V_PULSE1_POSITION_B_0,	// 420
	DC_DISP_V_PULSE1_POSITION_C_0,	// 421
	DC_DISP_V_PULSE2_CONTROL_0,	// 422
	DC_DISP_V_PULSE2_POSITION_A_0,	// 423
	DC_DISP_V_PULSE3_CONTROL_0,	// 424
	DC_DISP_V_PULSE3_POSITION_A_0,	// 425
	DC_DISP_M0_CONTROL_0,   	// 426
	DC_DISP_M1_CONTROL_0,   	// 427
	DC_DISP_DI_CONTROL_0,   	// 428
	DC_DISP_PP_CONTROL_0,   	// 429
	DC_DISP_PP_SELECT_A_0,  	// 42A
	DC_DISP_PP_SELECT_B_0,  	// 42B
	DC_DISP_PP_SELECT_C_0,  	// 42C
	DC_DISP_PP_SELECT_D_0,  	// 42D
	DC_DISP_DISP_CLOCK_CONTROL_0,	// 42E
	DC_DISP_DISP_INTERFACE_CONTROL_0,//42F
	DC_DISP_DISP_COLOR_CONTROL_0,	// 430
	DC_DISP_SHIFT_CLOCK_OPTIONS_0,	// 431
	DC_DISP_DATA_ENABLE_OPTIONS_0,	// 432
	DC_DISP_SERIAL_INTERFACE_OPTIONS_0,	// 433
	DC_DISP_LCD_SPI_OPTIONS_0,	// 434
	DC_DISP_BORDER_COLOR_0, 	// 435
	DC_DISP_COLOR_KEY0_LOWER_0,	// 436
	DC_DISP_COLOR_KEY0_UPPER_0,	// 437
	DC_DISP_COLOR_KEY1_LOWER_0,	// 438
	DC_DISP_COLOR_KEY1_UPPER_0,	// 439
	_DC_DISP_UNUSED_43A,
	_DC_DISP_UNUSED_43B,
	DC_DISP_CURSOR_FOREGROUND_0,	// 43C - IMPORTANT
	DC_DISP_CURSOR_BACKGROUND_0,	// 43D - IMPORTANT
	DC_DISP_CURSOR_START_ADDR_0,	// 43E - IMPORTANT
	DC_DISP_CURSOR_START_ADDR_NS_0,	// 43F - IMPORTANT
	DC_DISP_CURSOR_POSITION_0,	// 440 - IMPORTANT
	DC_DISP_CURSOR_POSITION_NS_0,	// 441 - IMPORTANT
	DC_DISP_INIT_SEQ_CONTROL_0,	// 442
	DC_DISP_SPI_INIT_SEQ_DATA_A_0,	// 443
	DC_DISP_SPI_INIT_SEQ_DATA_B_0,	// 444
	DC_DISP_SPI_INIT_SEQ_DATA_C_0,	// 445
	DC_DISP_SPI_INIT_SEQ_DATA_D_0,	// 446

	DC_DISP_DC_MCCIF_FIFOCTRL_0 = 0x480,
	DC_DISP_MCCIF_DISPLAY0A_HYST_0,	// 481
	DC_DISP_MCCIF_DISPLAY0B_HYST_0,	// 482
	DC_DISP_MCCIF_DISPLAY0C_HYST_0,	// 483
	DC_DISP_MCCIF_DISPLAY1B_HYST_0,	// 484

	DC_DISP_DAC_CRT_CTRL_0 = 0x4C0,
	DC_DISP_DISP_MISC_CONTROL_0,	// 4C1

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
	DC_WIN_A_BLEND_1WIN_0,
	DC_WIN_A_BLEND_2WIN_B_0,
	DC_WIN_A_BLEND_2WIN_C_0,
	DC_WIN_A_BLEND_3WIN_BC_0,
	DC_WIN_A_HP_FETCH_CONTROL_0,

	
	DC_WINBUF_A_START_ADDR_0 = 0x800,
	DC_WINBUF_A_START_ADDR_NS_0,
	DC_WINBUF_A_ADDR_H_OFFSET_0,
	DC_WINBUF_A_ADDR_H_OFFSET_NS_0,
	DC_WINBUF_A_ADDR_V_OFFSET_0,
	DC_WINBUF_A_ADDR_V_OFFSET_NS_0,
};

#if DEBUG || DUMP_REGISTERS
const char * const csaTegra2Vid_RegisterNames[] = {
	[0x000] = "DC_CMD_GENERAL_INCR_SYNCPT_0",
	"DC_CMD_GENERAL_INCR_SYNCPT_CNTRL_0",
	"DC_CMD_GENERAL_INCR_SYNCPT_ERROR_0",
	[0x008] = "DC_CMD_WIN_A_INCR_SYNCPT_0",
	"DC_CMD_WIN_A_INCR_SYNCPT_CNTRL_0",
	"DC_CMD_WIN_A_INCR_SYNCPT_ERROR_0",
	[0x010] = "DC_CMD_WIN_B_INCR_SYNCPT_0",
	"DC_CMD_WIN_B_INCR_SYNCPT_CNTRL_0",
	"DC_CMD_WIN_B_INCR_SYNCPT_ERROR_0",
	[0x018] = "DC_CMD_WIN_C_INCR_SYNCPT_0",
	"DC_CMD_WIN_C_INCR_SYNCPT_CNTRL_0",
	"DC_CMD_WIN_C_INCR_SYNCPT_ERROR_0",
	[0x028] = "DC_CMD_CONT_SYNCPT_VSYNC_0",
	[0x030] = "DC_CMD_CTXSW_0",
	"DC_CMD_DISPLAY_COMMAND_OPTION0_0",
	"DC_CMD_DISPLAY_COMMAND_0",
	"DC_CMD_SIGNAL_RAISE_0",
	[0x036] = "DC_CMD_DISPLAY_POWER_CONTROL_0",
	"DC_CMD_INT_STATUS_0",
	"DC_CMD_INT_MASK_0",
	"DC_CMD_INT_ENABLE_0",
	"DC_CMD_INT_TYPE_0",
	"DC_CMD_INT_POLARITY_0",
	"DC_CMD_SIGNAL_RAISE1_0",
	"DC_CMD_SIGNAL_RAISE2_0",
	"DC_CMD_SIGNAL_RAISE3_0",
	
	[0x040] = "DC_CMD_STATE_ACCESS_0",
	"DC_CMD_STATE_CONTROL_0",
	"DC_CMD_DISPLAY_WINDOW_HEADER_0",	// 042
	"DC_CMD_REG_ACT_CONTROL_0",	// 043

	[0x300] = "DC_COM_CRC_CONTROL_0",
	"DC_COM_CRC_CHECKSUM_0",  	// 301
	"DC_COM_PIN_OUTPUT_ENABLE0_0",	// 302
	"DC_COM_PIN_OUTPUT_ENABLE1_0",	// 303
	"DC_COM_PIN_OUTPUT_ENABLE2_0",	// 304
	"DC_COM_PIN_OUTPUT_ENABLE3_0",	// 305
	"DC_COM_PIN_OUTPUT_POLARITY0_0",	// 306
	"DC_COM_PIN_OUTPUT_POLARITY1_0",	// 307
	"DC_COM_PIN_OUTPUT_POLARITY2_0",	// 308
	"DC_COM_PIN_OUTPUT_POLARITY3_0",	// 309
	"DC_COM_PIN_OUTPUT_DATA0_0",	// 30A
	"DC_COM_PIN_OUTPUT_DATA1_0",	// 30B
	"DC_COM_PIN_OUTPUT_DATA2_0",	// 30C
	"DC_COM_PIN_OUTPUT_DATA3_0",	// 30D
	"DC_COM_PIN_INPUT_ENABLE0_0",	// 30E
	"DC_COM_PIN_INPUT_ENABLE1_0",	// 30F
	"DC_COM_PIN_INPUT_ENABLE2_0",	// 310
	"DC_COM_PIN_INPUT_ENABLE3_0",	// 311
	"DC_COM_PIN_INPUT_DATA0_0",	// 312
	"DC_COM_PIN_INPUT_DATA1_0",	// 313
	"DC_COM_PIN_OUTPUT_SELECT0_0",	// 314
	"DC_COM_PIN_OUTPUT_SELECT1_0",	// 315
	"DC_COM_PIN_OUTPUT_SELECT2_0",	// 316
	"DC_COM_PIN_OUTPUT_SELECT3_0",	// 317
	"DC_COM_PIN_OUTPUT_SELECT4_0",	// 318
	"DC_COM_PIN_OUTPUT_SELECT5_0",	// 319
	"DC_COM_PIN_OUTPUT_SELECT6_0",	// 31A
	"DC_COM_PIN_MISC_CONTROL_0",	// 31B
	// TODO: Complete

	[0x400] = "DC_DISP_DISP_SIGNAL_OPTIONS0_0",
	"DC_DISP_DISP_SIGNAL_OPTIONS1_0", // 401
	"DC_DISP_DISP_WIN_OPTIONS_0",	// 402
	"DC_DISP_MEM_HIGH_PRIORITY_0",	// 403
	"DC_DISP_MEM_HIGH_PRIORITY_TIMER_0",	// 404
	"DC_DISP_DISP_TIMING_OPTIONS_0",	// 405
	"DC_DISP_REF_TO_SYNC_0",  	// 406 (TrimSlice 0x0001 000B)
	"DC_DISP_SYNC_WIDTH_0",   	// 407 (TrimSlice 0x0004 003A)
	"DC_DISP_BACK_PORCH_0",   	// 408 (TrimSlice 0x0004 003A)
	"DC_DISP_DISP_ACTIVE_0",  	// 409 (TrimSlice 0x0300 0400)
	"DC_DISP_FRONT_PORCH_0",  	// 40A (TrimSlice 0x0004 003A)
	"DC_DISP_H_PULSE0_CONTROL_0",	// 40B
	"DC_DISP_H_PULSE0_POSITION_A_0",	// 40C
	"DC_DISP_H_PULSE0_POSITION_B_0",	// 40D
	"DC_DISP_H_PULSE0_POSITION_C_0",	// 40E
	"DC_DISP_H_PULSE0_POSITION_D_0",	// 40F
	"DC_DISP_H_PULSE1_CONTROL_0",	// 410
	"DC_DISP_H_PULSE1_POSITION_A_0",	// 411
	"DC_DISP_H_PULSE1_POSITION_B_0",	// 412
	"DC_DISP_H_PULSE1_POSITION_C_0",	// 413
	"DC_DISP_H_PULSE1_POSITION_D_0",	// 414
	"DC_DISP_H_PULSE2_CONTROL_0",	// 415
	"DC_DISP_H_PULSE2_POSITION_A_0",	// 416
	"DC_DISP_H_PULSE2_POSITION_B_0",	// 417
	"DC_DISP_H_PULSE2_POSITION_C_0",	// 418
	"DC_DISP_H_PULSE2_POSITION_D_0",	// 419
	"DC_DISP_V_PULSE0_CONTROL_0",	// 41A
	"DC_DISP_V_PULSE0_POSITION_A_0",	// 41B
	"DC_DISP_V_PULSE0_POSITION_B_0",	// 41C
	"DC_DISP_V_PULSE0_POSITION_C_0",	// 41D
	"DC_DISP_V_PULSE1_CONTROL_0",	// 41E
	"DC_DISP_V_PULSE1_POSITION_A_0",	// 41F
	"DC_DISP_V_PULSE1_POSITION_B_0",	// 420
	"DC_DISP_V_PULSE1_POSITION_C_0",	// 421
	"DC_DISP_V_PULSE2_CONTROL_0",	// 422
	"DC_DISP_V_PULSE2_POSITION_A_0",	// 423
	"DC_DISP_V_PULSE3_CONTROL_0",	// 424
	"DC_DISP_V_PULSE3_POSITION_A_0",	// 425
	"DC_DISP_M0_CONTROL_0",   	// 426
	"DC_DISP_M1_CONTROL_0",   	// 427
	"DC_DISP_DI_CONTROL_0",   	// 428
	"DC_DISP_PP_CONTROL_0",   	// 429
	"DC_DISP_PP_SELECT_A_0",  	// 42A
	"DC_DISP_PP_SELECT_B_0",  	// 42B
	"DC_DISP_PP_SELECT_C_0",  	// 42C
	"DC_DISP_PP_SELECT_D_0",  	// 42D
	"DC_DISP_DISP_CLOCK_CONTROL_0",	// 42E
	"DC_DISP_DISP_INTERFACE_CONTROL_0",//42F
	"DC_DISP_DISP_COLOR_CONTROL_0",	// 430
	"DC_DISP_SHIFT_CLOCK_OPTIONS_0",	// 431
	"DC_DISP_DATA_ENABLE_OPTIONS_0",	// 432
	"DC_DISP_SERIAL_INTERFACE_OPTIONS_0",	// 433
	"DC_DISP_LCD_SPI_OPTIONS_0",	// 434
	"DC_DISP_BORDER_COLOR_0", 	// 435
	"DC_DISP_COLOR_KEY0_LOWER_0",	// 436
	"DC_DISP_COLOR_KEY0_UPPER_0",	// 437
	"DC_DISP_COLOR_KEY1_LOWER_0",	// 438
	"DC_DISP_COLOR_KEY1_UPPER_0",	// 439
	"_DC_DISP_UNUSED_43A",
	"_DC_DISP_UNUSED_43B",
	"DC_DISP_CURSOR_FOREGROUND_0",	// 43C - IMPORTANT
	"DC_DISP_CURSOR_BACKGROUND_0",	// 43D - IMPORTANT
	"DC_DISP_CURSOR_START_ADDR_0",	// 43E - IMPORTANT
	"DC_DISP_CURSOR_START_ADDR_NS_0",	// 43F - IMPORTANT
	"DC_DISP_CURSOR_POSITION_0",	// 440 - IMPORTANT
	"DC_DISP_CURSOR_POSITION_NS_0",	// 441 - IMPORTANT
	"DC_DISP_INIT_SEQ_CONTROL_0",	// 442
	"DC_DISP_SPI_INIT_SEQ_DATA_A_0",	// 443
	"DC_DISP_SPI_INIT_SEQ_DATA_B_0",	// 444
	"DC_DISP_SPI_INIT_SEQ_DATA_C_0",	// 445
	"DC_DISP_SPI_INIT_SEQ_DATA_D_0",	// 446

	[0x480] = "DC_DISP_DC_MCCIF_FIFOCTRL_0",
	"DC_DISP_MCCIF_DISPLAY0A_HYST_0",	// 481
	"DC_DISP_MCCIF_DISPLAY0B_HYST_0",	// 482
	"DC_DISP_MCCIF_DISPLAY0C_HYST_0",	// 483
	"DC_DISP_MCCIF_DISPLAY1B_HYST_0",	// 484

	[0x4C0] = "DC_DISP_DAC_CRT_CTRL_0",
	"DC_DISP_DISP_MISC_CONTROL_0",	// 4C1

	[0x500] = "DC_WINC_A_COLOR_PALETTE_0",
	[0x600] = "DC_WINC_A_PALETTE_COLOR_EXT_0",
	[0x700] = "DC_WIN_A_WIN_OPTIONS_0",
	"DC_WIN_A_BYTE_SWAP_0",   	// 701
	"DC_WIN_A_BUFFER_CONTROL_0",	// 702
	"DC_WIN_A_COLOR_DEPTH_0", 	// 703
	"DC_WIN_A_POSITION_0",    	// 704
	"DC_WIN_A_SIZE_0",        	// 705 (TrimSlice 0x0300 0400)
	"DC_WIN_A_PRESCALED_SIZE_0",
	"DC_WIN_A_H_INITIAL_DDA_0",
	"DC_WIN_A_V_INITIAL_DDA_0",
	"DC_WIN_A_DDA_INCREMENT_0",
	"DC_WIN_A_LINE_STRIDE_0",
	"DC_WIN_A_BUF_STRIDE_0",
	"DC_WIN_A_BUFFER_ADDR_MODE_0",
	"DC_WIN_A_DV_CONTROL_0",
	"DC_WIN_A_BLEND_NOKEY_0",
	"DC_WIN_A_BLEND_1WIN_0",
	"DC_WIN_A_BLEND_2WIN_B_0",
	"DC_WIN_A_BLEND_2WIN_C_0",
	"DC_WIN_A_BLEND_3WIN_BC_0",
	"DC_WIN_A_HP_FETCH_CONTROL_0",
	
	[0x800] = "DC_WINBUF_A_START_ADDR_0",
	[0x801] = "DC_WINBUF_A_START_ADDR_NS_0",
	[0x806] = "DC_WINBUF_A_ADDR_H_OFFSET_0",
	[0x807] = "DC_WINBUF_A_ADDR_H_OFFSET_NS_0",
	[0x808] = "DC_WINBUF_A_ADDR_V_OFFSET_0",
	[0x809] = "DC_WINBUF_A_ADDR_V_OFFSET_NS_0",
	[0x80A] = "DC_WINBUF_A_UFLOW_STATUS"
};
#endif

#endif

