/*
 * Acess2
 * Common Binary Loader
 */
#define DEBUG	0
#include <acess.h>
#include <binary.h>
#include <mm_virt.h>

// === CONSTANTS ===
#define BIN_LOWEST	MM_USER_MIN		// 1MiB
#define BIN_GRANUALITY	0x10000		// 64KiB
#define BIN_HIGHEST	(USER_LIB_MAX-BIN_GRANUALITY)		// Just below the kernel
#define	KLIB_LOWEST	MM_MODULE_MIN
#define KLIB_GRANUALITY	0x10000		// 32KiB
#define	KLIB_HIGHEST	(MM_MODULE_MAX-KLIB_GRANUALITY)

// === TYPES ===
typedef struct sKernelBin {
	struct sKernelBin	*Next;
	void	*Base;
	tBinary	*Info;
} tKernelBin;

// === IMPORTS ===
extern int	Proc_Clone(Uint *Err, Uint Flags);
extern char	*Threads_GetName(int ID);
extern Uint	MM_ClearUser(void);
extern void	Proc_StartUser(Uint Entrypoint, Uint *Bases, int ArgC, char **ArgV, char **EnvP, int DataSize);
extern tKernelSymbol	gKernelSymbols[];
extern tKernelSymbol	gKernelSymbolsEnd[];
extern tBinaryType	gELF_Info;

// === PROTOTYPES ===
 int	Proc_Execve(const char *File, const char **ArgV, const char **EnvP);
Uint	Binary_Load(const char *file, Uint *entryPoint);
tBinary *Binary_GetInfo(const char *truePath);
Uint	Binary_MapIn(tBinary *binary);
Uint	Binary_IsMapped(tBinary *binary);
tBinary *Binary_DoLoad(const char *truePath);
void	Binary_Dereference(tBinary *Info);
#if 0
Uint	Binary_Relocate(void *Base);
#endif
Uint	Binary_GetSymbolEx(const char *Name, Uint *Value);
#if 0
Uint	Binary_FindSymbol(void *Base, const char *Name, Uint *Val);
#endif

// === GLOBALS ===
tShortSpinlock	glBinListLock;
tBinary	*glLoadedBinaries = NULL;
char	**gsaRegInterps = NULL;
 int	giRegInterps = 0;
tShortSpinlock	glKBinListLock;
tKernelBin	*glLoadedKernelLibs;
tBinaryType	*gRegBinTypes = &gELF_Info;
 
// === FUNCTIONS ===
/**
 * \brief Registers a binary type
 */
int Binary_RegisterType(tBinaryType *Type)
{
	Type->Next = gRegBinTypes;
	gRegBinTypes = Type;
	return 1;
}

/**
 * \fn int Proc_Spawn(const char *Path)
 */
int Proc_Spawn(const char *Path)
{
	char	stackPath[strlen(Path)+1];
	ENTER("sPath", Path);
	
	strcpy(stackPath, Path);
	
	LOG("stackPath = '%s'\n", stackPath);
	
	if(Proc_Clone(NULL, CLONE_VM) == 0)
	{
		// CHILD
		const char	*args[2] = {stackPath, NULL};
		LOG("stackPath = '%s'\n", stackPath);
		Proc_Execve(stackPath, args, &args[1]);
		for(;;);
	}
	LEAVE('i', 0);
	return 0;
}

/**
 * \fn int Proc_Execve(char *File, char **ArgV, char **EnvP)
 * \brief Replace the current user image with another
 * \param File	File to load as the next image
 * \param ArgV	Arguments to pass to user
 * \param EnvP	User's environment
 * \note Called Proc_ for historical reasons
 */
int Proc_Execve(const char *File, const char **ArgV, const char **EnvP)
{
	 int	argc, envc, i;
	 int	argenvBytes;
	char	**argenvBuf, *strBuf;
	char	**argvSaved, **envpSaved;
	char	*savedFile;
	Uint	entry;
	Uint	bases[2] = {0};
	
	ENTER("sFile pArgV pEnvP", File, ArgV, EnvP);
	
	// --- Save File, ArgV and EnvP (also get argc)
	
	// Count Arguments, Environment Variables and total string sizes
	argenvBytes = 0;
	for( argc = 0; ArgV && ArgV[argc]; argc++ )
		argenvBytes += strlen(ArgV[argc])+1;
	for( envc = 0; EnvP && EnvP[envc]; envc++ )
		argenvBytes += strlen(EnvP[envc])+1;
	argenvBytes = (argenvBytes + sizeof(void*)-1) & ~(sizeof(void*)-1);
	argenvBytes += (argc+1)*sizeof(void*) + (envc+1)*sizeof(void*);
	
	// Allocate
	argenvBuf = malloc(argenvBytes);
	if(argenvBuf == NULL) {
		Log_Error("Binary", "Proc_Execve - What the hell? The kernel is out of heap space");
		LEAVE('i', 0);
		return 0;
	}
	strBuf = (char*)argenvBuf + (argc+1)*sizeof(void*) + (envc+1)*sizeof(void*);
	
	// Populate
	argvSaved = argenvBuf;
	for( i = 0; i < argc; i++ )
	{
		argvSaved[i] = strBuf;
		strcpy(argvSaved[i], ArgV[i]);
		strBuf += strlen(ArgV[i])+1;
	}
	argvSaved[i] = NULL;
	envpSaved = &argvSaved[i+1];
	for( i = 0; i < envc; i++ )
	{
		envpSaved[i] = strBuf;
		strcpy(envpSaved[i], EnvP[i]);
		strBuf += strlen(EnvP[i])+1;
	}
	envpSaved[i] = NULL;
	
	savedFile = malloc(strlen(File)+1);
	strcpy(savedFile, File);
	
	// --- Set Process Name
	Threads_SetName(File);
	
	// --- Clear User Address space
	MM_ClearUser();
	
	// --- Load new binary
	bases[0] = Binary_Load(savedFile, &entry);
	free(savedFile);
	if(bases[0] == 0)
	{
		Log_Warning("Binary", "Proc_Execve - Unable to load '%s'", Threads_GetName(-1));
		LEAVE('-');
		Threads_Exit(0, -10);
		for(;;);
	}
	
	LOG("entry = 0x%x, bases[0] = 0x%x", entry, bases[0]);
	LEAVE('-');
	// --- And... Jump to it
	Proc_StartUser(entry, bases, argc, argvSaved, envpSaved, argenvBytes);
	for(;;);	// Tell GCC that we never return
}

/**
 * \fn Uint Binary_Load(char *file, Uint *entryPoint)
 * \brief Load a binary into the current address space
 * \param file	Path to binary to load
 * \param entryPoint	Pointer for exectuable entry point
 */
Uint Binary_Load(const char *file, Uint *entryPoint)
{
	char	*sTruePath;
	tBinary	*pBinary;
	Uint	base = -1;

	ENTER("sfile pentryPoint", file, entryPoint);
	
	// Sanity Check Argument
	if(file == NULL) {
		LEAVE('x', 0);
		return 0;
	}

	// Get True File Path
	sTruePath = VFS_GetTruePath(file);
	LOG("sTruePath = %p", sTruePath);
	
	if(sTruePath == NULL) {
		Log_Warning("Binary", "%p '%s' does not exist.", file, file);
		LEAVE('x', 0);
		return 0;
	}
	
	LOG("sTruePath = '%s'", sTruePath);
	
	// TODO: Also get modifcation time

	// Check if the binary has already been loaded
	if( !(pBinary = Binary_GetInfo(sTruePath)) )
		pBinary = Binary_DoLoad(sTruePath);	// Else load it
	
	// Clean Up
	free(sTruePath);
	
	// Error Check
	if(pBinary == NULL) {
		LEAVE('x', 0);
		return 0;
	}
	
	#if 0
	if( (base = Binary_IsMapped(pBinary)) ) {
		LEAVE('x', base);
		return base;
	}
	#endif
	
	// Map into process space
	base = Binary_MapIn(pBinary);	// If so then map it in
	
	// Check for errors
	if(base == 0) {
		LEAVE('x', 0);
		return 0;
	}
	
	// Interpret
	if(pBinary->Interpreter) {
		Uint start;
		if( Binary_Load(pBinary->Interpreter, &start) == 0 ) {
			LEAVE('x', 0);
			return 0;
		}
		*entryPoint = start;
	}
	else
		*entryPoint = pBinary->Entry - pBinary->Base + base;
	
	// Return
	LOG("*entryPoint = 0x%x", *entryPoint);
	LEAVE('x', base);
	return base;	// Pass the base as an argument to the user if there is an interpreter
}

/**
 * \brief Finds a matching binary entry
 * \param TruePath	File Identifier (True path name)
 */
tBinary *Binary_GetInfo(const char *TruePath)
{
	tBinary	*pBinary;
	pBinary = glLoadedBinaries;
	while(pBinary)
	{
		if(strcmp(pBinary->TruePath, TruePath) == 0)
			return pBinary;
		pBinary = pBinary->Next;
	}
	return NULL;
}

/**
 \fn Uint Binary_MapIn(tBinary *binary)
 \brief Maps an already-loaded binary into an address space.
 \param binary	Pointer to globally stored data.
*/
Uint Binary_MapIn(tBinary *binary)
{
	Uint	base;
	Uint	addr;
	 int	i;
	
	// Reference Executable (Makes sure that it isn't unloaded)
	binary->ReferenceCount ++;
	
	// Get Binary Base
	base = binary->Base;
	
	// Check if base is free
	if(base != 0)
	{
		for(i=0;i<binary->NumPages;i++)
		{
			if( MM_GetPhysAddr( binary->Pages[i].Virtual & ~0xFFF ) ) {
				base = 0;
				LOG("Address 0x%x is taken\n", binary->Pages[i].Virtual & ~0xFFF);
				break;
			}
		}
	}
	
	// Check if the executable has no base or it is not free
	if(base == 0)
	{
		// If so, give it a base
		base = BIN_HIGHEST;
		while(base >= BIN_LOWEST)
		{
			for(i=0;i<binary->NumPages;i++)
			{
				addr = binary->Pages[i].Virtual & ~0xFFF;
				addr -= binary->Base;
				addr += base;
				if( MM_GetPhysAddr( addr ) )	break;
			}
			// If space was found, break
			if(i == binary->NumPages)		break;
			// Else decrement pointer and try again
			base -= BIN_GRANUALITY;
		}
	}
	
	// Error Check
	if(base < BIN_LOWEST) {
		Log_Warning("BIN", "Executable '%s' cannot be loaded, no space", binary->TruePath);
		return 0;
	}
	
	// Map Executable In
	for(i=0;i<binary->NumPages;i++)
	{
		addr = binary->Pages[i].Virtual & ~0xFFF;
		addr -= binary->Base;
		addr += base;
		LOG("%i - 0x%x to 0x%x", i, addr, binary->Pages[i].Physical);
		MM_Map( addr, (Uint) (binary->Pages[i].Physical) );
		
		// Read-Only?
		if( binary->Pages[i].Flags & BIN_PAGEFLAG_RO)
			MM_SetFlags( addr, MM_PFLAG_RO, -1 );
		else
			MM_SetFlags( addr, MM_PFLAG_COW, -1 );
		
		// Execute?
		if( binary->Pages[i].Flags & BIN_PAGEFLAG_EXEC )
			MM_SetFlags( addr, MM_PFLAG_EXEC, -1 );
		else
			MM_SetFlags( addr, MM_PFLAG_EXEC, 0 );
		
	}
	
	Log_Debug("Binary", "PID %i - Mapped '%s' to 0x%x", Threads_GetPID(), binary->TruePath, base);
	
	//LOG("*0x%x = 0x%x\n", binary->Pages[0].Virtual, *(Uint*)binary->Pages[0].Virtual);
	
	return base;
}

#if 0
/**
 * \fn Uint Binary_IsMapped(tBinary *binary)
 * \brief Check if a binary is already mapped into the address space
 * \param binary	Binary information to check
 * \return Current Base or 0
 */
Uint Binary_IsMapped(tBinary *binary)
{
	Uint	iBase;
	
	// Check prefered base
	iBase = binary->Base;
	if(MM_GetPage( iBase ) == (binary->Pages[0].Physical & ~0xFFF))
		return iBase;
	
	for(iBase = BIN_HIGHEST;
		iBase >= BIN_LOWEST;
		iBase -= BIN_GRANUALITY)
	{
		if(MM_GetPage( iBase ) == (binary->Pages[0].Physical & ~0xFFF))
			return iBase;
	}
	
	return 0;
}
#endif

/**
 * \fn tBinary *Binary_DoLoad(char *truePath)
 * \brief Loads a binary file into memory
 * \param truePath	Absolute filename of binary
 */
tBinary *Binary_DoLoad(const char *truePath)
{
	tBinary	*pBinary;
	 int	fp, i;
	Uint	ident;
	tBinaryType	*bt = gRegBinTypes;
	
	ENTER("struePath", truePath);
	
	// Open File
	fp = VFS_Open(truePath, VFS_OPENFLAG_READ);
	if(fp == -1) {
		LOG("Unable to load file, access denied");
		LEAVE('n');
		return NULL;
	}
	
	// Read File Type
	VFS_Read(fp, 4, &ident);
	VFS_Seek(fp, 0, SEEK_SET);
	
	for(; bt; bt = bt->Next)
	{
		if( (ident & bt->Mask) != (Uint)bt->Ident )
			continue;
		pBinary = bt->Load(fp);
		break;
	}
	if(!bt) {
		Log_Warning("BIN", "'%s' is an unknown file type. (%02x %02x %02x %02x)",
			truePath, ident&0xFF, (ident>>8)&0xFF, (ident>>16)&0xFF, (ident>>24)&0xFF);
		LEAVE('n');
		return NULL;
	}
	
	// Error Check
	if(pBinary == NULL) {
		LEAVE('n');
		return NULL;
	}
	
	// Initialise Structure
	pBinary->ReferenceCount = 0;
	pBinary->TruePath = strdup(truePath);
	
	// Debug Information
	LOG("Interpreter: '%s'", pBinary->Interpreter);
	LOG("Base: 0x%x, Entry: 0x%x", pBinary->Base, pBinary->Entry);
	LOG("NumPages: %i", pBinary->NumPages);
	
	// Read Data
	for(i = 0; i < pBinary->NumPages; i ++)
	{
		Uint	dest;
		tPAddr	paddr;
		paddr = (Uint)MM_AllocPhys();
		if(paddr == 0) {
			Log_Warning("BIN", "Binary_DoLoad - Physical memory allocation failed");
			for( ; i--; ) {
				MM_DerefPhys( pBinary->Pages[i].Physical );
			}
			return NULL;
		}
		MM_RefPhys( paddr );	// Make sure it is _NOT_ freed until we want it to be
		dest = MM_MapTemp( paddr );
		dest += pBinary->Pages[i].Virtual & 0xFFF;
		LOG("dest = 0x%x, paddr = 0x%x", dest, paddr);
		LOG("Pages[%i]={Physical:0x%llx,Virtual:%p,Size:0x%x}",
			i, pBinary->Pages[i].Physical, pBinary->Pages[i].Virtual, pBinary->Pages[i].Size);
		
		// Pure Empty Page
		if(pBinary->Pages[i].Physical == -1) {
			LOG("%i - ZERO", i);
			memset( (void*)dest, 0, 1024 - (pBinary->Pages[i].Virtual & 0xFFF) );
		}
		else
		{
			VFS_Seek( fp, pBinary->Pages[i].Physical, 1 );
			// If the address is not aligned, or the page is not full
			// sized, copy part of it
			if( (dest & 0xFFF) > 0 || pBinary->Pages[i].Size < 0x1000)
			{
				// Validate the size to prevent Kernel page faults
				// Clips to one page and prints a warning
				if( pBinary->Pages[i].Size + (dest & 0xFFF) > 0x1000) {
					Log_Warning("Binary", "Loader error: Page %i (%p) of '%s' is %i bytes (> 4096)",
						i, pBinary->Pages[i].Virtual, truePath,
						(dest&0xFFF) + pBinary->Pages[i].Size);
					pBinary->Pages[i].Size = 0x1000 - (dest & 0xFFF);
				}		
				LOG("%i - 0x%llx - 0x%x bytes",
					i, pBinary->Pages[i].Physical, pBinary->Pages[i].Size);
				// Zero from `dest` to the end of the page
				memset( (void*)dest, 0, 0x1000 - (dest&0xFFF) );
				// Read in the data
				VFS_Read( fp, pBinary->Pages[i].Size, (void*)dest );
			}
			// Full page
			else
			{
				// Check if the page is oversized
				if(pBinary->Pages[i].Size > 0x1000)
					Log_Warning("Binary", "Loader error - Page %i (%p) of '%s' is %i bytes (> 4096)",
						i, pBinary->Pages[i].Virtual, truePath,
						pBinary->Pages[i].Size);
				// Read data
				LOG("%i - 0x%x", i, pBinary->Pages[i].Physical);
				VFS_Read( fp, 0x1000, (void*)dest );
			}
		}
		pBinary->Pages[i].Physical = paddr;
		MM_FreeTemp( dest );
	}
	LOG("Page Count: %i", pBinary->NumPages);
	
	// Close File
	VFS_Close(fp);
	
	// Add to the list
	SHORTLOCK(&glBinListLock);
	pBinary->Next = glLoadedBinaries;
	glLoadedBinaries = pBinary;
	SHORTREL(&glBinListLock);
	
	// Return
	LEAVE('p', pBinary);
	return pBinary;
}

/**
 * \fn void Binary_Unload(void *Base)
 * \brief Unload / Unmap a binary
 * \param Base	Loaded Base
 * \note Currently used only for kernel libaries
 */
void Binary_Unload(void *Base)
{
	tKernelBin	*pKBin;
	tKernelBin	*prev = NULL;
	 int	i;
	
	if((Uint)Base < 0xC0000000)
	{
		// TODO: User Binaries
		Log_Warning("BIN", "Unloading user binaries is currently unimplemented");
		return;
	}
	
	// Kernel Libraries
	for(pKBin = glLoadedKernelLibs;
		pKBin;
		prev = pKBin, pKBin = pKBin->Next)
	{
		// Check the base
		if(pKBin->Base != Base)	continue;
		// Deallocate Memory
		for(i = 0; i < pKBin->Info->NumPages; i++) {
			MM_Deallocate( (Uint)Base + (i << 12) );
		}
		// Dereference Binary
		Binary_Dereference( pKBin->Info );
		// Remove from list
		if(prev)	prev->Next = pKBin->Next;
		else		glLoadedKernelLibs = pKBin->Next;
		// Free Kernel Lib
		free(pKBin);
		return;
	}
}

/**
 * \fn void Binary_Dereference(tBinary *Info)
 * \brief Dereferences and if nessasary, deletes a binary
 * \param Info	Binary information structure
 */
void Binary_Dereference(tBinary *Info)
{
	// Decrement reference count
	Info->ReferenceCount --;
	
	// Check if it is still in use
	if(Info->ReferenceCount)	return;
	
	/// \todo Implement binary freeing
}

/**
 * \fn char *Binary_RegInterp(char *Path)
 * \brief Registers an Interpreter
 * \param Path	Path to interpreter provided by executable
 */
char *Binary_RegInterp(char *Path)
{
	 int	i;
	// NULL Check Argument
	if(Path == NULL)	return NULL;
	// NULL Check the array
	if(gsaRegInterps == NULL)
	{
		giRegInterps = 1;
		gsaRegInterps = malloc( sizeof(char*) );
		gsaRegInterps[0] = malloc( strlen(Path) );
		strcpy(gsaRegInterps[0], Path);
		return gsaRegInterps[0];
	}
	
	// Scan Array
	for( i = 0; i < giRegInterps; i++ )
	{
		if(strcmp(gsaRegInterps[i], Path) == 0)
			return gsaRegInterps[i];
	}
	
	// Interpreter is not in list
	giRegInterps ++;
	gsaRegInterps = malloc( sizeof(char*)*giRegInterps );
	gsaRegInterps[i] = malloc( strlen(Path) );
	strcpy(gsaRegInterps[i], Path);
	return gsaRegInterps[i];
}

// ============
// Kernel Binary Handling
// ============
/**
 * \fn void *Binary_LoadKernel(const char *File)
 * \brief Load a binary into kernel space
 * \note This function shares much with #Binary_Load, but does it's own mapping
 * \param File	File to load into the kernel
 */
void *Binary_LoadKernel(const char *File)
{
	char	*sTruePath;
	tBinary	*pBinary;
	tKernelBin	*pKBinary;
	Uint	base = -1;
	Uint	addr;
	 int	i;

	ENTER("sFile", File);
	
	// Sanity Check Argument
	if(File == NULL) {
		LEAVE('n');
		return 0;
	}

	// Get True File Path
	sTruePath = VFS_GetTruePath(File);
	if(sTruePath == NULL) {
		LEAVE('n');
		return 0;
	}
	
	// Check if the binary has already been loaded
	if( (pBinary = Binary_GetInfo(sTruePath)) )
	{
		for(pKBinary = glLoadedKernelLibs;
			pKBinary;
			pKBinary = pKBinary->Next )
		{
			if(pKBinary->Info == pBinary) {
				LEAVE('p', pKBinary->Base);
				return pKBinary->Base;
			}
		}
	}
	else
		pBinary = Binary_DoLoad(sTruePath);	// Else load it
	
	// Error Check
	if(pBinary == NULL) {
		LEAVE('n');
		return NULL;
	}
	
	// --------------
	// Now pBinary is valid (either freshly loaded or only user mapped)
	// So, map it into kernel space
	// --------------
	
	// Reference Executable (Makes sure that it isn't unloaded)
	pBinary->ReferenceCount ++;
	
	// Check compiled base
	base = pBinary->Base;
	// - Sanity Check
	if(base < KLIB_LOWEST || base > KLIB_HIGHEST || base + (pBinary->NumPages<<12) > KLIB_HIGHEST) {
		base = 0;
	}
	// - Check if it is a valid base address
	if(base != 0)
	{
		for(i=0;i<pBinary->NumPages;i++)
		{
			if( MM_GetPhysAddr( pBinary->Pages[i].Virtual & ~0xFFF ) ) {
				base = 0;
				LOG("Address 0x%x is taken\n", pBinary->Pages[i].Virtual & ~0xFFF);
				break;
			}
		}
	}
	
	// Check if the executable has no base or it is not free
	if(base == 0)
	{
		// If so, give it a base
		base = KLIB_LOWEST;
		while(base < KLIB_HIGHEST)
		{
			for(i = 0; i < pBinary->NumPages; i++)
			{
				addr = pBinary->Pages[i].Virtual & ~0xFFF;
				addr -= pBinary->Base;
				addr += base;
				if( MM_GetPhysAddr( addr ) )	break;
			}
			// If space was found, break
			if(i == pBinary->NumPages)		break;
			// Else decrement pointer and try again
			base += KLIB_GRANUALITY;
		}
	}
	
	// - Error Check
	if(base >= KLIB_HIGHEST) {
		Log_Warning("BIN", "Executable '%s' cannot be loaded into kernel, no space", pBinary->TruePath);
		Binary_Dereference( pBinary );
		LEAVE('n');
		return 0;
	}
	
	LOG("base = 0x%x", base);
	
	// - Map binary in
	LOG("pBinary = {NumPages:%i, Pages=%p}", pBinary->NumPages, pBinary->Pages);
	for(i = 0; i < pBinary->NumPages; i++)
	{
		addr = pBinary->Pages[i].Virtual & ~0xFFF;
		addr -= pBinary->Base;
		addr += base;
		LOG("%i - 0x%x to 0x%x", i, addr, pBinary->Pages[i].Physical);
		MM_Map( addr, (Uint) (pBinary->Pages[i].Physical) );
		MM_SetFlags( addr, MM_PFLAG_KERNEL, MM_PFLAG_KERNEL );
		
		if( pBinary->Pages[i].Flags & BIN_PAGEFLAG_RO)	// Read-Only?
			MM_SetFlags( addr, MM_PFLAG_RO, MM_PFLAG_KERNEL );
	}

	// Relocate Library
	if( !Binary_Relocate( (void*)base ) )
	{
		Log_Warning("BIN", "Relocation of '%s' failed, unloading", sTruePath);
		Binary_Unload( (void*)base );
		Binary_Dereference( pBinary );
		LEAVE('n');
		return 0;
	}
	
	// Add to list (relocator must look at itself manually, not via Binary_GetSymbol)
	pKBinary = malloc(sizeof(*pKBinary));
	pKBinary->Base = (void*)base;
	pKBinary->Info = pBinary;
	SHORTLOCK( &glKBinListLock );
	pKBinary->Next = glLoadedKernelLibs;
	glLoadedKernelLibs = pKBinary;
	SHORTREL( &glKBinListLock );
	
	LEAVE('p', base);
	return (void*)base;
}

/**
 * \fn Uint Binary_Relocate(void *Base)
 * \brief Relocates a loaded binary (used by kernel libraries)
 * \param Base	Loaded base address of binary
 * \return Boolean Success
 */
Uint Binary_Relocate(void *Base)
{
	Uint32	ident = *(Uint32*) Base;
	tBinaryType	*bt = gRegBinTypes;
	
	for(; bt; bt = bt->Next)
	{
		if( (ident & bt->Mask) == (Uint)bt->Ident )
			return bt->Relocate( (void*)Base);
	}
	
	Log_Warning("BIN", "%p is an unknown file type. (%02x %02x %02x %02x)",
		Base, ident&0xFF, (ident>>8)&0xFF, (ident>>16)&0xFF, (ident>>24)&0xFF);
	return 0;
}

/**
 * \fn int Binary_GetSymbol(char *Name, Uint *Val)
 * \brief Get a symbol value
 * \return Value of symbol or -1 on error
 * 
 * Gets the value of a symbol from either the currently loaded
 * libraries or the kernel's exports.
 */
int Binary_GetSymbol(const char *Name, Uint *Val)
{
	if( Binary_GetSymbolEx(Name, Val) )	return 1;
	return 0;
}

/**
 * \fn Uint Binary_GetSymbolEx(char *Name, Uint *Value)
 * \brief Get a symbol value
 * 
 * Gets the value of a symbol from either the currently loaded
 * libraries or the kernel's exports.
 */
Uint Binary_GetSymbolEx(const char *Name, Uint *Value)
{
	 int	i;
	tKernelBin	*pKBin;
	 int	numKSyms = ((Uint)&gKernelSymbolsEnd-(Uint)&gKernelSymbols)/sizeof(tKernelSymbol);
	
	// Scan Kernel
	for( i = 0; i < numKSyms; i++ )
	{
		if(strcmp(Name, gKernelSymbols[i].Name) == 0) {
			*Value = gKernelSymbols[i].Value;
			return 1;
		}
	}
	
	// Scan Loaded Libraries
	for(pKBin = glLoadedKernelLibs;
		pKBin;
		pKBin = pKBin->Next )
	{
		if( Binary_FindSymbol(pKBin->Base, Name, Value) ) {
			return 1;
		}
	}
	
	Log_Warning("BIN", "Unable to find symbol '%s'", Name);
	return 0;
}

/**
 * \fn Uint Binary_FindSymbol(void *Base, char *Name, Uint *Val)
 * \brief Get a symbol from the specified library
 * \param Base	Base address
 * \param Name	Name of symbol to find
 * \param Val	Pointer to place final value
 */
Uint Binary_FindSymbol(void *Base, const char *Name, Uint *Val)
{
	Uint32	ident = *(Uint32*) Base;
	tBinaryType	*bt = gRegBinTypes;
	
	for(; bt; bt = bt->Next)
	{
		if( (ident & bt->Mask) == (Uint)bt->Ident )
			return bt->GetSymbol(Base, Name, Val);
	}
	
	Log_Warning("BIN", "Binary_FindSymbol - %p is an unknown file type. (%02x %02x %02x %02x)",
		Base, ident&0xFF, ident>>8, ident>>16, ident>>24);
	return 0;
}

// === EXPORTS ===
EXPORT(Binary_FindSymbol);
EXPORT(Binary_Unload);
