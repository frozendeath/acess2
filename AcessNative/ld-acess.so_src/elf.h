/**
 * \file elf.h
 * \brief ELF Exeutable Loader
 */
#ifndef _BIN_ELF_H
#define _BIN_ELF_H

#include <stdint.h>

/**
 * \brief ELF File Header
 */
struct sElf32_Ehdr
{
	union {
		char	ident[16];	//!< Identifier Bytes
		struct {
			uint32_t	Ident1;
			uint32_t	Ident2;
			uint32_t	HashTable;
			uint32_t	SymTable;
		} misc;
	};
	uint16_t	filetype;	//!< File Type
	uint16_t	machine;	//!< Machine / Arch
	uint32_t	version;	//!< Version (File?)
	uint32_t	entrypoint;	//!< Entry Point
	uint32_t	phoff;	//!< Program Header Offset
	uint32_t	shoff;	//!< Section Header Offset
	uint32_t	flags;	//!< Flags
	uint16_t	headersize;	//!< Header Size
	uint16_t	phentsize;	//!< Program Header Entry Size
	uint16_t	phentcount;	//!< Program Header Entry Count
	uint16_t	shentsize;	//!< Section Header Entry Size
	uint16_t	shentcount;	//!< Section Header Entry Count
	uint16_t	shstrindex;	//!< Section Header String Table Index
} __attribute__ ((packed));

/**
 * \brief Executable Types
 */
enum eElf32_ExecTypes
{
	ET_NONE	= 0,	//!< NULL Type
	ET_REL	= 1,	//!< Relocatable (Object)
	ET_EXEC	= 2,	//!< Executable
	ET_DYN	= 3,	//!< Dynamic Library
	ET_CORE	= 4,	//!< Core?
	ET_LOPROC = 0xFF00,	//!< Low Impl Defined
	ET_HIPROC = 0xFFFF	//!< High Impl Defined
};

/**
 \name Section IDs
 \{
*/
#define SHN_UNDEF		0	//!< Undefined Section
#define SHN_LORESERVE	0xFF00	//!< Low Reserved
#define SHN_LOPROC		0xFF00	//!< Low Impl Defined
#define SHN_HIPROC		0xFF1F	//!< High Impl Defined
#define SHN_ABS			0xFFF1	//!< Absolute Address (Base: 0, Size: -1)
#define SHN_COMMON		0xFFF2	//!< Common
#define SHN_HIRESERVE	0xFFFF	//!< High Reserved
//! \}

/**
 \enum eElfSectionTypes
 \brief ELF Section Types
*/
enum eElfSectionTypes {
	SHT_NULL,	//0
	SHT_PROGBITS,	//1
	SHT_SYMTAB,	//2
	SHT_STRTAB,	//3
	SHT_RELA,	//4
	SHT_HASH,	//5
	SHT_DYNAMIC,	//6
	SHT_NOTE,	//7
	SHT_NOBITS,	//8
	SHT_REL,	//9
	SHT_SHLIB,	//A
	SHT_DYNSYM,	//B
	SHT_LAST,	//C
	SHT_LOPROC = 0x70000000,
	SHT_HIPROC = 0x7fffffff,
	SHT_LOUSER = 0x80000000,
	SHT_HIUSER = 0xffffffff
};

#define SHF_WRITE	0x1
#define SHF_ALLOC	0x2
#define SHF_EXECINSTR	0x4
#define SHF_MASKPROC	0xf0000000

struct sElf32_Shent {
	uint32_t	name;
	uint32_t	type;
	uint32_t	flags;
	uint32_t	address;
	uint32_t	offset;
	uint32_t	size;
	uint32_t	link;
	uint32_t	info;
	uint32_t	addralign;
	uint32_t	entsize;
} __attribute__ ((packed));	//sizeof = 40

struct elf_sym_s {
	union {
		uint32_t	nameOfs;
		uint32_t	name;
	};
	uint32_t	value;	//Address
	uint32_t	size;
	uint8_t	info;
	uint8_t	other;
	uint16_t	shndx;
} __attribute__ ((packed));
#define	STN_UNDEF	0	// Undefined Symbol

enum {
	PT_NULL,	//0
	PT_LOAD,	//1
	PT_DYNAMIC,	//2
	PT_INTERP,	//3
	PT_NOTE,	//4
	PT_SHLIB,	//5
	PT_PHDR,	//6
	PT_LOPROC = 0x70000000,
	PT_HIPROC = 0x7fffffff
};

struct sElf32_Phdr {
	uint32_t	Type;
	uint32_t	Offset;
	uint32_t	VAddr;
	uint32_t	PAddr;
	uint32_t	FileSize;
	uint32_t	MemSize;
	uint32_t	Flags;
	uint32_t	Align;
} __attribute__ ((packed));

struct elf32_rel_s {
	uint32_t	r_offset;
	uint32_t	r_info;
} __attribute__ ((packed));

struct elf32_rela_s {
	uint32_t	r_offset;
	uint32_t	r_info;
	int32_t	r_addend;
} __attribute__ ((packed));

enum {
	R_386_NONE = 0,	// none
	R_386_32,	// S+A
	R_386_PC32,	// S+A-P
	R_386_GOT32,	// G+A-P
	R_386_PLT32,	// L+A-P
	R_386_COPY,	// none
	R_386_GLOB_DAT,	// S
	R_386_JMP_SLOT,	// S
	R_386_RELATIVE,	// B+A
	R_386_GOTOFF,	// S+A-GOT
	R_386_GOTPC,	// GOT+A-P
	R_386_LAST	// none
};

#define	ELF32_R_SYM(i)	((i)>>8)	// Takes an info value and returns a symbol index
#define	ELF32_R_TYPE(i)	((i)&0xFF)	// Takes an info value and returns a type
#define	ELF32_R_INFO(s,t)	(((s)<<8)+((t)&0xFF))	// Takes a type and symbol index and returns an info value

struct elf32_dyn_s {
	uint32_t	d_tag;
	uint32_t	d_val;	//Also d_ptr
} __attribute__ ((packed));

enum {
	DT_NULL,	//!< Marks End of list
	DT_NEEDED,	//!< Offset in strtab to needed library
	DT_PLTRELSZ,	//!< Size in bytes of PLT
	DT_PLTGOT,	//!< Address of PLT/GOT
	DT_HASH,	//!< Address of symbol hash table
	DT_STRTAB,	//!< String Table address
	DT_SYMTAB,	//!< Symbol Table address
	DT_RELA,	//!< Relocation table address
	DT_RELASZ,	//!< Size of relocation table
	DT_RELAENT,	//!< Size of entry in relocation table
	DT_STRSZ,	//!< Size of string table
	DT_SYMENT,	//!< Size of symbol table entry
	DT_INIT,	//!< Address of initialisation function
	DT_FINI,	//!< Address of termination function
	DT_SONAME,	//!< String table offset of so name
	DT_RPATH,	//!< String table offset of library path
	DT_SYMBOLIC,//!< Reverse order of symbol searching for library, search libs first then executable
	DT_REL,		//!< Relocation Entries (Elf32_Rel instead of Elf32_Rela)
	DT_RELSZ,	//!< Size of above table (bytes)
	DT_RELENT,	//!< Size of entry in above table
	DT_PLTREL,	//!< Relocation entry of PLT
	DT_DEBUG,	//!< Debugging Entry - Unknown contents
	DT_TEXTREL,	//!< Indicates that modifcations to a non-writeable segment may occur
	DT_JMPREL,	//!< Address of PLT only relocation entries
	DT_LOPROC = 0x70000000,	//!< Low Definable
	DT_HIPROC = 0x7FFFFFFF	//!< High Definable
};

typedef struct sElf32_Ehdr	Elf32_Ehdr;
typedef struct sElf32_Phdr	Elf32_Phdr;
typedef struct sElf32_Shent	Elf32_Shent;
typedef struct elf_sym_s	elf_symtab;
typedef struct elf_sym_s	Elf32_Sym;
typedef struct elf32_rel_s	Elf32_Rel;
typedef struct elf32_rela_s	Elf32_Rela;
typedef struct elf32_dyn_s	Elf32_Dyn;

#endif	// defined(_EXE_ELF_H)
