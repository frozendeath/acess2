/*
 * AcessOS Basic C Library
 */
#include "stdio_int.h"
#include "lib.h"
#include <stdio.h>
#include <sys/sys.h>

#define USE_CPUID	0

// === PROTOTYPES ===
#if USE_CPUID
static void	cpuid(uint32_t Num, uint32_t *EAX, uint32_t *EBX, uint32_t *EDX, uint32_t *ECX);
#endif
void	ErrorHandler(int Fault);

// === GLOBALS ===
extern char **_envp;
extern struct sFILE	_iob[];
extern struct sFILE	*stdin;
extern struct sFILE	*stdout;
extern struct sFILE	*stderr;
// --- CPU Features ---
#if USE_CPUID
tCPUID	gCPU_Features;
#endif

// === CODE ===
/**
 * \fn int SoMain()
 * \brief Stub Entrypoint
 * \param BaseAddress	Unused - Load Address of libc
 * \param argc	Unused - Argument Count (0 for current version of ld-acess)
 * \param argv	Unused - Arguments (NULL for current version of ld-acess)
 * \param envp	Environment Pointer
 */
int SoMain(unsigned int BaseAddress, int argc, char **argv, char **envp)
{
	// Init for env.c
	_envp = envp;
	
	// Init FileIO Pointers
	stdin = &_iob[0];
	stdin->FD = 0;	stdin->Flags = FILE_FLAG_MODE_READ;
	stdout = &_iob[1];
	stdout->FD = 1;	stdout->Flags = FILE_FLAG_MODE_WRITE;
	stderr = &_iob[2];
	stderr->FD = 2;	stderr->Flags = FILE_FLAG_MODE_WRITE;
	
	#if USE_CPUID
	{
		uint32_t	ecx, edx;
		cpuid(1, NULL, NULL, &edx, &ecx);
		gCPU_Features.SSE  = edx & (1 << 25);	// SSE
		gCPU_Features.SSE2 = edx & (1 << 26);	// SSE2
	}
	#endif
	
	_SysSetFaultHandler(ErrorHandler);
	
	return 1;
}

void ErrorHandler(int Fault)
{
	fprintf(stderr, "Fault = %i\n", Fault);
}

#if USE_CPUID
/**
 * \brief Call the CPUID opcode
 */
static void cpuid(uint32_t Num, uint32_t *EAX, uint32_t *EBX, uint32_t *EDX, uint32_t *ECX)
{
	uint32_t	eax, ebx, edx, ecx;
	
	__asm__ __volatile__ (
		"cpuid"
		: "=a"(eax), "=c"(ecx), "=d"(edx)
		: "a"(Num)
		);
	
	if(EAX)	*EAX = eax;
	if(EBX)	*EBX = ebx;
	if(EDX)	*EDX = edx;
	if(ECX)	*ECX = ecx;
}
#endif
