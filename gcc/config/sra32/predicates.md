;; -*- mode: lisp -*-
;; Predicates for the SRA32 target.

(define_predicate "sra32_imm12_operand"
  (match_code "const_int")
{
  return sra32_imm12_p (INTVAL (op));
})

(define_predicate "sra32_arith_operand"
  (ior (match_operand 0 "register_operand")
       (match_operand 0 "sra32_imm12_operand")))

(define_predicate "sra32_comparison_operator"
  (match_code "eq,ne,lt,gt,le,ge,ltu,gtu,leu,geu"))

(define_predicate "symbol_ref_operand"
  (match_code "symbol_ref"))
