/* Copyright (C) 1998-2022 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library.  If not, see
   <https://www.gnu.org/licenses/>.  */

/* Thumb requires excessive IT insns here.  */
#define NO_THUMB
//#include <sysdep.h>
//#include <arm-features.h>

/*
 * Data preload for architectures that support it (ARM V5TE and above)
 */
#if (!defined (__ARM_ARCH_2__) && !defined (__ARM_ARCH_3__) \
     && !defined (__ARM_ARCH_3M__) && !defined (__ARM_ARCH_4__) \
     && !defined (__ARM_ARCH_4T__) && !defined (__ARM_ARCH_5__) \
     && !defined (__ARM_ARCH_5T__))
#define PLD(code...)    code
#else
#define PLD(code...)
#endif

/*
 * This can be used to enable code to cacheline align the source pointer.
 * Experiments on tested architectures (StrongARM and XScale) didn't show
 * this a worthwhile thing to do.  That might be different in the future.
 */
//#define CALGN(code...)        code
#define CALGN(code...)

/*
 * Endian independent macros for shifting bytes within registers.
 */
#ifndef __ARMEB__
#define PULL            lsr
#define PUSH            lsl
#else
#define PULL            lsl
#define PUSH            lsr
#endif

.syntax unified
.fpu neon
.text
.align 4
.global memset
.type memset, %function

/* void *memset (dstpp, c, len) */

//ENTRY(memset)
memset:
    mov     r3, r0              @ Save original pointer for return
    cmp     r2, #128             @ Only use PLD and NEON for large blocks
    bls     .L_small_copy       @ Fallback for small sizes

    @ 1. Prepare NEON register with the fill value
    uxtb    r1, r1              @ Ensure value is a single byte
    vdup.8  q0, r1              @ Broadcast byte to 16-byte Q0
    vmov    q1, q0              @ Duplicate across Q1 for 32-byte stores

    @ 2. Align destination pointer to 64-byte boundary for max performance
.L_align_to_64:
    tst     r3, #63
    beq     .L_pld_loop_prep
    strb    r1, [r3], #1
    subs    r2, r2, #1
    bne     .L_align_to_64

.L_pld_loop_prep:
    @ Initial prefetch distance (typically 2-3 cache lines ahead)
    pld     [r3, #64]
    pld     [r3, #128]

    @ 3. Main Loop: Store 64 bytes per iteration with PLD
.L_pld_loop:
    cmp     r2, #128            @ Ensure enough bytes for another prefetch
    blo     .L_neon_cleanup

   @ commented  pld     [r3, #192]           Preload 3 cache lines ahead
    vst1.8  {q0, q1}, [r3]!     @ Store first 32 bytes
    vst1.8  {q0, q1}, [r3]!     @ Store next 32 bytes
    subs    r2, r2, #64
    bhs     .L_pld_loop

    @ 4. Cleanup remaining NEON-sized blocks (no more prefetching)
.L_neon_cleanup:
    cmp     r2, #16
    blo     .L_small_copy
    vst1.8  {q0}, [r3]!
    subs    r2, r2, #16
    bhs     .L_neon_cleanup

    @ 5. Final byte-by-byte copy for remainder
.L_small_copy:
    @ Scalar path (copied from original sysdeps/arm/memset.S)
    @ r3 = dst, r1 = byte value (will be masked/expanded), r2 = len (<64 or tail)
    cmp     r2, #8
    bcc     2f                  @ less than 8 bytes to move

1:
    tst     r3, #3              @ aligned yet?
    strbne  r1, [r3], #1
    subne   r2, r2, #1
    bne     1b

    and     r1, r1, #255        @ clear any sign bits
    orr     r1, r1, r1, lsl $8
    orr     r1, r1, r1, lsl $16
    mov     ip, r1

1:
    subs    r2, r2, #8
    stmiacs r3!, {r1, ip}       @ store up to 32 bytes per loop iteration
    subscs  r2, r2, #8
    stmiacs r3!, {r1, ip}
    subscs  r2, r2, #8
    stmiacs r3!, {r1, ip}
    subscs  r2, r2, #8
    stmiacs r3!, {r1, ip}
    bcs     1b

    and     r2, r2, #7
2:
    subs    r2, r2, #1          @ store up to 4 bytes per loop iteration
    strbcs  r1, [r3], #1
    subscs  r2, r2, #1
    strbcs  r1, [r3], #1
    subscs  r2, r2, #1
    strbcs  r1, [r3], #1
    subscs  r2, r2, #1
    strbcs  r1, [r3], #1
    bcs     2b

.L_done:
    bx      lr                  @ Return original r0

//END(memset)
//libc_hidden_builtin_def (memset)
