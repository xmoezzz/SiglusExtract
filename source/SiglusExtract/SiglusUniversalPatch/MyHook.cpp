#include "MyHook.h"
#include <Psapi.h>
#include <TlHelp32.h>

#ifdef _M_X64

#define C_NONE    0x00
#define C_MODRM   0x01
#define C_IMM8    0x02
#define C_IMM16   0x04
#define C_IMM_P66 0x10
#define C_REL8    0x20
#define C_REL32   0x40
#define C_GROUP   0x80
#define C_ERROR   0xff

#define PRE_ANY  0x00
#define PRE_NONE 0x01
#define PRE_F2   0x02
#define PRE_F3   0x04
#define PRE_66   0x08
#define PRE_67   0x10
#define PRE_LOCK 0x20
#define PRE_SEG  0x40
#define PRE_ALL  0xff

#define DELTA_OPCODES      0x4a
#define DELTA_FPU_REG      0xfd
#define DELTA_FPU_MODRM    0x104
#define DELTA_PREFIXES     0x13c
#define DELTA_OP_LOCK_OK   0x1ae
#define DELTA_OP2_LOCK_OK  0x1c6
#define DELTA_OP_ONLY_MEM  0x1d8
#define DELTA_OP2_ONLY_MEM 0x1e7

unsigned char hde64_table[] =
{
	0xa5, 0xaa, 0xa5, 0xb8, 0xa5, 0xaa, 0xa5, 0xaa, 0xa5, 0xb8, 0xa5, 0xb8, 0xa5, 0xb8, 0xa5,
	0xb8, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xac, 0xc0, 0xcc, 0xc0, 0xa1, 0xa1,
	0xa1, 0xa1, 0xb1, 0xa5, 0xa5, 0xa6, 0xc0, 0xc0, 0xd7, 0xda, 0xe0, 0xc0, 0xe4, 0xc0, 0xea,
	0xea, 0xe0, 0xe0, 0x98, 0xc8, 0xee, 0xf1, 0xa5, 0xd3, 0xa5, 0xa5, 0xa1, 0xea, 0x9e, 0xc0,
	0xc0, 0xc2, 0xc0, 0xe6, 0x03, 0x7f, 0x11, 0x7f, 0x01, 0x7f, 0x01, 0x3f, 0x01, 0x01, 0xab,
	0x8b, 0x90, 0x64, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x92, 0x5b, 0x5b, 0x76, 0x90, 0x92, 0x92,
	0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x6a, 0x73, 0x90,
	0x5b, 0x52, 0x52, 0x52, 0x52, 0x5b, 0x5b, 0x5b, 0x5b, 0x77, 0x7c, 0x77, 0x85, 0x5b, 0x5b,
	0x70, 0x5b, 0x7a, 0xaf, 0x76, 0x76, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b,
	0x5b, 0x5b, 0x86, 0x01, 0x03, 0x01, 0x04, 0x03, 0xd5, 0x03, 0xd5, 0x03, 0xcc, 0x01, 0xbc,
	0x03, 0xf0, 0x03, 0x03, 0x04, 0x00, 0x50, 0x50, 0x50, 0x50, 0xff, 0x20, 0x20, 0x20, 0x20,
	0x01, 0x01, 0x01, 0x01, 0xc4, 0x02, 0x10, 0xff, 0xff, 0xff, 0x01, 0x00, 0x03, 0x11, 0xff,
	0x03, 0xc4, 0xc6, 0xc8, 0x02, 0x10, 0x00, 0xff, 0xcc, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x01, 0x03, 0x01, 0xff, 0xff, 0xc0, 0xc2, 0x10, 0x11, 0x02, 0x03, 0x01, 0x01,
	0x01, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x10,
	0x10, 0x10, 0x10, 0x02, 0x10, 0x00, 0x00, 0xc6, 0xc8, 0x02, 0x02, 0x02, 0x02, 0x06, 0x00,
	0x04, 0x00, 0x02, 0xff, 0x00, 0xc0, 0xc2, 0x01, 0x01, 0x03, 0x03, 0x03, 0xca, 0x40, 0x00,
	0x0a, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x33, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xff, 0xbf, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0xff, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
	0x00, 0x00, 0x00, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00,
	0xff, 0x40, 0x40, 0x40, 0x40, 0x41, 0x49, 0x40, 0x40, 0x40, 0x40, 0x4c, 0x42, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x4f, 0x44, 0x53, 0x40, 0x40, 0x40, 0x44, 0x57, 0x43,
	0x5c, 0x40, 0x60, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x64, 0x66, 0x6e, 0x6b, 0x40, 0x40, 0x6a, 0x46, 0x40, 0x40, 0x44, 0x46, 0x40,
	0x40, 0x5b, 0x44, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x06, 0x06, 0x01, 0x06,
	0x06, 0x02, 0x06, 0x06, 0x00, 0x06, 0x00, 0x0a, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x07, 0x07,
	0x06, 0x02, 0x0d, 0x06, 0x06, 0x06, 0x0e, 0x05, 0x05, 0x02, 0x02, 0x00, 0x00, 0x04, 0x04,
	0x04, 0x04, 0x05, 0x06, 0x06, 0x06, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x08, 0x00, 0x10,
	0x00, 0x18, 0x00, 0x20, 0x00, 0x28, 0x00, 0x30, 0x00, 0x80, 0x01, 0x82, 0x01, 0x86, 0x00,
	0xf6, 0xcf, 0xfe, 0x3f, 0xab, 0x00, 0xb0, 0x00, 0xb1, 0x00, 0xb3, 0x00, 0xba, 0xf8, 0xbb,
	0x00, 0xc0, 0x00, 0xc1, 0x00, 0xc7, 0xbf, 0x62, 0xff, 0x00, 0x8d, 0xff, 0x00, 0xc4, 0xff,
	0x00, 0xc5, 0xff, 0x00, 0xff, 0xff, 0xeb, 0x01, 0xff, 0x0e, 0x12, 0x08, 0x00, 0x13, 0x09,
	0x00, 0x16, 0x08, 0x00, 0x17, 0x09, 0x00, 0x2b, 0x09, 0x00, 0xae, 0xff, 0x07, 0xb2, 0xff,
	0x00, 0xb4, 0xff, 0x00, 0xb5, 0xff, 0x00, 0xc3, 0x01, 0x00, 0xc7, 0xff, 0xbf, 0xe7, 0x08,
	0x00, 0xf0, 0x02, 0x00
};

unsigned int hde64_disasm(const void *code, hde64s *hs)
{
	uint8_t x, c, *p = (uint8_t *)code, cflags, opcode, pref = 0;
	uint8_t *ht = hde64_table, m_mod, m_reg, m_rm, disp_size = 0;
	uint8_t op64 = 0;

	__stosb((LPBYTE)hs, 0, sizeof(hde64s));

	for (x = 16; x; x--)
		switch (c = *p++) {
		case 0xf3:
			hs->p_rep = c;
			pref |= PRE_F3;
			break;
		case 0xf2:
			hs->p_rep = c;
			pref |= PRE_F2;
			break;
		case 0xf0:
			hs->p_lock = c;
			pref |= PRE_LOCK;
			break;
		case 0x26: case 0x2e: case 0x36:
		case 0x3e: case 0x64: case 0x65:
			hs->p_seg = c;
			pref |= PRE_SEG;
			break;
		case 0x66:
			hs->p_66 = c;
			pref |= PRE_66;
			break;
		case 0x67:
			hs->p_67 = c;
			pref |= PRE_67;
			break;
		default:
			goto pref_done;
	}
pref_done:

	hs->flags = (uint32_t)pref << 23;

	if (!pref)
		pref |= PRE_NONE;

	if ((c & 0xf0) == 0x40) {
		hs->flags |= F_PREFIX_REX;
		if ((hs->rex_w = (c & 0xf) >> 3) && (*p & 0xf8) == 0xb8)
			op64++;
		hs->rex_r = (c & 7) >> 2;
		hs->rex_x = (c & 3) >> 1;
		hs->rex_b = c & 1;
		if (((c = *p++) & 0xf0) == 0x40) {
			opcode = c;
			goto error_opcode;
		}
	}

	if ((hs->opcode = c) == 0x0f) {
		hs->opcode2 = c = *p++;
		ht += DELTA_OPCODES;
	}
	else if (c >= 0xa0 && c <= 0xa3) {
		op64++;
		if (pref & PRE_67)
			pref |= PRE_66;
		else
			pref &= ~PRE_66;
	}

	opcode = c;
	cflags = ht[ht[opcode / 4] + (opcode % 4)];

	if (cflags == C_ERROR) {
	error_opcode:
		hs->flags |= F_ERROR | F_ERROR_OPCODE;
		cflags = 0;
		if ((opcode & -3) == 0x24)
			cflags++;
	}

	x = 0;
	if (cflags & C_GROUP) {
		uint16_t t;
		t = *(uint16_t *)(ht + (cflags & 0x7f));
		cflags = (uint8_t)t;
		x = (uint8_t)(t >> 8);
	}

	if (hs->opcode2) {
		ht = hde64_table + DELTA_PREFIXES;
		if (ht[ht[opcode / 4] + (opcode % 4)] & pref)
			hs->flags |= F_ERROR | F_ERROR_OPCODE;
	}

	if (cflags & C_MODRM) {
		hs->flags |= F_MODRM;
		hs->modrm = c = *p++;
		hs->modrm_mod = m_mod = c >> 6;
		hs->modrm_rm = m_rm = c & 7;
		hs->modrm_reg = m_reg = (c & 0x3f) >> 3;

		if (x && ((x << m_reg) & 0x80))
			hs->flags |= F_ERROR | F_ERROR_OPCODE;

		if (!hs->opcode2 && opcode >= 0xd9 && opcode <= 0xdf) {
			uint8_t t = opcode - 0xd9;
			if (m_mod == 3) {
				ht = hde64_table + DELTA_FPU_MODRM + t * 8;
				t = ht[m_reg] << m_rm;
			}
			else {
				ht = hde64_table + DELTA_FPU_REG;
				t = ht[t] << m_reg;
			}
			if (t & 0x80)
				hs->flags |= F_ERROR | F_ERROR_OPCODE;
		}

		if (pref & PRE_LOCK) {
			if (m_mod == 3) {
				hs->flags |= F_ERROR | F_ERROR_LOCK;
			}
			else {
				uint8_t *table_end, op = opcode;
				if (hs->opcode2) {
					ht = hde64_table + DELTA_OP2_LOCK_OK;
					table_end = ht + DELTA_OP_ONLY_MEM - DELTA_OP2_LOCK_OK;
				}
				else {
					ht = hde64_table + DELTA_OP_LOCK_OK;
					table_end = ht + DELTA_OP2_LOCK_OK - DELTA_OP_LOCK_OK;
					op &= -2;
				}
				for (; ht != table_end; ht++)
					if (*ht++ == op) {
						if (!((*ht << m_reg) & 0x80))
							goto no_lock_error;
						else
							break;
					}
				hs->flags |= F_ERROR | F_ERROR_LOCK;
			no_lock_error:
				;
			}
		}

		if (hs->opcode2) {
			switch (opcode) {
			case 0x20: case 0x22:
				m_mod = 3;
				if (m_reg > 4 || m_reg == 1)
					goto error_operand;
				else
					goto no_error_operand;
			case 0x21: case 0x23:
				m_mod = 3;
				if (m_reg == 4 || m_reg == 5)
					goto error_operand;
				else
					goto no_error_operand;
			}
		}
		else {
			switch (opcode) {
			case 0x8c:
				if (m_reg > 5)
					goto error_operand;
				else
					goto no_error_operand;
			case 0x8e:
				if (m_reg == 1 || m_reg > 5)
					goto error_operand;
				else
					goto no_error_operand;
			}
		}

		if (m_mod == 3) {
			uint8_t *table_end;
			if (hs->opcode2) {
				ht = hde64_table + DELTA_OP2_ONLY_MEM;
				table_end = ht + sizeof(hde64_table) - DELTA_OP2_ONLY_MEM;
			}
			else {
				ht = hde64_table + DELTA_OP_ONLY_MEM;
				table_end = ht + DELTA_OP2_ONLY_MEM - DELTA_OP_ONLY_MEM;
			}
			for (; ht != table_end; ht += 2)
				if (*ht++ == opcode) {
					if (*ht++ & pref && !((*ht << m_reg) & 0x80))
						goto error_operand;
					else
						break;
				}
			goto no_error_operand;
		}
		else if (hs->opcode2) {
			switch (opcode) {
			case 0x50: case 0xd7: case 0xf7:
				if (pref & (PRE_NONE | PRE_66))
					goto error_operand;
				break;
			case 0xd6:
				if (pref & (PRE_F2 | PRE_F3))
					goto error_operand;
				break;
			case 0xc5:
				goto error_operand;
			}
			goto no_error_operand;
		}
		else
			goto no_error_operand;

	error_operand:
		hs->flags |= F_ERROR | F_ERROR_OPERAND;
	no_error_operand:

		c = *p++;
		if (m_reg <= 1) {
			if (opcode == 0xf6)
				cflags |= C_IMM8;
			else if (opcode == 0xf7)
				cflags |= C_IMM_P66;
		}

		switch (m_mod) {
		case 0:
			if (pref & PRE_67) {
				if (m_rm == 6)
					disp_size = 2;
			}
			else
				if (m_rm == 5)
					disp_size = 4;
			break;
		case 1:
			disp_size = 1;
			break;
		case 2:
			disp_size = 2;
			if (!(pref & PRE_67))
				disp_size <<= 1;
		}

		if (m_mod != 3 && m_rm == 4) {
			hs->flags |= F_SIB;
			p++;
			hs->sib = c;
			hs->sib_scale = c >> 6;
			hs->sib_index = (c & 0x3f) >> 3;
			if ((hs->sib_base = c & 7) == 5 && !(m_mod & 1))
				disp_size = 4;
		}

		p--;
		switch (disp_size) {
		case 1:
			hs->flags |= F_DISP8;
			hs->disp.disp8 = *p;
			break;
		case 2:
			hs->flags |= F_DISP16;
			hs->disp.disp16 = *(uint16_t *)p;
			break;
		case 4:
			hs->flags |= F_DISP32;
			hs->disp.disp32 = *(uint32_t *)p;
		}
		p += disp_size;
	}
	else if (pref & PRE_LOCK)
		hs->flags |= F_ERROR | F_ERROR_LOCK;

	if (cflags & C_IMM_P66) {
		if (cflags & C_REL32) {
			if (pref & PRE_66) {
				hs->flags |= F_IMM16 | F_RELATIVE;
				hs->imm.imm16 = *(uint16_t *)p;
				p += 2;
				goto disasm_done;
			}
			goto rel32_ok;
		}
		if (op64) {
			hs->flags |= F_IMM64;
			hs->imm.imm64 = *(uint64_t *)p;
			p += 8;
		}
		else if (!(pref & PRE_66)) {
			hs->flags |= F_IMM32;
			hs->imm.imm32 = *(uint32_t *)p;
			p += 4;
		}
		else
			goto imm16_ok;
	}


	if (cflags & C_IMM16) {
	imm16_ok:
		hs->flags |= F_IMM16;
		hs->imm.imm16 = *(uint16_t *)p;
		p += 2;
	}
	if (cflags & C_IMM8) {
		hs->flags |= F_IMM8;
		hs->imm.imm8 = *p++;
	}

	if (cflags & C_REL32) {
	rel32_ok:
		hs->flags |= F_IMM32 | F_RELATIVE;
		hs->imm.imm32 = *(uint32_t *)p;
		p += 4;
	}
	else if (cflags & C_REL8) {
		hs->flags |= F_IMM8 | F_RELATIVE;
		hs->imm.imm8 = *p++;
	}

disasm_done:

	if ((hs->len = (uint8_t)(p - (uint8_t *)code)) > 15) {
		hs->flags |= F_ERROR | F_ERROR_LENGTH;
		hs->len = 15;
	}

	return (unsigned int)hs->len;
}


#else

#define C_NONE    0x00
#define C_MODRM   0x01
#define C_IMM8    0x02
#define C_IMM16   0x04
#define C_IMM_P66 0x10
#define C_REL8    0x20
#define C_REL32   0x40
#define C_GROUP   0x80
#define C_ERROR   0xff

#define PRE_ANY  0x00
#define PRE_NONE 0x01
#define PRE_F2   0x02
#define PRE_F3   0x04
#define PRE_66   0x08
#define PRE_67   0x10
#define PRE_LOCK 0x20
#define PRE_SEG  0x40
#define PRE_ALL  0xff

#define DELTA_OPCODES      0x4a
#define DELTA_FPU_REG      0xf1
#define DELTA_FPU_MODRM    0xf8
#define DELTA_PREFIXES     0x130
#define DELTA_OP_LOCK_OK   0x1a1
#define DELTA_OP2_LOCK_OK  0x1b9
#define DELTA_OP_ONLY_MEM  0x1cb
#define DELTA_OP2_ONLY_MEM 0x1da

unsigned char hde32_table[] =
{
	0xa3, 0xa8, 0xa3, 0xa8, 0xa3, 0xa8, 0xa3, 0xa8, 0xa3, 0xa8, 0xa3, 0xa8, 0xa3, 0xa8, 0xa3,
	0xa8, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xac, 0xaa, 0xb2, 0xaa, 0x9f, 0x9f,
	0x9f, 0x9f, 0xb5, 0xa3, 0xa3, 0xa4, 0xaa, 0xaa, 0xba, 0xaa, 0x96, 0xaa, 0xa8, 0xaa, 0xc3,
	0xc3, 0x96, 0x96, 0xb7, 0xae, 0xd6, 0xbd, 0xa3, 0xc5, 0xa3, 0xa3, 0x9f, 0xc3, 0x9c, 0xaa,
	0xaa, 0xac, 0xaa, 0xbf, 0x03, 0x7f, 0x11, 0x7f, 0x01, 0x7f, 0x01, 0x3f, 0x01, 0x01, 0x90,
	0x82, 0x7d, 0x97, 0x59, 0x59, 0x59, 0x59, 0x59, 0x7f, 0x59, 0x59, 0x60, 0x7d, 0x7f, 0x7f,
	0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x9a, 0x88, 0x7d,
	0x59, 0x50, 0x50, 0x50, 0x50, 0x59, 0x59, 0x59, 0x59, 0x61, 0x94, 0x61, 0x9e, 0x59, 0x59,
	0x85, 0x59, 0x92, 0xa3, 0x60, 0x60, 0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x59,
	0x59, 0x59, 0x9f, 0x01, 0x03, 0x01, 0x04, 0x03, 0xd5, 0x03, 0xcc, 0x01, 0xbc, 0x03, 0xf0,
	0x10, 0x10, 0x10, 0x10, 0x50, 0x50, 0x50, 0x50, 0x14, 0x20, 0x20, 0x20, 0x20, 0x01, 0x01,
	0x01, 0x01, 0xc4, 0x02, 0x10, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0xc0, 0xc2, 0x10, 0x11,
	0x02, 0x03, 0x11, 0x03, 0x03, 0x04, 0x00, 0x00, 0x14, 0x00, 0x02, 0x00, 0x00, 0xc6, 0xc8,
	0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xca,
	0x01, 0x01, 0x01, 0x00, 0x06, 0x00, 0x04, 0x00, 0xc0, 0xc2, 0x01, 0x01, 0x03, 0x01, 0xff,
	0xff, 0x01, 0x00, 0x03, 0xc4, 0xc4, 0xc6, 0x03, 0x01, 0x01, 0x01, 0xff, 0x03, 0x03, 0x03,
	0xc8, 0x40, 0x00, 0x0a, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x33, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xbf, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00,
	0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x7f, 0x00, 0x00, 0xff, 0x4a, 0x4a, 0x4a, 0x4a, 0x4b, 0x52, 0x4a, 0x4a, 0x4a, 0x4a, 0x4f,
	0x4c, 0x4a, 0x4a, 0x4a, 0x4a, 0x4a, 0x4a, 0x4a, 0x4a, 0x55, 0x45, 0x40, 0x4a, 0x4a, 0x4a,
	0x45, 0x59, 0x4d, 0x46, 0x4a, 0x5d, 0x4a, 0x4a, 0x4a, 0x4a, 0x4a, 0x4a, 0x4a, 0x4a, 0x4a,
	0x4a, 0x4a, 0x4a, 0x4a, 0x4a, 0x61, 0x63, 0x67, 0x4e, 0x4a, 0x4a, 0x6b, 0x6d, 0x4a, 0x4a,
	0x45, 0x6d, 0x4a, 0x4a, 0x44, 0x45, 0x4a, 0x4a, 0x00, 0x00, 0x00, 0x02, 0x0d, 0x06, 0x06,
	0x06, 0x06, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x06, 0x00, 0x06, 0x06, 0x02, 0x06,
	0x00, 0x0a, 0x0a, 0x07, 0x07, 0x06, 0x02, 0x05, 0x05, 0x02, 0x02, 0x00, 0x00, 0x04, 0x04,
	0x04, 0x04, 0x00, 0x00, 0x00, 0x0e, 0x05, 0x06, 0x06, 0x06, 0x01, 0x06, 0x00, 0x00, 0x08,
	0x00, 0x10, 0x00, 0x18, 0x00, 0x20, 0x00, 0x28, 0x00, 0x30, 0x00, 0x80, 0x01, 0x82, 0x01,
	0x86, 0x00, 0xf6, 0xcf, 0xfe, 0x3f, 0xab, 0x00, 0xb0, 0x00, 0xb1, 0x00, 0xb3, 0x00, 0xba,
	0xf8, 0xbb, 0x00, 0xc0, 0x00, 0xc1, 0x00, 0xc7, 0xbf, 0x62, 0xff, 0x00, 0x8d, 0xff, 0x00,
	0xc4, 0xff, 0x00, 0xc5, 0xff, 0x00, 0xff, 0xff, 0xeb, 0x01, 0xff, 0x0e, 0x12, 0x08, 0x00,
	0x13, 0x09, 0x00, 0x16, 0x08, 0x00, 0x17, 0x09, 0x00, 0x2b, 0x09, 0x00, 0xae, 0xff, 0x07,
	0xb2, 0xff, 0x00, 0xb4, 0xff, 0x00, 0xb5, 0xff, 0x00, 0xc3, 0x01, 0x00, 0xc7, 0xff, 0xbf,
	0xe7, 0x08, 0x00, 0xf0, 0x02, 0x00
};

unsigned int hde32_disasm(const void *code, hde32s *hs)
{
	uint8_t x, c, *p = (uint8_t *)code, cflags, opcode, pref = 0;
	uint8_t *ht = hde32_table, m_mod, m_reg, m_rm, disp_size = 0;

	__stosb((LPBYTE)hs, 0, sizeof(hde32s));

	for (x = 16; x; x--)
		switch (c = *p++) {
		case 0xf3:
			hs->p_rep = c;
			pref |= PRE_F3;
			break;
		case 0xf2:
			hs->p_rep = c;
			pref |= PRE_F2;
			break;
		case 0xf0:
			hs->p_lock = c;
			pref |= PRE_LOCK;
			break;
		case 0x26: case 0x2e: case 0x36:
		case 0x3e: case 0x64: case 0x65:
			hs->p_seg = c;
			pref |= PRE_SEG;
			break;
		case 0x66:
			hs->p_66 = c;
			pref |= PRE_66;
			break;
		case 0x67:
			hs->p_67 = c;
			pref |= PRE_67;
			break;
		default:
			goto pref_done;
	}
pref_done:

	hs->flags = (uint32_t)pref << 23;

	if (!pref)
		pref |= PRE_NONE;

	if ((hs->opcode = c) == 0x0f) {
		hs->opcode2 = c = *p++;
		ht += DELTA_OPCODES;
	}
	else if (c >= 0xa0 && c <= 0xa3) {
		if (pref & PRE_67)
			pref |= PRE_66;
		else
			pref &= ~PRE_66;
	}

	opcode = c;
	cflags = ht[ht[opcode / 4] + (opcode % 4)];

	if (cflags == C_ERROR) {
		hs->flags |= F_ERROR | F_ERROR_OPCODE;
		cflags = 0;
		if ((opcode & -3) == 0x24)
			cflags++;
	}

	x = 0;
	if (cflags & C_GROUP) {
		uint16_t t;
		t = *(uint16_t *)(ht + (cflags & 0x7f));
		cflags = (uint8_t)t;
		x = (uint8_t)(t >> 8);
	}

	if (hs->opcode2) {
		ht = hde32_table + DELTA_PREFIXES;
		if (ht[ht[opcode / 4] + (opcode % 4)] & pref)
			hs->flags |= F_ERROR | F_ERROR_OPCODE;
	}

	if (cflags & C_MODRM) {
		hs->flags |= F_MODRM;
		hs->modrm = c = *p++;
		hs->modrm_mod = m_mod = c >> 6;
		hs->modrm_rm = m_rm = c & 7;
		hs->modrm_reg = m_reg = (c & 0x3f) >> 3;

		if (x && ((x << m_reg) & 0x80))
			hs->flags |= F_ERROR | F_ERROR_OPCODE;

		if (!hs->opcode2 && opcode >= 0xd9 && opcode <= 0xdf) {
			uint8_t t = opcode - 0xd9;
			if (m_mod == 3) {
				ht = hde32_table + DELTA_FPU_MODRM + t * 8;
				t = ht[m_reg] << m_rm;
			}
			else {
				ht = hde32_table + DELTA_FPU_REG;
				t = ht[t] << m_reg;
			}
			if (t & 0x80)
				hs->flags |= F_ERROR | F_ERROR_OPCODE;
		}

		if (pref & PRE_LOCK) {
			if (m_mod == 3) {
				hs->flags |= F_ERROR | F_ERROR_LOCK;
			}
			else {
				uint8_t *table_end, op = opcode;
				if (hs->opcode2) {
					ht = hde32_table + DELTA_OP2_LOCK_OK;
					table_end = ht + DELTA_OP_ONLY_MEM - DELTA_OP2_LOCK_OK;
				}
				else {
					ht = hde32_table + DELTA_OP_LOCK_OK;
					table_end = ht + DELTA_OP2_LOCK_OK - DELTA_OP_LOCK_OK;
					op &= -2;
				}
				for (; ht != table_end; ht++)
					if (*ht++ == op) {
						if (!((*ht << m_reg) & 0x80))
							goto no_lock_error;
						else
							break;
					}
				hs->flags |= F_ERROR | F_ERROR_LOCK;
			no_lock_error:
				;
			}
		}

		if (hs->opcode2) {
			switch (opcode) {
			case 0x20: case 0x22:
				m_mod = 3;
				if (m_reg > 4 || m_reg == 1)
					goto error_operand;
				else
					goto no_error_operand;
			case 0x21: case 0x23:
				m_mod = 3;
				if (m_reg == 4 || m_reg == 5)
					goto error_operand;
				else
					goto no_error_operand;
			}
		}
		else {
			switch (opcode) {
			case 0x8c:
				if (m_reg > 5)
					goto error_operand;
				else
					goto no_error_operand;
			case 0x8e:
				if (m_reg == 1 || m_reg > 5)
					goto error_operand;
				else
					goto no_error_operand;
			}
		}

		if (m_mod == 3) {
			uint8_t *table_end;
			if (hs->opcode2) {
				ht = hde32_table + DELTA_OP2_ONLY_MEM;
				table_end = ht + sizeof(hde32_table) - DELTA_OP2_ONLY_MEM;
			}
			else {
				ht = hde32_table + DELTA_OP_ONLY_MEM;
				table_end = ht + DELTA_OP2_ONLY_MEM - DELTA_OP_ONLY_MEM;
			}
			for (; ht != table_end; ht += 2)
				if (*ht++ == opcode) {
					if (*ht++ & pref && !((*ht << m_reg) & 0x80))
						goto error_operand;
					else
						break;
				}
			goto no_error_operand;
		}
		else if (hs->opcode2) {
			switch (opcode) {
			case 0x50: case 0xd7: case 0xf7:
				if (pref & (PRE_NONE | PRE_66))
					goto error_operand;
				break;
			case 0xd6:
				if (pref & (PRE_F2 | PRE_F3))
					goto error_operand;
				break;
			case 0xc5:
				goto error_operand;
			}
			goto no_error_operand;
		}
		else
			goto no_error_operand;

	error_operand:
		hs->flags |= F_ERROR | F_ERROR_OPERAND;
	no_error_operand:

		c = *p++;
		if (m_reg <= 1) {
			if (opcode == 0xf6)
				cflags |= C_IMM8;
			else if (opcode == 0xf7)
				cflags |= C_IMM_P66;
		}

		switch (m_mod) {
		case 0:
			if (pref & PRE_67) {
				if (m_rm == 6)
					disp_size = 2;
			}
			else
				if (m_rm == 5)
					disp_size = 4;
			break;
		case 1:
			disp_size = 1;
			break;
		case 2:
			disp_size = 2;
			if (!(pref & PRE_67))
				disp_size <<= 1;
		}

		if (m_mod != 3 && m_rm == 4 && !(pref & PRE_67)) {
			hs->flags |= F_SIB;
			p++;
			hs->sib = c;
			hs->sib_scale = c >> 6;
			hs->sib_index = (c & 0x3f) >> 3;
			if ((hs->sib_base = c & 7) == 5 && !(m_mod & 1))
				disp_size = 4;
		}

		p--;
		switch (disp_size) {
		case 1:
			hs->flags |= F_DISP8;
			hs->disp.disp8 = *p;
			break;
		case 2:
			hs->flags |= F_DISP16;
			hs->disp.disp16 = *(uint16_t *)p;
			break;
		case 4:
			hs->flags |= F_DISP32;
			hs->disp.disp32 = *(uint32_t *)p;
		}
		p += disp_size;
	}
	else if (pref & PRE_LOCK)
		hs->flags |= F_ERROR | F_ERROR_LOCK;

	if (cflags & C_IMM_P66) {
		if (cflags & C_REL32) {
			if (pref & PRE_66) {
				hs->flags |= F_IMM16 | F_RELATIVE;
				hs->imm.imm16 = *(uint16_t *)p;
				p += 2;
				goto disasm_done;
			}
			goto rel32_ok;
		}
		if (pref & PRE_66) {
			hs->flags |= F_IMM16;
			hs->imm.imm16 = *(uint16_t *)p;
			p += 2;
		}
		else {
			hs->flags |= F_IMM32;
			hs->imm.imm32 = *(uint32_t *)p;
			p += 4;
		}
	}

	if (cflags & C_IMM16) {
		if (hs->flags & F_IMM32) {
			hs->flags |= F_IMM16;
			hs->disp.disp16 = *(uint16_t *)p;
		}
		else if (hs->flags & F_IMM16) {
			hs->flags |= F_2IMM16;
			hs->disp.disp16 = *(uint16_t *)p;
		}
		else {
			hs->flags |= F_IMM16;
			hs->imm.imm16 = *(uint16_t *)p;
		}
		p += 2;
	}
	if (cflags & C_IMM8) {
		hs->flags |= F_IMM8;
		hs->imm.imm8 = *p++;
	}

	if (cflags & C_REL32) {
	rel32_ok:
		hs->flags |= F_IMM32 | F_RELATIVE;
		hs->imm.imm32 = *(uint32_t *)p;
		p += 4;
	}
	else if (cflags & C_REL8) {
		hs->flags |= F_IMM8 | F_RELATIVE;
		hs->imm.imm8 = *p++;
	}

disasm_done:

	if ((hs->len = (uint8_t)(p - (uint8_t *)code)) > 15) {
		hs->flags |= F_ERROR | F_ERROR_LENGTH;
		hs->len = 15;
	}

	return (unsigned int)hs->len;
}

#endif

#pragma pack(push, 1)

// 8-bit relative jump.
typedef struct _JMP_REL_SHORT
{
	UINT8  opcode;      // EB xx: JMP +2+xx
	UINT8  operand;
} JMP_REL_SHORT, *PJMP_REL_SHORT;

// 32-bit direct relative jump/call.
typedef struct _JMP_REL
{
	UINT8  opcode;      // E9/E8 xxxxxxxx: JMP/CALL +5+xxxxxxxx
	UINT32 operand;     // Relative destination address
} JMP_REL, *PJMP_REL, CALL_REL;

// 64-bit indirect absolute jump.
typedef struct _JMP_ABS
{
	UINT8  opcode0;     // FF25 00000000: JMP [+6]
	UINT8  opcode1;
	UINT32 dummy;
	UINT64 address;     // Absolute destination address
} JMP_ABS, *PJMP_ABS;

// 64-bit indirect absolute call.
typedef struct _CALL_ABS
{
	UINT8  opcode0;     // FF15 00000002: CALL [+6]
	UINT8  opcode1;
	UINT32 dummy0;
	UINT8  dummy1;      // EB 08:         JMP +10
	UINT8  dummy2;
	UINT64 address;     // Absolute destination address
} CALL_ABS;

// 32-bit direct relative conditional jumps.
typedef struct _JCC_REL
{
	UINT8  opcode0;     // 0F8* xxxxxxxx: J** +6+xxxxxxxx
	UINT8  opcode1;
	UINT32 operand;     // Relative destination address
} JCC_REL;

// 64bit indirect absolute conditional jumps that x64 lacks.
typedef struct _JCC_ABS
{
	UINT8  opcode;      // 7* 0E:         J** +16
	UINT8  dummy0;
	UINT8  dummy1;      // FF25 00000000: JMP [+6]
	UINT8  dummy2;
	UINT32 dummy3;
	UINT64 address;     // Absolute destination address
} JCC_ABS;

#pragma pack(pop)

typedef struct _TRAMPOLINE
{
	LPVOID pTarget;         // [In] Address of the target function.
	LPVOID pDetour;         // [In] Address of the detour function.
	LPVOID pTrampoline;     // [In] Buffer address for the trampoline and relay function.

#ifdef _M_X64
	LPVOID pRelay;          // [Out] Address of the relay function.
#endif
	BOOL   patchAbove;      // [Out] Should use the hot patch area?
	UINT   nIP;             // [Out] Number of the instruction boundaries.
	UINT8  oldIPs[8];       // [Out] Instruction boundaries of the target function.
	UINT8  newIPs[8];       // [Out] Instruction boundaries of the trampoline function.
} TRAMPOLINE, *PTRAMPOLINE;


#ifdef _M_X64
#define MEMORY_SLOT_SIZE 64
#else
#define MEMORY_SLOT_SIZE 32
#endif


// Size of each memory block. (= page size of VirtualAlloc)
#define MEMORY_BLOCK_SIZE 0x1000

// Max range for seeking a memory block. (= 1024MB)
#define MAX_MEMORY_RANGE 0x40000000

// Memory protection flags to check the executable address.
#define PAGE_EXECUTE_FLAGS \
    (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)

// Memory slot.
typedef struct _MEMORY_SLOT
{
	union
	{
		struct _MEMORY_SLOT *pNext;
		UINT8 buffer[MEMORY_SLOT_SIZE];
	};
} MEMORY_SLOT, *PMEMORY_SLOT;

// Memory block info. Placed at the head of each block.
typedef struct _MEMORY_BLOCK
{
	struct _MEMORY_BLOCK *pNext;
	PMEMORY_SLOT pFree;         // First element of the free slot list.
	UINT usedCount;
} MEMORY_BLOCK, *PMEMORY_BLOCK;

// First element of the memory block list.
PMEMORY_BLOCK g_pMemoryBlocks;


VOID InitializeBuffer(VOID)
{
	// Nothing to do for now.
}


VOID UninitializeBuffer(VOID)
{
	PMEMORY_BLOCK pBlock = g_pMemoryBlocks;
	g_pMemoryBlocks = NULL;

	while (pBlock)
	{
		PMEMORY_BLOCK pNext = pBlock->pNext;
		VirtualFree(pBlock, 0, MEM_RELEASE);
		pBlock = pNext;
	}
}


#ifdef _M_X64
static LPVOID FindPrevFreeRegion(LPVOID pAddress, LPVOID pMinAddr, DWORD dwAllocationGranularity)
{
	ULONG_PTR tryAddr = (ULONG_PTR)pAddress;

	// Round down to the next allocation granularity.
	tryAddr -= tryAddr % dwAllocationGranularity;

	// Start from the previous allocation granularity multiply.
	tryAddr -= dwAllocationGranularity;

	while (tryAddr >= (ULONG_PTR)pMinAddr)
	{
		MEMORY_BASIC_INFORMATION mbi;
		if (VirtualQuery((LPVOID)tryAddr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) == 0)
			break;

		if (mbi.State == MEM_FREE)
			return (LPVOID)tryAddr;

		if ((ULONG_PTR)mbi.AllocationBase < dwAllocationGranularity)
			break;

		tryAddr = (ULONG_PTR)mbi.AllocationBase - dwAllocationGranularity;
	}

	return NULL;
}
#endif


#ifdef _M_X64
static LPVOID FindNextFreeRegion(LPVOID pAddress, LPVOID pMaxAddr, DWORD dwAllocationGranularity)
{
	ULONG_PTR tryAddr = (ULONG_PTR)pAddress;

	// Round down to the next allocation granularity.
	tryAddr -= tryAddr % dwAllocationGranularity;

	// Start from the next allocation granularity multiply.
	tryAddr += dwAllocationGranularity;

	while (tryAddr <= (ULONG_PTR)pMaxAddr)
	{
		MEMORY_BASIC_INFORMATION mbi;
		if (VirtualQuery((LPVOID)tryAddr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) == 0)
			break;

		if (mbi.State == MEM_FREE)
			return (LPVOID)tryAddr;

		tryAddr = (ULONG_PTR)mbi.BaseAddress + mbi.RegionSize;

		// Round up to the next allocation granularity.
		tryAddr += dwAllocationGranularity - 1;
		tryAddr -= tryAddr % dwAllocationGranularity;
	}

	return NULL;
}
#endif


static PMEMORY_BLOCK GetMemoryBlock(LPVOID pOrigin)
{
	PMEMORY_BLOCK pBlock;
#ifdef _M_X64
	ULONG_PTR minAddr;
	ULONG_PTR maxAddr;

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	minAddr = (ULONG_PTR)si.lpMinimumApplicationAddress;
	maxAddr = (ULONG_PTR)si.lpMaximumApplicationAddress;

	// pOrigin ¡À 512MB
	if ((ULONG_PTR)pOrigin > MAX_MEMORY_RANGE && minAddr < (ULONG_PTR)pOrigin - MAX_MEMORY_RANGE)
		minAddr = (ULONG_PTR)pOrigin - MAX_MEMORY_RANGE;

	if (maxAddr >(ULONG_PTR)pOrigin + MAX_MEMORY_RANGE)
		maxAddr = (ULONG_PTR)pOrigin + MAX_MEMORY_RANGE;

	// Make room for MEMORY_BLOCK_SIZE bytes.
	maxAddr -= MEMORY_BLOCK_SIZE - 1;
#endif

	// Look the registered blocks for a reachable one.
	for (pBlock = g_pMemoryBlocks; pBlock != NULL; pBlock = pBlock->pNext)
	{
#ifdef _M_X64
		// Ignore the blocks too far.
		if ((ULONG_PTR)pBlock < minAddr || (ULONG_PTR)pBlock >= maxAddr)
			continue;
#endif
		// The block has at least one unused slot.
		if (pBlock->pFree != NULL)
			return pBlock;
	}

#ifdef _M_X64
	// Alloc a new block above if not found.
	{
		LPVOID pAlloc = pOrigin;
		while ((ULONG_PTR)pAlloc >= minAddr)
		{
			pAlloc = FindPrevFreeRegion(pAlloc, (LPVOID)minAddr, si.dwAllocationGranularity);
			if (pAlloc == NULL)
				break;

			pBlock = (PMEMORY_BLOCK)VirtualAlloc(
				pAlloc, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (pBlock != NULL)
				break;
		}
	}

	// Alloc a new block below if not found.
	if (pBlock == NULL)
	{
		LPVOID pAlloc = pOrigin;
		while ((ULONG_PTR)pAlloc <= maxAddr)
		{
			pAlloc = FindNextFreeRegion(pAlloc, (LPVOID)maxAddr, si.dwAllocationGranularity);
			if (pAlloc == NULL)
				break;

			pBlock = (PMEMORY_BLOCK)VirtualAlloc(
				pAlloc, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (pBlock != NULL)
				break;
		}
	}
#else
	// In x86 mode, a memory block can be placed anywhere.
	pBlock = (PMEMORY_BLOCK)VirtualAlloc(
		NULL, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#endif

	if (pBlock != NULL)
	{
		// Build a linked list of all the slots.
		PMEMORY_SLOT pSlot = (PMEMORY_SLOT)pBlock + 1;
		pBlock->pFree = NULL;
		pBlock->usedCount = 0;
		do
		{
			pSlot->pNext = pBlock->pFree;
			pBlock->pFree = pSlot;
			pSlot++;
		} while ((ULONG_PTR)pSlot - (ULONG_PTR)pBlock <= MEMORY_BLOCK_SIZE - MEMORY_SLOT_SIZE);

		pBlock->pNext = g_pMemoryBlocks;
		g_pMemoryBlocks = pBlock;
	}

	return pBlock;
}


LPVOID AllocateBuffer(LPVOID pOrigin)
{
	PMEMORY_SLOT  pSlot;
	PMEMORY_BLOCK pBlock = GetMemoryBlock(pOrigin);
	if (pBlock == NULL)
		return NULL;

	// Remove an unused slot from the list.
	pSlot = pBlock->pFree;
	pBlock->pFree = pSlot->pNext;
	pBlock->usedCount++;

	return pSlot;
}

//-------------------------------------------------------------------------
VOID FreeBuffer(LPVOID pBuffer)
{
	PMEMORY_BLOCK pBlock = g_pMemoryBlocks;
	PMEMORY_BLOCK pPrev = NULL;
	ULONG_PTR pTargetBlock = ((ULONG_PTR)pBuffer / MEMORY_BLOCK_SIZE) * MEMORY_BLOCK_SIZE;

	while (pBlock != NULL)
	{
		if ((ULONG_PTR)pBlock == pTargetBlock)
		{
			PMEMORY_SLOT pSlot = (PMEMORY_SLOT)pBuffer;

			// Restore the released slot to the list.
			pSlot->pNext = pBlock->pFree;
			pBlock->pFree = pSlot;
			pBlock->usedCount--;

			// Free if unused.
			if (pBlock->usedCount == 0)
			{
				if (pPrev)
					pPrev->pNext = pBlock->pNext;
				else
					g_pMemoryBlocks = pBlock->pNext;

				VirtualFree(pBlock, 0, MEM_RELEASE);
			}

			break;
		}

		pPrev = pBlock;
		pBlock = pBlock->pNext;
	}
}


BOOL IsExecutableAddress(LPVOID pAddress)
{
	MEMORY_BASIC_INFORMATION mi;
	VirtualQuery(pAddress, &mi, sizeof(MEMORY_BASIC_INFORMATION));

	return (mi.State == MEM_COMMIT && (mi.Protect & PAGE_EXECUTE_FLAGS));
}


#ifndef ARRAYSIZE
#define ARRAYSIZE(A) (sizeof(A)/sizeof((A)[0]))
#endif

#ifdef _M_X64
typedef hde64s HDE;
#define HDE_DISASM(code, hs) hde64_disasm(code, hs)
#else
typedef hde32s HDE;
#define HDE_DISASM(code, hs) hde32_disasm(code, hs)
#endif



// Maximum size of a trampoline function.
#ifdef _M_X64
#define TRAMPOLINE_MAX_SIZE (MEMORY_SLOT_SIZE - sizeof(JMP_ABS))
#else
#define TRAMPOLINE_MAX_SIZE MEMORY_SLOT_SIZE
#endif

//-------------------------------------------------------------------------
static BOOL IsCodePadding(LPBYTE pInst, UINT size)
{
	UINT i;

	if (pInst[0] != 0x00 && pInst[0] != 0x90 && pInst[0] != 0xCC)
		return FALSE;

	for (i = 1; i < size; ++i)
	{
		if (pInst[i] != pInst[0])
			return FALSE;
	}
	return TRUE;
}

//-------------------------------------------------------------------------
BOOL CreateTrampolineFunction(PTRAMPOLINE ct)
{
#ifdef _M_X64
	CALL_ABS call = {
		0xFF, 0x15, 0x00000002, // FF15 00000002: CALL [RIP+8]
		0xEB, 0x08,             // EB 08:         JMP +10
		0x0000000000000000ULL   // Absolute destination address
	};
	JMP_ABS jmp = {
		0xFF, 0x25, 0x00000000, // FF25 00000000: JMP [RIP+6]
		0x0000000000000000ULL   // Absolute destination address
	};
	JCC_ABS jcc = {
		0x70, 0x0E,             // 7* 0E:         J** +16
		0xFF, 0x25, 0x00000000, // FF25 00000000: JMP [RIP+6]
		0x0000000000000000ULL   // Absolute destination address
	};
#else
	CALL_REL call =
	{
		0xE8,                   // E8 xxxxxxxx: CALL +5+xxxxxxxx
		0x00000000              // Relative destination address
	};
	JMP_REL jmp = {
		0xE9,                   // E9 xxxxxxxx: JMP +5+xxxxxxxx
		0x00000000              // Relative destination address
	};
	JCC_REL jcc = {
		0x0F, 0x80,             // 0F8* xxxxxxxx: J** +6+xxxxxxxx
		0x00000000              // Relative destination address
	};
#endif

	UINT8     oldPos = 0;
	UINT8     newPos = 0;
	ULONG_PTR jmpDest = 0;     // Destination address of an internal jump.
	BOOL      finished = FALSE; // Is the function completed?
#ifdef _M_X64
	UINT8     instBuf[16];
#endif

	ct->patchAbove = FALSE;
	ct->nIP = 0;

	do
	{
		HDE       hs;
		UINT      copySize;
		LPVOID    pCopySrc;
		ULONG_PTR pOldInst = (ULONG_PTR)ct->pTarget + oldPos;
		ULONG_PTR pNewInst = (ULONG_PTR)ct->pTrampoline + newPos;

		copySize = HDE_DISASM((LPVOID)pOldInst, &hs);
		if (hs.flags & F_ERROR)
			return FALSE;

		pCopySrc = (LPVOID)pOldInst;
		if (oldPos >= sizeof(JMP_REL))
		{
			// The trampoline function is long enough.
			// Complete the function with the jump to the target function.
#ifdef _M_X64
			jmp.address = pOldInst;
#else
			jmp.operand = (UINT32)(pOldInst - (pNewInst + sizeof(jmp)));
#endif
			pCopySrc = &jmp;
			copySize = sizeof(jmp);

			finished = TRUE;
		}
#ifdef _M_X64
		else if ((hs.modrm & 0xC7) == 0x05)
		{
			// Instructions using RIP relative addressing. (ModR/M = 00???101B)

			// Modify the RIP relative address.
			PUINT32 pRelAddr;

			__movsb(instBuf, (LPBYTE)pOldInst, copySize);

			pCopySrc = instBuf;

			// Relative address is stored at (instruction length - immediate value length - 4).
			pRelAddr = (PUINT32)(instBuf + hs.len - ((hs.flags & 0x3C) >> 2) - 4);
			*pRelAddr
				= (UINT32)((pOldInst + hs.len + (INT32)hs.disp.disp32) - (pNewInst + hs.len));

			// Complete the function if JMP (FF /4).
			if (hs.opcode == 0xFF && hs.modrm_reg == 4)
				finished = TRUE;
		}
#endif
		else if (hs.opcode == 0xE8)
		{
			// Direct relative CALL
			ULONG_PTR dest = pOldInst + hs.len + (INT32)hs.imm.imm32;
#ifdef _M_X64
			call.address = dest;
#else
			call.operand = (UINT32)(dest - (pNewInst + sizeof(call)));
#endif
			pCopySrc = &call;
			copySize = sizeof(call);
		}
		else if ((hs.opcode & 0xFD) == 0xE9)
		{
			// Direct relative JMP (EB or E9)
			ULONG_PTR dest = pOldInst + hs.len;

			if (hs.opcode == 0xEB) // isShort jmp
				dest += (INT8)hs.imm.imm8;
			else
				dest += (INT32)hs.imm.imm32;

			// Simply copy an internal jump.
			if ((ULONG_PTR)ct->pTarget <= dest
				&& dest < ((ULONG_PTR)ct->pTarget + sizeof(JMP_REL)))
			{
				if (jmpDest < dest)
					jmpDest = dest;
			}
			else
			{
#ifdef _M_X64
				jmp.address = dest;
#else
				jmp.operand = (UINT32)(dest - (pNewInst + sizeof(jmp)));
#endif
				pCopySrc = &jmp;
				copySize = sizeof(jmp);

				// Exit the function If it is not in the branch
				finished = (pOldInst >= jmpDest);
			}
		}
		else if ((hs.opcode & 0xF0) == 0x70
			|| (hs.opcode & 0xFC) == 0xE0
			|| (hs.opcode2 & 0xF0) == 0x80)
		{
			// Direct relative Jcc
			ULONG_PTR dest = pOldInst + hs.len;

			if ((hs.opcode & 0xF0) == 0x70      // Jcc
				|| (hs.opcode & 0xFC) == 0xE0)  // LOOPNZ/LOOPZ/LOOP/JECXZ
				dest += (INT8)hs.imm.imm8;
			else
				dest += (INT32)hs.imm.imm32;

			// Simply copy an internal jump.
			if ((ULONG_PTR)ct->pTarget <= dest
				&& dest < ((ULONG_PTR)ct->pTarget + sizeof(JMP_REL)))
			{
				if (jmpDest < dest)
					jmpDest = dest;
			}
			else if ((hs.opcode & 0xFC) == 0xE0)
			{
				// LOOPNZ/LOOPZ/LOOP/JCXZ/JECXZ to the outside are not supported.
				return FALSE;
			}
			else
			{
				UINT8 cond = ((hs.opcode != 0x0F ? hs.opcode : hs.opcode2) & 0x0F);
#ifdef _M_X64
				// Invert the condition in x64 mode to simplify the conditional jump logic.
				jcc.opcode = 0x71 ^ cond;
				jcc.address = dest;
#else
				jcc.opcode1 = 0x80 | cond;
				jcc.operand = (UINT32)(dest - (pNewInst + sizeof(jcc)));
#endif
				pCopySrc = &jcc;
				copySize = sizeof(jcc);
			}
		}
		else if ((hs.opcode & 0xFE) == 0xC2)
		{
			// RET (C2 or C3)

			// Complete the function if not in a branch.
			finished = (pOldInst >= jmpDest);
		}

		// Can't alter the instruction length in a branch.
		if (pOldInst < jmpDest && copySize != hs.len)
			return FALSE;

		// Trampoline function is too large.
		if ((newPos + copySize) > TRAMPOLINE_MAX_SIZE)
			return FALSE;

		// Trampoline function has too many instructions.
		if (ct->nIP >= ARRAYSIZE(ct->oldIPs))
			return FALSE;

		ct->oldIPs[ct->nIP] = oldPos;
		ct->newIPs[ct->nIP] = newPos;
		ct->nIP++;

		__movsb((LPBYTE)ct->pTrampoline + newPos, (LPBYTE)pCopySrc, copySize);

		newPos += copySize;
		oldPos += hs.len;
	} while (!finished);

	// Is there enough place for a long jump?
	if (oldPos < sizeof(JMP_REL)
		&& !IsCodePadding((LPBYTE)ct->pTarget + oldPos, sizeof(JMP_REL) - oldPos))
	{
		// Is there enough place for a short jump?
		if (oldPos < sizeof(JMP_REL_SHORT)
			&& !IsCodePadding((LPBYTE)ct->pTarget + oldPos, sizeof(JMP_REL_SHORT) - oldPos))
		{
			return FALSE;
		}

		// Can we place the long jump above the function?
		if (!IsExecutableAddress((LPBYTE)ct->pTarget - sizeof(JMP_REL)))
			return FALSE;

		if (!IsCodePadding((LPBYTE)ct->pTarget - sizeof(JMP_REL), sizeof(JMP_REL)))
			return FALSE;

		ct->patchAbove = TRUE;
	}

#ifdef _M_X64
	// Create a relay function.
	jmp.address = (ULONG_PTR)ct->pDetour;

	ct->pRelay = (LPBYTE)ct->pTrampoline + newPos;
	RtlCopyMemory(ct->pRelay, &jmp, sizeof(jmp));
#endif

	return TRUE;
}

// Initial capacity of the HOOK_ENTRY buffer.
#define INITIAL_HOOK_CAPACITY   32

// Initial capacity of the thread IDs buffer.
#define INITIAL_THREAD_CAPACITY 128

// Special hook position values.
#define INVALID_HOOK_POS UINT_MAX
#define ALL_HOOKS_POS    UINT_MAX

// Freeze() action argument defines.
#define ACTION_DISABLE      0
#define ACTION_ENABLE       1
#define ACTION_APPLY_QUEUED 2

// Thread access rights for suspending/resuming threads.
#define THREAD_ACCESS \
    (THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION | THREAD_SET_CONTEXT)

// Hook information.
typedef struct _HOOK_ENTRY
{
	LPVOID pTarget;             // Address of the target function.
	LPVOID pDetour;             // Address of the detour or relay function.
	LPVOID pTrampoline;         // Address of the trampoline function.
	UINT8  backup[8];           // Original prologue of the target function.

	UINT8  patchAbove : 1;     // Uses the hot patch area.
	UINT8  isEnabled : 1;     // Enabled.
	UINT8  queueEnable : 1;     // Queued for enabling/disabling when != isEnabled.

	UINT   nIP : 4;             // Count of the instruction boundaries.
	UINT8  oldIPs[8];           // Instruction boundaries of the target function.
	UINT8  newIPs[8];           // Instruction boundaries of the trampoline function.
} HOOK_ENTRY, *PHOOK_ENTRY;

// Suspended threads for Freeze()/Unfreeze().
typedef struct _FROZEN_THREADS
{
	LPDWORD pItems;         // Data heap
	UINT    capacity;       // Size of allocated data heap, items
	UINT    size;           // Actual number of data items
} FROZEN_THREADS, *PFROZEN_THREADS;


// Spin lock flag for EnterSpinLock()/LeaveSpinLock().
volatile LONG g_isLocked = FALSE;

// Private heap handle. If not NULL, this library is initialized.
HANDLE g_hHeap = NULL;

// Hook entries.
struct
{
	PHOOK_ENTRY pItems;     // Data heap
	UINT        capacity;   // Size of allocated data heap, items
	UINT        size;       // Actual number of data items
} g_hooks;


// Returns INVALID_HOOK_POS if not found.
static UINT FindHookEntry(LPVOID pTarget)
{
	UINT i;
	for (i = 0; i < g_hooks.size; ++i)
	{
		if ((ULONG_PTR)pTarget == (ULONG_PTR)g_hooks.pItems[i].pTarget)
			return i;
	}

	return INVALID_HOOK_POS;
}

static PHOOK_ENTRY AddHookEntry()
{
	if (g_hooks.pItems == NULL)
	{
		g_hooks.capacity = INITIAL_HOOK_CAPACITY;
		g_hooks.pItems = (PHOOK_ENTRY)HeapAlloc(
			g_hHeap, 0, g_hooks.capacity * sizeof(HOOK_ENTRY));
		if (g_hooks.pItems == NULL)
			return NULL;
	}
	else if (g_hooks.size >= g_hooks.capacity)
	{
		PHOOK_ENTRY p = (PHOOK_ENTRY)HeapReAlloc(
			g_hHeap, 0, g_hooks.pItems, (g_hooks.capacity * 2) * sizeof(HOOK_ENTRY));
		if (p == NULL)
			return NULL;

		g_hooks.capacity *= 2;
		g_hooks.pItems = p;
	}

	return &g_hooks.pItems[g_hooks.size++];
}


static void DeleteHookEntry(UINT pos)
{
	if (pos < g_hooks.size - 1)
		g_hooks.pItems[pos] = g_hooks.pItems[g_hooks.size - 1];

	g_hooks.size--;

	if (g_hooks.capacity / 2 >= INITIAL_HOOK_CAPACITY && g_hooks.capacity / 2 >= g_hooks.size)
	{
		PHOOK_ENTRY p = (PHOOK_ENTRY)HeapReAlloc(
			g_hHeap, 0, g_hooks.pItems, (g_hooks.capacity / 2) * sizeof(HOOK_ENTRY));
		if (p == NULL)
			return;

		g_hooks.capacity /= 2;
		g_hooks.pItems = p;
	}
}


static DWORD_PTR FindOldIP(PHOOK_ENTRY pHook, DWORD_PTR ip)
{
	UINT i;

	if (pHook->patchAbove && ip == ((DWORD_PTR)pHook->pTarget - sizeof(JMP_REL)))
		return (DWORD_PTR)pHook->pTarget;

	for (i = 0; i < pHook->nIP; ++i)
	{
		if (ip == ((DWORD_PTR)pHook->pTrampoline + pHook->newIPs[i]))
			return (DWORD_PTR)pHook->pTarget + pHook->oldIPs[i];
	}

#ifdef _M_X64
	// Check relay function.
	if (ip == (DWORD_PTR)pHook->pDetour)
		return (DWORD_PTR)pHook->pTarget;
#endif

	return 0;
}


static DWORD_PTR FindNewIP(PHOOK_ENTRY pHook, DWORD_PTR ip)
{
	UINT i;
	for (i = 0; i < pHook->nIP; ++i)
	{
		if (ip == ((DWORD_PTR)pHook->pTarget + pHook->oldIPs[i]))
			return (DWORD_PTR)pHook->pTrampoline + pHook->newIPs[i];
	}

	return 0;
}

//-------------------------------------------------------------------------
static void ProcessThreadIPs(HANDLE hThread, UINT pos, UINT action)
{
	// If the thread suspended in the overwritten area,
	// move IP to the proper address.

	CONTEXT c;
#ifdef _M_X64
	DWORD64 *pIP = &c.Rip;
#else
	DWORD   *pIP = &c.Eip;
#endif
	UINT count;

	c.ContextFlags = CONTEXT_CONTROL;
	if (!GetThreadContext(hThread, &c))
		return;

	if (pos == ALL_HOOKS_POS)
	{
		pos = 0;
		count = g_hooks.size;
	}
	else
	{
		count = pos + 1;
	}

	for (; pos < count; ++pos)
	{
		PHOOK_ENTRY pHook = &g_hooks.pItems[pos];
		BOOL        enable;
		DWORD_PTR   ip;

		switch (action)
		{
		case ACTION_DISABLE:
			enable = FALSE;
			break;

		case ACTION_ENABLE:
			enable = TRUE;
			break;

		case ACTION_APPLY_QUEUED:
			enable = pHook->queueEnable;
			break;
		}
		if (pHook->isEnabled == enable)
			continue;

		if (enable)
			ip = FindNewIP(pHook, *pIP);
		else
			ip = FindOldIP(pHook, *pIP);

		if (ip != 0)
		{
			*pIP = ip;
			SetThreadContext(hThread, &c);
		}
	}
}


static VOID EnumerateThreads(PFROZEN_THREADS pThreads)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 te;
		te.dwSize = sizeof(THREADENTRY32);
		if (Thread32First(hSnapshot, &te))
		{
			do
			{
				if (te.dwSize >= (FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(DWORD))
					&& te.th32OwnerProcessID == GetCurrentProcessId()
					&& te.th32ThreadID != GetCurrentThreadId())
				{
					if (pThreads->pItems == NULL)
					{
						pThreads->capacity = INITIAL_THREAD_CAPACITY;
						pThreads->pItems
							= (LPDWORD)HeapAlloc(g_hHeap, 0, pThreads->capacity * sizeof(DWORD));
						if (pThreads->pItems == NULL)
							break;
					}
					else if (pThreads->size >= pThreads->capacity)
					{
						LPDWORD p = (LPDWORD)HeapReAlloc(
							g_hHeap, 0, pThreads->pItems, (pThreads->capacity * 2) * sizeof(DWORD));
						if (p == NULL)
							break;

						pThreads->capacity *= 2;
						pThreads->pItems = p;
					}
					pThreads->pItems[pThreads->size++] = te.th32ThreadID;
				}

				te.dwSize = sizeof(THREADENTRY32);
			} while (Thread32Next(hSnapshot, &te));
		}
		CloseHandle(hSnapshot);
	}
}

//-------------------------------------------------------------------------
static VOID Freeze(PFROZEN_THREADS pThreads, UINT pos, UINT action)
{
	pThreads->pItems = NULL;
	pThreads->capacity = 0;
	pThreads->size = 0;
	EnumerateThreads(pThreads);

	if (pThreads->pItems != NULL)
	{
		UINT i;
		for (i = 0; i < pThreads->size; ++i)
		{
			HANDLE hThread = OpenThread(THREAD_ACCESS, FALSE, pThreads->pItems[i]);
			if (hThread != NULL)
			{
				SuspendThread(hThread);
				ProcessThreadIPs(hThread, pos, action);
				CloseHandle(hThread);
			}
		}
	}
}

//-------------------------------------------------------------------------
static VOID Unfreeze(PFROZEN_THREADS pThreads)
{
	if (pThreads->pItems != NULL)
	{
		UINT i;
		for (i = 0; i < pThreads->size; ++i)
		{
			HANDLE hThread = OpenThread(THREAD_ACCESS, FALSE, pThreads->pItems[i]);
			if (hThread != NULL)
			{
				ResumeThread(hThread);
				CloseHandle(hThread);
			}
		}

		HeapFree(g_hHeap, 0, pThreads->pItems);
	}
}

//-------------------------------------------------------------------------
static MH_STATUS EnableHookLL(UINT pos, BOOL enable)
{
	PHOOK_ENTRY pHook = &g_hooks.pItems[pos];
	DWORD  oldProtect;
	SIZE_T patchSize = sizeof(JMP_REL);
	LPBYTE pPatchTarget = (LPBYTE)pHook->pTarget;

	if (pHook->patchAbove)
	{
		pPatchTarget -= sizeof(JMP_REL);
		patchSize += sizeof(JMP_REL_SHORT);
	}

	if (!VirtualProtect(pPatchTarget, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect))
		return MH_ERROR_MEMORY_PROTECT;

	if (enable)
	{
		PJMP_REL pJmp = (PJMP_REL)pPatchTarget;
		pJmp->opcode = 0xE9;
		pJmp->operand = (UINT32)((LPBYTE)pHook->pDetour - (pPatchTarget + sizeof(JMP_REL)));

		if (pHook->patchAbove)
		{
			PJMP_REL_SHORT pShortJmp = (PJMP_REL_SHORT)pHook->pTarget;
			pShortJmp->opcode = 0xEB;
			pShortJmp->operand = (UINT8)(0 - (sizeof(JMP_REL_SHORT) + sizeof(JMP_REL)));
		}
	}
	else
	{
		if (pHook->patchAbove)
			memcpy(pPatchTarget, pHook->backup, sizeof(JMP_REL) + sizeof(JMP_REL_SHORT));
		else
			memcpy(pPatchTarget, pHook->backup, sizeof(JMP_REL));
	}

	VirtualProtect(pPatchTarget, patchSize, oldProtect, &oldProtect);

	// Just-in-case measure.
	FlushInstructionCache(GetCurrentProcess(), pPatchTarget, patchSize);

	pHook->isEnabled = enable;
	pHook->queueEnable = enable;

	return MH_OK;
}

//-------------------------------------------------------------------------
static MH_STATUS EnableAllHooksLL(BOOL enable)
{
	MH_STATUS status = MH_OK;
	UINT i, first = INVALID_HOOK_POS;

	for (i = 0; i < g_hooks.size; ++i)
	{
		if (g_hooks.pItems[i].isEnabled != enable)
		{
			first = i;
			break;
		}
	}

	if (first != INVALID_HOOK_POS)
	{
		FROZEN_THREADS threads;
		Freeze(&threads, ALL_HOOKS_POS, enable ? ACTION_ENABLE : ACTION_DISABLE);

		for (i = first; i < g_hooks.size; ++i)
		{
			if (g_hooks.pItems[i].isEnabled != enable)
			{
				status = EnableHookLL(i, enable);
				if (status != MH_OK)
					break;
			}
		}

		Unfreeze(&threads);
	}

	return status;
}

//-------------------------------------------------------------------------
static VOID EnterSpinLock(VOID)
{
	SIZE_T spinCount = 0;

	// Wait until the flag is FALSE.
	while (InterlockedCompareExchange(&g_isLocked, TRUE, FALSE) != FALSE)
	{
		// No need to generate a memory barrier here, since InterlockedCompareExchange()
		// generates a full memory barrier itself.

		// Prevent the loop from being too busy.
		if (spinCount < 32)
			Ps::Sleep(0);
		else
			Ps::Sleep(1);

		spinCount++;
	}
}

//-------------------------------------------------------------------------
static VOID LeaveSpinLock(VOID)
{
	// No need to generate a memory barrier here, since InterlockedExchange()
	// generates a full memory barrier itself.

	InterlockedExchange(&g_isLocked, FALSE);
}

//-------------------------------------------------------------------------
MH_STATUS WINAPI MH_Initialize(VOID)
{
	MH_STATUS status = MH_OK;

	EnterSpinLock();

	if (g_hHeap == NULL)
	{
		g_hHeap = HeapCreate(0, 0, 0);
		if (g_hHeap != NULL)
		{
			// Initialize the internal function buffer.
			InitializeBuffer();
		}
		else
		{
			status = MH_ERROR_MEMORY_ALLOC;
		}
	}
	else
	{
		status = MH_ERROR_ALREADY_INITIALIZED;
	}

	LeaveSpinLock();

	return status;
}

//-------------------------------------------------------------------------
MH_STATUS WINAPI MH_Uninitialize(VOID)
{
	MH_STATUS status = MH_OK;

	EnterSpinLock();

	if (g_hHeap != NULL)
	{
		status = EnableAllHooksLL(FALSE);
		if (status == MH_OK)
		{
			// Free the internal function buffer.

			// HeapFree is actually not required, but some tools detect a false
			// memory leak without HeapFree.

			UninitializeBuffer();

			HeapFree(g_hHeap, 0, g_hooks.pItems);
			HeapDestroy(g_hHeap);

			g_hHeap = NULL;

			g_hooks.pItems = NULL;
			g_hooks.capacity = 0;
			g_hooks.size = 0;
		}
	}
	else
	{
		status = MH_ERROR_NOT_INITIALIZED;
	}

	LeaveSpinLock();

	return status;
}

//-------------------------------------------------------------------------
MH_STATUS WINAPI MH_CreateHook(LPVOID pTarget, LPVOID pDetour, LPVOID *ppOriginal)
{
	MH_STATUS status = MH_OK;

	EnterSpinLock();

	if (g_hHeap != NULL)
	{
		if (IsExecutableAddress(pTarget) && IsExecutableAddress(pDetour))
		{
			UINT pos = FindHookEntry(pTarget);
			if (pos == INVALID_HOOK_POS)
			{
				LPVOID pBuffer = AllocateBuffer(pTarget);
				if (pBuffer != NULL)
				{
					TRAMPOLINE ct;

					ct.pTarget = pTarget;
					ct.pDetour = pDetour;
					ct.pTrampoline = pBuffer;
					if (CreateTrampolineFunction(&ct))
					{
						PHOOK_ENTRY pHook = AddHookEntry();
						if (pHook != NULL)
						{
							pHook->pTarget = ct.pTarget;
#ifdef _M_X64
							pHook->pDetour = ct.pRelay;
#else
							pHook->pDetour = ct.pDetour;
#endif
							pHook->pTrampoline = ct.pTrampoline;
							pHook->patchAbove = ct.patchAbove;
							pHook->isEnabled = FALSE;
							pHook->queueEnable = FALSE;
							pHook->nIP = ct.nIP;
							memcpy(pHook->oldIPs, ct.oldIPs, ARRAYSIZE(ct.oldIPs));
							memcpy(pHook->newIPs, ct.newIPs, ARRAYSIZE(ct.newIPs));

							// Back up the target function.

							if (ct.patchAbove)
							{
								memcpy(
									pHook->backup,
									(LPBYTE)pTarget - sizeof(JMP_REL),
									sizeof(JMP_REL) + sizeof(JMP_REL_SHORT));
							}
							else
							{
								memcpy(pHook->backup, pTarget, sizeof(JMP_REL));
							}

							if (ppOriginal != NULL)
								*ppOriginal = pHook->pTrampoline;
						}
						else
						{
							status = MH_ERROR_MEMORY_ALLOC;
						}
					}
					else
					{
						status = MH_ERROR_UNSUPPORTED_FUNCTION;
					}

					if (status != MH_OK)
					{
						FreeBuffer(pBuffer);
					}
				}
				else
				{
					status = MH_ERROR_MEMORY_ALLOC;
				}
			}
			else
			{
				status = MH_ERROR_ALREADY_CREATED;
			}
		}
		else
		{
			status = MH_ERROR_NOT_EXECUTABLE;
		}
	}
	else
	{
		status = MH_ERROR_NOT_INITIALIZED;
	}

	LeaveSpinLock();

	return status;
}

//-------------------------------------------------------------------------
MH_STATUS WINAPI MH_RemoveHook(LPVOID pTarget)
{
	MH_STATUS status = MH_OK;

	EnterSpinLock();

	if (g_hHeap != NULL)
	{
		UINT pos = FindHookEntry(pTarget);
		if (pos != INVALID_HOOK_POS)
		{
			if (g_hooks.pItems[pos].isEnabled)
			{
				FROZEN_THREADS threads;
				Freeze(&threads, pos, ACTION_DISABLE);

				status = EnableHookLL(pos, FALSE);

				Unfreeze(&threads);
			}

			if (status == MH_OK)
			{
				FreeBuffer(g_hooks.pItems[pos].pTrampoline);
				DeleteHookEntry(pos);
			}
		}
		else
		{
			status = MH_ERROR_NOT_CREATED;
		}
	}
	else
	{
		status = MH_ERROR_NOT_INITIALIZED;
	}

	LeaveSpinLock();

	return status;
}

//-------------------------------------------------------------------------
static MH_STATUS EnableHook(LPVOID pTarget, BOOL enable)
{
	MH_STATUS status = MH_OK;

	EnterSpinLock();

	if (g_hHeap != NULL)
	{
		if (pTarget == MH_ALL_HOOKS)
		{
			status = EnableAllHooksLL(enable);
		}
		else
		{
			FROZEN_THREADS threads;
			UINT pos = FindHookEntry(pTarget);
			if (pos != INVALID_HOOK_POS)
			{
				if (g_hooks.pItems[pos].isEnabled != enable)
				{
					Freeze(&threads, pos, ACTION_ENABLE);

					status = EnableHookLL(pos, enable);

					Unfreeze(&threads);
				}
				else
				{
					status = enable ? MH_ERROR_ENABLED : MH_ERROR_DISABLED;
				}
			}
			else
			{
				status = MH_ERROR_NOT_CREATED;
			}
		}
	}
	else
	{
		status = MH_ERROR_NOT_INITIALIZED;
	}

	LeaveSpinLock();

	return status;
}

//-------------------------------------------------------------------------
MH_STATUS WINAPI MH_EnableHook(LPVOID pTarget)
{
	return EnableHook(pTarget, TRUE);
}

//-------------------------------------------------------------------------
MH_STATUS WINAPI MH_DisableHook(LPVOID pTarget)
{
	return EnableHook(pTarget, FALSE);
}

//-------------------------------------------------------------------------
static MH_STATUS QueueHook(LPVOID pTarget, BOOL queueEnable)
{
	MH_STATUS status = MH_OK;

	EnterSpinLock();

	if (g_hHeap != NULL)
	{
		if (pTarget == MH_ALL_HOOKS)
		{
			UINT i;
			for (i = 0; i < g_hooks.size; ++i)
				g_hooks.pItems[i].queueEnable = queueEnable;
		}
		else
		{
			UINT pos = FindHookEntry(pTarget);
			if (pos != INVALID_HOOK_POS)
			{
				g_hooks.pItems[pos].queueEnable = queueEnable;
			}
			else
			{
				status = MH_ERROR_NOT_CREATED;
			}
		}
	}
	else
	{
		status = MH_ERROR_NOT_INITIALIZED;
	}

	LeaveSpinLock();

	return status;
}

//-------------------------------------------------------------------------
MH_STATUS WINAPI MH_QueueEnableHook(LPVOID pTarget)
{
	return QueueHook(pTarget, TRUE);
}

//-------------------------------------------------------------------------
MH_STATUS WINAPI MH_QueueDisableHook(LPVOID pTarget)
{
	return QueueHook(pTarget, FALSE);
}

//-------------------------------------------------------------------------
MH_STATUS WINAPI MH_ApplyQueued(VOID)
{
	MH_STATUS status = MH_OK;
	UINT i, first = INVALID_HOOK_POS;

	EnterSpinLock();

	if (g_hHeap != NULL)
	{
		for (i = 0; i < g_hooks.size; ++i)
		{
			if (g_hooks.pItems[i].isEnabled != g_hooks.pItems[i].queueEnable)
			{
				first = i;
				break;
			}
		}

		if (first != INVALID_HOOK_POS)
		{
			FROZEN_THREADS threads;
			Freeze(&threads, ALL_HOOKS_POS, ACTION_APPLY_QUEUED);

			for (i = first; i < g_hooks.size; ++i)
			{
				PHOOK_ENTRY pHook = &g_hooks.pItems[i];
				if (pHook->isEnabled != pHook->queueEnable)
				{
					status = EnableHookLL(i, pHook->queueEnable);
					if (status != MH_OK)
						break;
				}
			}

			Unfreeze(&threads);
		}
	}
	else
	{
		status = MH_ERROR_NOT_INITIALIZED;
	}

	LeaveSpinLock();

	return status;
}

//-------------------------------------------------------------------------
MH_STATUS WINAPI MH_CreateHookApiEx(
	LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour,
	LPVOID *ppOriginal, LPVOID *ppTarget)
{
	HMODULE hModule;
	LPVOID  pTarget;

	hModule = GetModuleHandleW(pszModule);
	if (hModule == NULL)
		return MH_ERROR_MODULE_NOT_FOUND;

	pTarget = (LPVOID)GetProcAddress(hModule, pszProcName);
	if (pTarget == NULL)
		return MH_ERROR_FUNCTION_NOT_FOUND;

	if (ppTarget != NULL)
		*ppTarget = pTarget;

	return MH_CreateHook(pTarget, pDetour, ppOriginal);
}

//-------------------------------------------------------------------------
MH_STATUS WINAPI MH_CreateHookApi(
	LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, LPVOID *ppOriginal)
{
	return MH_CreateHookApiEx(pszModule, pszProcName, pDetour, ppOriginal, NULL);
}

//-------------------------------------------------------------------------
const char * WINAPI MH_StatusToString(MH_STATUS status)
{
#define MH_ST2STR(x)    \
    case x:             \
        return #x;

	switch (status) {
		MH_ST2STR(MH_UNKNOWN)
			MH_ST2STR(MH_OK)
			MH_ST2STR(MH_ERROR_ALREADY_INITIALIZED)
			MH_ST2STR(MH_ERROR_NOT_INITIALIZED)
			MH_ST2STR(MH_ERROR_ALREADY_CREATED)
			MH_ST2STR(MH_ERROR_NOT_CREATED)
			MH_ST2STR(MH_ERROR_ENABLED)
			MH_ST2STR(MH_ERROR_DISABLED)
			MH_ST2STR(MH_ERROR_NOT_EXECUTABLE)
			MH_ST2STR(MH_ERROR_UNSUPPORTED_FUNCTION)
			MH_ST2STR(MH_ERROR_MEMORY_ALLOC)
			MH_ST2STR(MH_ERROR_MEMORY_PROTECT)
			MH_ST2STR(MH_ERROR_MODULE_NOT_FOUND)
			MH_ST2STR(MH_ERROR_FUNCTION_NOT_FOUND)
	}

#undef MH_ST2STR

	return "(unknown)";
}


BOOL NTAPI IATPatch32(PVOID hMod, LPCSTR szDllName, LPVOID pfnOrg, LPVOID pfnNew)
{
	LPCSTR                     szLibName;
	PIMAGE_IMPORT_DESCRIPTOR   pImportDesc;
	PIMAGE_THUNK_DATA          pThunk;
	ULONG                      dwOldProtect, dwRVA;
	PBYTE                      pAddr;

	pAddr = (PBYTE)hMod;
	pAddr += *((DWORD*)&pAddr[0x3C]);
	dwRVA = *((DWORD*)&pAddr[0x80]);

	pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)hMod + dwRVA);

	for (; pImportDesc->Name; pImportDesc++)
	{
		szLibName = (LPCSTR)((DWORD)hMod + pImportDesc->Name);
		if (!lstrcmpiA(szLibName, szDllName))
		{
			pThunk = (PIMAGE_THUNK_DATA)((DWORD)hMod + pImportDesc->FirstThunk);
			for (; pThunk->u1.Function; pThunk++)
			{
				if (pThunk->u1.Function == (DWORD)pfnOrg)
				{
					VirtualProtect((LPVOID)&pThunk->u1.Function, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect);
					pThunk->u1.Function = (DWORD)pfnNew;
					VirtualProtect((LPVOID)&pThunk->u1.Function, 4, dwOldProtect, &dwOldProtect);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

BOOL NTAPI IATPatch64(HMODULE hMod, LPCSTR DllName, LPVOID orgProc, LPVOID newProc)
{
	IMAGE_DOS_HEADER*                 pDosHeader;
	IMAGE_OPTIONAL_HEADER64*          pOptHeader;
	IMAGE_IMPORT_DESCRIPTOR*          pImportDesc;
	IMAGE_THUNK_DATA*                 pThunk;
	PULONG_PTR                        lpAddr;
	DWORD                             dwOldProtect;

	pDosHeader = (IMAGE_DOS_HEADER*)hMod;
	pOptHeader = (IMAGE_OPTIONAL_HEADER64 *)((BYTE*)hMod + pDosHeader->e_lfanew + 24);
	pImportDesc = (IMAGE_IMPORT_DESCRIPTOR*)((BYTE*)hMod + pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	while (pImportDesc->FirstThunk)
	{
		LPCSTR pszDllName = (LPCSTR)((PBYTE)hMod + pImportDesc->Name);

		if (lstrcmpiA(pszDllName, DllName) == 0)
			break;

		pImportDesc++;
	}

	if (pImportDesc->FirstThunk)
	{
		pThunk = (IMAGE_THUNK_DATA*)((BYTE*)hMod + pImportDesc->FirstThunk);

		while (pThunk->u1.Function)
		{
			lpAddr = (PULONG_PTR)&(pThunk->u1.Function);
			if (*lpAddr == (ULONG_PTR)orgProc)
			{
				VirtualProtect(lpAddr, sizeof(LPVOID), PAGE_EXECUTE_READWRITE, &dwOldProtect);
				*lpAddr = (ULONG_PTR)newProc;
				return TRUE;
			}
			pThunk++;
		}
	}
	return FALSE;
}

BOOL NTAPI EATPatch32(LPCSTR ModName, LPCSTR FunName, ULONG64 ProxyFunAddr)
{
	DWORD                         Addr;
	DWORD                         Index;
	HMODULE                       hMod;
	DWORD                         Protect;
	PIMAGE_DOS_HEADER             DosHeader;
	PIMAGE_OPTIONAL_HEADER        Opthdr;
	PIMAGE_EXPORT_DIRECTORY       Export;
	PULONG                        pAddressOfFunctions;
	PULONG                        pAddressOfNames;
	PUSHORT                       pAddressOfNameOrdinals;
	BOOL                          Result;

	hMod = GetModuleHandleA(ModName);
	DosHeader = (PIMAGE_DOS_HEADER)hMod;
	Opthdr = (PIMAGE_OPTIONAL_HEADER)((DWORD)hMod + DosHeader->e_lfanew + 24);
	Export = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)DosHeader + Opthdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	pAddressOfFunctions = (ULONG*)((BYTE*)hMod + Export->AddressOfFunctions);
	pAddressOfNames = (ULONG*)((BYTE*)hMod + Export->AddressOfNames);
	pAddressOfNameOrdinals = (USHORT*)((BYTE*)hMod + Export->AddressOfNameOrdinals);
	Result = FALSE;

	for (ULONG_PTR i = 0; i <Export->NumberOfNames; i++)
	{
		Index = pAddressOfNameOrdinals[i];
		LPCSTR pFuncName = (LPCSTR)((BYTE*)hMod + pAddressOfNames[i]);

		if (lstrcmpiA((LPCSTR)pFuncName, FunName) == 0)
		{
			Addr = pAddressOfFunctions[Index];
			VirtualProtect(&pAddressOfFunctions[Index], 0x1000, PAGE_READWRITE, &Protect);
			pAddressOfFunctions[Index] = (DWORD)ProxyFunAddr - (DWORD)hMod;
			Result = TRUE;
			break;
		}
	}
	return Result;
}

BOOL NTAPI EATPatch64(LPCSTR ModName, LPCSTR FunName, ULONG64 ProxyFunAddr)
{
	HANDLE                      hMod;
	PVOID                       BaseAddress = NULL;
	IMAGE_DOS_HEADER*           pDosHeader;
	IMAGE_OPTIONAL_HEADER64*    OptHdr;
	PIMAGE_EXPORT_DIRECTORY     Exports;
	USHORT                      Index;
	DWORD                       OldProtect;
	ULONG                       Addr;
	PUCHAR                      pFuncName;
	PULONG                      pAddressOfFunctions, pAddressOfNames;
	PUSHORT                     pAddressOfNameOrdinals;
	MODULEINFO                  ModuleInfo;
	BOOL                        Result;

	Result = FALSE;
	BaseAddress = GetModuleHandleA(ModName);

	RtlZeroMemory(&ModuleInfo, sizeof(ModuleInfo));
	GetModuleInformation(GetCurrentProcess(), (HMODULE)BaseAddress, &ModuleInfo, sizeof(MODULEINFO));
	VirtualProtect(BaseAddress, ModuleInfo.SizeOfImage, PAGE_EXECUTE_READWRITE, &OldProtect);

	hMod = BaseAddress;
	pDosHeader = (IMAGE_DOS_HEADER *)hMod;
	OptHdr = (IMAGE_OPTIONAL_HEADER64 *)((BYTE*)hMod + pDosHeader->e_lfanew + 24);
	Exports = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)pDosHeader + OptHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	pAddressOfFunctions = (ULONG*)((BYTE*)hMod + Exports->AddressOfFunctions);
	pAddressOfNames = (ULONG*)((BYTE*)hMod + Exports->AddressOfNames);
	pAddressOfNameOrdinals = (USHORT*)((BYTE*)hMod + Exports->AddressOfNameOrdinals);


	for (ULONG_PTR i = 0; i < Exports->NumberOfNames; i++)
	{
		Index = pAddressOfNameOrdinals[i];
		Addr = pAddressOfFunctions[Index];
		pFuncName = (PUCHAR)((BYTE*)hMod + pAddressOfNames[i]);
		Addr = pAddressOfFunctions[Index];

		if (!lstrcmpA((LPCSTR)pFuncName, FunName))
		{
			pAddressOfFunctions[Index] = (ULONG)((ULONG64)ProxyFunAddr - (ULONG64)hMod);
			Result = TRUE;
			break;
		}
	}
	return Result;
}


FORCEINLINE NTSTATUS PatchCode(LPVOID lpStartAddress, LPVOID lpCode, ULONG_PTR Size)
{
	NTSTATUS    Status;
	DWORD       OldProtect;

	auto BOOL_TO_NTSTATUS = [](NTSTATUS& nStatus, BOOL bVar)->NTSTATUS
	{
		nStatus = bVar ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
		return nStatus;
	};

	LOOP_ONCE
	{

		if (!lpStartAddress || !lpCode || !Size)
		{
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		if (!IsStatusSuccess(
			BOOL_TO_NTSTATUS(Status,
			VirtualProtect(lpStartAddress, Size, PAGE_EXECUTE_READWRITE, &OldProtect))))
			return Status;

		RtlCopyMemory(lpStartAddress, lpCode, Size);

		if (!IsStatusSuccess(
			BOOL_TO_NTSTATUS(Status,
			VirtualProtect(lpStartAddress, Size, OldProtect, &OldProtect))))
			return Status;
	}
	Status = STATUS_SUCCESS;

	return Status;
}


NTSTATUS NTAPI InlineRestoreMemory(PVOID Target)
{
	return MH_RemoveHook(Target) == 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

static BOOL g_InlineInited = FALSE;

NTSTATUS NTAPI InlinePatchMemory(PINLINE_PATCH_DATA Data, ULONG_PTR Size)
{
	NTSTATUS        Status;
	ULONG           MinStatus;

	MinStatus = MH_OK;
	if (!g_InlineInited)
	{
		MinStatus = MH_Initialize();
		g_InlineInited = TRUE;
	}

	if (!Data || !Size)
		return STATUS_INVALID_PARAMETER;

	auto MH_STATUS_TRANSLATION = [](NTSTATUS& Status, ULONG MinStatus)->LONG
	{
		switch (MinStatus)
		{
		case MH_UNKNOWN:
			Status = STATUS_UNSUCCESSFUL;
			break;

		case MH_OK:
			Status = STATUS_SUCCESS;
			break;

		case MH_ERROR_ALREADY_INITIALIZED:
			Status = STATUS_ALREADY_REGISTERED;
			break;

			//ok
		case MH_ERROR_NOT_INITIALIZED:
			Status = STATUS_REGISTRY_RECOVERED;
			break;

		case MH_ERROR_ALREADY_CREATED:
			Status = STATUS_ALREADY_COMMITTED;
			break;

		case MH_ERROR_NOT_CREATED:
		case MH_ERROR_MODULE_NOT_FOUND:
		case MH_ERROR_ENABLED:
		case MH_ERROR_DISABLED:
			Status = STATUS_UNSUCCESSFUL;
			break;


		case MH_ERROR_NOT_EXECUTABLE:
			Status = STATUS_ILLEGAL_INSTRUCTION;
			break;

		case MH_ERROR_UNSUPPORTED_FUNCTION:
			Status = STATUS_ILLEGAL_FUNCTION;
			break;

		case MH_ERROR_MEMORY_ALLOC:
			Status = STATUS_MEMORY_NOT_ALLOCATED;
			break;

		case MH_ERROR_MEMORY_PROTECT:
			Status = STATUS_ACCESS_DENIED;
			break;

		case MH_ERROR_FUNCTION_NOT_FOUND:
			Status = STATUS_ILLEGAL_FUNCTION;
			break;
		}
		return Status;
	};

	if (!IsStatusSuccess(MH_STATUS_TRANSLATION(Status, MinStatus)))
		return Status;


	for (ULONG_PTR i = 0; i < Size; i++)
	{
		if (!IsStatusSuccess(
			MH_STATUS_TRANSLATION(Status,
			MH_CreateHook(Data[i].pTarget, Data[i].pHook, Data[i].pDetour))))
			return Status;
	}

	if (!IsStatusSuccess(MH_STATUS_TRANSLATION(Status, MH_EnableHook(MH_ALL_HOOKS))))
	{
		Status = STATUS_INVALID_HANDLE;
		return Status;
	}

	return Status;
}

NTSTATUS NTAPI IATPatchMemory(PIAT_PATCH_DATA Data, ULONG_PTR Size)
{
	NTSTATUS        Status;

	if (!Data || !Size)
		return STATUS_INVALID_PARAMETER;

	auto BOOL_TO_NTSTATUS = [](NTSTATUS& nStatus, BOOL bVar)->NTSTATUS
	{
		nStatus = bVar ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
		return nStatus;
	};

	for (ULONG_PTR i = 0; i < Size; i++)
	{
		if (!IsStatusSuccess(
			BOOL_TO_NTSTATUS(
			Status,
#ifdef  _M_X64
			IATPatch64(
#else
			IATPatch32(
#endif
			Data[i].hModule,
			Data[i].DllName,
			Data[i].SourceFunction,
			Data[i].DestFunction))))
			return Status;
	}
	return Status;
}

NTSTATUS NTAPI EATPatchMemory(PEAT_PATCH_DATA Data, ULONG_PTR Size)
{
	NTSTATUS        Status;

	if (!Data || !Size)
		return STATUS_INVALID_PARAMETER;

	auto BOOL_TO_NTSTATUS = [](NTSTATUS& nStatus, BOOL bVar)->NTSTATUS
	{
		nStatus = bVar ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
		return nStatus;
	};

	for (ULONG_PTR i = 0; i < Size; i++)
	{
		if (!IsStatusSuccess(
			BOOL_TO_NTSTATUS(
			Status,
#ifdef  _M_X64
			EATPatch64(
#else
			EATPatch32(
#endif
			Data[i].ModName,
			Data[i].FunName,
			Data[i].ProxyFunAddr))))
			return Status;
	}
	return Status;
}


NTSTATUS NTAPI CodePatchMemory(PCODE_PATCH_DATA Data, ULONG_PTR Size)
{
	NTSTATUS  Status;

	if (!Data || !Size)
		return STATUS_INVALID_PARAMETER;

	auto GET_STATUS = [](NTSTATUS& nStatus, NTSTATUS ParamStatus)->NTSTATUS
	{
		nStatus = ParamStatus;
		return nStatus;
	};

	for (ULONG_PTR i = 0; i < Size; i++)
	{
		if (!IsStatusSuccess(GET_STATUS(Status, PatchCode(Data[i].lpAddress, Data[i].pCode, Data[i].CodeSize))))
			return Status;
	}

	return Status;
}

