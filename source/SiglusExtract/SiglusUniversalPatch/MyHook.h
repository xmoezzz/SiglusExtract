#pragma once
#include <my.h>

typedef struct _IAT_PATCH_DATA
{
	LPVOID   hModule;
	LPVOID   SourceFunction;
	LPVOID   DestFunction;
	LPCSTR   DllName;
}IAT_PATCH_DATA, *PIAT_PATCH_DATA;

typedef struct _EAT_PATCH_DATA
{
	LPCSTR   ModName;
	LPCSTR   FunName;
	ULONG64  ProxyFunAddr;
}EAT_PATCH_DATA, *PEAT_PATCH_DATA;

typedef struct _INLINE_PATCH_DATA
{
	LPVOID   pTarget;
	LPVOID   pHook;
	LPVOID*  pDetour;
}INLINE_PATCH_DATA, *PINLINE_PATCH_DATA;

typedef struct _CODE_PATCH_DATA
{
	LPVOID   lpAddress;
	LPVOID   pCode;
	ULONG    CodeSize;
}CODE_PATCH_DATA, *PCODE_PATCH_DATA;


enum HOOK_INFO
{
	IAT_HOOK,
	EAT_HOOK,
	INLINE_HOOK,
	MEMORY_HOOK
};


#define MH_ALL_HOOKS NULL

typedef enum MH_STATUS
{
	// Unknown error. Should not be returned.
	MH_UNKNOWN = -1,

	// Successful.
	MH_OK = 0,

	// MinHook is already initialized.
	MH_ERROR_ALREADY_INITIALIZED,

	// MinHook is not initialized yet, or already uninitialized.
	MH_ERROR_NOT_INITIALIZED,

	// The hook for the specified target function is already created.
	MH_ERROR_ALREADY_CREATED,

	// The hook for the specified target function is not created yet.
	MH_ERROR_NOT_CREATED,

	// The hook for the specified target function is already enabled.
	MH_ERROR_ENABLED,

	// The hook for the specified target function is not enabled yet, or already
	// disabled.
	MH_ERROR_DISABLED,

	// The specified pointer is invalid. It points the address of non-allocated
	// and/or non-executable region.
	MH_ERROR_NOT_EXECUTABLE,

	// The specified target function cannot be hooked.
	MH_ERROR_UNSUPPORTED_FUNCTION,

	// Failed to allocate memory.
	MH_ERROR_MEMORY_ALLOC,

	// Failed to change the memory protection.
	MH_ERROR_MEMORY_PROTECT,

	// The specified module is not loaded.
	MH_ERROR_MODULE_NOT_FOUND,

	// The specified function is not found.
	MH_ERROR_FUNCTION_NOT_FOUND
}MH_STATUS;

#include <stdint.h>

#ifdef _M_X64

#define F_MODRM         0x00000001
#define F_SIB           0x00000002
#define F_IMM8          0x00000004
#define F_IMM16         0x00000008
#define F_IMM32         0x00000010
#define F_IMM64         0x00000020
#define F_DISP8         0x00000040
#define F_DISP16        0x00000080
#define F_DISP32        0x00000100
#define F_RELATIVE      0x00000200
#define F_ERROR         0x00001000
#define F_ERROR_OPCODE  0x00002000
#define F_ERROR_LENGTH  0x00004000
#define F_ERROR_LOCK    0x00008000
#define F_ERROR_OPERAND 0x00010000
#define F_PREFIX_REPNZ  0x01000000
#define F_PREFIX_REPX   0x02000000
#define F_PREFIX_REP    0x03000000
#define F_PREFIX_66     0x04000000
#define F_PREFIX_67     0x08000000
#define F_PREFIX_LOCK   0x10000000
#define F_PREFIX_SEG    0x20000000
#define F_PREFIX_REX    0x40000000
#define F_PREFIX_ANY    0x7f000000

#define PREFIX_SEGMENT_CS   0x2e
#define PREFIX_SEGMENT_SS   0x36
#define PREFIX_SEGMENT_DS   0x3e
#define PREFIX_SEGMENT_ES   0x26
#define PREFIX_SEGMENT_FS   0x64
#define PREFIX_SEGMENT_GS   0x65
#define PREFIX_LOCK         0xf0
#define PREFIX_REPNZ        0xf2
#define PREFIX_REPX         0xf3
#define PREFIX_OPERAND_SIZE 0x66
#define PREFIX_ADDRESS_SIZE 0x67

#pragma pack(push,1)

typedef struct
{
	uint8_t len;
	uint8_t p_rep;
	uint8_t p_lock;
	uint8_t p_seg;
	uint8_t p_66;
	uint8_t p_67;
	uint8_t rex;
	uint8_t rex_w;
	uint8_t rex_r;
	uint8_t rex_x;
	uint8_t rex_b;
	uint8_t opcode;
	uint8_t opcode2;
	uint8_t modrm;
	uint8_t modrm_mod;
	uint8_t modrm_reg;
	uint8_t modrm_rm;
	uint8_t sib;
	uint8_t sib_scale;
	uint8_t sib_index;
	uint8_t sib_base;
	union
	{
		uint8_t  imm8;
		uint16_t imm16;
		uint32_t imm32;
		uint64_t imm64;
	} imm;
	union
	{
		uint8_t  disp8;
		uint16_t disp16;
		uint32_t disp32;
	} disp;
	uint32_t flags;
} hde64s;

#pragma pack(pop)


#else

#define F_MODRM         0x00000001
#define F_SIB           0x00000002
#define F_IMM8          0x00000004
#define F_IMM16         0x00000008
#define F_IMM32         0x00000010
#define F_DISP8         0x00000020
#define F_DISP16        0x00000040
#define F_DISP32        0x00000080
#define F_RELATIVE      0x00000100
#define F_2IMM16        0x00000800
#define F_ERROR         0x00001000
#define F_ERROR_OPCODE  0x00002000
#define F_ERROR_LENGTH  0x00004000
#define F_ERROR_LOCK    0x00008000
#define F_ERROR_OPERAND 0x00010000
#define F_PREFIX_REPNZ  0x01000000
#define F_PREFIX_REPX   0x02000000
#define F_PREFIX_REP    0x03000000
#define F_PREFIX_66     0x04000000
#define F_PREFIX_67     0x08000000
#define F_PREFIX_LOCK   0x10000000
#define F_PREFIX_SEG    0x20000000
#define F_PREFIX_ANY    0x3f000000

#define PREFIX_SEGMENT_CS   0x2e
#define PREFIX_SEGMENT_SS   0x36
#define PREFIX_SEGMENT_DS   0x3e
#define PREFIX_SEGMENT_ES   0x26
#define PREFIX_SEGMENT_FS   0x64
#define PREFIX_SEGMENT_GS   0x65
#define PREFIX_LOCK         0xf0
#define PREFIX_REPNZ        0xf2
#define PREFIX_REPX         0xf3
#define PREFIX_OPERAND_SIZE 0x66
#define PREFIX_ADDRESS_SIZE 0x67

#pragma pack(push,1)

typedef struct
{
	uint8_t len;
	uint8_t p_rep;
	uint8_t p_lock;
	uint8_t p_seg;
	uint8_t p_66;
	uint8_t p_67;
	uint8_t opcode;
	uint8_t opcode2;
	uint8_t modrm;
	uint8_t modrm_mod;
	uint8_t modrm_reg;
	uint8_t modrm_rm;
	uint8_t sib;
	uint8_t sib_scale;
	uint8_t sib_index;
	uint8_t sib_base;
	union
	{
		uint8_t  imm8;
		uint16_t imm16;
		uint32_t imm32;
	} imm;
	union
	{
		uint8_t  disp8;
		uint16_t disp16;
		uint32_t disp32;
	} disp;
	uint32_t flags;
} hde32s;

#pragma pack(pop)


#endif



NTSTATUS NTAPI InlinePatchMemory(PINLINE_PATCH_DATA Data, ULONG_PTR Size);
NTSTATUS NTAPI IATPatchMemory(PIAT_PATCH_DATA    Data, ULONG_PTR Size);
NTSTATUS NTAPI EATPatchMemory(PEAT_PATCH_DATA    Data, ULONG_PTR Size);
NTSTATUS NTAPI CodePatchMemory(PCODE_PATCH_DATA   Data, ULONG_PTR Size);
NTSTATUS NTAPI SetNopCode(LPBYTE Code, ULONG_PTR Size);
//DYNAMIC_ADDRESS

NTSTATUS NTAPI InlineRestoreMemory(PVOID Target);

