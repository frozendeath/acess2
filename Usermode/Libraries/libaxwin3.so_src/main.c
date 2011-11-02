/*
 * AxWin3 Interface Library
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Entrypoint and setup
 */
#include <axwin3/axwin.h>
#include "include/ipc.h"

// === CODE ===
int SoMain(void *Base, int argc, const char *argv[], const char **envp)
{
	// TODO: Parse the environment for the AXWIN3_PID variable
	return 0;
}

void AxWin3_MainLoop(void)
{
	tAxWin_IPCMessage	*msg;
	 int	bExit = 0;	

	while(!bExit)
	{
		msg = AxWin3_int_GetIPCMessage();
		
		// TODO: Handle message
	}
}

