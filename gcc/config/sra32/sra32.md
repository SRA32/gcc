;; -*- mode: lisp -*-
;; Machine description for the SRA32 architecture.
;; Companion files: sra32.cc, sra32.h, sra32-protos.h, predicates.md, constraints.md

(include "predicates.md")
(include "constraints.md")

;; UNSPECs

(define_c_enum "unspec" [
  UNSPEC_RFL       ; read flags register (rfl)
  UNSPEC_WFL       ; write flags register (wfl)
  UNSPEC_RCR       ; read control register (rcr)
  UNSPEC_WCR       ; write control register (wcr)
])

(define_c_enum "unspecv" [
  UNSPECV_FENCE    ; fence instruction
  UNSPECV_SYSC     ; syscall instruction
  UNSPECV_IRET     ; interrupt return
])

(define_constants
  [(X0_REGNUM   0)
   (RP_REGNUM   1)
   (SP_REGNUM   2)
   (FP_REGNUM   3)
   (CC_REGNUM   32)
  ])

;; Attributes

;; Every insn is one 32-bit word, no compressed/variable-length forms.
(define_attr "length" "" (const_int 4))

;; Nop

(define_insn "nop"
  [(const_int 0)]
  ""
  "nop")

;; Moves

(define_expand "movsi"
  [(set (match_operand:SI 0 "nonimmediate_operand")
        (match_operand:SI 1 "general_operand"))]
  ""
{
  if (!register_operand (operands[0], SImode)
      && !register_operand (operands[1], SImode))
    operands[1] = force_reg (SImode, operands[1]);
})

(define_insn "*movsi_internal"
  [(set (match_operand:SI 0 "nonimmediate_operand" "=r,r,r,r,m")
        (match_operand:SI 1 "general_operand"       "r,I,i,m,r"))]
  "register_operand (operands[0], SImode) || register_operand (operands[1], SImode)"
  "@
   mov\t%0, %1
   addi\t%0, $x0, %1
   li\t%0, %1
   lw\t%0, %1
   sw\t%1, %0")

(define_expand "movhi"
  [(set (match_operand:HI 0 "nonimmediate_operand")
        (match_operand:HI 1 "general_operand"))]
  ""
{
  if (MEM_P (operands[0]) && MEM_P (operands[1]))
    operands[1] = force_reg (HImode, operands[1]);
})

(define_insn "*movhi_internal"
  [(set (match_operand:HI 0 "nonimmediate_operand" "=r,r,r,r,m")
        (match_operand:HI 1 "general_operand"       "r,I,i,m,r"))]
  ""
  "@
   mov\t%0, %1
   addi\t%0, $x0, %1
   li\t%0, %1
   lh\t%0, %1
   sh\t%1, %0")

(define_expand "movqi"
  [(set (match_operand:QI 0 "nonimmediate_operand")
        (match_operand:QI 1 "general_operand"))]
  ""
{
  if (MEM_P (operands[0]) && MEM_P (operands[1]))
    operands[1] = force_reg (QImode, operands[1]);
})

(define_insn "*movqi_internal"
  [(set (match_operand:QI 0 "nonimmediate_operand" "=r,r,r,r,m")
        (match_operand:QI 1 "general_operand"       "r,I,i,m,r"))]
  ""
  "@
   mov\t%0, %1
   addi\t%0, $x0, %1
   li\t%0, %1
   lb\t%0, %1
   sb\t%1, %0")

;; Byte/half sign/zero-extend

(define_insn "extendqisi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (sign_extend:SI (match_operand:QI 1 "register_operand" "r")))]
  ""
  "seb\t%0, %1")

(define_insn "extendhisi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (sign_extend:SI (match_operand:HI 1 "register_operand" "r")))]
  ""
  "seh\t%0, %1")

;; ASSUMPTION: lb/lh zero-extend on load (per prior confirmation).
(define_expand "zero_extendqisi2"
  [(set (match_operand:SI 0 "register_operand")
        (zero_extend:SI (match_operand:QI 1 "general_operand")))]
  ""
{
  if (MEM_P (operands[1]))
    {
      emit_insn (gen_zero_extendqisi2_mem (operands[0], operands[1]));
      DONE;
    }
  rtx tmp = gen_reg_rtx (SImode);
  emit_move_insn (tmp, gen_lowpart (SImode, operands[1]));
  emit_insn (gen_andsi3 (operands[0], tmp, GEN_INT (0xff)));
  DONE;
})

(define_insn "zero_extendqisi2_mem"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (zero_extend:SI (match_operand:QI 1 "memory_operand" "m")))]
  ""
  "lb\t%0, %1")

(define_expand "zero_extendhisi2"
  [(set (match_operand:SI 0 "register_operand")
        (zero_extend:SI (match_operand:HI 1 "general_operand")))]
  ""
{
  if (MEM_P (operands[1]))
    {
      emit_insn (gen_zero_extendhisi2_mem (operands[0], operands[1]));
      DONE;
    }
  rtx tmp = gen_reg_rtx (SImode);
  emit_move_insn (tmp, gen_lowpart (SImode, operands[1]));
  emit_insn (gen_andsi3 (operands[0], tmp, GEN_INT (0xffff)));
  DONE;
})

(define_insn "zero_extendhisi2_mem"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (zero_extend:SI (match_operand:HI 1 "memory_operand" "m")))]
  ""
  "lh\t%0, %1")

;; Stack push/pop

(define_insn "movsi_push"
  [(set (mem:SI (pre_dec:SI (reg:SI SP_REGNUM)))
        (match_operand:SI 0 "register_operand" "r"))]
  ""
  "push\t%0")

(define_insn "movsi_pop"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (mem:SI (post_inc:SI (reg:SI SP_REGNUM))))]
  ""
  "pop\t%0")

;; Arith logic

(define_code_iterator ARITH_OP [plus mult and ior xor ashift lshiftrt ashiftrt])

;; Split between optab and mnemonic to satisfy what GCC wants.
(define_code_attr arith_optab [(plus "add") (mult "mul") (and "and") (ior "ior")
                                (xor "xor") (ashift "ashl") (lshiftrt "lshr") (ashiftrt "ashr")])

(define_code_attr arith_mnem [(plus "add") (mult "mul") (and "and") (ior "or")
                               (xor "xor") (ashift "shl") (lshiftrt "shr") (ashiftrt "sar")])

(define_expand "<arith_optab>si3"
  [(set (match_operand:SI 0 "register_operand")
        (ARITH_OP:SI (match_operand:SI 1 "register_operand")
                      (match_operand:SI 2 "general_operand")))]
  ""
{
  if (register_operand (operands[2], SImode))
    emit_insn (gen_<arith_optab>si3_reg (operands[0], operands[1], operands[2]));
  else if (CONST_INT_P (operands[2]) && sra32_imm12_p (INTVAL (operands[2])))
    emit_insn (gen_<arith_optab>si3_imm (operands[0], operands[1], operands[2]));
  else
    emit_insn (gen_<arith_optab>si3_reg (operands[0], operands[1], force_reg (SImode, operands[2])));
  DONE;
})

(define_insn "<arith_optab>si3_reg"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (ARITH_OP:SI (match_operand:SI 1 "register_operand" "r")
                      (match_operand:SI 2 "register_operand" "r")))]
  ""
  "<arith_mnem>\t%0, %1, %2")

(define_insn "<arith_optab>si3_imm"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (ARITH_OP:SI (match_operand:SI 1 "register_operand" "r")
                      (match_operand:SI 2 "sra32_imm12_operand" "i")))]
  ""
  "<arith_mnem>i\t%0, %1, %2")

;; Handle const - reg
(define_expand "subsi3"
  [(set (match_operand:SI 0 "register_operand")
        (minus:SI (match_operand:SI 1 "general_operand")
                   (match_operand:SI 2 "general_operand")))]
  ""
{
  if (!register_operand (operands[1], SImode))
    operands[1] = force_reg (SImode, operands[1]);
  if (register_operand (operands[2], SImode))
    emit_insn (gen_subsi3_reg (operands[0], operands[1], operands[2]));
  else if (CONST_INT_P (operands[2]) && sra32_imm12_p (INTVAL (operands[2])))
    emit_insn (gen_subsi3_imm (operands[0], operands[1], operands[2]));
  else
    emit_insn (gen_subsi3_reg (operands[0], operands[1], force_reg (SImode, operands[2])));
  DONE;
})

(define_insn "subsi3_reg"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (minus:SI (match_operand:SI 1 "register_operand" "r")
                   (match_operand:SI 2 "register_operand" "r")))]
  ""
  "sub\t%0, %1, %2")

(define_insn "subsi3_imm"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (minus:SI (match_operand:SI 1 "register_operand" "r")
                   (match_operand:SI 2 "sra32_imm12_operand" "i")))]
  ""
  "subi\t%0, %1, %2")

(define_insn "divsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (div:SI (match_operand:SI 1 "register_operand" "r")
                 (match_operand:SI 2 "register_operand" "r")))]
  ""
  "div\t%0, %1, %2")

(define_insn "udivsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (udiv:SI (match_operand:SI 1 "register_operand" "r")
                  (match_operand:SI 2 "register_operand" "r")))]
  ""
  "udiv\t%0, %1, %2")

(define_insn "modsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (mod:SI (match_operand:SI 1 "register_operand" "r")
                 (match_operand:SI 2 "register_operand" "r")))]
  ""
  "rem\t%0, %1, %2")

(define_insn "umodsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (umod:SI (match_operand:SI 1 "register_operand" "r")
                  (match_operand:SI 2 "register_operand" "r")))]
  ""
  "urem\t%0, %1, %2")

(define_insn "one_cmplsi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (not:SI (match_operand:SI 1 "register_operand" "r")))]
  ""
  "not\t%0, %1")

(define_insn "negsi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (neg:SI (match_operand:SI 1 "register_operand" "r")))]
  ""
  "sub\t%0, $x0, %1")

;; Comparisons / branches

(define_insn "cmpsi"
  [(set (reg:CC CC_REGNUM)
        (compare:CC (match_operand:SI 0 "register_operand" "r")
                     (match_operand:SI 1 "register_operand" "r")))]
  ""
  "cmp\t%0, %1")

(define_expand "cbranchsi4"
  [(set (pc)
        (if_then_else (match_operator 0 "sra32_comparison_operator"
                        [(match_operand:SI 1 "register_operand")
                         (match_operand:SI 2 "register_operand")])
                       (label_ref (match_operand 3))
                       (pc)))]
  ""
{
  sra32_expand_cbranch (operands);
  DONE;
})

(define_code_iterator BR_CODE [eq ne lt gt le ge ltu gtu leu geu])
(define_code_attr br_mnem [(eq "br.eq") (ne "br.ne") (lt "br.lt") (gt "br.gt")
                            (le "br.le") (ge "br.ge") (ltu "br.ltu") (gtu "br.gtu")
                            (leu "br.leu") (geu "br.geu")])

(define_insn "sra32_b<code>"
  [(set (pc)
        (if_then_else (BR_CODE (reg:CC CC_REGNUM) (const_int 0))
                       (label_ref (match_operand 0))
                       (pc)))]
  ""
  "<br_mnem>\t%l0")

(define_insn "jump"
  [(set (pc) (label_ref (match_operand 0)))]
  ""
  "br\t%l0")

(define_insn "indirect_jump"
  [(set (pc) (match_operand:SI 0 "register_operand" "r"))]
  ""
  "spc\t%0")

;; Conditional move (helps with unary conditions)

(define_expand "movsicc"
  [(set (match_operand:SI 0 "register_operand")
        (if_then_else:SI (match_operand 1 "sra32_comparison_operator")
                          (match_operand:SI 2 "register_operand")
                          (match_operand:SI 3 "register_operand")))]
  ""
{
  sra32_expand_movcc (operands);
  DONE;
})

(define_insn "sra32_cmov_<code>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (if_then_else:SI (BR_CODE (reg:CC CC_REGNUM) (const_int 0))
                          (match_operand:SI 1 "register_operand" "r")
                          (match_operand:SI 2 "register_operand" "0")))]
  ""
  "cmov.<code>\t%0, %1")

;; Return / prologue / epilogue

(define_expand "return"
  [(return)]
  ""
{
  sra32_expand_epilogue ();
  DONE;
})

(define_insn "sra32_return_internal"
  [(return)
   (use (reg:SI RP_REGNUM))]
  ""
  "spc\t$rp")

(define_expand "prologue"
  [(const_int 0)]
  ""
{ sra32_expand_prologue (); DONE; })

(define_expand "epilogue"
  [(const_int 0)]
  ""
{ sra32_expand_epilogue (); DONE; })

(define_expand "call"
  [(call (match_operand:SI 0 "memory_operand")
         (match_operand 1 "general_operand"))]
  ""
{
  rtx addr = XEXP (operands[0], 0);
  if (GET_CODE (addr) == SYMBOL_REF || GET_CODE (addr) == LABEL_REF)
    emit_call_insn (gen_call_direct (addr, operands[1]));
  else
    emit_call_insn (gen_call_indirect (force_reg (SImode, addr), operands[1]));
  DONE;
})

(define_insn "call_direct"
  [(call (mem:SI (match_operand:SI 0 "symbol_ref_operand" "i")) (match_operand 1))
   (clobber (reg:SI RP_REGNUM))]
  ""
  "call\t%0")

(define_insn "call_indirect"
  [(call (mem:SI (match_operand:SI 0 "register_operand" "r")) (match_operand 1))
   (clobber (reg:SI RP_REGNUM))]
  ""
  "calr\t%0")

(define_expand "call_value"
  [(set (match_operand 0)
        (call (match_operand:SI 1 "memory_operand")
              (match_operand 2 "general_operand")))]
  ""
{
  rtx addr = XEXP (operands[1], 0);
  if (GET_CODE (addr) == SYMBOL_REF || GET_CODE (addr) == LABEL_REF)
    emit_call_insn (gen_call_value_direct (operands[0], addr, operands[2]));
  else
    emit_call_insn (gen_call_value_indirect (operands[0], force_reg (SImode, addr), operands[2]));
  DONE;
})

(define_insn "call_value_direct"
  [(set (match_operand 0 "register_operand" "=r")
        (call (mem:SI (match_operand:SI 1 "symbol_ref_operand" "i")) (match_operand 2)))
   (clobber (reg:SI RP_REGNUM))]
  ""
  "call\t%1")

(define_insn "call_value_indirect"
  [(set (match_operand 0 "register_operand" "=r")
        (call (mem:SI (match_operand:SI 1 "register_operand" "r")) (match_operand 2)))
   (clobber (reg:SI RP_REGNUM))]
  ""
  "calr\t%1")
