/* safe_iop
 * License:: BSD (See safe_iop_LICENSE file)
 * Author:: Will Drewry <redpig@dataspill.org>
 *
 * See README for usage.
 *
 * Namespace: This library uses the prefix sop_.
 *
 * Known issues:
 * - GCC < 4.3 warns when using a constant value as it compiles out always 
 *   pass or fail tests  (e.g., 1 > 0 with s8,s16)
 *
 * To Do:
 * = next milestone (0.5.0)
 * - Autogenerate test cases for all op-type-type combinations
 * - Add while() and for() test cases for inc and dec
 * = long term/never:
 * - break up sop_safe_cast into smaller macros using suffix concatenation to
 *   minimize repeated code and make it easier to review
 * - Consider ways to do safe casting with operator awareness to
 *   allow cases where an addition of a negative signed value may be safe
 *   as a subtraction, for example. (Perhaps using checked type promotion
 *   similarly to compilers)
 *
 * History:
 * = [next milestone]
 * - Use cpp concatenation to minimize code duplication
 * -- E.g., sop_addx no longer expands sop_sadd and sop_uadd at each callsite
 * - Re-namespaced to sop_
 * - All tests pass under pcc
 * - Added pointer type markup which allows for (e.g.) u64=u32+u32.
 * - Refactored the code again due to massive increase in generated code size
 * -- Rewrote to support passing consts and compilers without typeof()
 * -- Added sop_<op>x  -- primary interface
 * -- Added sop_<op>x[num] - convenience interface
 * -- Added sop_incx and sop_decx
 * = 0.4.0 (never released)
 * - Compiles under pcc (but tests fail due to max # calculations)
 * - Refactored nearly all of the code
 * - Removed -DSAFE_IOP_COMPAT
 * - Add support for differently typed/signed operands in sop_iopf format
 * - Added negative tests to add T_<op>_*()s
 * - [BUG] Fixed signed addition. Two negatives were failing!
 * - Extended sop_iopf to support more types. Still needs more testing.
 * - Added more mixed interface tests
 * - Added safe type casting (automagically)
 * - Added basic speed tests (not accurate at all yet)
 * - Added sop_inc/sop_dec
 * - Licensed all subsequent work BSD for clarity of code ownership
 * = 0.3.1
 * - fixed typo/bug in sop_sadd (backported from 0.4.0/trunk above)
 * = 0.3
 * - solidified code into a smaller number of macros and functions
 * - added typeless functions using gcc magic (typeof)
 * - deprecrated old interfaces (-DSAFE_IOP_COMPAT)
 * - discover size maximums automagically
 * - separated test cases for easier understanding
 * - significantly expanded test cases
 * - derive type maximums and minimums internally (checked in testing)
 * = 0.2
 * - Removed dependence on twos complement arithmetic to allow macro-ized
 *   definitions
 * - Added (s)size_t support
 * - Added (u)int8,16,64 support
 * - Added portable inlining
 * - Added support for NULL result pointers
 * - Added support for header-only use (sop_iop.c only needed for sop_iopf)
 * = 0.1
 * - Initial release
 *
 * Contributors & thanks:
 * - peter@valchev.net for his review, comments, and enthusiasm
 * - Diego 'Flameeyes' Petteno for his analysis, use, and bug reporting
 * - thanks to Google for contributing some work upstream earlier in the project
 *
 * Copyright (c) 2007,2008 Will Drewry <redpig@dataspill.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* This library supplies a set of standard functions for performing and
 * checking safe integer operations. The code is based on examples from
 * https://www.securecoding.cert.org/confluence/display/seccode/INT32-C.+Ensure+that+operations+on+signed+integers+do+not+result+in+overflow
 */
#ifndef _SAFE_IOP_H
#define _SAFE_IOP_H
#include <assert.h>  /* for convenience NULL check  */
#include <limits.h>  /* for CHAR_BIT */
#include <stdint.h> /* [u]int<bits>_t, [U]INT64_MAX */
#include <sys/types.h> /* for [s]size_t */

#define SAFE_IOP_VERSION "0.5.0rc1"

/* sopf
 *
 * Takes in a character array which specifies the operations
 * to perform on a given value. The value will be assumed to be
 * of the type specified for each operation.
 *
 * Currently accepted format syntax is:
 *   [type_marker]operation[type_marker]...
 * The type marker may be any of the following:
 * - s[8,16,32,64] for signed of size 8-bit, etc
 * - u[8,16,32,64] for unsigned of size 8-bit, etc
 * If no type_marker is specified, it is assumed to be s32.
 * If a left-hand side type-marker is given, then that will
 * become the default for all remaining operands.
 * E.g.,
 *   sop_iopf(&dst, "u16**+", a, b, c. d);
 * is equivalent to ((a*b)*c)+d all of type u16.
 * This function uses FIFO and not any other order of operations/precedence.
 *
 * The operation must be one of the following:
 * - *   -- multiplication
 * - /   -- division
 * - -   -- subtraction
 * - +   -- addition
 * - %   -- modulo (remainder)
 * - <<  -- left shift
 * - >>  -- right shift
 *
 * Args:
 * - pointer to the final result
 * - array of format characters
 * - all remaining arguments are derived from the format
 * Output:
 * - Returns 1 on success leaving the result in value
 * - Returns 0 on failure leaving the contents of value *unknown*
 * Caveats:
 * - This function is only provided if sop_iop.c is compiled and linked
 *   into the source.  Otherwise only macro-based functions are available.
 */
int sopf(void *result, const char *const fmt, ...);


/* Type markup macros
 * These macros are the user mechanism for marking up
 * types without giving the exact type name.  This
 * serves primarily as short-hand for long type names,
 * but also provides a simple mechanism for automatically
 * getting whether a type is signed in a programmatic fashion.
 *
 * These are used _only_ with the generic compiler interfaces
 * and not with the GNU C compiler interfaces. See the comment
 * at the start of the "Generic (x) interface macros" section
 * for example usage.
 */
#define sop_typeof_sop_s8(_a) int8_t
#define sop_signed_sop_s8(_a) 1
#define sop_valueof_sop_s8(_a) _a
/* These macros are used internally for expanding the
 * sign-less interface (sop_addx) to the correct function
 * without needing a runtime check (is signed).  In addition,
 * it means that the macros for signed and unsigned operations
 * won't be expanded everywhere.
 */
#define sop_add_sop_s8(_a) sop_sadd
#define sop_sub_sop_s8(_a) sop_ssub
#define sop_mul_sop_s8(_a) sop_smul
#define sop_div_sop_s8(_a) sop_sdiv
#define sop_mod_sop_s8(_a) sop_smod
#define sop_shl_sop_s8(_a) sop_sshl
#define sop_shr_sop_s8(_a) sop_sshr


#define sop_typeof_sop_u8(_a) uint8_t
#define sop_signed_sop_u8(_a) 0
#define sop_valueof_sop_u8(_a) _a
#define sop_add_sop_u8(_a) sop_uadd
#define sop_sub_sop_u8(_a) sop_usub
#define sop_mul_sop_u8(_a) sop_umul
#define sop_div_sop_u8(_a) sop_udiv
#define sop_mod_sop_u8(_a) sop_umod
#define sop_shl_sop_u8(_a) sop_ushl
#define sop_shr_sop_u8(_a) sop_ushr



#define sop_typeof_sop_s16(_a) int16_t
#define sop_signed_sop_s16(_a) 1
#define sop_valueof_sop_s16(_a) _a
#define sop_add_sop_s16(_a) sop_sadd
#define sop_sub_sop_s16(_a) sop_ssub
#define sop_mul_sop_s16(_a) sop_smul
#define sop_div_sop_s16(_a) sop_sdiv
#define sop_mod_sop_s16(_a) sop_smod
#define sop_shl_sop_s16(_a) sop_sshl
#define sop_shr_sop_s16(_a) sop_sshr


#define sop_typeof_sop_u16(_a) uint16_t
#define sop_signed_sop_u16(_a) 0
#define sop_valueof_sop_u16(_a) _a
#define sop_add_sop_u16(_a) sop_uadd
#define sop_sub_sop_u16(_a) sop_usub
#define sop_mul_sop_u16(_a) sop_umul
#define sop_div_sop_u16(_a) sop_udiv
#define sop_mod_sop_u16(_a) sop_umod
#define sop_shl_sop_u16(_a) sop_ushl
#define sop_shr_sop_u16(_a) sop_ushr



#define sop_typeof_sop_s32(_a) int32_t
#define sop_signed_sop_s32(_a) 1
#define sop_valueof_sop_s32(_a) _a
#define sop_add_sop_s32(_a) sop_sadd
#define sop_sub_sop_s32(_a) sop_ssub
#define sop_mul_sop_s32(_a) sop_smul
#define sop_div_sop_s32(_a) sop_sdiv
#define sop_mod_sop_s32(_a) sop_smod
#define sop_shl_sop_s32(_a) sop_sshl
#define sop_shr_sop_s32(_a) sop_sshr


#define sop_typeof_sop_u32(_a) uint32_t
#define sop_signed_sop_u32(_a) 0
#define sop_valueof_sop_u32(_a) _a
#define sop_add_sop_u32(_a) sop_uadd
#define sop_sub_sop_u32(_a) sop_usub
#define sop_mul_sop_u32(_a) sop_umul
#define sop_div_sop_u32(_a) sop_udiv
#define sop_mod_sop_u32(_a) sop_umod
#define sop_shl_sop_u32(_a) sop_ushl
#define sop_shr_sop_u32(_a) sop_ushr



#define sop_typeof_sop_s64(_a) int64_t
#define sop_signed_sop_s64(_a) 1
#define sop_valueof_sop_s64(_a) _a
#define sop_add_sop_s64(_a) sop_sadd
#define sop_sub_sop_s64(_a) sop_ssub
#define sop_mul_sop_s64(_a) sop_smul
#define sop_div_sop_s64(_a) sop_sdiv
#define sop_mod_sop_s64(_a) sop_smod
#define sop_shl_sop_s64(_a) sop_sshl
#define sop_shr_sop_s64(_a) sop_sshr


#define sop_typeof_sop_u64(_a) uint64_t
#define sop_signed_sop_u64(_a) 0
#define sop_valueof_sop_u64(_a) _a
#define sop_add_sop_u64(_a) sop_uadd
#define sop_sub_sop_u64(_a) sop_usub
#define sop_mul_sop_u64(_a) sop_umul
#define sop_div_sop_u64(_a) sop_udiv
#define sop_mod_sop_u64(_a) sop_umod
#define sop_shl_sop_u64(_a) sop_ushl
#define sop_shr_sop_u64(_a) sop_ushr



#define sop_typeof_sop_sl(_a) signed long
#define sop_signed_sop_sl(_a) 1
#define sop_valueof_sop_sl(_a) _a
#define sop_add_sop_sl(_a) sop_sadd
#define sop_sub_sop_sl(_a) sop_ssub
#define sop_mul_sop_sl(_a) sop_smul
#define sop_div_sop_sl(_a) sop_sdiv
#define sop_mod_sop_sl(_a) sop_smod
#define sop_shl_sop_sl(_a) sop_sshl
#define sop_shr_sop_sl(_a) sop_sshr


#define sop_typeof_sop_ul(_a) unsigned long
#define sop_signed_sop_ul(_a) 0
#define sop_valueof_sop_ul(_a) _a
#define sop_add_sop_ul(_a) sop_uadd
#define sop_sub_sop_ul(_a) sop_usub
#define sop_mul_sop_ul(_a) sop_umul
#define sop_div_sop_ul(_a) sop_udiv
#define sop_mod_sop_ul(_a) sop_umod
#define sop_shl_sop_ul(_a) sop_ushl
#define sop_shr_sop_ul(_a) sop_ushr



#define sop_typeof_sop_sll(_a) signed long long
#define sop_signed_sop_sll(_a) 1
#define sop_valueof_sop_sll(_a) _a
#define sop_add_sop_sll(_a) sop_sadd
#define sop_sub_sop_sll(_a) sop_ssub
#define sop_mul_sop_sll(_a) sop_smul
#define sop_div_sop_sll(_a) sop_sdiv
#define sop_mod_sop_sll(_a) sop_smod
#define sop_shl_sop_sll(_a) sop_sshl
#define sop_shr_sop_sll(_a) sop_sshr


#define sop_typeof_sop_ull(_a) unsigned long long
#define sop_signed_sop_ull(_a) 0
#define sop_valueof_sop_ull(_a) _a
#define sop_add_sop_ull(_a) sop_uadd
#define sop_sub_sop_ull(_a) sop_usub
#define sop_mul_sop_ull(_a) sop_umul
#define sop_div_sop_ull(_a) sop_udiv
#define sop_mod_sop_ull(_a) sop_umod
#define sop_shl_sop_ull(_a) sop_ushl
#define sop_shr_sop_ull(_a) sop_ushr



#define sop_typeof_sop_si(_a) signed int
#define sop_signed_sop_si(_a) 1
#define sop_valueof_sop_si(_a) _a
#define sop_add_sop_si(_a) sop_sadd
#define sop_sub_sop_si(_a) sop_ssub
#define sop_mul_sop_si(_a) sop_smul
#define sop_div_sop_si(_a) sop_sdiv
#define sop_mod_sop_si(_a) sop_smod
#define sop_shl_sop_si(_a) sop_sshl
#define sop_shr_sop_si(_a) sop_sshr


#define sop_typeof_sop_ui(_a) unsigned int
#define sop_signed_sop_ui(_a) 0
#define sop_valueof_sop_ui(_a) _a
#define sop_add_sop_ui(_a) sop_uadd
#define sop_sub_sop_ui(_a) sop_usub
#define sop_mul_sop_ui(_a) sop_umul
#define sop_div_sop_ui(_a) sop_udiv
#define sop_mod_sop_ui(_a) sop_umod
#define sop_shl_sop_ui(_a) sop_ushl
#define sop_shr_sop_ui(_a) sop_ushr



#define sop_typeof_sop_sc(_a) signed char
#define sop_signed_sop_sc(_a) 1
#define sop_valueof_sop_sc(_a) _a
#define sop_add_sop_sc(_a) sop_sadd
#define sop_sub_sop_sc(_a) sop_ssub
#define sop_mul_sop_sc(_a) sop_smul
#define sop_div_sop_sc(_a) sop_sdiv
#define sop_mod_sop_sc(_a) sop_smod
#define sop_shl_sop_sc(_a) sop_sshl
#define sop_shr_sop_sc(_a) sop_sshr


#define sop_typeof_sop_uc(_a) unsigned char
#define sop_signed_sop_uc(_a) 0
#define sop_valueof_sop_uc(_a) _a
#define sop_add_sop_uc(_a) sop_uadd
#define sop_sub_sop_uc(_a) sop_usub
#define sop_mul_sop_uc(_a) sop_umul
#define sop_div_sop_uc(_a) sop_udiv
#define sop_mod_sop_uc(_a) sop_umod
#define sop_shl_sop_uc(_a) sop_ushl
#define sop_shr_sop_uc(_a) sop_ushr



#define sop_typeof_sop_sszt(_a) ssize_t
#define sop_signed_sop_sszt(_a) 1
#define sop_valueof_sop_sszt(_a) _a
#define sop_add_sop_sszt(_a) sop_sadd
#define sop_sub_sop_sszt(_a) sop_ssub
#define sop_mul_sop_sszt(_a) sop_smul
#define sop_div_sop_sszt(_a) sop_sdiv
#define sop_mod_sop_sszt(_a) sop_smod
#define sop_shl_sop_sszt(_a) sop_sshl
#define sop_shr_sop_sszt(_a) sop_sshr


#define sop_typeof_sop_szt(_a) size_t
#define sop_signed_sop_szt(_a) 0
#define sop_valueof_sop_szt(_a) _a
#define sop_add_sop_szt(_a) sop_uadd
#define sop_sub_sop_szt(_a) sop_usub
#define sop_mul_sop_szt(_a) sop_umul
#define sop_div_sop_szt(_a) sop_udiv
#define sop_mod_sop_szt(_a) sop_umod
#define sop_shl_sop_szt(_a) sop_ushl
#define sop_shr_sop_szt(_a) sop_ushr



/* This allows NULL to be passed in to the generic
 * interface macros without needing to mark them up
 * with a type (like sop_blah(NULL)). Instead, NULL
 * will work.
 */
#define sop_typeof_NULL intmax_t  /* silences gcc complaints when the macos expand */
#define sop_signed_NULL 0
#define sop_valueof_NULL 0
/* Macros for handling pointer presence/non-presence. */
#define sop_safe_cast_NULL sop_safe_cast_np

#define sop_safe_cast_sop_u8(_X)   sop_safe_cast_p
#define sop_safe_cast_sop_s8(_X)   sop_safe_cast_p
#define sop_safe_cast_sop_u16(_X)  sop_safe_cast_p
#define sop_safe_cast_sop_s16(_X)  sop_safe_cast_p
#define sop_safe_cast_sop_u32(_X)  sop_safe_cast_p
#define sop_safe_cast_sop_s32(_X)  sop_safe_cast_p
#define sop_safe_cast_sop_u64(_X)  sop_safe_cast_p
#define sop_safe_cast_sop_s64(_X)  sop_safe_cast_p
#define sop_safe_cast_sop_uc(_X) sop_safe_cast_p
#define sop_safe_cast_sop_sc(_X) sop_safe_cast_p
#define sop_safe_cast_sop_ui(_X) sop_safe_cast_p
#define sop_safe_cast_sop_si(_X) sop_safe_cast_p
#define sop_safe_cast_sop_ul(_X) sop_safe_cast_p
#define sop_safe_cast_sop_sl(_X) sop_safe_cast_p
#define sop_safe_cast_sop_ull(_X) sop_safe_cast_p
#define sop_safe_cast_sop_sll(_X) sop_safe_cast_p
#define sop_safe_cast_sop_szt(_X) sop_safe_cast_p
#define sop_safe_cast_sop_sszt(_X) sop_safe_cast_p

/* Since we detect NULLness with a conditional, just return 0 for this case */
#define sop_add_NULL(_A,_B,_C,_D,_E,_F,_G,_H,_I) 0
#define sop_sub_NULL(_A,_B,_C,_D,_E,_F,_G,_H,_I) 0
#define sop_mul_NULL(_A,_B,_C,_D,_E,_F,_G,_H,_I) 0
#define sop_div_NULL(_A,_B,_C,_D,_E,_F,_G,_H,_I) 0
#define sop_mod_NULL(_A,_B,_C,_D,_E,_F,_G,_H,_I) 0
#define sop_shl_NULL(_A,_B,_C,_D,_E,_F,_G,_H,_I) 0
#define sop_shr_NULL(_A,_B,_C,_D,_E,_F,_G,_H,_I) 0

/*****************************************************************************
 * Safe-checking Implementation Macros
 *****************************************************************************
 * The macros below are used for the implementation of the specific
 * operation (along with helpers).  Each operation will take the format:
 *   sop_[u|s]<op>(...)
 * 'u' and 's' represent unsigned and signed checks, respectively. The macros
 * take the sign and type for both operands, but only the sign and type of the
 * first operand, 'a', is used for the operation.  These macros assume
 * type-casting is safe on the given operands when they are called. (The
 * sop_safe_cast macro in this section performs just that test.)  Despite this,
 * the sign and type of the second operand, 'b', have been left in in case of
 * future need.
 */

/* sop_assert
 * An assert() wrapper which still performs the operation when NDEBUG called
 * and is safe in if statements.
 */
#ifdef NDEBUG
#  define sop_assert(x) ((x) ? 1 : 0)
#else
#  define sop_assert(x) (assert(x),1)
#endif

/* use a nice prefix :) */
#define __sop(x) OPAQUE_SAFE_IOP_PREFIX_ ## x
#define OPAQUE_SAFE_IOP_PREFIX_var(x) OPAQUE_SAFE_IOP_PREFIX_VARIABLE_ ## x
#define OPAQUE_SAFE_IOP_PREFIX_m(x) OPAQUE_SAFE_IOP_PREFIX_MACRO_ ## x
#define OPAQUE_SAFE_IOP_PREFIX_f(x) OPAQUE_SAFE_IOP_PREFIX_FN_ ## x


/* Determine maximums and minimums for the platform dynamically
 * without relying on a limits.h file.  As a bonus, the compiler
 * may be able to optimize the expression out at compile-time since
 * it should resolve all values to fixed numbers.
 */
#define OPAQUE_SAFE_IOP_PREFIX_MACRO_smin(_type) \
  (_type)((_type)(~0)<<(sizeof(_type)*CHAR_BIT-1))

#define OPAQUE_SAFE_IOP_PREFIX_MACRO_smax(_type) \
   (_type)(-(OPAQUE_SAFE_IOP_PREFIX_MACRO_smin(_type)+1))

#define OPAQUE_SAFE_IOP_PREFIX_MACRO_umax(_type) ((_type)~0)


/*** Same-type addition macros ***/
#define sop_uadd(_ptr_sign, _ptr_type, _ptr, \
                 _a_sign, _a_type, _a, _b_sign, _b_type, _b) \
 ((((_ptr_type)(_b) <= \
      ((_ptr_type)(__sop(m)(umax)(_ptr_type) - (_ptr_type)(_a))) ? 1 : 0)) \
   ? \
     (((void *)(_ptr)) != NULL ? \
       *((_ptr_type *)(_ptr)) = ((_ptr_type)(_a) + (_ptr_type)(_b)), 1 : 1) \
 : 0)

#define sop_sadd(_ptr_sign, _ptr_type, _ptr, \
                 _a_sign, _a_type, _a, _b_sign, _b_type, _b) \
   (((((_ptr_type)(_b) > (_ptr_type)0) && \
       ((_ptr_type)(_a) > (_ptr_type)0)) \
     ? /*>0*/  \
       ((_ptr_type)(_a) > \
         (_ptr_type)(__sop(m)(smax)(_ptr_type) - \
         (_ptr_type)(_b)) ? 0 : 1) \
     : \
      /* <0 */ \
      ((!((_ptr_type)(_b) > (_ptr_type)0) && \
               !((_ptr_type)(_a) > (_ptr_type)0)) ? \
        (((_ptr_type)(_a) < \
          (_ptr_type)(__sop(m)(smin)(_ptr_type) - \
                      (_ptr_type)(_b))) ? 0 : 1) : 1) \
     ) \
   ? /* Now assign if needed */ \
     (((void *)(_ptr)) != NULL ? \
       *((_ptr_type *)(_ptr)) = ((_ptr_type)(_a) + (_ptr_type)(_b)),\
       1 \
       : \
       1 \
     ) \
   : \
     0 \
   )

/*** Same-type subtraction macros ***/
#define sop_usub(_ptr_sign, _ptr_type, _ptr, \
                 _a_sign, _a_type, _a, _b_sign, _b_type, _b) \
  ((_ptr_type)(_a) >= (_ptr_type)(_b) ? (((void *)(_ptr)) != NULL ? \
    *((_ptr_type*)(_ptr)) = ((_ptr_type)(_a) - (_ptr_type)(_b)),1 : 1) : 0 )

#define sop_ssub(_ptr_sign, _ptr_type, _ptr, \
                  _a_sign, _a_type, _a, _b_sign, _b_type, _b) ( \
  (!((_ptr_type)(_b) <= 0 && \
     (_ptr_type)(_a) > (__sop(m)(smax)(_ptr_type) + (_ptr_type)(_b))) && \
   !((_ptr_type)(_b) > 0 && \
     (_ptr_type)(_a) < (__sop(m)(smin)(_ptr_type) + (_ptr_type)(_b)))) \
  ? \
    (((void *)(_ptr)) != NULL ? *((_ptr_type *)(_ptr)) = \
                    ((_ptr_type)(_a) - (_ptr_type)(_b)), 1 : 1) \
  : \
    0)


/*** Same-type multiplication macros ***/
#define sop_umul(_ptr_sign, _ptr_type, _ptr, \
                 _a_sign, _a_type, _a, _b_sign, _b_type, _b) ( \
  (!(_ptr_type)(_b) || \
   (_ptr_type)(_a) <= (__sop(m)(umax)(_ptr_type) / (_ptr_type)(_b))) \
  ? \
    ((((void *)(_ptr)) != NULL) ? *((_ptr_type*)(_ptr)) = \
                        ((_ptr_type)(_a)) * ((_ptr_type)(_b)),1 : 1) \
  : \
    0)

#define sop_smul(_ptr_sign, _ptr_type, _ptr, \
                 _a_sign, _a_type, _a, _b_sign, _b_type, _b) \
  ((((_ptr_type)(_a) > 0) ?  /* a is positive */ \
    (((_ptr_type)(_b) > 0) ?  /* b and a are positive */ \
       (((_ptr_type)(_a) > (__sop(m)(smax)(_ptr_type) / ((_ptr_type)(_b)))) ? 0 : 1) \
     : /* a positive, b non-positive */ \
       (((_ptr_type)(_b) < (__sop(m)(smin)(_ptr_type) / (_ptr_type)(_a))) ? 0 : 1)) \
   : /* a is non-positive */ \
    (((_ptr_type)(_b) > 0) ? /* a is non-positive, b is positive */ \
      (((_ptr_type)(_a) < (__sop(m)(smin)(_ptr_type) / ((_ptr_type)(_b)))) ? 0 : 1) \
     : /* a and b are non-positive */ \
      ((((_ptr_type)(_a) != 0) && \
       (((_ptr_type)(_b)) < (__sop(m)(smax)(_ptr_type) / (_ptr_type)(_a)))) ? \
         0 : 1) \
      ) \
  ) /* end if a and b are non-positive */ \
  ? \
    (((void *)(_ptr)) != NULL ? *((_ptr_type*)(_ptr)) = \
      ((_ptr_type)(_a) * ((_ptr_type)(_b))),1 : 1) \
  : 0)

/*** Same-type division macros ***/

/* div-by-zero is the only thing addressed */
#define sop_udiv(_ptr_sign, _ptr_type, _ptr, \
                 _a_sign, _a_type, _a, _b_sign, _b_type, _b) \
  (((_ptr_type)(_b) != 0) ? ((((void *)(_ptr)) != NULL) ? \
                  *((_ptr_type*)(_ptr)) = ((_ptr_type)(_a) / (_ptr_type)(_b)),1 : \
                   1) \
              : 0)

/* Addreses div by zero and smin -1 */
#define sop_sdiv(_ptr_sign, _ptr_type, _ptr, \
                 _a_sign, _a_type, _a, _b_sign, _b_type, _b) \
  ((((_ptr_type)(_b) != 0) && \
   (((_ptr_type)(_a) != __sop(m)(smin)(_ptr_type)) || \
   /* GCC type-limits hack:  \
    * whines if b is unsigned even with a cast, but should extend fine. */ \
    ((_b_type)(_b) != (_b_type)-1))) \
   ? \
    ((((void *)(_ptr)) != NULL) ? *((_ptr_type*)(_ptr)) = \
      ((_ptr_type)(_a) / (_ptr_type)(_b)),1 : 1) \
  : \
    0 \
  ) \


/*** Same-type modulo macros ***/
/* mod-by-zero is the only thing addressed */
#define sop_umod(_ptr_sign, _ptr_type, _ptr, \
                 _a_sign, _a_type, _a, _b_sign, _b_type, _b) \
  (((_ptr_type)(_b) != 0) ? ((((void *)(_ptr)) != NULL) ? \
    *((_ptr_type*)(_ptr)) = ((_ptr_type)(_a) % (_ptr_type)(_b)),1 : 1) : 0)

/* Addreses mod by zero and smin -1 */
#define sop_smod(_ptr_sign, _ptr_type, _ptr, \
                 _a_sign, _a_type, _a, _b_sign, _b_type, _b) \
  ((((_ptr_type)(_b) != 0) && \
   (((_ptr_type)(_a) != __sop(m)(smin)(_ptr_type)) || \
   /* GCC type-limits hack: */ \
    ((_b_type)(_b) != (_b_type)-1))) \
   ? \
    ((((void *)(_ptr)) != NULL) ? *((_ptr_type*)(_ptr)) = \
      ((_ptr_type)(_a) % (_ptr_type)(_b)),1 : 1) \
  : \
    0 \
  ) \

/*** Same-type left-shift macros ***/
#define sop_sshl(_ptr_sign, _ptr_type, _ptr, \
                 _a_sign, _a_type, _a, _b_sign, _b_type, _b) \
  /* GCC type-limit hack: \
   * should just check if < 0 */ \
  ((!((_ptr_type)(_a) > 0 || (_ptr_type)(_a) == 0) || \
    !((_ptr_type)(_b) > 0 || (_ptr_type)(_b) == 0) || \
      ((_ptr_type)(_b) >= sizeof(_ptr_type)*CHAR_BIT) || \
      ((_ptr_type)(_a) > (__sop(m)(smax)(_ptr_type) >> ((_ptr_type)(_b))))) ? \
    0 \
  : ((((void *)(_ptr)) != NULL) ? *((_ptr_type*)(_ptr)) = \
      (_ptr_type)(_a) << (_ptr_type)(_b),1 : 1))

#define sop_ushl(_ptr_sign, _ptr_type, _ptr, \
                 _a_sign, _a_type, _a, _b_sign, _b_type, _b) \
  ((((_ptr_type)(_b) >= (sizeof(_ptr_type)*CHAR_BIT)) || \
   (((_ptr_type)(_a)) > (__sop(m)(umax)(_ptr_type) >> ((_ptr_type)(_b))))) \
  ? \
    0 \
  : \
    ((((void *)(_ptr)) != NULL) ? *((_ptr_type*)(_ptr)) = \
      (_ptr_type)(_a) << (_ptr_type)(_b),1 :  1))

/*** Same-type right-shift macros ***/
/* XXX: CERT doesnt recommend failing on -a, but it is undefined */
#define sop_sshr(_ptr_sign, _ptr_type, _ptr, \
                 _a_sign, _a_type, _a, _b_sign, _b_type, _b) \
  ((!((_ptr_type)(_a) > 0 || (_ptr_type)(_a) == 0) || \
      !((_ptr_type)(_b) > 0 || (_ptr_type)(_b) == 0) || \
      ((_ptr_type)(_b) >= sizeof(_ptr_type)*CHAR_BIT)) ? \
    0 \
  : \
    ((((void *)(_ptr)) != NULL) ? *((_ptr_type*)(_ptr)) = \
      (_ptr_type)(_a) >> (_ptr_type)(_b),1 : 1) \
  )

/* this doesn't complain if 0 >> n. */
#define sop_ushr(_ptr_sign, _ptr_type, _ptr, \
                 _a_sign, _a_type, _a, _b_sign, _b_type, _b) \
  (((_ptr_type)(_b) >= (_ptr_type)(sizeof(_ptr_type)*CHAR_BIT)) ? \
    0 : ((((void *)(_ptr)) != NULL) ? \
         *((_ptr_type*)(_ptr)) = ((_ptr_type)(_a) >> (_ptr_type)(_b)),1 : 1))


/* sop_safe_cast
 * sop_safe_cast takes the signedness, type, and value of two variables. It
 * then returns true if second variable can be safely cast to the first
 * variable's type and sign without changing value.
 *
 * This function is used internally in safe-iop but is exposed to allow
 * use if there is a need.
 */
#define sop_safe_cast(_a_sign, _a_type, _a, _b_sign, _b_type, _b) \
  ((sizeof(_a_type) == sizeof(_b_type)) \
  ? \
    /* sign change */ \
    ((!_a_sign && !_b_sign) ? \
      1 \
    : \
      ((_a_sign && _b_sign) \
      ? \
        1 \
      : \
        ((!_a_sign && _b_sign) \
        ? \
          (((_b) > (_b_type)0 || (_b) == (_b_type)0) ? 1 : 0) \
        : \
          ((_a_sign && !_b_sign) \
          ? \
      /* since they are the same size, the comparison cast should be safe */ \
            (((_b) < (_b_type)__sop(m)(smax)(_a_type) || \
             (_b) == (_b_type)__sop(m)(smax)(_a_type)) ? 1: 0) \
          : \
            0 \
          ) \
        ) \
      ) \
    ) \
  : \
    ((sizeof(_a_type) > sizeof(_b_type)) \
    ? \
      /* cast up: this allows -1, e.g., which means extension. */ \
      ((!_a_sign && !_b_sign) \
      ? \
        1 \
      : \
        ((_a_sign && _b_sign) \
        ? \
          1 \
        : \
          ((!_a_sign && _b_sign) \
          ? \
            (((_b) == (_b_type)0 || (_b) > (_b_type)0) ? 1 : 0)\
          : \
            ((_a_sign && !_b_sign) \
            ? \
              /* this is true by default */ \
              ((__sop(m)(smax)(_a_type) >= __sop(m)(umax)(_b_type)) \
              ? \
                 1 \
              : \
                /* This will safely truncate given that smax(a) <= umax(b) */ \
                (((_b) < (_b_type)__sop(m)(smax)(_a_type) || \
                 (_b) == (_b_type)__sop(m)(smax)(_a_type)) \
                ? \
                  1 \
                : \
                  0 \
                ) \
              ) \
            : \
              0 \
            ) \
          ) \
        ) \
      ) \
    : \
      ((sizeof(_a_type) < sizeof(_b_type)) \
      ? \
        /* cast down (loss of precision) */ \
        ((!_a_sign && !_b_sign) \
        ? \
          (((_b) == (_b_type)__sop(m)(umax)(_a_type)) \
          ? \
            1 \
          : \
            (((_b) < (_b_type)__sop(m)(umax)(_a_type)) ? 1 : 0) \
          ) \
        : \
          ((_a_sign && _b_sign) \
          ? \
            ((((_b) > (_b_type)__sop(m)(smin)(_a_type) || \
               (_b) == (_b_type)__sop(m)(smin)(_a_type)) && \
              ((_b) < (_b_type)__sop(m)(smax)(_a_type) || \
               (_b) == (_b_type)__sop(m)(smax)(_a_type))) \
            ? \
              1 \
            : \
              0 \
            ) \
          : \
            ((!_a_sign && _b_sign) \
            ? \
              /* GCC type-limit hack: \
               * XXX: need a test for this in case umax of a smaller \
               * type could exceed smax of a larger type. The other direction \
               * could truncate _b though so it's a challene either way. */ \
              ((((_b) > (_b_type)0 || (_b) == (_b_type)0) && \
               (((_b) < (_b_type)__sop(m)(umax)(_a_type)) || \
                ((_b) == (_b_type)__sop(m)(umax)(_a_type)))) \
              ? \
                1 \
              : \
                0 \
              ) \
            : \
              ((_a_sign && !_b_sign) \
              ? \
                /* this should safely extend */ \
                (((_b) < (_b_type)__sop(m)(smax)(_a_type) || \
                  (_b) == (_b_type)__sop(m)(smax)(_a_type)) \
                ? \
                  1 \
                : \
                  0 \
                ) \
              : \
                0 \
              ) \
            ) \
          ) \
        ) \
      : \
        0 \
      ) \
    ) \
  )


/* These functions allow compile-time resolution of whether _ptr is non-NULL.
 * In the future, concatenation tactics like these and the ones used with the
 * operations may be employed to compartmentalize the sop_safe_cast tests, but
 * that pushes extra work on preprocessors tests for portability (CHAR_BIT == 8)
 * is int and int32 or an int16, etc. */
#define sop_safe_cast_np(_ptr_sign, _ptr_type, _ptr, \
                         _a_sign, _a_type, _a, \
                         _b_sign, _b_type, _b) \
  sop_safe_cast(_a_sign, _a_type, _a, _b_sign, _b_type, _b)

#define sop_safe_cast_p(_ptr_sign, _ptr_type, _ptr, \
                        _a_sign, _a_type, _a, \
                        _b_sign, _b_type, _b) \
  sop_safe_cast(_ptr_sign, _ptr_type, _ptr, _a_sign, _a_type, _a) && \
  sop_safe_cast(_ptr_sign, _ptr_type, _ptr, _b_sign, _b_type, _b)


/*****************************************************************************
 * Generic (x) interface macros
 *****************************************************************************
 * These macros are known to work with GCC as well as PCC and perhaps other C99
 * compatible compilers.  Due to the limitations of the C99 standard, these
 * macros are _NOT_ side effect free and the arguments require a custom
 * type-markup.
 *
 * Instead of requiring the specification of the type for each variable,
 * short-hand macros are provided which provide a simple interface:
 *   uint32_t a = 100, b = 200;
 *   uint64_t c;
 *   if (!sop_mulx(sop_u64(&c), sop_u32(a), sop_u32(b)) abort();
 * In addition, this interface automatically handles testing for cast-safety.
 * All operands will be cast to the type/signedness of the left-most operand unless
 * there is a destination pointer. If there is a pointer, as above, the values will
 * be cast to that type, if possible, for the operations.
 * 
 * Ιn the example above, that is a's type: uint32_t.
 *
 * The type markup macros available are listed at the top of the file.
 *
 * With respect to side effects, never call sop_<op>x[#] with a operand that
 * may have side effects. For example:
 * [BAD!]  sop_addx(sio_u32(buf++), sop_s32(a--), sop_s16(--b));
 *
 */
#define sop_addx(_ptr, _a, _b) \
  (sop_safe_cast_##_ptr(\
    sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
    sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
    sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b) ? \
    /* PCC won't do an extra dereference here! */ \
    (((void *)(sop_valueof_##_ptr)) ? \
      sop_add_##_ptr( \
                 sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
                 sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b) \
    : \
      sop_add_##_a( \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_ptr, \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
                 sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b)) \
  : 0)

#define sop_subx(_ptr, _a, _b) \
  (sop_safe_cast_##_ptr(\
    sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
    sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
    sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b) ? \
    /* PCC won't do an extra dereference here! */ \
    (((void *)(sop_valueof_##_ptr)) ? \
      sop_sub_##_ptr( \
                 sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
                 sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b) \
    : \
      sop_sub_##_a( \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_ptr, \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
                 sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b)) \
  : 0)

#define sop_mulx(_ptr, _a, _b) \
  (sop_safe_cast_##_ptr(\
    sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
    sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
    sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b) ? \
    /* PCC won't do an extra dereference here! */ \
    (((void *)(sop_valueof_##_ptr)) ? \
      sop_mul_##_ptr( \
                 sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
                 sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b) \
    : \
      sop_mul_##_a( \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_ptr, \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
                 sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b)) \
  : 0)

#define sop_divx(_ptr, _a, _b) \
  (sop_safe_cast_##_ptr(\
    sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
    sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
    sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b) ? \
    /* PCC won't do an extra dereference here! */ \
    (((void *)(sop_valueof_##_ptr)) ? \
      sop_div_##_ptr( \
                 sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
                 sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b) \
    : \
      sop_div_##_a( \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_ptr, \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
                 sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b)) \
  : 0)

#define sop_modx(_ptr, _a, _b) \
  (sop_safe_cast_##_ptr(\
    sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
    sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
    sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b) ? \
    /* PCC won't do an extra dereference here! */ \
    (((void *)(sop_valueof_##_ptr)) ? \
      sop_mod_##_ptr( \
                 sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
                 sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b) \
    : \
      sop_mod_##_a( \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_ptr, \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
                 sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b)) \
  : 0)

#define sop_shlx(_ptr, _a, _b) \
  (sop_safe_cast_##_ptr(\
    sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
    sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
    sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b) ? \
    /* PCC won't do an extra dereference here! */ \
    (((void *)(sop_valueof_##_ptr)) ? \
      sop_shl_##_ptr( \
                 sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
                 sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b) \
    : \
      sop_shl_##_a( \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_ptr, \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
                 sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b)) \
  : 0)

#define sop_shrx(_ptr, _a, _b) \
  (sop_safe_cast_##_ptr(\
    sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
    sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
    sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b) ? \
    /* PCC won't do an extra dereference here! */ \
    (((void *)(sop_valueof_##_ptr)) ? \
      sop_shr_##_ptr( \
                 sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
                 sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b) \
    : \
      sop_shr_##_a( \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_ptr, \
                 sop_signed_##_a, sop_typeof_##_a, sop_valueof_##_a, \
                 sop_signed_##_b, sop_typeof_##_b, sop_valueof_##_b)) \
  : 0)

/* Generic interface convenience functions */

/* sop_incx
 * Increments the value stored in a variable by one.
 * Example:
 *   int i;
 *   for (i = 0; i <= max && sop_incx(sop_s32(i); ) { ... }
 * This will increment until i == max or the variable would overflow (i=INT_MAX).
 */
#define sop_incx(_p) \
    sop_add_##_p(sop_signed_##_p, sop_typeof_##_p, &(sop_valueof_##_p), \
                 sop_signed_##_p, sop_typeof_##_p, sop_valueof_##_p, \
                 sop_signed_##_p, sop_typeof_##_p, 1)

/* sop_decx
 * Decrements the value stored in a variable by one.
 * Example:
 *   unsigned int i = 1024;
 *   while (sop_decx(sop_u32(i)) { ... }
 * This will decrement until the variable would underflow (i==0).
 */
#define sop_decx(_p) \
  sop_sub_##_p(sop_signed_##_p, sop_typeof_##_p, &(sop_valueof_##_p), \
               sop_signed_##_p, sop_typeof_##_p, sop_valueof_##_p, \
               sop_signed_##_p, sop_typeof_##_p, 1)

/* sop_<op>x[3-5]
 * These functions allow for the easy repetition of the same operation.
 * For instance, sop_mulx3 will multiply 3 integers together if they can
 * be safely cast to the type of the destination pointer and do not result
 * in an overflow or underflow.
 *
 * For example:
 *   if (!sop_mulx3(sop_u32(&image_sz), sop_u32(w), sop_u32(h), sop_u16(depth)))
 *     goto ERR_handle_bad_dimensions;
 */
#define sop_addx3(_ptr, _A, _B, _C) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
       sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
       sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_sadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_sadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
        :  \
          sop_uadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_uadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_addx4(_ptr, _A, _B, _C, _D) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_sadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_sadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_sadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
        :  \
          sop_uadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_uadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_uadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_addx5(_ptr, _A, _B, _C, _D, _E) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_sadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_sadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_sadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
          sop_sadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
        :  \
          sop_uadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_uadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_uadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
          sop_uadd(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_subx3(_ptr, _A, _B, _C) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_ssub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_ssub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
        :  \
          sop_usub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_usub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_subx4(_ptr, _A, _B, _C, _D) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_ssub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_ssub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_ssub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
        :  \
          sop_usub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_usub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_usub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_subx5(_ptr, _A, _B, _C, _D, _E) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_ssub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_ssub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_ssub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
          sop_ssub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
        :  \
          sop_usub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_usub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_usub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
          sop_usub(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_mulx3(_ptr, _A, _B, _C) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_smul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_smul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
        :  \
          sop_umul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_umul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_mulx4(_ptr, _A, _B, _C, _D) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_smul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_smul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_smul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
        :  \
          sop_umul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_umul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_umul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_mulx5(_ptr, _A, _B, _C, _D, _E) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_smul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_smul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_smul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
          sop_smul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
        :  \
          sop_umul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_umul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_umul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
          sop_umul(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_divx3(_ptr, _A, _B, _C) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_sdiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_sdiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
        :  \
          sop_udiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_udiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_divx4(_ptr, _A, _B, _C, _D) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_sdiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_sdiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_sdiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
        :  \
          sop_udiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_udiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_udiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_divx5(_ptr, _A, _B, _C, _D, _E) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_sdiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_sdiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_sdiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
          sop_sdiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
        :  \
          sop_udiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_udiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_udiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
          sop_udiv(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_modx3(_ptr, _A, _B, _C) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_smod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_smod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
        :  \
          sop_umod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_umod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_modx4(_ptr, _A, _B, _C, _D) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_smod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_smod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_smod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
        :  \
          sop_umod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_umod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_umod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_modx5(_ptr, _A, _B, _C, _D, _E) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_smod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_smod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_smod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
          sop_smod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
        :  \
          sop_umod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_umod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_umod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
          sop_umod(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_shlx3(_ptr, _A, _B, _C) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_sshl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_sshl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
        :  \
          sop_ushl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_ushl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_shlx4(_ptr, _A, _B, _C, _D) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_sshl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_sshl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_sshl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
        :  \
          sop_ushl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_ushl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_ushl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_shlx5(_ptr, _A, _B, _C, _D, _E) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_sshl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_sshl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_sshl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
          sop_sshl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
        :  \
          sop_ushl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_ushl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_ushl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
          sop_ushl(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_shrx3(_ptr, _A, _B, _C) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_sshr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_sshr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
        :  \
          sop_ushr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_ushr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_shrx4(_ptr, _A, _B, _C, _D) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_sshr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_sshr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_sshr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
        :  \
          sop_ushr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_ushr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_ushr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )

#define sop_shrx5(_ptr, _A, _B, _C, _D, _E) \
    (sop_assert(((void *)(sop_valueof_##_ptr)) != NULL) \
    ? \
      (sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
      sop_safe_cast(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
                     sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
      ? \
        (sop_signed_##_ptr \
        ? \
          sop_sshr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_sshr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_sshr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
          sop_sshr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
        :  \
          sop_ushr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_A, sop_typeof_##_A, sop_valueof_##_A, \
            sop_signed_##_B, sop_typeof_##_B, sop_valueof_##_B) && \
          sop_ushr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_C, sop_typeof_##_C, sop_valueof_##_C) && \
          sop_ushr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_D, sop_typeof_##_D, sop_valueof_##_D) && \
          sop_ushr(sop_signed_##_ptr, sop_typeof_##_ptr, sop_valueof_##_ptr, \
            sop_signed_##_ptr, sop_typeof_##_ptr, \
              (*(sop_typeof_##_ptr *)(sop_valueof_##_ptr)), \
            sop_signed_##_E, sop_typeof_##_E, sop_valueof_##_E) \
        ) \
      : \
        0 \
      ) \
    : \
      0 \
    )


/*****************************************************************************
 * GNU C interface macros
 *****************************************************************************
 * The macros below make use of two GNU C extensions:
 * - statement blocks as expressions ({ ... })
 * - typeof()
 * These functions act as convenience interfaces for GCC users, but lack the
 * breadth of functionality of the generic interface.
 * Benefits:
 * - side-effect-less macros: (sop_add(cur++, ...) is OK)
 * - no type markup: less work from you
 * Limitations:
 * - Casts to the type of the first operand (a) instead of the pointer
 * - Cannot handle types with special attributes, like 'const'
 *
 * Αs with the 'x' interfaces, _dst can be NULL.  However, this also extends to
 * the convenience functions like sop_add3() unlike in the generic interface.
 */

#if defined(__GNUC__)

/* Helpers for the GNUC interface */
#define OPAQUE_SAFE_IOP_PREFIX_MACRO_is_signed(__sA) \
  (OPAQUE_SAFE_IOP_PREFIX_MACRO_smin(typeof(__sA)) <= ((typeof(__sA))0))

/* Actual interface */
#define sop_add(_dst, _A, _B) ({ \
  /* Protect against side effects */ \
  typeof(_A) __sop(var)(_a) = (_A); \
  typeof(_B) __sop(var)(_b) = (_B); \
  typeof(_A) *__sop(var)(_ptr) = (_dst); \
  int __sop(var)(ok) =  \
    (sop_safe_cast(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                   __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b)) ? \
      ( __sop(m)(is_signed)(_A) ? \
          sop_sadd(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_ptr),\
                    __sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                    __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b)) \
        :  \
          sop_uadd(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_ptr), \
                    __sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                    __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b))) \
      : 0 ); \
   __sop(var)(ok); \
})

#define sop_sub(_dst, _A, _B) ({ \
  /* Protect against side effects */ \
  typeof(_A) __sop(var)(_a) = (_A); \
  typeof(_B) __sop(var)(_b) = (_B); \
  typeof(_A) *__sop(var)(_ptr) = (_dst); \
  int __sop(var)(ok) =  \
    (sop_safe_cast(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                   __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b)) ? \
      ( __sop(m)(is_signed)(_A) ? \
          sop_ssub(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_ptr),\
                    __sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                    __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b)) \
        :  \
          sop_usub(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_ptr), \
                    __sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                    __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b))) \
      : 0 ); \
   __sop(var)(ok); \
})

#define sop_mul(_dst, _A, _B) ({ \
  /* Protect against side effects */ \
  typeof(_A) __sop(var)(_a) = (_A); \
  typeof(_B) __sop(var)(_b) = (_B); \
  typeof(_A) *__sop(var)(_ptr) = (_dst); \
  int __sop(var)(ok) =  \
    (sop_safe_cast(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                   __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b)) ? \
      ( __sop(m)(is_signed)(_A) ? \
          sop_smul(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_ptr),\
                    __sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                    __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b)) \
        :  \
          sop_umul(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_ptr), \
                    __sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                    __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b))) \
      : 0 ); \
   __sop(var)(ok); \
})

#define sop_div(_dst, _A, _B) ({ \
  /* Protect against side effects */ \
  typeof(_A) __sop(var)(_a) = (_A); \
  typeof(_B) __sop(var)(_b) = (_B); \
  typeof(_A) *__sop(var)(_ptr) = (_dst); \
  int __sop(var)(ok) =  \
    (sop_safe_cast(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                   __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b)) ? \
      ( __sop(m)(is_signed)(_A) ? \
          sop_sdiv(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_ptr),\
                    __sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                    __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b)) \
        :  \
          sop_udiv(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_ptr), \
                    __sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                    __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b))) \
      : 0 ); \
   __sop(var)(ok); \
})

#define sop_mod(_dst, _A, _B) ({ \
  /* Protect against side effects */ \
  typeof(_A) __sop(var)(_a) = (_A); \
  typeof(_B) __sop(var)(_b) = (_B); \
  typeof(_A) *__sop(var)(_ptr) = (_dst); \
  int __sop(var)(ok) =  \
    (sop_safe_cast(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                   __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b)) ? \
      ( __sop(m)(is_signed)(_A) ? \
          sop_smod(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_ptr),\
                    __sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                    __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b)) \
        :  \
          sop_umod(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_ptr), \
                    __sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                    __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b))) \
      : 0 ); \
   __sop(var)(ok); \
})

#define sop_shl(_dst, _A, _B) ({ \
  /* Protect against side effects */ \
  typeof(_A) __sop(var)(_a) = (_A); \
  typeof(_B) __sop(var)(_b) = (_B); \
  typeof(_A) *__sop(var)(_ptr) = (_dst); \
  int __sop(var)(ok) =  \
    (sop_safe_cast(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                   __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b)) ? \
      ( __sop(m)(is_signed)(_A) ? \
          sop_sshl(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_ptr),\
                    __sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                    __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b)) \
        :  \
          sop_ushl(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_ptr), \
                    __sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                    __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b))) \
      : 0 ); \
   __sop(var)(ok); \
})

#define sop_shr(_dst, _A, _B) ({ \
  /* Protect against side effects */ \
  typeof(_A) __sop(var)(_a) = (_A); \
  typeof(_B) __sop(var)(_b) = (_B); \
  typeof(_A) *__sop(var)(_ptr) = (_dst); \
  int __sop(var)(ok) =  \
    (sop_safe_cast(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                   __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b)) ? \
      ( __sop(m)(is_signed)(_A) ? \
          sop_sshr(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_ptr),\
                    __sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                    __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b)) \
        :  \
          sop_ushr(__sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_ptr), \
                    __sop(m)(is_signed)(_A), typeof(_A), __sop(var)(_a), \
                    __sop(m)(is_signed)(_B), typeof(_B), __sop(var)(_b))) \
      : 0 ); \
   __sop(var)(ok); \
})

/* Helper macros for performing repeated operations in one call */
#define sop_add3(_ptr, _A, _B, _C) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_A) __sop(var)(r) = 0; \
   (sop_add(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
    sop_add((_ptr), __sop(var)(r), __sop(var)(c))); })

#define sop_add4(_ptr, _A, _B, _C, _D) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_D) __sop(var)(d) = (_D); \
   typeof(_A) __sop(var)(r) = 0; \
  (sop_add(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
   sop_add(&(__sop(var)(r)), __sop(var)(r), __sop(var)(c)) && \
   sop_add((_ptr), __sop(var)(r), (__sop(var)(d)))); })

#define sop_add5(_ptr, _A, _B, _C, _D, _E) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_D) __sop(var)(d) = (_D); \
   typeof(_E) __sop(var)(e) = (_E); \
   typeof(_A) __sop(var)(r) = 0; \
  (sop_add(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
   sop_add(&(__sop(var)(r)), __sop(var)(r), __sop(var)(c)) && \
   sop_add(&(__sop(var)(r)), __sop(var)(r), __sop(var)(d)) && \
   sop_add((_ptr), __sop(var)(r), __sop(var)(e))); })

/* These are sequentially performed */
#define sop_sub3(_ptr, _A, _B, _C) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_A) __sop(var)(r) = 0; \
   (sop_sub(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
    sop_sub((_ptr), __sop(var)(r), __sop(var)(c))); })

#define sop_sub4(_ptr, _A, _B, _C, _D) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_D) __sop(var)(d) = (_D); \
   typeof(_A) __sop(var)(r) = 0; \
  (sop_sub(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
   sop_sub(&(__sop(var)(r)), __sop(var)(r), __sop(var)(c)) && \
   sop_sub((_ptr), __sop(var)(r), (__sop(var)(d)))); })

#define sop_sub5(_ptr, _A, _B, _C, _D, _E) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_D) __sop(var)(d) = (_D); \
   typeof(_E) __sop(var)(e) = (_E); \
   typeof(_A) __sop(var)(r) = 0; \
   (sop_sub(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
    sop_sub(&(__sop(var)(r)), __sop(var)(r), __sop(var)(c)) && \
    sop_sub(&(__sop(var)(r)), __sop(var)(r), __sop(var)(d)) && \
    sop_sub((_ptr), __sop(var)(r), __sop(var)(e))); })


#define sop_mul3(_ptr, _A, _B, _C) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_A) __sop(var)(r) = 0; \
   (sop_mul(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
    sop_mul((_ptr), __sop(var)(r), __sop(var)(c))); })

#define sop_mul4(_ptr, _A, _B, _C, _D) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_D) __sop(var)(d) = (_D); \
   typeof(_A) __sop(var)(r) = 0; \
  (sop_mul(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
   sop_mul(&(__sop(var)(r)), __sop(var)(r), __sop(var)(c)) && \
   sop_mul((_ptr), __sop(var)(r), (__sop(var)(d)))); })

#define sop_mul5(_ptr, _A, _B, _C, _D, _E) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_D) __sop(var)(d) = (_D); \
   typeof(_E) __sop(var)(e) = (_E); \
   typeof(_A) __sop(var)(r) = 0; \
  (sop_mul(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
   sop_mul(&(__sop(var)(r)), __sop(var)(r), __sop(var)(c)) && \
   sop_mul(&(__sop(var)(r)), __sop(var)(r), __sop(var)(d)) && \
   sop_mul((_ptr), __sop(var)(r), __sop(var)(e))); })

#define sop_div3(_ptr, _A, _B, _C) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_A) __sop(var)(r) = 0; \
   (sop_div(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
    sop_div((_ptr), __sop(var)(r), __sop(var)(c))); })

#define sop_div4(_ptr, _A, _B, _C, _D) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_D) __sop(var)(d) = (_D); \
   typeof(_A) __sop(var)(r) = 0; \
  (sop_div(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
   sop_div(&(__sop(var)(r)), __sop(var)(r), __sop(var)(c)) && \
   sop_div((_ptr), __sop(var)(r), (__sop(var)(d)))); })

#define sop_div5(_ptr, _A, _B, _C, _D, _E) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_D) __sop(var)(d) = (_D); \
   typeof(_E) __sop(var)(e) = (_E); \
   typeof(_A) __sop(var)(r) = 0; \
  (sop_div(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
   sop_div(&(__sop(var)(r)), __sop(var)(r), __sop(var)(c)) && \
   sop_div(&(__sop(var)(r)), __sop(var)(r), __sop(var)(d)) && \
   sop_div((_ptr), __sop(var)(r), __sop(var)(e))); })

#define sop_mod3(_ptr, _A, _B, _C) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_A) __sop(var)(r) = 0; \
   (sop_mod(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
    sop_mod((_ptr), __sop(var)(r), __sop(var)(c))); })

#define sop_mod4(_ptr, _A, _B, _C, _D) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_D) __sop(var)(d) = (_D); \
   typeof(_A) __sop(var)(r) = 0; \
   (sop_mod(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
    sop_mod(&(__sop(var)(r)), __sop(var)(r), __sop(var)(c)) && \
    sop_mod((_ptr), __sop(var)(r), (__sop(var)(d)))); })

#define sop_mod5(_ptr, _A, _B, _C, _D, _E) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C), \
   typeof(_D) __sop(var)(d) = (_D); \
   typeof(_E) __sop(var)(e) = (_E); \
   typeof(_A) __sop(var)(r) = 0; \
   (sop_mod(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
    sop_mod(&(__sop(var)(r)), __sop(var)(r), __sop(var)(c)) && \
    sop_mod(&(__sop(var)(r)), __sop(var)(r), __sop(var)(d)) && \
    sop_mod((_ptr), __sop(var)(r), __sop(var)(e))); })

#define sop_shl3(_ptr, _A, _B, _C) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_A) __sop(var)(r) = 0; \
   (sop_shl(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
    sop_shl((_ptr), __sop(var)(r), __sop(var)(c))); })

#define sop_shl4(_ptr, _A, _B, _C, _D) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_D) __sop(var)(d) = (_D); \
   typeof(_A) __sop(var)(r) = 0; \
   (sop_shl(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
    sop_shl(&(__sop(var)(r)), __sop(var)(r), __sop(var)(c)) && \
    sop_shl((_ptr), __sop(var)(r), (__sop(var)(d)))); })

#define sop_shl5(_ptr, _A, _B, _C, _D, _E) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C), \
   typeof(_D) __sop(var)(d) = (_D); \
   typeof(_E) __sop(var)(e) = (_E); \
   typeof(_A) __sop(var)(r) = 0; \
   (sop_shl(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
    sop_shl(&(__sop(var)(r)), __sop(var)(r), __sop(var)(c)) && \
    sop_shl(&(__sop(var)(r)), __sop(var)(r), __sop(var)(d)) && \
    sop_shl((_ptr), __sop(var)(r), __sop(var)(e))); })

#define sop_shr3(_ptr, _A, _B, _C) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_A) __sop(var)(r) = 0; \
   (sop_shr(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
    sop_shr((_ptr), __sop(var)(r), __sop(var)(c))); })

#define sop_shr4(_ptr, _A, _B, _C, _D) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C); \
   typeof(_D) __sop(var)(d) = (_D); \
   typeof(_A) __sop(var)(r) = 0; \
   (sop_shr(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
    sop_shr(&(__sop(var)(r)), __sop(var)(r), __sop(var)(c)) && \
    sop_shr((_ptr), __sop(var)(r), (__sop(var)(d)))); })

#define sop_shr5(_ptr, _A, _B, _C, _D, _E) \
({ typeof(_A) __sop(var)(a) = (_A); \
   typeof(_B) __sop(var)(b) = (_B); \
   typeof(_C) __sop(var)(c) = (_C), \
   typeof(_D) __sop(var)(d) = (_D); \
   typeof(_E) __sop(var)(e) = (_E); \
   typeof(_A) __sop(var)(r) = 0; \
   (sop_shr(&(__sop(var)(r)), __sop(var)(a), __sop(var)(b)) && \
    sop_shr(&(__sop(var)(r)), __sop(var)(r), __sop(var)(c)) && \
    sop_shr(&(__sop(var)(r)), __sop(var)(r), __sop(var)(d)) && \
    sop_shr((_ptr), __sop(var)(r), __sop(var)(e))); })

#define sop_inc(_a)  sop_add(&(_a), (_a), 1)
#define sop_dec(_a)  sop_sub(&(_a), (_a), 1)

#endif /* __GNUC__ */

#endif  /* _SAFE_IOP_H */
