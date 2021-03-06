/*
 * AcessOS Microkernel Version
 * proc.h
 */
#ifndef _PROC_H
#define _PROC_H

// === TYPES ==
typedef struct sTSS {
	Uint32	Link;
	Uint32	ESP0, SS0;
	Uint32	ESP1, SS1;
	Uint32	ESP2, SS2;
	Uint32	CR3;
	Uint32	EIP;
	Uint32	EFLAGS;
	Uint32	EAX, ECX, EDX, EBX;
	Uint32	ESP, EBP, ESI, EDI;
	Uint32	ES, CS, DS, SS, FS, GS;
	Uint32	LDTR;
	Uint16	Resvd, IOPB;	// IO Permissions Bitmap
} __attribute__((packed)) tTSS;

typedef struct {
	#if USE_PAE
	Uint	PDPT[4];
	#else
	Uint32	CR3;
	#endif
} tMemoryState;

// 512 bytes, 16 byte aligned
typedef struct sSSEState
{
	char	data[512];
} tSSEState;

typedef struct {
	Uint	EIP, ESP;
	Uint32	UserCS, UserEIP;
	tSSEState	*SSE;
	 int	bSSEModified;
} tTaskState;

#include <threads_int.h>

#define USER_MAX	KERNEL_BASE

#endif
