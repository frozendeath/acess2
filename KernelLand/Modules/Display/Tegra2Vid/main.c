/**
 * main.c
 * - Driver core
 */
#define DEBUG	0
#define VERSION	((0<<8)|10)
#include <acess.h>
#include <errno.h>
#include <modules.h>
#include <vfs.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <api_drv_video.h>
#include <lib/keyvalue.h>
#include <options.h>	// ARM Arch
#include "tegra2.h"

#define ABS(a)	((a)>0?(a):-(a))

// === PROTOTYPES ===
// Driver
 int	Tegra2Vid_Install(char **Arguments);
void	Tegra2Vid_Uninstall();
// Internal
// Filesystem
Uint64	Tegra2Vid_Read(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer);
Uint64	Tegra2Vid_Write(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer);
 int	Tegra2Vid_IOCtl(tVFS_Node *node, int id, void *data);
// -- Internals
 int	Tegra2Vid_int_SetMode(int Mode);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, Tegra2Vid, Tegra2Vid_Install, NULL, NULL);
tDevFS_Driver	gTegra2Vid_DriverStruct = {
	NULL, "Tegra2Vid",
	{
	.Read = Tegra2Vid_Read,
	.Write = Tegra2Vid_Write,
	.IOCtl = Tegra2Vid_IOCtl
	}
};
// -- Options
tPAddr	gTegra2Vid_PhysBase = TEGRA2VID_BASE;
 int	gbTegra2Vid_IsVersatile = 1;
// -- KeyVal parse rules
const tKeyVal_ParseRules	gTegra2Vid_KeyValueParser = {
	NULL,
	{
		{"Base", "P", &gTegra2Vid_PhysBase},
		{NULL, NULL, NULL}
	}
};
// -- Driver state
 int	giTegra2Vid_CurrentMode = 0;
 int	giTegra2Vid_BufferMode;
size_t	giTegra2Vid_FramebufferSize;
Uint32	*gpTegra2Vid_IOMem;
tPAddr	gTegra2Vid_FramebufferPhys;
void	*gpTegra2Vid_Framebuffer;
// -- Misc
tDrvUtil_Video_BufInfo	gTegra2Vid_DrvUtil_BufInfo;
tVideo_IOCtl_Pos	gTegra2Vid_CursorPos;

// === CODE ===
/**
 */
int Tegra2Vid_Install(char **Arguments)
{
	return MODULE_ERR_NOTNEEDED;
//	KeyVal_Parse(&gTegra2Vid_KeyValueParser, Arguments);

	gpTegra2Vid_IOMem = (void*)MM_MapHWPages(gTegra2Vid_PhysBase, 256/4);
	{
		Log_Debug("Tegra2Vid", "Display CMD Registers");
		for( int i = 0x000; i <= 0x01A; i ++ )
			Log_Debug("Tegra2Vid", "[0x%03x] = 0x%08x", i, gpTegra2Vid_IOMem[i]);
		for( int i = 0x028; i <= 0x043; i ++ )
			Log_Debug("Tegra2Vid", "[0x%03x] = 0x%08x", i, gpTegra2Vid_IOMem[i]);
		Log_Debug("Tegra2Vid", "Display COM Registers");
		for( int i = 0x300; i <= 0x329; i ++ )
			Log_Debug("Tegra2Vid", "[0x%03x] = 0x%08x", i, gpTegra2Vid_IOMem[i]);
		Log_Debug("Tegra2Vid", "Display DISP Registers");
		for( int i = 0x400; i <= 0x446; i ++ )
			Log_Debug("Tegra2Vid", "[0x%03x] = 0x%08x", i, gpTegra2Vid_IOMem[i]);
		for( int i = 0x480; i <= 0x484; i ++ )
			Log_Debug("Tegra2Vid", "[0x%03x] = 0x%08x", i, gpTegra2Vid_IOMem[i]);
		for( int i = 0x4C0; i <= 0x4C1; i ++ )
			Log_Debug("Tegra2Vid", "[0x%03x] = 0x%08x", i, gpTegra2Vid_IOMem[i]);

		Log_Debug("Tegra2Vid", "WINC_A Registers");
		for( int i = 0x700; i <= 0x714; i ++ )
			Log_Debug("Tegra2Vid", "[0x%03x] = 0x%08x", i, gpTegra2Vid_IOMem[i]);
		Log_Debug("Tegra2Vid", "WINBUF_A");
		for( int i = 0x800; i <= 0x80A; i ++ )
			Log_Debug("Tegra2Vid", "[0x%03x] = 0x%08x", i, gpTegra2Vid_IOMem[i]);
	}
//	return 1;

	// HACK!!!
//	{
//		int	w = 1980, h = 1080;
//		gpTegra2Vid_IOMem[DC_DISP_DISP_ACTIVE_0] = (h << 16) | w;
//		gpTegra2Vid_IOMem[DC_WIN_A_SIZE_0] = (h << 16) | w;
//		gpTegra2Vid_IOMem[DC_WIN_A_PRESCALED_SIZE_0] = (h << 16) | w;
//	}

	giTegra2Vid_FramebufferSize =
		(gpTegra2Vid_IOMem[DC_WIN_A_SIZE_0]&0xFFFF)
		*(gpTegra2Vid_IOMem[DC_WIN_A_SIZE_0]>>16)*4;

	Log_Debug("Tegra2Vid", "giTegra2Vid_FramebufferSize = 0x%x", giTegra2Vid_FramebufferSize);
	gpTegra2Vid_Framebuffer = MM_MapHWPages(
		gpTegra2Vid_IOMem[DC_WINBUF_A_START_ADDR_0],
		(giTegra2Vid_FramebufferSize+PAGE_SIZE-1)/PAGE_SIZE
		);
	memset(gpTegra2Vid_Framebuffer, 0xFF, 0x1000);

	gpTegra2Vid_IOMem[DC_WIN_A_WIN_OPTIONS_0] &= ~0x40;
	gpTegra2Vid_IOMem[DC_WIN_A_COLOR_DEPTH_0] = 12;	// Could be 13 (BGR/RGB)
	gTegra2Vid_DrvUtil_BufInfo.Width = 1024;
	gTegra2Vid_DrvUtil_BufInfo.Height = 768;
	gTegra2Vid_DrvUtil_BufInfo.Pitch = 1024*4;
	gTegra2Vid_DrvUtil_BufInfo.Depth = 32;
	gTegra2Vid_DrvUtil_BufInfo.Framebuffer = gpTegra2Vid_Framebuffer;


//	Tegra2Vid_int_SetMode(4);

	DevFS_AddDevice( &gTegra2Vid_DriverStruct );

	return 0;
}

/**
 * \brief Clean up resources for driver unloading
 */
void Tegra2Vid_Uninstall()
{
}

/**
 * \brief Read from the framebuffer
 */
Uint64 Tegra2Vid_Read(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer)
{
	return 0;
}

/**
 * \brief Write to the framebuffer
 */
Uint64 Tegra2Vid_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	gTegra2Vid_DrvUtil_BufInfo.BufferFormat = giTegra2Vid_BufferMode;
	return DrvUtil_Video_WriteLFB(&gTegra2Vid_DrvUtil_BufInfo, Offset, Length, Buffer);
}

const char *csaTegra2Vid_IOCtls[] = {DRV_IOCTLNAMES, DRV_VIDEO_IOCTLNAMES, NULL};

/**
 * \brief Handle messages to the device
 */
int Tegra2Vid_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	 int	ret = -2;
	ENTER("pNode iID pData", Node, ID, Data);
	
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_VIDEO, "PL110", VERSION, csaTegra2Vid_IOCtls);

	case VIDEO_IOCTL_SETBUFFORMAT:
		DrvUtil_Video_RemoveCursor( &gTegra2Vid_DrvUtil_BufInfo );
		ret = giTegra2Vid_BufferMode;
		if(Data)	giTegra2Vid_BufferMode = *(int*)Data;
		if(gTegra2Vid_DrvUtil_BufInfo.BufferFormat == VIDEO_BUFFMT_TEXT)
			DrvUtil_Video_SetCursor( &gTegra2Vid_DrvUtil_BufInfo, &gDrvUtil_TextModeCursor );
		break;
	
	case VIDEO_IOCTL_GETSETMODE:
		if(Data)
		{
			 int	newMode;
			
			if( !CheckMem(Data, sizeof(int)) )
				LEAVE_RET('i', -1);
			
			newMode = *(int*)Data;
			
			if(newMode < 0 || newMode >= ciTegra2Vid_ModeCount)
				LEAVE_RET('i', -1);

			if(newMode != giTegra2Vid_CurrentMode)
			{
				giTegra2Vid_CurrentMode = newMode;
				Tegra2Vid_int_SetMode( newMode );
			}
		}
		ret = giTegra2Vid_CurrentMode;
		break;
	
	case VIDEO_IOCTL_FINDMODE:
		{
		tVideo_IOCtl_Mode *mode = Data;
		 int	closest, closestArea, reqArea = 0;
		if(!Data || !CheckMem(Data, sizeof(tVideo_IOCtl_Mode)))
			LEAVE_RET('i', -1);
		if( mode->bpp != 32 )
			LEAVE_RET('i', 0);
		if( mode->flags	!= 0 )
			LEAVE_RET('i', 0);

		ret = 0;

		for( int i = 0; i < ciTegra2Vid_ModeCount; i ++ )
		{
			 int	area;
			if(mode->width == caTegra2Vid_Modes[i].W && mode->height == caTegra2Vid_Modes[i].H) {
				mode->id = i;
				ret = 1;
				break;
			}
			
			area = caTegra2Vid_Modes[i].W * caTegra2Vid_Modes[i].H;
			if(!reqArea) {
				reqArea = mode->width * mode->height;
				closest = i;
				closestArea = area;
			}
			else if( ABS(area - reqArea) < ABS(closestArea - reqArea) ) {
				closest = i;
				closestArea = area;
			}
		}
		
		if( ret == 0 )
		{
			mode->id = closest;
			ret = 1;
		}
		mode->width = caTegra2Vid_Modes[mode->id].W;
		mode->height = caTegra2Vid_Modes[mode->id].H;
		break;
		}
	
	case VIDEO_IOCTL_MODEINFO:
		{
		tVideo_IOCtl_Mode *mode = Data;
		if(!Data || !CheckMem(Data, sizeof(tVideo_IOCtl_Mode)))
			LEAVE_RET('i', -1);
		if(mode->id < 0 || mode->id >= ciTegra2Vid_ModeCount)
			LEAVE_RET('i', 0);
		

		mode->bpp = 32;
		mode->flags = 0;
		mode->width = caTegra2Vid_Modes[mode->id].W;
		mode->height = caTegra2Vid_Modes[mode->id].H;

		ret = 1;
		break;
		}
	
	case VIDEO_IOCTL_SETCURSOR:
		if( !Data || !CheckMem(Data, sizeof(tVideo_IOCtl_Pos)) )
			LEAVE_RET('i', -1);

		DrvUtil_Video_RemoveCursor( &gTegra2Vid_DrvUtil_BufInfo );
		
		gTegra2Vid_CursorPos = *(tVideo_IOCtl_Pos*)Data;
		if(gTegra2Vid_DrvUtil_BufInfo.BufferFormat == VIDEO_BUFFMT_TEXT)
			DrvUtil_Video_DrawCursor(
				&gTegra2Vid_DrvUtil_BufInfo,
				gTegra2Vid_CursorPos.x*giVT_CharWidth,
				gTegra2Vid_CursorPos.y*giVT_CharHeight
				);
		else
			DrvUtil_Video_DrawCursor(
				&gTegra2Vid_DrvUtil_BufInfo,
				gTegra2Vid_CursorPos.x,
				gTegra2Vid_CursorPos.y
				);
		break;
	
	default:
		LEAVE('i', -2);
		return -2;
	}
	
	LEAVE('i', ret);
	return ret;
}

//
//
//

int Tegra2Vid_int_SetMode(int Mode)
{
	const struct sTegra2_Disp_Mode	*mode = &caTegra2Vid_Modes[Mode];
	 int	w = mode->W, h = mode->H;	// Horizontal/Vertical Active
	*(Uint32*)(gpTegra2Vid_IOMem + DC_DISP_FRONT_PORCH_0) = (mode->VFP << 16) | mode->HFP; 
	*(Uint32*)(gpTegra2Vid_IOMem + DC_DISP_SYNC_WIDTH_0)  = (mode->HS << 16)  | mode->HS;
	*(Uint32*)(gpTegra2Vid_IOMem + DC_DISP_BACK_PORCH_0)  = (mode->VBP << 16) | mode->HBP;
	*(Uint32*)(gpTegra2Vid_IOMem + DC_DISP_DISP_ACTIVE_0) = (mode->H << 16)   | mode->W;

	*(Uint32*)(gpTegra2Vid_IOMem + DC_WIN_A_POSITION_0) = 0;
	*(Uint32*)(gpTegra2Vid_IOMem + DC_WIN_A_SIZE_0) = (h << 16) | w;
	*(Uint32*)(gpTegra2Vid_IOMem + DC_DISP_DISP_COLOR_CONTROL_0) = 0x8;	// BASE888
	*(Uint32*)(gpTegra2Vid_IOMem + DC_WIN_A_COLOR_DEPTH_0) = 12;	// Could be 13 (BGR/RGB)
	*(Uint32*)(gpTegra2Vid_IOMem + DC_WIN_A_PRESCALED_SIZE_0) = (h << 16) | w;

	Log_Debug("Tegra2Vid", "Mode %i (%ix%i) selected", Mode, w, h);

	if( !gpTegra2Vid_Framebuffer || w*h*4 != giTegra2Vid_FramebufferSize )
	{
		if( gpTegra2Vid_Framebuffer )
		{
			// TODO: Free framebuffer for reallocation
		}

		giTegra2Vid_FramebufferSize = w*h*4;		

		gpTegra2Vid_Framebuffer = (void*)MM_AllocDMA(
			(giTegra2Vid_FramebufferSize + PAGE_SIZE-1) / PAGE_SIZE,
			32,
			&gTegra2Vid_FramebufferPhys
			);
		// TODO: Catch allocation failures
		Log_Debug("Tegra2Vid", "0x%x byte framebuffer at %p (%P phys)",
				giTegra2Vid_FramebufferSize,
				gpTegra2Vid_Framebuffer,
				gTegra2Vid_FramebufferPhys
				);
		
		// Tell hardware
		*(Uint32*)(gpTegra2Vid_IOMem + DC_WINBUF_A_START_ADDR_0) = gTegra2Vid_FramebufferPhys;
		*(Uint32*)(gpTegra2Vid_IOMem + DC_WINBUF_A_ADDR_V_OFFSET_0) = 0;	// Y offset
		*(Uint32*)(gpTegra2Vid_IOMem + DC_WINBUF_A_ADDR_H_OFFSET_0) = 0;	// X offset
	}

	return 0;
}