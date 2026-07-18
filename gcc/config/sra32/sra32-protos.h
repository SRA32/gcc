/* sra32-protos.h */
#ifndef GCC_SRA32_PROTOS_H
#define GCC_SRA32_PROTOS_H

/* Frame / prologue-epilogue */
extern void sra32_expand_prologue (void);
extern void sra32_expand_epilogue (void);
extern int  sra32_initial_elimination_offset (int, int);

extern bool sra32_imm12_p (HOST_WIDE_INT);

#ifdef RTX_CODE
extern rtx sra32_return_addr_rtx (int, rtx);
extern void sra32_expand_cbranch (rtx *);
extern void sra32_expand_movcc (rtx *);
#endif /* RTX_CODE */

#ifdef TREE_CODE
#endif /* TREE_CODE */

#endif /* GCC_SRA32_PROTOS_H */