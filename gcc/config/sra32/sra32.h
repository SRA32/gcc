/* Target Definitions for sra32.
   Copyright (C) 2026 Free Software Foundation, Inc.
   Contributed by Astrido <https://github.com/asterd-og>

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#ifndef GCC_SRA32_H
#define GCC_SRA32_H

#undef  STARTFILE_SPEC
#define STARTFILE_SPEC "crt0%O%s %{msim:crti.o%s} %{!msim:crti-hw.o%s} crtbegin.o%s"

#undef  ENDFILE_SPEC
#define ENDFILE_SPEC "crtend.o%s crtn.o%s"

#undef LIB_SPEC
#define LIB_SPEC "%{!shared:%{!symbolic:-lc}} \
                   %{mcompress:--relax} \
                   %{msim:-Tsim.ld}"

#undef  LINK_SPEC
#define LINK_SPEC "%{h*} %{v:-V} \
                   %{static:-Bstatic} %{shared:-shared} %{symbolic:-Bsymbolic}"

#define REGISTER_NAMES {          \
  "$x0", "$rp", "$sp", "$fp",     \
	"$r0", "$r1", "$r2", "$r3",     \
	"$r4", "$r5", "$r6", "$r7",     \
	"$r8", "$r9", "$r10", "$r11",   \
	"$r12", "$r13", "$r14", "$r15", \
	"$r16", "$r17", "$r18", "$r19", \
	"$r20", "$r21", "$r22", "$r23", \
	"$r24", "$r25", "$r26", "$r27", \
	"?cc", "?ap" \
}

#define X0_REGNUM 0
#define RP_REGNUM 1
#define SP_REGNUM 2
#define FP_REGNUM 3
#define R0_REGNUM 4
#define R15_REGNUM (R0_REGNUM+15)
#define R27_REGNUM (R0_REGNUM+27)
#define CC_REGNUM 32
#define AP_REGNUM 33
#define FIRST_PSEUDO_REGISTER 34

#define STACK_POINTER_REGNUM SP_REGNUM
#define FRAME_POINTER_REGNUM FP_REGNUM
#define HARD_FRAME_POINTER_REGNUM FP_REGNUM
#define ARG_POINTER_REGNUM AP_REGNUM

// Only x0, rp, sp, fp, and CC are fixed
#define FIXED_REGISTERS { 1, 1, 1, 1, \
                          0, 0, 0, 0, \
                          0, 0, 0, 0, \
                          0, 0, 0, 0, \
                          0, 0, 0, 0, \
                          0, 0, 0, 0, \
                          0, 0, 0, 0, \
                          0, 0, 0, 0, 1, 1 }

// x0-fp, r15-r27 are caller-saved
#define CALL_USED_REGISTERS { 1, 1, 1, 1, \
			      1, 1, 1, 1, \
			      1, 1, 1, 1, \
			      1, 1, 1, 1, \
			      1, 1, 1, 0, \
			      0, 0, 0, 0, \
			      0, 0, 0, 0, \
			      0, 0, 0, 0, 1, 1 }

enum reg_class
{
  NO_REGS,
  GENERAL_REGS,
  CC_REGS,
  ALL_REGS,
  LIM_REG_CLASSES
};

#define N_REG_CLASSES ((int) LIM_REG_CLASSES)

#define REG_CLASS_CONTENTS                                            \
  {                                                                    \
    { 0x000000000ULL },                       /* NO_REGS */           \
    { (1ULL << AP_REGNUM) | (((1ULL << 32) - 1) & ~(1ULL << X0_REGNUM)) }, \
    { (1ULL << CC_REGNUM) },                       /* CC_REG */    \
    { ((1ULL << FIRST_PSEUDO_REGISTER) - 1) & ~(1ULL << X0_REGNUM) } /* All regs except x0 */ \
  }

#define REG_CLASS_NAMES {\
    "NO_REGS", \
    "GENERAL_REGS", \
    "CC_REGS", \
    "ALL_REGS" }

/* We can't copy to or from our CC register. */
#define AVOID_CCMODE_COPIES 1

#define REGNO_REG_CLASS(REGNO) ((REGNO) == CC_REGNUM ? CC_REGS : GENERAL_REGS)

#define BASE_REG_CLASS GENERAL_REGS

#define INDEX_REG_CLASS NO_REGS

/* Data types */
#define INT_TYPE_SIZE 32
#define SHORT_TYPE_SIZE 16
#define LONG_TYPE_SIZE 32
#define LONG_LONG_TYPE_SIZE 64

#define FLOAT_TYPE_SIZE 32
#define DOUBLE_TYPE_SIZE 64
#define LONG_DOUBLE_TYPE_SIZE 64

#define DEFAULT_SIGNED_CHAR 1

#undef  SIZE_TYPE
#define SIZE_TYPE "unsigned int"

#undef  PTRDIFF_TYPE
#define PTRDIFF_TYPE "int"

#undef  WCHAR_TYPE
#define WCHAR_TYPE "long int"

#undef  WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE BITS_PER_WORD

/* The Overall Framework of an Assembler File */

#undef  ASM_SPEC
#define ASM_COMMENT_START "#"
#define ASM_APP_ON ""
#define ASM_APP_OFF ""
#define LOCAL_LABEL_PREFIX	".L"

#define FILE_ASM_OP     "\t.file\n"

#define TEXT_SECTION_ASM_OP   "\t.text"
#define DATA_SECTION_ASM_OP   "\t.data"
#define RODATA_SECTION_ASM_OP "\t.rodata"
#define BSS_SECTION_ASM_OP    "\t.bss"

#define ASM_OUTPUT_REG_PUSH(STREAM, REGNO) \
  fprintf (STREAM, "\tpush\t%d\n", REGNO)

#define ASM_OUTPUT_REG_POP(STREAM, REGNO) \
  fprintf (STREAM, "\tpop\t%d\n", REGNO)

#define ASM_OUTPUT_ALIGN(STREAM,POWER) \
  fprintf (STREAM, "\t.align\t%d\n", POWER);

#define GLOBAL_ASM_OP "\t.global\t"

#define ASM_OUTPUT_ADDR_VEC_ELT(FILE, VALUE)  \
    fprintf (FILE, "\t.long\t%d\n", VALUE)				\

#define CUMULATIVE_ARGS unsigned int

#define ACCUMULATE_OUTGOING_ARGS 0

#define INIT_CUMULATIVE_ARGS(CUM,FNTYPE,LIBNAME,FNDECL,N_NAMED_ARGS) \
  (CUM = 0)

/* STACK AND CALLING */

#define PUSH_ARGS 1

#define PUSH_ROUNDING(BYTES)  (BYTES) 

#define STACK_GROWS_DOWNWARD 1

#define FIRST_PARM_OFFSET(F) 0

#define FRAME_GROWS_DOWNWARD 1

#define EXIT_IGNORE_STACK 0

#define EPILOGUE_USES(R) (R == RP_REGNUM)

#define INCOMING_RETURN_ADDR_RTX gen_rtx_REG (Pmode, RP_REGNUM)

// Is this fine?
#define RETURN_ADDR_RTX(COUNT, FRAMEADDR) \
  sra32_return_addr_rtx ((COUNT), (FRAMEADDR))

/* Storage Layout */

#define BITS_BIG_ENDIAN 0
#define BYTES_BIG_ENDIAN 0
#define WORDS_BIG_ENDIAN 0

#define BRANCH_COST(speed_p, predictable_p)     2

/* Define this macro as a C expression which is nonzero if accessing
   less than a word of memory (i.e. a `char' or a `short') is no
   faster than accessing a word of memory.  */
#define SLOW_BYTE_ACCESS 1

#define STORE_FLAG_VALUE                        1

#define BITS_PER_UNIT 8
#define BITS_PER_WORD 32
#define UNITS_PER_WORD 4
#define POINTER_SIZE 32
#define Pmode SImode
#define FUNCTION_MODE SImode
/* Stack pointer boundary in bits. */
#define STACK_BOUNDARY 32
/* All arguments on the stack should have this alignment. */
#define PARM_BOUNDARY 32
/* Alignment required for a function entry point. */
#define FUNCTION_BOUNDARY 32
/* Alignment of field after `int : 0' in a structure. */
#define EMPTY_FIELD_BOUNDARY  32
/* No data type wants to be aligned rounder than this. */
#define BIGGEST_ALIGNMENT 32
/* The best alignment to use in cases where we have a choice. */
#define FASTEST_ALIGNMENT 32
#define STRICT_ALIGNMENT 1

#define DATA_ALIGNMENT(TYPE, ALIGN)             \
  (((ALIGN < BITS_PER_WORD) && \
  (TREE_CODE (TYPE) == ARRAY_TYPE \
  || TREE_CODE (TYPE) == UNION_TYPE \
  || TREE_CODE (TYPE) == RECORD_TYPE)) ? BITS_PER_WORD : (ALIGN))

#define STRUCTURE_SIZE_BOUNDARY 8

#define FUNCTION_PROFILER(file, labelno) \
  fprintf (file, "/* profiler %d */\n", (labelno))

/* Trampolines for Nested Functions. TODO  */
#define TRAMPOLINE_SIZE 24

/* Alignment required for trampolines, in bits.  */
#define TRAMPOLINE_ALIGNMENT 32

#define PROMOTE_MODE(MODE, UNSIGNEDP, TYPE) \
do { \
  if (((MODE) == HImode) \
   || ((MODE) == QImode)) \
       (MODE) = SImode;  \
} while (0)

#define ELIMINABLE_REGS \
  {{ ARG_POINTER_REGNUM,   STACK_POINTER_REGNUM }, \
   { ARG_POINTER_REGNUM,   HARD_FRAME_POINTER_REGNUM }, \
   { FRAME_POINTER_REGNUM, STACK_POINTER_REGNUM }, \
   { FRAME_POINTER_REGNUM, HARD_FRAME_POINTER_REGNUM }}

#define INITIAL_ELIMINATION_OFFSET(FROM, TO, OFFSET) \
  ((OFFSET) = sra32_initial_elimination_offset ((FROM), (TO)))

/* Registers R15-R27 can be used for arguments when calling. */
#define FUNCTION_ARG_REGNO_P(r) (r >= R15_REGNUM && r <= R27_REGNUM)

#define HARD_REGNO_OK_FOR_BASE_P(REGNO) \
  ((unsigned) (REGNO) < FIRST_PSEUDO_REGISTER \
   && (REGNO) != CC_REGNUM \
   && (REGNO) != X0_REGNUM)

/* A C expression which is nonzero if register number NUM is suitable
   for use as a base register in operand addresses.  */
#ifdef REG_OK_STRICT
#define REGNO_OK_FOR_BASE_P(NUM)                                       \
  (((unsigned) (NUM) < FIRST_PSEUDO_REGISTER                           \
    && HARD_REGNO_OK_FOR_BASE_P (NUM))                                 \
   || ((unsigned) (NUM) >= FIRST_PSEUDO_REGISTER                       \
       && reg_renumber[(NUM)] >= 0                                     \
       && HARD_REGNO_OK_FOR_BASE_P (reg_renumber[(NUM)])))
#else
#define REGNO_OK_FOR_BASE_P(NUM)                 \
  ((NUM) >= FIRST_PSEUDO_REGISTER || HARD_REGNO_OK_FOR_BASE_P (NUM))
#endif

/* A C expression which is nonzero if register number NUM is suitable
   for use as an index register in operand addresses.  */
#define REGNO_OK_FOR_INDEX_P(NUM) 0 // You cannot do lw $r0, $r1($r2)

#define MOVE_MAX 4

#define SHIFT_COUNT_TRUNCATED 1

#define LOAD_EXTEND_OP(MEM) ZERO_EXTEND

#define MAX_REGS_PER_ADDRESS 1

#define CASE_VECTOR_MODE SImode

/* Run-time Target Specification */

#define TARGET_CPU_CPP_BUILTINS() \
  { \
    builtin_define ("__SRA32__");          \
  }

#define HAS_LONG_UNCOND_BRANCH true

#define NO_FUNCTION_CSE 1

#endif /* GCC_SRA32_H */
