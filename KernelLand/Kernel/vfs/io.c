/*
 * AcessMicro VFS
 * - File IO Passthru's
 */
#define DEBUG	0
#include <acess.h>
#include "vfs.h"
#include "vfs_int.h"

// === CODE ===
/**
 * \fn Uint64 VFS_Read(int FD, Uint64 Length, void *Buffer)
 * \brief Read data from a node (file)
 */
Uint64 VFS_Read(int FD, Uint64 Length, void *Buffer)
{
	tVFS_Handle	*h;
	Uint64	ret;
	
	ENTER("iFD XLength pBuffer", FD, Length, Buffer);
	
	h = VFS_GetHandle(FD);
	if(!h)	LEAVE_RET('i', -1);
	
	if( !(h->Mode & VFS_OPENFLAG_READ) || h->Node->Flags & VFS_FFLAG_DIRECTORY )
		LEAVE_RET('i', -1);

	if(!h->Node->Type || !h->Node->Type->Read)	LEAVE_RET('i', 0);
	
	ret = h->Node->Type->Read(h->Node, h->Position, Length, Buffer);
	if(ret == -1)	LEAVE_RET('i', -1);
	
	h->Position += ret;
	LEAVE('X', ret);
	return ret;
}

/**
 * \fn Uint64 VFS_ReadAt(int FD, Uint64 Offset, Uint64 Length, void *Buffer)
 * \brief Read data from a given offset (atomic)
 */
Uint64 VFS_ReadAt(int FD, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tVFS_Handle	*h;
	Uint64	ret;
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;
	
	if( !(h->Mode & VFS_OPENFLAG_READ) )	return -1;
	if( h->Node->Flags & VFS_FFLAG_DIRECTORY )	return -1;

	if( !h->Node->Type || !h->Node->Type->Read) {
		Warning("VFS_ReadAt - Node %p, does not have a read method", h->Node);
		return 0;
	}
	ret = h->Node->Type->Read(h->Node, Offset, Length, Buffer);
	if(ret == -1)	return -1;
	return ret;
}

/**
 * \fn Uint64 VFS_Write(int FD, Uint64 Length, const void *Buffer)
 * \brief Read data from a node (file)
 */
Uint64 VFS_Write(int FD, Uint64 Length, const void *Buffer)
{
	tVFS_Handle	*h;
	Uint64	ret;
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;
	
	if( !(h->Mode & VFS_OPENFLAG_WRITE) )	return -1;
	if( h->Node->Flags & VFS_FFLAG_DIRECTORY )	return -1;

	if( !h->Node->Type || !h->Node->Type->Write )	return 0;
	
	ret = h->Node->Type->Write(h->Node, h->Position, Length, Buffer);
	if(ret == -1)	return -1;

	h->Position += ret;
	return ret;
}

/**
 * \fn Uint64 VFS_WriteAt(int FD, Uint64 Offset, Uint64 Length, const void *Buffer)
 * \brief Write data to a file at a given offset
 */
Uint64 VFS_WriteAt(int FD, Uint64 Offset, Uint64 Length, const void *Buffer)
{
	tVFS_Handle	*h;
	Uint64	ret;
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;
	
	if( !(h->Mode & VFS_OPENFLAG_WRITE) )	return -1;
	if( h->Node->Flags & VFS_FFLAG_DIRECTORY )	return -1;

	if(!h->Node->Type || !h->Node->Type->Write)	return 0;
	ret = h->Node->Type->Write(h->Node, Offset, Length, Buffer);

	if(ret == -1)	return -1;
	return ret;
}

/**
 * \fn Uint64 VFS_Tell(int FD)
 * \brief Returns the current file position
 */
Uint64 VFS_Tell(int FD)
{
	tVFS_Handle	*h;
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;
	
	return h->Position;
}

/**
 * \fn int VFS_Seek(int FD, Sint64 Offset, int Whence)
 * \brief Seek to a new location
 * \param FD	File descriptor
 * \param Offset	Where to go
 * \param Whence	From where
 */
int VFS_Seek(int FD, Sint64 Offset, int Whence)
{
	tVFS_Handle	*h;
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;
	
	//Log_Debug("VFS", "VFS_Seek: (fd=0x%x, Offset=0x%llx, Whence=%i)",
	//	FD, Offset, Whence);
	
	// Set relative to current position
	if(Whence == 0) {
		h->Position += Offset;
		return 0;
	}
	
	// Set relative to end of file
	if(Whence < 0) {
		if( h->Node->Size == -1 )	return -1;

		h->Position = h->Node->Size - Offset;
		return 0;
	}
	
	// Set relative to start of file
	h->Position = Offset;
	return 0;
}

/**
 * \fn int VFS_IOCtl(int FD, int ID, void *Buffer)
 * \brief Call an IO Control on a file
 */
int VFS_IOCtl(int FD, int ID, void *Buffer)
{
	tVFS_Handle	*h;
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;

	if(!h->Node->Type || !h->Node->Type->IOCtl)	return -1;
	return h->Node->Type->IOCtl(h->Node, ID, Buffer);
}

/**
 * \fn int VFS_FInfo(int FD, tFInfo *Dest, int MaxACLs)
 * \brief Retrieve file information
 * \return Number of ACLs stored
 */
int VFS_FInfo(int FD, tFInfo *Dest, int MaxACLs)
{
	tVFS_Handle	*h;
	 int	max;
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;

	if( h->Mount )
		Dest->mount = h->Mount->Identifier;
	else
		Dest->mount = 0;
	Dest->inode = h->Node->Inode;	
	Dest->uid = h->Node->UID;
	Dest->gid = h->Node->GID;
	Dest->size = h->Node->Size;
	Dest->atime = h->Node->ATime;
	Dest->ctime = h->Node->MTime;
	Dest->mtime = h->Node->CTime;
	Dest->numacls = h->Node->NumACLs;
	
	Dest->flags = 0;
	if(h->Node->Flags & VFS_FFLAG_DIRECTORY)	Dest->flags |= 0x10;
	if(h->Node->Flags & VFS_FFLAG_SYMLINK)	Dest->flags |= 0x20;
	
	max = (MaxACLs < h->Node->NumACLs) ? MaxACLs : h->Node->NumACLs;
	memcpy(&Dest->acls, h->Node->ACLs, max*sizeof(tVFS_ACL));
	
	return max;
}

// === EXPORTS ===
EXPORT(VFS_Read);
EXPORT(VFS_Write);
EXPORT(VFS_ReadAt);
EXPORT(VFS_WriteAt);
EXPORT(VFS_IOCtl);
EXPORT(VFS_Seek);
EXPORT(VFS_Tell);