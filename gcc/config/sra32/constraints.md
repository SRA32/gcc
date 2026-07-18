;; -*- mode: lisp -*-
;; Register and immediate constraints for the SRA32 target.
;;
;; Reminder of the letters already implicitly used above in the .md file:
;;   "r" = GENERAL_REGS (standard, built-in -- no need to redefine)
;;   "i" = immediate     (standard, built-in)
;;   "m" = memory        (standard, built-in)
;; The entries below only cover the target-specific ones.

;; Register class constraint

(define_register_constraint "y" "CC_REGS"
  "The condition-code (flags) register")

;; Immediate constraints

;; 'I' signed 12 bit immediate
;; (alui, load/store offset, rcr/wcr index).
(define_constraint "I"
  "A signed 12-bit immediate (-2048..2047)"
  (and (match_code "const_int")
       (match_test "sra32_imm12_p (ival)")))

;; 'L' shift amount, 0..31
(define_constraint "L"
  "A shift amount in 0..31"
  (and (match_code "const_int")
       (match_test "ival >= 0 && ival <= 31")))

;; Memory constraint

;; 'Q' memory operand restricted to base(+imm12) addressing.
(define_memory_constraint "Q"
  "A base register plus optional 12-bit signed displacement"
  (match_operand 0 "memory_operand"))