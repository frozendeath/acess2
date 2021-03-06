/*
 * Acess OS
 * Ext2 Driver Version 1
 */
/**
 * \file write.c
 * \brief Second Extended Filesystem Driver
 * \todo Implement file full write support
 */
#define DEBUG	0
#define VERBOSE	0
#include "ext2_common.h"

// === PROTOYPES ===
Uint32		Ext2_int_AllocateBlock(tExt2_Disk *Disk, Uint32 PrevBlock);
void	Ext2_int_DeallocateBlock(tExt2_Disk *Disk, Uint32 Block);

// === CODE ===
/**
 * \brief Write to a file
 */
size_t Ext2_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags)
{
	tExt2_Disk	*disk = Node->ImplPtr;
	tExt2_Inode	*inode = (void*)(Node+1);
	Uint64	base;
	Uint64	retLen;
	Uint	block;
	Uint64	allocSize;
	 int	bNewBlocks = 0;
	
	//Debug_HexDump("Ext2_Write", Buffer, Length);

	// TODO: Handle (Flags & VFS_IOFLAG_NOBLOCK)	
	
	// Get the ammount of space already allocated
	// - Round size up to block size
	// - block size is a power of two, so this will work
	allocSize = (inode->i_size + disk->BlockSize-1) & ~(disk->BlockSize-1);
	LOG("allocSize = %llx, Offset=%llx", allocSize, Offset);
	
	// Are we writing to inside the allocated space?
	if( Offset > allocSize )	return 0;
	
	if( Offset < allocSize )
	{
		// Will we go out of it?
		if(Offset + Length > allocSize) {
			bNewBlocks = 1;
			retLen = allocSize - Offset;
		} else
			retLen = Length;
		
		// Within the allocated space
		block = Offset / disk->BlockSize;
		Offset %= disk->BlockSize;
		base = Ext2_int_GetBlockAddr(disk, inode->i_block, block);
		
		// Write only block (if only one)
		if(Offset + retLen <= disk->BlockSize) {
			VFS_WriteAt(disk->FD, base+Offset, retLen, Buffer);
			if(!bNewBlocks)	return Length;
			goto addBlocks;	// Ugh! A goto, but it seems unavoidable
		}
		
		// Write First Block
		VFS_WriteAt(disk->FD, base+Offset, disk->BlockSize-Offset, Buffer);
		Buffer += disk->BlockSize-Offset;
		retLen -= disk->BlockSize-Offset;
		block ++;
		
		// Write middle blocks
		while(retLen > disk->BlockSize)
		{
			base = Ext2_int_GetBlockAddr(disk, inode->i_block, block);
			VFS_WriteAt(disk->FD, base, disk->BlockSize, Buffer);
			Buffer += disk->BlockSize;
			retLen -= disk->BlockSize;
			block ++;
		}
		
		// Write last block
		base = Ext2_int_GetBlockAddr(disk, inode->i_block, block);
		VFS_WriteAt(disk->FD, base, retLen, Buffer);
		if(!bNewBlocks)	return Length;	// Writing in only allocated space
	}
	else
		base = Ext2_int_GetBlockAddr(disk, inode->i_block, allocSize/disk->BlockSize-1);
	
addBlocks:
	// Allocate blocks and copy data to them
	retLen = Length - (allocSize-Offset);
	while( retLen > 0  )
	{
		size_t	blk_len = (retLen < disk->BlockSize ? retLen : disk->BlockSize);
		// Allocate a block
		block = Ext2_int_AllocateBlock(disk, base/disk->BlockSize);
		if(!block)	return Length - retLen;
		// Add it to this inode
		if( Ext2_int_AppendBlock(Node, inode, block) ) {
			Log_Warning("Ext2", "Appending %x to inode %p:%X failed",
				block, disk, Node->Inode);
			Ext2_int_DeallocateBlock(disk, block);
			goto ret;
		}
		// Copy data to the node
		base = block * disk->BlockSize;
		VFS_WriteAt(disk->FD, base, blk_len, Buffer);
		// Update pointer and size remaining
		Buffer += blk_len;
		retLen -= blk_len;
	}


ret:
	retLen = Length - retLen;
	if( retLen )
	{
		// TODO: When should the size update be committed?
		inode->i_size += retLen;
		Node->Size += retLen;
		Node->Flags |= VFS_FFLAG_DIRTY;
		//Ext2_int_WriteInode(disk, Node->Inode, inode);
	}
	return retLen;
}

/**
 * \fn Uint32 Ext2_int_AllocateBlock(tExt2_Disk *Disk, Uint32 PrevBlock)
 * \brief Allocate a block from the best possible location
 * \param Disk	EXT2 Disk Information Structure
 * \param PrevBlock	Previous block ID in the file
 */
Uint32 Ext2_int_AllocateBlock(tExt2_Disk *Disk, Uint32 PrevBlock)
{
	 int	bpg = Disk->SuperBlock.s_blocks_per_group;
	Uint	firstgroup = PrevBlock / bpg;
	Uint	blockgroup = firstgroup;
	tExt2_Group	*bg;

	// TODO: Need to do locking on the bitmaps	

	// Are there any free blocks?
	if(Disk->SuperBlock.s_free_blocks_count == 0)
		return 0;

	// First: Check the next block after `PrevBlock`
	 int	iblock = (PrevBlock + 1) % Disk->SuperBlock.s_blocks_per_group;
	//LOG("iblock = %i, Disk=%p, blockgroup=%i", iblock, Disk, blockgroup);
	if( iblock != 0 && Disk->Groups[blockgroup].bg_free_blocks_count > 0 )
	{
		//LOG("Checking %i:%i", blockgroup, iblock);
		
		bg = &Disk->Groups[blockgroup];
		
		const int sector_size = 512;
		Uint8 buf[sector_size];
		 int	byte = (iblock/8) % sector_size;
		Uint8	bit = 1 << (iblock % 8);
		 int	ofs = (iblock/8) / sector_size * sector_size;
		byte %= sector_size;
		Uint64	vol_ofs = Disk->BlockSize*bg->bg_block_bitmap+ofs;
		VFS_ReadAt(Disk->FD, vol_ofs, sector_size, buf);

		//LOG("buf@%llx[%i] = %02x (& %02x)", vol_ofs, byte, buf[byte], bit);
	
		if( (buf[byte] & bit) == 0 )
		{
			// Free block - nice and contig allocation
			buf[byte] |= bit;
			VFS_WriteAt(Disk->FD, vol_ofs, sector_size, buf);

			bg->bg_free_blocks_count --;
			Disk->SuperBlock.s_free_blocks_count --;
			#if EXT2_UPDATE_WRITEBACK
			Ext2_int_UpdateSuperblock(Disk);
			#endif
			return PrevBlock + 1;
		}
		// Used... darnit
		// Fall through and search further
	}

	// Second: Search for a group with free blocks
	while( blockgroup < Disk->GroupCount && Disk->Groups[blockgroup].bg_free_blocks_count == 0 )
		blockgroup ++;
	if( Disk->Groups[blockgroup].bg_free_blocks_count == 0 )
	{
		LOG("Roll over");
		blockgroup = 0;
		while( blockgroup < firstgroup && Disk->Groups[blockgroup].bg_free_blocks_count == 0 )
			blockgroup ++;
	}
	if( Disk->Groups[blockgroup].bg_free_blocks_count == 0 ) {
		Log_Notice("Ext2", "Ext2_int_AllocateBlock - Out of blockss on %p, but superblock says some free",
			Disk);
		return 0;
	}
	//LOG("BG%i has free blocks", blockgroup);

	// Search the bitmap for a free block
	bg = &Disk->Groups[blockgroup];	
	 int	ofs = 0;
	do {
		const int sector_size = 512;
		Uint8 buf[sector_size];
		Uint64	vol_ofs = Disk->BlockSize*bg->bg_block_bitmap+ofs;
		VFS_ReadAt(Disk->FD, vol_ofs, sector_size, buf);

		int byte, bit;
		for( byte = 0; byte < sector_size && buf[byte] == 0xFF; byte ++ )
			;
		if( byte < sector_size )
		{
			//LOG("buf@%llx[%i] = %02x", vol_ofs, byte, buf[byte]);
			for( bit = 0; bit < 8 && buf[byte] & (1 << bit); bit ++)
				;
			ASSERT(bit != 8);
			buf[byte] |= 1 << bit;
			VFS_WriteAt(Disk->FD, vol_ofs, sector_size, buf);

			bg->bg_free_blocks_count --;
			Disk->SuperBlock.s_free_blocks_count --;

			#if EXT2_UPDATE_WRITEBACK
			Ext2_int_UpdateSuperblock(Disk);
			#endif

			Uint32	ret = blockgroup * Disk->SuperBlock.s_blocks_per_group + byte * 8 + bit;
			LOG("Allocated 0x%x", ret);
			return ret;
		}
	} while(ofs < Disk->SuperBlock.s_blocks_per_group / 8);
	
	Log_Notice("Ext2", "Ext2_int_AllocateBlock - Out of block in group %p:%i but header reported free",
		Disk, blockgroup);
	return 0;
}

/**
 * \brief Deallocates a block
 */
void Ext2_int_DeallocateBlock(tExt2_Disk *Disk, Uint32 Block)
{
	Log_Warning("Ext2", "TODO: Impliment Ext2_int_DeallocateBlock");
}

/**
 * \brief Append a block to an inode
 */
int Ext2_int_AppendBlock(tVFS_Node *Node, tExt2_Inode *Inode, Uint32 Block)
{
	tExt2_Disk	*Disk = Node->ImplPtr;
	 int	nBlocks;
	 int	dwPerBlock = Disk->BlockSize / 4;
	Uint32	*blocks;
	Uint32	id1, id2;
	
	nBlocks = (Inode->i_size + Disk->BlockSize - 1) / Disk->BlockSize;

	LOG("Append 0x%x to inode [%i]", Block, nBlocks);
	
	// Direct Blocks
	if( nBlocks < 12 ) {
		Inode->i_block[nBlocks] = Block;
		return 0;
	}
	
	blocks = malloc( Disk->BlockSize );
	if(!blocks)	return 1;
	
	nBlocks -= 12;
	// Single Indirect
	if( nBlocks < dwPerBlock)
	{
		LOG("Indirect 1 %i", nBlocks);
		// Allocate/Get Indirect block
		if( nBlocks == 0 ) {
			Inode->i_block[12] = Ext2_int_AllocateBlock(Disk, Inode->i_block[0]);
			if( !Inode->i_block[12] ) {
				Log_Warning("Ext2", "Allocating indirect block failed");
				free(blocks);
				return 1;
			}
			memset(blocks, 0, Disk->BlockSize); 
		}
		else
			VFS_ReadAt(Disk->FD, Inode->i_block[12]*Disk->BlockSize, Disk->BlockSize, blocks);
		
		blocks[nBlocks] = Block;
		
		VFS_WriteAt(Disk->FD, Inode->i_block[12]*Disk->BlockSize, Disk->BlockSize, blocks);
		Node->Flags |= VFS_FFLAG_DIRTY;
		free(blocks);
		return 0;
	}
	
	nBlocks -= dwPerBlock;
	// Double Indirect
	if( nBlocks < dwPerBlock*dwPerBlock )
	{
		LOG("Indirect 2 %i/%i", nBlocks/dwPerBlock, nBlocks%dwPerBlock);
		// Allocate/Get Indirect block
		if( nBlocks == 0 ) {
			Inode->i_block[13] = Ext2_int_AllocateBlock(Disk, Inode->i_block[0]);
			if( !Inode->i_block[13] ) {
				Log_Warning("Ext2", "Allocating double indirect block failed");
				free(blocks);
				return 1;
			}
			memset(blocks, 0, Disk->BlockSize);
			Node->Flags |= VFS_FFLAG_DIRTY;
		}
		else
			VFS_ReadAt(Disk->FD, Inode->i_block[13]*Disk->BlockSize, Disk->BlockSize, blocks);
		
		// Allocate / Get Indirect lvl2 Block
		if( nBlocks % dwPerBlock == 0 ) {
			id1 = Ext2_int_AllocateBlock(Disk, Inode->i_block[0]);
			if( !id1 ) {
				free(blocks);
				Log_Warning("Ext2", "Allocating double indirect block (l2) failed");
				return 1;
			}
			blocks[nBlocks/dwPerBlock] = id1;
			// Write back indirect 1 block
			VFS_WriteAt(Disk->FD, Inode->i_block[13]*Disk->BlockSize, Disk->BlockSize, blocks);
			memset(blocks, 0, Disk->BlockSize);
		}
		else {
			id1 = blocks[nBlocks / dwPerBlock];
			VFS_ReadAt(Disk->FD, id1*Disk->BlockSize, Disk->BlockSize, blocks);
		}
		
		blocks[nBlocks % dwPerBlock] = Block;
		
		VFS_WriteAt(Disk->FD, id1*Disk->BlockSize, Disk->BlockSize, blocks);
		free(blocks);
		return 0;
	}
	
	nBlocks -= dwPerBlock*dwPerBlock;
	// Triple Indirect
	if( nBlocks < dwPerBlock*dwPerBlock*dwPerBlock )
	{
		// Allocate/Get Indirect block
		if( nBlocks == 0 ) {
			Inode->i_block[14] = Ext2_int_AllocateBlock(Disk, Inode->i_block[0]);
			if( !Inode->i_block[14] ) {
				Log_Warning("Ext2", "Allocating triple indirect block failed");
				free(blocks);
				return 1;
			}
			memset(blocks, 0, Disk->BlockSize);
			Node->Flags |= VFS_FFLAG_DIRTY;
		}
		else
			VFS_ReadAt(Disk->FD, Inode->i_block[14]*Disk->BlockSize, Disk->BlockSize, blocks);
		
		// Allocate / Get Indirect lvl2 Block
		if( (nBlocks/dwPerBlock) % dwPerBlock == 0 && nBlocks % dwPerBlock == 0 )
		{
			id1 = Ext2_int_AllocateBlock(Disk, Inode->i_block[0]);
			if( !id1 ) {
				Log_Warning("Ext2", "Allocating triple indirect block (l2) failed");
				free(blocks);
				return 1;
			}
			blocks[nBlocks/dwPerBlock] = id1;
			// Write back indirect 1 block
			VFS_WriteAt(Disk->FD, Inode->i_block[14]*Disk->BlockSize, Disk->BlockSize, blocks);
			memset(blocks, 0, Disk->BlockSize);
		}
		else {
			id1 = blocks[nBlocks / (dwPerBlock*dwPerBlock)];
			VFS_ReadAt(Disk->FD, id1*Disk->BlockSize, Disk->BlockSize, blocks);
		}
		
		// Allocate / Get Indirect Level 3 Block
		if( nBlocks % dwPerBlock == 0 ) {
			id2 = Ext2_int_AllocateBlock(Disk, id1);
			if( !id2 ) {
				Log_Warning("Ext2", "Allocating triple indirect block (l3) failed");
				free(blocks);
				return 1;
			}
			blocks[(nBlocks/dwPerBlock)%dwPerBlock] = id2;
			// Write back indirect 1 block
			VFS_WriteAt(Disk->FD, id1*Disk->BlockSize, Disk->BlockSize, blocks);
			memset(blocks, 0, Disk->BlockSize);
		}
		else {
			id2 = blocks[(nBlocks/dwPerBlock)%dwPerBlock];
			VFS_ReadAt(Disk->FD, id2*Disk->BlockSize, Disk->BlockSize, blocks);
		}
		
		blocks[nBlocks % dwPerBlock] = Block;
		
		VFS_WriteAt(Disk->FD, id2*Disk->BlockSize, Disk->BlockSize, blocks);
		free(blocks);
		return 0;
	}
	
	Log_Warning("Ext2", "Inode ?? cannot have a block appended to it, all indirects used");
	free(blocks);
	return 1;
}
