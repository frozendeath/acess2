/*
 * AcessOS Microkernel Version
 * debug.c
 */
#include <acess.h>
#include <stdarg.h>
#include <debug_hooks.h>

#define	DEBUG_MAX_LINE_LEN	256
#define	LOCK_DEBUG_OUTPUT	0	// Avoid interleaving of output lines?
#define TRACE_TO_KTERM  	0	// Send ENTER/DEBUG/LEAVE/Debug to the VTerm

// === IMPORTS ===
extern void	KernelPanic_SetMode(void);
extern void	KernelPanic_PutChar(char Ch);
extern void	IPStack_SendDebugText(const char *Text);
extern void	VT_SetTerminal(int TerminalID);

// === PROTOTYPES ===
static void	Debug_Putchar(char ch);
static void	Debug_Puts(int bUseKTerm, const char *Str);
void	Debug_FmtS(int bUseKTerm, const char *format, ...);
bool	Debug_Fmt(int bUseKTerm, const char *format, va_list args);
void	Debug_SetKTerminal(const char *File);

// === GLOBALS ===
 int	gDebug_Level = 0;
 int	giDebug_KTerm = -1;
 int	gbDebug_IsKPanic = 0;
volatile int	gbInPutChar = 0;
#if LOCK_DEBUG_OUTPUT
tShortSpinlock	glDebug_Lock;
#endif
// - Disabled because it breaks shit
 int	gbSendNetworkDebug = 0;

// === CODE ===
static void Debug_Putchar(char ch)
{
	Debug_PutCharDebug(ch);
	
	if( gbDebug_IsKPanic )
		KernelPanic_PutChar(ch);

	if( gbDebug_IsKPanic < 2 )
	{
		if(gbInPutChar)	return ;
		gbInPutChar = 1;
		if(giDebug_KTerm != -1)
			VFS_Write(giDebug_KTerm, 1, &ch);
		gbInPutChar = 0;
	}
	
	if( gbSendNetworkDebug )
	{
		char str[2] = {ch, 0};
		IPStack_SendDebugText(str);
	}
}

static void Debug_Puts(int UseKTerm, const char *Str)
{
	 int	len = 0;
	
	Debug_PutStringDebug(Str);
	
	if( gbDebug_IsKPanic )
	{		
		for( len = 0; Str[len]; len ++ )
			KernelPanic_PutChar( Str[len] );
	}
	else
		for( len = 0; Str[len]; len ++ );

	if( gbSendNetworkDebug )
		IPStack_SendDebugText(Str);

	// Output to the kernel terminal
	if( UseKTerm && gbDebug_IsKPanic < 2 && giDebug_KTerm != -1 && gbInPutChar == 0)
	{
		gbInPutChar = 1;
		VFS_Write(giDebug_KTerm, len, Str);
		gbInPutChar = 0;
	}
}

bool Debug_Fmt(int bUseKTerm, const char *format, va_list args)
{
	char	buf[DEBUG_MAX_LINE_LEN];
	buf[DEBUG_MAX_LINE_LEN-1] = 0;
	size_t len = vsnprintf(buf, DEBUG_MAX_LINE_LEN-1, format, args);
	Debug_Puts(bUseKTerm, buf);
	if( len > DEBUG_MAX_LINE_LEN-1 ) {
		// do something
		Debug_Puts(bUseKTerm, "[...]");
		return false;
	}
	return true;
}

void Debug_FmtS(int bUseKTerm, const char *format, ...)
{
	va_list	args;	
	va_start(args, format);
	Debug_Fmt(bUseKTerm, format, args);
	va_end(args);
}

void Debug_KernelPanic(void)
{
	// 5 nested panics? Fuck it
	if( gbDebug_IsKPanic > 5 )
		HALT_CPU();
	gbDebug_IsKPanic ++;
	if( gbDebug_IsKPanic == 1 )
	{
		#if LOCK_DEBUG_OUTPUT
		SHORTREL(&glDebug_Lock);
		#endif
		VT_SetTerminal(7);
	}
	KernelPanic_SetMode();
}

/**
 * \fn void LogF(const char *Msg, ...)
 * \brief Raw debug log (no new line, no prefix)
 * \return True if all of the provided text was printed
 */
bool LogF(const char *Fmt, ...)
{
	#if LOCK_DEBUG_OUTPUT
	if(CPU_HAS_LOCK(&glDebug_Lock)) {
		Debug_Puts("[#]");
		return true;
	}
	SHORTLOCK(&glDebug_Lock);
	#endif
	
	va_list	args;
	va_start(args, Fmt);
	bool rv = Debug_Fmt(1, Fmt, args);
	va_end(args);
	
	#if LOCK_DEBUG_OUTPUT
	SHORTREL(&glDebug_Lock);
	#endif
	return rv;
}
/**
 * \fn void Debug(const char *Msg, ...)
 * \brief Print only to the debug channel (not KTerm)
 */
void Debug(const char *Fmt, ...)
{
	va_list	args;
	
	#if LOCK_DEBUG_OUTPUT
	if(CPU_HAS_LOCK(&glDebug_Lock))	return ;
	SHORTLOCK(&glDebug_Lock);
	#endif

	Debug_Puts(TRACE_TO_KTERM, "Debug: ");
	va_start(args, Fmt);
	Debug_Fmt(TRACE_TO_KTERM, Fmt, args);
	va_end(args);
	Debug_Puts(TRACE_TO_KTERM, "\r\n");
	#if LOCK_DEBUG_OUTPUT
	SHORTREL(&glDebug_Lock);
	#endif
}


void LogFV(const char *Fmt, va_list args)
{
	#if LOCK_DEBUG_OUTPUT
	if(CPU_HAS_LOCK(&glDebug_Lock))	return ;
	SHORTLOCK(&glDebug_Lock);
	#endif

	Debug_Fmt(1, Fmt, args);
	
	#if LOCK_DEBUG_OUTPUT
	SHORTREL(&glDebug_Lock);
	#endif
}

void LogV(const char *Fmt, va_list args)
{
	#if LOCK_DEBUG_OUTPUT
	if(CPU_HAS_LOCK(&glDebug_Lock))	return ;
	SHORTLOCK(&glDebug_Lock);
	#endif

	Debug_Puts(1, "Log: ");
	Debug_Fmt(1, Fmt, args);
	Debug_Puts(1, "\r\n");
	
	#if LOCK_DEBUG_OUTPUT
	SHORTREL(&glDebug_Lock);
	#endif
}

/**
 * \fn void Log(const char *Msg, ...)
 */
void Log(const char *Fmt, ...)
{
	va_list	args;
	va_start(args, Fmt);
	LogV(Fmt, args);
	va_end(args);
}

void Warning(const char *Fmt, ...)
{
	va_list	args;
	
	#if LOCK_DEBUG_OUTPUT
	if(CPU_HAS_LOCK(&glDebug_Lock))	return ;
	SHORTLOCK(&glDebug_Lock);
	#endif
	
	Debug_Puts(1, "Warning: ");
	va_start(args, Fmt);
	Debug_Fmt(1, Fmt, args);
	va_end(args);
	Debug_Putchar('\r');
	Debug_Putchar('\n');
	
	#if LOCK_DEBUG_OUTPUT
	SHORTREL(&glDebug_Lock);
	#endif
}
void Panic(const char *Fmt, ...)
{
	va_list	args;
	
	#if LOCK_DEBUG_OUTPUT
	if( !CPU_HAS_LOCK(&glDebug_Lock) )
		SHORTLOCK(&glDebug_Lock);
	#endif
	// And never SHORTREL
	
	Debug_KernelPanic();
	
	Debug_Puts(1, "\x1b[31m");
	Debug_Puts(1, "Panic: ");
	va_start(args, Fmt);
	Debug_Fmt(1, Fmt, args);
	va_end(args);
	Debug_Puts(1, "\x1b[0m\r\n");

	Proc_PrintBacktrace();
	//Threads_Dump();
	//Heap_Dump();

	HALT_CPU();
}

void Debug_SetKTerminal(const char *File)
{
	if(giDebug_KTerm != -1) {
		// Clear FD to -1 before closing (prevents writes to closed FD)
		int oldfd = giDebug_KTerm;
		giDebug_KTerm = -1;
		VFS_Close(oldfd);
	}
	giDebug_KTerm = VFS_Open(File, VFS_OPENFLAG_WRITE);
}

void Debug_Enter(const char *FuncName, const char *ArgTypes, ...)
{
	va_list	args;
	 int	i;
	 int	pos;
	tTID	tid = Threads_GetTID();
	 
	#if LOCK_DEBUG_OUTPUT
	if(CPU_HAS_LOCK(&glDebug_Lock))	return ;
	SHORTLOCK(&glDebug_Lock);
	#endif

	i = gDebug_Level ++;

	va_start(args, ArgTypes);

	Debug_FmtS(TRACE_TO_KTERM, "%014lli ", now());
	while(i--)	Debug_Puts(TRACE_TO_KTERM, " ");

	Debug_Puts(TRACE_TO_KTERM, FuncName);
	Debug_FmtS(TRACE_TO_KTERM, "[%i]", tid);
	Debug_Puts(TRACE_TO_KTERM, ": (");

	while(*ArgTypes)
	{
		pos = strpos(ArgTypes, ' ');
		if(pos == -1 || pos > 1) {
			if(pos == -1)
				Debug_Puts(TRACE_TO_KTERM, ArgTypes+1);
			else {
				Debug_FmtS(TRACE_TO_KTERM, "%.*s", pos-1, ArgTypes+1);
			}
			Debug_Puts(TRACE_TO_KTERM, "=");
		}
		switch(*ArgTypes)
		{
		case 'p':	Debug_FmtS(TRACE_TO_KTERM, "%p", va_arg(args, void*));	break;
		case 'P':	Debug_FmtS(TRACE_TO_KTERM, "%P", va_arg(args, tPAddr));	break;
		case 's':	Debug_FmtS(TRACE_TO_KTERM, "'%s'", va_arg(args, char*));	break;
		case 'i':	Debug_FmtS(TRACE_TO_KTERM, "%i", va_arg(args, int));	break;
		case 'u':	Debug_FmtS(TRACE_TO_KTERM, "%u", va_arg(args, Uint));	break;
		case 'x':	Debug_FmtS(TRACE_TO_KTERM, "0x%x", va_arg(args, Uint));	break;
		case 'b':	Debug_FmtS(TRACE_TO_KTERM, "0b%b", va_arg(args, Uint));	break;
		case 'X':	Debug_FmtS(TRACE_TO_KTERM, "0x%llx", va_arg(args, Uint64));	break;	// Extended (64-Bit)
		case 'B':	Debug_FmtS(TRACE_TO_KTERM, "0b%llb", va_arg(args, Uint64));	break;	// Extended (64-Bit)
		}
		if(pos != -1) {
			Debug_Puts(TRACE_TO_KTERM, ", ");
		}

		if(pos == -1)	break;
		ArgTypes = &ArgTypes[pos+1];
	}

	va_end(args);
	Debug_Puts(TRACE_TO_KTERM, ")\r\n");
	
	#if LOCK_DEBUG_OUTPUT
	SHORTREL(&glDebug_Lock);
	#endif
}

void Debug_Log(const char *FuncName, const char *Fmt, ...)
{
	va_list	args;
	 int	i = gDebug_Level;
	tTID	tid = Threads_GetTID();

	#if LOCK_DEBUG_OUTPUT
	if(CPU_HAS_LOCK(&glDebug_Lock))	return ;
	SHORTLOCK(&glDebug_Lock);
	#endif

	Debug_FmtS(TRACE_TO_KTERM, "%014lli ", now());
	while(i--)	Debug_Puts(TRACE_TO_KTERM, " ");

	Debug_Puts(TRACE_TO_KTERM, FuncName);
	Debug_FmtS(TRACE_TO_KTERM, "[%i]", tid);
	Debug_Puts(TRACE_TO_KTERM, ": ");

	va_start(args, Fmt);
	Debug_Fmt(TRACE_TO_KTERM, Fmt, args);
	va_end(args);

	Debug_Puts(TRACE_TO_KTERM, "\r\n");
	
	#if LOCK_DEBUG_OUTPUT
	SHORTREL(&glDebug_Lock);
	#endif
}

void Debug_Leave(const char *FuncName, char RetType, ...)
{
	va_list	args;
	 int	i;
	tTID	tid = Threads_GetTID();

	#if LOCK_DEBUG_OUTPUT
	if(CPU_HAS_LOCK(&glDebug_Lock))	return ;
	SHORTLOCK(&glDebug_Lock);
	#endif
	
	i = --gDebug_Level;

	va_start(args, RetType);

	if( i == -1 ) {
		gDebug_Level = 0;
		i = 0;
	}
	Debug_FmtS(TRACE_TO_KTERM, "%014lli ", now());
	// Indenting
	while(i--)	Debug_Puts(TRACE_TO_KTERM, " ");

	Debug_Puts(TRACE_TO_KTERM, FuncName);
	Debug_FmtS(TRACE_TO_KTERM, "[%i]", tid);
	Debug_Puts(TRACE_TO_KTERM, ": RETURN");

	// No Return
	if(RetType == '-') {
		Debug_Puts(TRACE_TO_KTERM, "\r\n");
		#if LOCK_DEBUG_OUTPUT
		SHORTREL(&glDebug_Lock);
		#endif
		return;
	}

	switch(RetType)
	{
	case 'n':	Debug_Puts(TRACE_TO_KTERM, " NULL");	break;
	case 'p':	Debug_Fmt(TRACE_TO_KTERM, " %p", args);	break;
	case 'P':	Debug_Fmt(TRACE_TO_KTERM, " %P", args);	break;	// PAddr
	case 's':	Debug_Fmt(TRACE_TO_KTERM, " '%s'", args);	break;
	case 'i':	Debug_Fmt(TRACE_TO_KTERM, " %i", args);	break;
	case 'u':	Debug_Fmt(TRACE_TO_KTERM, " %u", args);	break;
	case 'x':	Debug_Fmt(TRACE_TO_KTERM, " 0x%x", args);	break;
	// Extended (64-Bit)
	case 'X':	Debug_Fmt(TRACE_TO_KTERM, " 0x%llx", args);	break;
	}
	Debug_Puts(TRACE_TO_KTERM, "\r\n");

	va_end(args);
	
	#if LOCK_DEBUG_OUTPUT
	SHORTREL(&glDebug_Lock);
	#endif
}

void Debug_HexDump(const char *Header, const void *Data, size_t Length)
{
	const Uint8	*cdat = Data;
	Uint	pos = 0;
	LogF("%014lli ", now());
	Debug_Puts(1, Header);
	LogF(" (Hexdump of %p+%i)\r\n", Data, Length);

	#define	CH(n)	((' '<=cdat[(n)]&&cdat[(n)]<0x7F) ? cdat[(n)] : '.')

	while(Length >= 16)
	{
		LogF("%014lli Log: %04x:"
			" %02x %02x %02x %02x %02x %02x %02x %02x "
			" %02x %02x %02x %02x %02x %02x %02x %02x "
			" %c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c\r\n",
			now(),
			pos,
			cdat[ 0], cdat[ 1], cdat[ 2], cdat[ 3], cdat[ 4], cdat[ 5], cdat[ 6], cdat[ 7],
			cdat[ 8], cdat[ 9], cdat[10], cdat[11], cdat[12], cdat[13], cdat[14], cdat[15],
			CH(0),	CH(1),	CH(2),	CH(3),	CH(4),	CH(5),	CH(6),	CH(7),
			CH(8),	CH(9),	CH(10),	CH(11),	CH(12),	CH(13),	CH(14),	CH(15)
			);
		Length -= 16;
		cdat += 16;
		pos += 16;
	}

	{
		 int	i ;
		LogF("%014lli Log: %04x: ", now(), pos);
		for(i = 0; i < Length; i ++)
		{
			LogF("%02x ", cdat[i]);
		}
		for( ; i < 16; i ++)	LogF("   ");
		LogF(" ");
		for(i = 0; i < Length; i ++)
		{
			if( i == 8 )	LogF(" ");
			LogF("%c", CH(i));
		}
	
		Debug_Putchar('\r');
		Debug_Putchar('\n');
	}
}

// --- EXPORTS ---
EXPORT(Debug);
EXPORT(Log);
EXPORT(Warning);
EXPORT(Debug_Enter);
EXPORT(Debug_Log);
EXPORT(Debug_Leave);
