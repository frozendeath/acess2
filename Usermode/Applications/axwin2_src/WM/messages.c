/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#include "common.h"
#include <acess/sys.h>
#include <net.h>
#include <axwin/messages.h>

#define STATICBUF_SIZE	64

// === TYPES ===
typedef void tMessages_Handle_Callback(int, size_t,void*);

// === PROTOTYPES ===
void	Messages_PollIPC();
void	Messages_RespondIPC(int ID, size_t Length, void *Data);
void	Messages_Handle(tAxWin_Message *Msg, tMessages_Handle_Callback *Respond, int ID);

// === GLOBALS ===
 int	giIPCFileHandle;

// === CODE ===
void IPC_Init(void)
{
	// TODO: Check this
	giIPCFileHandle = open("/Devices/ip/loop/udpc", OPENFLAG_READ|OPENFLAG_EXEC);
//	ioctl(giIPCFileHandle, );
}

void Messages_PollIPC()
{
	 int	len;
	pid_t	tid = 0;
	char	staticBuf[STATICBUF_SIZE];
	tAxWin_Message	*msg;
	
	// Wait for a message
	while( (len = SysGetMessage(&tid, NULL)) == 0 )
		sleep();
	
	// Allocate the space for it
	if( len <= STATICBUF_SIZE )
		msg = (void*)staticBuf;
	else {
		msg = malloc( len );
		if(!msg) {
			fprintf(
				stderr,
				"ERROR - Unable to allocate message buffer, ignoring message from %i\n",
				tid);
			SysGetMessage(NULL, GETMSG_IGNORE);
			return ;
		}
	}
	
	// Get message data
	SysGetMessage(NULL, msg);
	
	Messages_Handle(msg, Messages_RespondIPC, tid);
}

void Messages_RespondIPC(int ID, size_t Length, void *Data)
{
	SysSendMessage(ID, Length, Data);
}

void Messages_Handle(tAxWin_Message *Msg, tMessages_Handle_Callback *Respond, int ID)
{
	switch(Msg->ID)
	{
	#if 0
	case MSG_SREQ_PING:
		Msg->ID = MSG_SRSP_VERSION;
		Msg->Size = 2;
		Msg->Data[0] = 0;
		Msg->Data[1] = 1;
		*(uint16_t*)&Msg->Data[2] = -1;
		Messages_RespondIPC(ID, sizeof(Msg->ID), Msg);
		break;
	#endif
	default:
		fprintf(stderr, "WARNING: Unknown message %i from %i (%p)\n", Msg->ID, ID, Respond);
		_SysDebug("WARNING: Unknown message %i from %i (%p)\n", Msg->ID, ID, Respond);
		break;
	}
}

