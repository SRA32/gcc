#define IN_TARGET_CODE 1

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "target.h"
#include "rtl.h"
#include "tree.h"
#include "stringpool.h"
#include "attribs.h"
#include "cfghooks.h"
#include "df.h"
#include "memmodel.h"
#include "tm_p.h"
#include "regs.h"
#include "emit-rtl.h"
#include "diagnostic-core.h"
#include "varasm.h"
#include "stor-layout.h"
#include "calls.h"
#include "output.h"
#include "flags.h"
#include "explow.h"
#include "expr.h"
#include "toplev.h"
#include "langhooks.h"
#include "opts.h"
#include "builtins.h"

/* This file should be included last.  */
#include "target-def.h"

#include <stdint.h>

#define LOSE_AND_RETURN(msgid, x)		\
  do						\
    {						\
      sra32_operand_lossage (msgid, x);		\
      return;					\
    } while (0)

#define SRA32_MAX_IMM12  2047
#define TMP_REGNUM R27_REGNUM

/* Worker function for TARGET_CAN_ELIMINATE.  */

static bool
sra32_can_eliminate (const int from ATTRIBUTE_UNUSED, const int to)
{
  return (to == STACK_POINTER_REGNUM) ? !frame_pointer_needed : true;
}

/* Worker function for TARGET_RETURN_IN_MEMORY.  */

static bool
sra32_return_in_memory (const_tree type, const_tree fntype ATTRIBUTE_UNUSED)
{
  const HOST_WIDE_INT size = int_size_in_bytes (type);
  return (size == -1 || size > 2 * UNITS_PER_WORD);
}

static rtx
sra32_function_value (const_tree valtype,
		      const_tree fntype_or_decl ATTRIBUTE_UNUSED,
		      bool outgoing ATTRIBUTE_UNUSED)
{
  return gen_rtx_REG (TYPE_MODE (valtype), R0_REGNUM);
}

static rtx
sra32_libcall_value (machine_mode mode,
		     const_rtx fun ATTRIBUTE_UNUSED)
{
  return gen_rtx_REG (mode, R0_REGNUM);
}

static bool
sra32_function_value_regno_p (const unsigned int regno)
{
  return (regno == R0_REGNUM);
}

static void
sra32_operand_lossage (const char *msgid, rtx op)
{
  debug_rtx (op);
  output_operand_lossage ("%s", msgid);
}

/* The PRINT_OPERAND_ADDRESS worker.  */

static void
sra32_print_operand_address (FILE *file, machine_mode, rtx x)
{
  switch (GET_CODE (x))
    {
    case REG:
      fprintf (file, "0(%s)", reg_names[REGNO (x)]);
      break;

    case PLUS:
			if (CONST_INT_P (XEXP (x, 0)) && REG_P (XEXP (x, 1)))
				{
					// Imm(T)
					fprintf (file, "%ld(%s)", (long) INTVAL (XEXP (x, 0)), reg_names[REGNO (XEXP (x, 1))]);
				}
			else if (REG_P (XEXP (x, 0)) && CONST_INT_P (XEXP (x, 1)))
				{
					// Imm(T) but reverse order
					fprintf (file, "%ld(%s)", (long) INTVAL (XEXP (x, 1)), reg_names[REGNO (XEXP (x, 0))]);
				}
			else
				output_operand_lossage ("invalid address");
			break;
    default:
      output_addr_const (file, x);
      break;
    }
}

/* The PRINT_OPERAND worker.  */

static void
sra32_print_operand (FILE *file, rtx x, int code)
{
  rtx operand = x;

  /* Print an operand as without a modifier letter.  */
  switch (GET_CODE (operand))
    {
    case REG:
      if (REGNO (operand) >= FIRST_PSEUDO_REGISTER)
	internal_error ("internal error: bad register: %d", REGNO (operand));
      fprintf (file, "%s", reg_names[REGNO (operand)]);
      return;

    case MEM:
      output_address (GET_MODE (XEXP (operand, 0)), XEXP (operand, 0));
      return;

    default:
      if (CONSTANT_P (operand))
	{
	  output_addr_const (file, operand);
	  return;
	}

      LOSE_AND_RETURN ("unexpected operand", x);
    }
}

/* Per-function machine data.  */
struct GTY(()) machine_function
{
   int callee_saved_reg_size;

   int local_vars_size;

   int size_for_adjusting_sp;

	 bool rp_needed;
	 bool fp_needed;
};

bool
sra32_imm12_p (HOST_WIDE_INT val)
{
  return val >= -2048 && val <= 2047;
}

/* Zero initialization is OK for all current fields.  */

static struct machine_function *
sra32_init_machine_status (void)
{
  return ggc_cleared_alloc<machine_function> ();
}

/* The TARGET_OPTION_OVERRIDE worker.  */
static void
sra32_option_override (void)
{
  /* Set the per-function-data initializer.  */
  init_machine_status = sra32_init_machine_status;
}

static bool
sra32_rp_needed (void)
{
	/* Non-leaf functions clobber RP when they call a function, so they must save/restor their own RP */
	return !crtl->is_leaf || df_regs_ever_live_p (RP_REGNUM);
}

/* Compute the size of the local area and the size to be adjusted by the
 * prologue and epilogue.  */

static void
sra32_compute_frame (void)
{
  /* For aligning the local variables.  */
  int stack_alignment = STACK_BOUNDARY / BITS_PER_UNIT;
  int padding_locals;
  int regno;

  /* Padding needed for each element of the frame.  */
  cfun->machine->local_vars_size = get_frame_size ();

  /* Align to the stack alignment.  */
  padding_locals = cfun->machine->local_vars_size % stack_alignment;
  if (padding_locals)
    padding_locals = stack_alignment - padding_locals;

  cfun->machine->local_vars_size += padding_locals;

  cfun->machine->callee_saved_reg_size = 0;

  /* Save callee-saved registers.  */
  for (regno = 0; regno < FIRST_PSEUDO_REGISTER; regno++) {
		if (regno == RP_REGNUM || regno == FP_REGNUM)
			continue; // Handled explicitly later

    if (df_regs_ever_live_p (regno) && (! call_used_or_fixed_reg_p (regno)))
      cfun->machine->callee_saved_reg_size += 4;
	}

	cfun->machine->rp_needed = sra32_rp_needed();
	cfun->machine->fp_needed = frame_pointer_needed;

  cfun->machine->size_for_adjusting_sp =
    crtl->args.pretend_args_size
    + cfun->machine->local_vars_size
    + (ACCUMULATE_OUTGOING_ARGS
       ? (HOST_WIDE_INT) crtl->outgoing_args_size : 0);
}

static void
sra32_adjust_sp (HOST_WIDE_INT delta, bool frame_related)
{
	rtx insn;
	HOST_WIDE_INT magnitude = (delta > 0) ? delta : -delta; // Delta > 0 shrinks the stack (prologue)
								// Delta < 0 grows it back (epilogue)

	if (magnitude == 0) return;

	if (magnitude > 3 * SRA32_MAX_IMM12)
		{
			rtx tmp = gen_rtx_REG (SImode, TMP_REGNUM);
			insn = emit_move_insn (tmp, GEN_INT (magnitude));
			if (frame_related) RTX_FRAME_RELATED_P (insn) = 1;

			insn = emit_insn (delta > 0
					   ? gen_subsi3 (stack_pointer_rtx, stack_pointer_rtx, tmp)
												 : gen_addsi3 (stack_pointer_rtx, stack_pointer_rtx, tmp));
			if (frame_related) RTX_FRAME_RELATED_P (insn) = 1;
			return;
		}

	while (magnitude > 0)
		{
			HOST_WIDE_INT chunk = MIN (magnitude, SRA32_MAX_IMM12);
			insn = emit_insn (delta > 0
					   ? gen_subsi3 (stack_pointer_rtx, stack_pointer_rtx, GEN_INT (chunk))
												 : gen_addsi3 (stack_pointer_rtx, stack_pointer_rtx, GEN_INT (chunk)));
			if (frame_related) RTX_FRAME_RELATED_P (insn) = 1;
			magnitude -= chunk;
		}
}

void
sra32_expand_prologue (void)
{
  int regno;
  rtx insn;

  sra32_compute_frame ();

  if (flag_stack_usage_info)
    current_function_static_stack_size =
		  cfun->machine->size_for_adjusting_sp
			+ cfun->machine->callee_saved_reg_size
			+ (cfun->machine->rp_needed ? 4 : 0)
			+ (cfun->machine->fp_needed ? 4 : 0);

	// RP Must be saved before any other register.
	if (cfun->machine->rp_needed)
		{
			insn = emit_insn (gen_movsi_push (gen_rtx_REG (Pmode, RP_REGNUM)));
			RTX_FRAME_RELATED_P (insn) = 1;
		}

	// Old FP next.
	if (cfun->machine->fp_needed)
		{
			insn = emit_insn (gen_movsi_push (gen_rtx_REG (Pmode, FP_REGNUM)));
			RTX_FRAME_RELATED_P (insn) = 1;
		}

  /* Remaining callee-saved GPRs.  */
  for (regno = 0; regno < FIRST_PSEUDO_REGISTER; regno++)
    {
      if (regno == RP_REGNUM || regno == FP_REGNUM) continue;
			if (df_regs_ever_live_p (regno) && !call_used_or_fixed_reg_p (regno))
				{
					insn = emit_insn (gen_movsi_push (gen_rtx_REG (Pmode, regno)));
					RTX_FRAME_RELATED_P (insn) = 1;
				}
    }

  // Establish FP pointing just below the saved-register area.
	if (cfun->machine->fp_needed)
		{
			insn = emit_move_insn (frame_pointer_rtx, stack_pointer_rtx);
			RTX_FRAME_RELATED_P (insn) = 1;
		}

	// Locals + outgoing args.
	if (cfun->machine->size_for_adjusting_sp > 0)
		sra32_adjust_sp (cfun->machine->size_for_adjusting_sp, true);
}

void
sra32_expand_epilogue (void)
{
	int regno;
	rtx insn;

	sra32_compute_frame ();

	// If we have a FP, restoring SP from it is simpler and right even if SP was adjusted dynamically after
	// the prologue (eg alloca). Otherwise just undo the arithmetic.
	if (cfun->machine->fp_needed)
		emit_move_insn (stack_pointer_rtx, frame_pointer_rtx);
	else if (cfun->machine->size_for_adjusting_sp > 0)
		sra32_adjust_sp (-cfun->machine->size_for_adjusting_sp, true);

	// Pop GPRs in reverse push order.
	for (regno = FIRST_PSEUDO_REGISTER - 1; regno >= 0; regno--)
		{
			if (regno == RP_REGNUM || regno == FP_REGNUM)
				continue;
			if (df_regs_ever_live_p (regno) && !call_used_or_fixed_reg_p (regno))
				emit_insn (gen_movsi_pop (gen_rtx_REG (Pmode, regno)));
		}

	// FP was pushed after RP so pop in order
	if (cfun->machine->fp_needed)
		emit_insn (gen_movsi_pop (gen_rtx_REG (Pmode, FP_REGNUM)));

	if (cfun->machine->rp_needed)
		emit_insn (gen_movsi_pop (gen_rtx_REG (Pmode, RP_REGNUM)));

	emit_jump_insn (gen_sra32_return_internal ());
}

int
sra32_initial_elimination_offset (int from, int to)
{
	int saved_regs_size;

	sra32_compute_frame ();

	saved_regs_size = (cfun->machine->rp_needed ? 4 : 0)
			   + (cfun->machine->fp_needed ? 4 : 0)
										 + cfun->machine->callee_saved_reg_size;

	if (from == FRAME_POINTER_REGNUM && to == HARD_FRAME_POINTER_REGNUM)
		return 0; // Virtual FP sits right where hard FP is (right above locals).

	if (from == FRAME_POINTER_REGNUM && to == STACK_POINTER_REGNUM)
		return cfun->machine->size_for_adjusting_sp; // Used when fp_needed is false, so GCC eliminates to sp instead of hard FP.

	if (from == ARG_POINTER_REGNUM && to == HARD_FRAME_POINTER_REGNUM)
		return saved_regs_size;

	if (from == ARG_POINTER_REGNUM && to == STACK_POINTER_REGNUM)
		return saved_regs_size + cfun->machine->size_for_adjusting_sp;

	gcc_unreachable ();
}

static bool
sra32_fixed_condition_code_regs (unsigned int *p1, unsigned int *p2)
{
	*p1 = CC_REGNUM;
	*p2 = INVALID_REGNUM;
	return true;
}

static bool
sra32_legitimate_constant_p (machine_mode mode ATTRIBUTE_UNUSED, rtx x ATTRIBUTE_UNUSED)
{
	// Every 32 bit constant is represented via li so nothing needs to be pooled
	return true;
}

static bool
sra32_reg_ok_for_base_p (const_rtx reg, bool strict_p)
{
  int regno = REGNO (reg);
  if (strict_p)
    return ((unsigned) regno < FIRST_PSEUDO_REGISTER && HARD_REGNO_OK_FOR_BASE_P (regno))
	    || ((unsigned) regno >= FIRST_PSEUDO_REGISTER
		&& reg_renumber[regno] >= 0
		&& HARD_REGNO_OK_FOR_BASE_P (reg_renumber[regno]));
  else
    return (unsigned) regno >= FIRST_PSEUDO_REGISTER || HARD_REGNO_OK_FOR_BASE_P (regno);
}

static bool
sra32_legitimate_address_p (machine_mode mode ATTRIBUTE_UNUSED, rtx x, bool strict_p, code_helper = ERROR_MARK)
{
  if (GET_CODE (x) == PLUS
      && CONST_INT_P (XEXP (x, 0))
      && REG_P (XEXP (x, 1))
      && sra32_reg_ok_for_base_p (XEXP (x, 1), strict_p)
      && IN_RANGE (INTVAL (XEXP (x, 0)), -2048, 2047))
    return true;

  if (GET_CODE (x) == PLUS
      && REG_P (XEXP (x, 0))
      && sra32_reg_ok_for_base_p (XEXP (x, 0), strict_p)
      && CONST_INT_P (XEXP (x, 1))
      && IN_RANGE (INTVAL (XEXP (x, 1)), -2048, 2047))
    return true;

  if (REG_P (x) && sra32_reg_ok_for_base_p (x, strict_p))
    return true;

  return false;
}

static rtx
sra32_legitimize_address (rtx x, rtx oldx ATTRIBUTE_UNUSED, machine_mode mode ATTRIBUTE_UNUSED)
{
  if (!sra32_legitimate_address_p (mode, x, false))
    x = force_reg (Pmode, x);

  return x;
}

static bool
sra32_hard_regno_mode_ok (unsigned int regno, machine_mode mode)
{
  if (regno == CC_REGNUM)
    return mode == CCmode;

  if (regno == AP_REGNUM)
    return mode == Pmode;

  if (regno == X0_REGNUM)
    return mode == SImode;

  return mode == SImode || mode == HImode || mode == QImode;
}

static unsigned int
sra32_hard_regno_nregs (unsigned int regno ATTRIBUTE_UNUSED, machine_mode mode)
{
  return CEIL (GET_MODE_SIZE (mode), UNITS_PER_WORD);
}

static bool
sra32_modes_tieable_p (machine_mode mode1, machine_mode mode2)
{
  if (mode1 == mode2)
    return true;

  // Other integer modes can coexist in SImod registers.
  return GET_MODE_SIZE (mode1) <= 4 && GET_MODE_SIZE (mode2) <= 4;
}

static bool
sra32_cannot_change_mode_class (machine_mode from ATTRIBUTE_UNUSED, machine_mode to ATTRIBUTE_UNUSED,
				enum reg_class rclass ATTRIBUTE_UNUSED)
{
  return false;
}

static HOST_WIDE_INT
sra32_starting_frame_offset (void)
{
  return 0;
}

static bool
sra32_frame_pointer_required (void)
{
  return false;
}

static bool
sra32_truly_noop_truncation (poly_uint64 outprec ATTRIBUTE_UNUSED, poly_uint64 inprec ATTRIBUTE_UNUSED)
{
  return true;
}

#define SRA32_NUM_ARG_REGS 12
#define SRA32_ARG_REG(N) (R15_REGNUM + (N))

#define SRA32_ARG_WORDS(MODE, TYPE) \
	(((MODE) != BLKmode \
		? GET_MODE_SIZE (MODE) : (unsigned) int_size_in_bytes (TYPE)) \
	 + UNITS_PER_WORD - 1) / UNITS_PER_WORD

static rtx
sra32_function_arg (cumulative_args_t cum_v, const function_arg_info &arg)
{
  CUMULATIVE_ARGS *cum = get_cumulative_args (cum_v);
  if (*cum >= SRA32_NUM_ARG_REGS)
    return NULL_RTX;
  return gen_rtx_REG (arg.mode, SRA32_ARG_REG (*cum));
}

static void
sra32_function_arg_advance (cumulative_args_t cum_v, const function_arg_info &arg)
{
  CUMULATIVE_ARGS *cum = get_cumulative_args (cum_v);
  *cum += SRA32_ARG_WORDS (arg.mode, arg.type);
}

static bool
sra32_pass_by_reference (cumulative_args_t, const function_arg_info &arg)
{
  if (arg.aggregate_type_p ())
    return true;

  unsigned HOST_WIDE_INT size = arg.type_size_in_bytes ();
  return size > (UNITS_PER_WORD * SRA32_NUM_ARG_REGS);
}

static int
sra32_arg_partial_bytes (cumulative_args_t cum_v, const function_arg_info &arg)
{
  CUMULATIVE_ARGS *cum = get_cumulative_args (cum_v);
  int bytes_left = 0, size = 0;

  if (*cum >= SRA32_NUM_ARG_REGS)
    return 0;

  if (sra32_pass_by_reference (cum_v, arg))
    size = UNITS_PER_WORD;
  else if (arg.type)
    {
      if (AGGREGATE_TYPE_P (arg.type))
	return 0;
      size = int_size_in_bytes (arg.type);
    }
  else
    size = GET_MODE_SIZE (arg.mode);

  bytes_left = (SRA32_NUM_ARG_REGS - *cum) * UNITS_PER_WORD;
  return (size > bytes_left) ? bytes_left : 0;
}

static void
sra32_setup_incoming_varargs (cumulative_args_t cum_v, const function_arg_info &,
			       int *pretend_size, int no_rtl)
{
  CUMULATIVE_ARGS *cum = get_cumulative_args (cum_v);
  int regno;
  int regs_left = SRA32_NUM_ARG_REGS - *cum;

  *pretend_size = (regs_left < 0) ? 0 : UNITS_PER_WORD * regs_left;
  if (no_rtl)
    return;

  for (regno = *cum; regno < SRA32_NUM_ARG_REGS; regno++)
    {
      rtx reg = gen_rtx_REG (SImode, SRA32_ARG_REG (regno));
      rtx addr = plus_constant (Pmode, arg_pointer_rtx, UNITS_PER_WORD * regno);
      rtx mem = gen_rtx_MEM (SImode, addr);
      set_mem_align (mem, BITS_PER_WORD);
      MEM_NOTRAP_P (mem) = 1;
      emit_move_insn (mem, reg);
    }
}

rtx
sra32_return_addr_rtx (int count, rtx frameaddr)
{
  if (count != 0)
    return NULL_RTX;

  sra32_compute_frame ();

  if (!cfun->machine->rp_needed)
    return gen_rtx_REG (Pmode, RP_REGNUM);

  if (cfun->machine->fp_needed)
    return gen_frame_mem (Pmode, plus_constant (Pmode, frame_pointer_rtx, 4));

  return gen_frame_mem (Pmode, plus_constant (Pmode, arg_pointer_rtx, -4));
}

/* Used in the MD file. */

void
sra32_expand_cbranch (rtx *operands)
{
  enum rtx_code code = GET_CODE (operands[0]);
  rtx op0 = operands[1];
  rtx op1 = operands[2];
  rtx label = operands[3];

  if (!register_operand (op0, SImode))
    op0 = force_reg (SImode, op0);
  if (!register_operand (op1, SImode))
    op1 = force_reg (SImode, op1);

  emit_insn (gen_cmpsi (op0, op1));

  switch (code)
    {
    case EQ: emit_jump_insn (gen_sra32_beq (label)); break;
    case NE: emit_jump_insn (gen_sra32_bne (label)); break;
    case LT: emit_jump_insn (gen_sra32_blt (label)); break;
    case GT: emit_jump_insn (gen_sra32_bgt (label)); break;
    case LE: emit_jump_insn (gen_sra32_ble (label)); break;
    case GE: emit_jump_insn (gen_sra32_bge (label)); break;
    case LTU: emit_jump_insn (gen_sra32_bltu (label)); break;
    case GTU: emit_jump_insn (gen_sra32_bgtu (label)); break;
    case LEU: emit_jump_insn (gen_sra32_bleu (label)); break;
    case GEU: emit_jump_insn (gen_sra32_bgeu (label)); break;
    default:
      gcc_unreachable ();
    }
}

void
sra32_expand_movcc (rtx *operands)
{
  rtx dest      = operands[0];
  rtx cmp       = operands[1];
  rtx true_val  = operands[2];
  rtx false_val = operands[3];
  enum rtx_code code = GET_CODE (cmp);
  rtx cmp_op0 = XEXP (cmp, 0);
  rtx cmp_op1 = XEXP (cmp, 1);

  if (!register_operand (true_val, SImode))
    true_val = force_reg (SImode, true_val);
  if (!register_operand (false_val, SImode))
    false_val = force_reg (SImode, false_val);
  if (!register_operand (cmp_op0, SImode))
    cmp_op0 = force_reg (SImode, cmp_op0);
  if (!register_operand (cmp_op1, SImode))
    cmp_op1 = force_reg (SImode, cmp_op1);

  emit_move_insn (dest, false_val);
  emit_insn (gen_cmpsi (cmp_op0, cmp_op1));

  switch (code)
    {
    case EQ: emit_insn (gen_sra32_cmov_eq (dest, true_val, dest)); break;
    case NE: emit_insn (gen_sra32_cmov_ne (dest, true_val, dest)); break;
    case LT: emit_insn (gen_sra32_cmov_lt (dest, true_val, dest)); break;
    case GT: emit_insn (gen_sra32_cmov_gt (dest, true_val, dest)); break;
    case LE: emit_insn (gen_sra32_cmov_le (dest, true_val, dest)); break;
    case GE: emit_insn (gen_sra32_cmov_ge (dest, true_val, dest)); break;
    case LTU: emit_insn (gen_sra32_cmov_ltu (dest, true_val, dest)); break;
    case GTU: emit_insn (gen_sra32_cmov_gtu (dest, true_val, dest)); break;
    case LEU: emit_insn (gen_sra32_cmov_leu (dest, true_val, dest)); break;
    case GEU: emit_insn (gen_sra32_cmov_geu (dest, true_val, dest)); break;
    default:
      gcc_unreachable ();
    }
}

#undef TARGET_CAN_ELIMINATE
#define TARGET_CAN_ELIMINATE sra32_can_eliminate
#undef TARGET_RETURN_IN_MEMORY
#define TARGET_RETURN_IN_MEMORY sra32_return_in_memory
#undef TARGET_FUNCTION_VALUE
#define TARGET_FUNCTION_VALUE sra32_function_value
#undef TARGET_LIBCALL_VALUE
#define TARGET_LIBCALL_VALUE sra32_libcall_value
#undef TARGET_FUNCTION_VALUE_REGNO_P
#define TARGET_FUNCTION_VALUE_REGNO_P sra32_function_value_regno_p
#undef TARGET_PRINT_OPERAND
#define TARGET_PRINT_OPERAND sra32_print_operand
#undef TARGET_PRINT_OPERAND_ADDRESS
#define TARGET_PRINT_OPERAND_ADDRESS sra32_print_operand_address
#undef TARGET_OPTION_OVERRIDE
#define TARGET_OPTION_OVERRIDE sra32_option_override
#undef TARGET_FIXED_CONDITION_CODE_REGS
#define TARGET_FIXED_CONDITION_CODE_REGS sra32_fixed_condition_code_regs
#undef TARGET_LEGITIMATE_CONSTANT_P
#define TARGET_LEGITIMATE_CONSTANT_P sra32_legitimate_constant_p
#undef TARGET_LEGITIMATE_ADDRESS_P
#define TARGET_LEGITIMATE_ADDRESS_P sra32_legitimate_address_p
#undef TARGET_LEGITIMIZE_ADDRESS
#define TARGET_LEGITIMIZE_ADDRESS sra32_legitimize_address
#undef TARGET_HARD_REGNO_MODE_OK
#define TARGET_HARD_REGNO_MODE_OK sra32_hard_regno_mode_ok
#undef TARGET_HARD_REGNO_NREGS
#define TARGET_HARD_REGNO_NREGS sra32_hard_regno_nregs
#undef TARGET_MODES_TIEABLE_P
#define TARGET_MODES_TIEABLE_P sra32_modes_tieable_p
#undef TARGET_CANNOT_CHANGE_MODE_CLASS
#define TARGET_CANNOT_CHANGE_MODE_CLASS sra32_cannot_change_mode_class
#undef TARGET_STARTING_FRAME_OFFSET
#define TARGET_STARTING_FRAME_OFFSET sra32_starting_frame_offset
#undef TARGET_FRAME_POINTER_REQUIRED
#define TARGET_FRAME_POINTER_REQUIRED sra32_frame_pointer_required
#undef TARGET_TRULY_NOOP_TRUNCATION
#define TARGET_TRULY_NOOP_TRUNCATION sra32_truly_noop_truncation
#undef TARGET_FUNCTION_ARG
#define TARGET_FUNCTION_ARG sra32_function_arg
#undef TARGET_FUNCTION_ARG_ADVANCE
#define TARGET_FUNCTION_ARG_ADVANCE sra32_function_arg_advance
#undef TARGET_PASS_BY_REFERENCE
#define TARGET_PASS_BY_REFERENCE sra32_pass_by_reference
#undef TARGET_ARG_PARTIAL_BYTES
#define TARGET_ARG_PARTIAL_BYTES sra32_arg_partial_bytes
#undef TARGET_SETUP_INCOMING_VARARGS
#define TARGET_SETUP_INCOMING_VARARGS sra32_setup_incoming_varargs

struct gcc_target targetm = TARGET_INITIALIZER;

#include "gt-sra32.h"
