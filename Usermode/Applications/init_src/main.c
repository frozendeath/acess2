/*
 * Acess2 System Init Task
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
//#include "common.h"

// === CONSTANTS ===
#define NUM_TERMS	4
#define	DEFAULT_TERMINAL	"/Devices/VTerm/0"
#define DEFAULT_SHELL	"/Acess/SBin/login"

#define ARRAY_SIZE(x)	((sizeof(x))/(sizeof((x)[0])))

// === PROTOTYPES ===

// === CODE ===
/**
 * \fn int main(int argc, char *argv[])
 * \brief Entrypoint
 */
int main(int argc, char *argv[])
{
	 int	tid;
	 int	i;
	char	termpath[sizeof(DEFAULT_TERMINAL)] = DEFAULT_TERMINAL;
	char	*child_argv[2] = {DEFAULT_SHELL, 0};
	
	// - Parse init script
	
	// - Start virtual terminals
	for( i = 0; i < NUM_TERMS; i++ )
	{		
		tid = clone(CLONE_VM, 0);
		if(tid == 0)
		{
			termpath[sizeof(DEFAULT_TERMINAL)-2] = '0' + i;
			
			_SysOpen(termpath, OPENFLAG_READ);	// Stdin
			_SysOpen(termpath, OPENFLAG_WRITE);	// Stdout
			_SysOpen(termpath, OPENFLAG_WRITE);	// Stderr
			execve(DEFAULT_SHELL, child_argv, NULL);
			for(;;)	;
		}
	}
	
	// TODO: Implement message watching
	for(;;)
		_SysWaitEvent(THREAD_EVENT_IPCMSG);
	
	return 42;
}

