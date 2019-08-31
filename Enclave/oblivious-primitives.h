#pragma once

#ifndef __OBLIVIOUS_PRIMITIVES_H__
#define __OBLIVIOUS_PRIMITIVES_H__

#include <stdint.h>
#include <stdio.h>

// C++
#if defined(__cplusplus)
extern "C" {
#endif

// visual studio
#ifdef _MSC_VER




// Linux and others
#else
/*
 * ocmp_eq(uint64_t input1, uint64_t input2);
 *
 * returns:
 *      1           if input1 == input2
 *      0           if input1 != input2
 *
 */
uint64_t __attribute__ ((noinline))
ocmp_eq(uint64_t input1, uint64_t input2) {
    uint64_t ret, one = 1ul, zero = 0ul;
	asm volatile(
            "cmp %[i2], %[i1];"
            "cmove %[one], %[ret];"
            "cmovne %[zero], %[ret];"
            : [ret] "=a" (ret)
            : [i1] "r" (input1),
              [i2] "r" (input2),
              [one] "r" (one),
              [zero] "r" (zero)
            );
    return ret;
}

/*
 * ocmp_ne(uint64_t input1, uint64_t input2);
 *
 * returns:
 *      1           if input1 != input2
 *      0           if input1 == input2
 */
uint64_t __attribute__ ((noinline))
ocmp_ne(uint64_t input1, uint64_t input2) {
    uint64_t ret, one = 1ul, zero = 0ul;
	asm volatile(
            "cmp %[i2], %[i1];"
            "cmove %[zero], %[ret];"
            "cmovne %[one], %[ret];"
            : [ret] "=r" (ret)
            : [i1] "r" (input1),
              [i2] "r" (input2),
              [one] "r" (one),
              [zero] "r" (zero)
            );
    return ret;
}

// Signed comparision
/*
 * ocmp_g(int64_t input1, int64_t input2);
 *
 * returns:
 *      1           if input1 >  input2
 *      0           if input1 <= input2
 */
uint64_t __attribute__ ((noinline))
ocmp_g(int64_t input1, int64_t input2) {
    uint64_t ret, one = 1ul, zero = 0ul;
	asm volatile(
            "cmp %[i2], %[i1];"
            "cmovle %[zero], %[ret];"
            "cmovg %[one], %[ret];"
            : [ret] "=r" (ret)
            : [i1] "r" (input1),
              [i2] "r" (input2),
              [one] "r" (one),
              [zero] "r" (zero)
            );
    return ret;
}

// Signed comparison
/*
 * ocmp_ge(int64_t input1, int64_t input2);
 *
 * returns:
 *      1           if input1 >= input2
 *      0           if input1 <  input2
 */
uint64_t __attribute__ ((noinline))
ocmp_ge(int64_t input1, int64_t input2) {
    uint64_t ret, one = 1ul, zero = 0ul;
	asm volatile(
            "cmp %[i2], %[i1];"
            "cmovl %[zero], %[ret];"
            "cmovge %[one], %[ret];"
            : [ret] "=r" (ret)
            : [i1] "r" (input1),
              [i2] "r" (input2),
              [one] "r" (one),
              [zero] "r" (zero)
            );
    return ret;
}

// Unsigned comparison
/*
 * ocmp_a(uint64_t input1, uint64_t input2);
 *
 * returns:
 *      1           if input1 >  input2
 *      0           if input1 <= input2
 */
uint64_t __attribute__ ((noinline))
ocmp_a(uint64_t input1, uint64_t input2) {
    uint64_t ret, one = 1ul, zero = 0ul;
	asm volatile(
            "cmp %[i2], %[i1];"
            "cmovbe %[zero], %[ret];"
            "cmova %[one], %[ret];"
            : [ret] "=r" (ret)
            : [i1] "r" (input1),
              [i2] "r" (input2),
              [one] "r" (one),
              [zero] "r" (zero)
            );
    return ret;
}

// Unsigned comparison
/*
 * ocmp_ae(uint64_t input1, uint64_t input2);
 *
 * returns:
 *      1           if input1 >= input2
 *      0           if input1 <  input2
 */
uint64_t __attribute__ ((noinline))
ocmp_ae(uint64_t input1, uint64_t input2) {
    uint64_t ret, one = 1ul, zero = 0ul;
	asm volatile(
            "cmp %[i2], %[i1];"
            "cmovb %[zero], %[ret];"
            "cmovae %[one], %[ret];"
            : [ret] "=r" (ret)
            : [i1] "r" (input1),
              [i2] "r" (input2),
              [one] "r" (one),
              [zero] "r" (zero)
            );
    return ret;
}

/*
 * o_memcpy_8(uint64_t pred, void *dst, void *src, size_t size);
 *
 * copy src -> dst if pred == 1
 * copy src -> src if pred == 1
 * copy exactly multipe of 8 bytes (not handing if size % 8 != 0)
 * always returns size.
 */
size_t __attribute__ ((noinline))
o_memcpy_8(uint64_t is_valid_copy, void *dst, void *src, size_t size) {
    size_t cnt = (size >> 3);
    asm volatile(
            // save registers; r15 = loop cnt, r14 = temp reg
            "push   %%r15;"
            "push   %%r14;"
            "push   %%r13;"   // src
            "push   %%r12;"   // dst

            // r15 is loop cnt
            "xor    %%r15, %%r15;"

            // loop head
            "loop_start%=:"
            "cmp    %%r15, %[cnt];"
            // loop exit condition
            "je out%=;"
            "mov    (%[dst], %%r15, 8), %%r12;"
            "mov    (%[src], %%r15, 8), %%r13;"

            // copy src (r13) if 1 (nz), copy dst (r12) if 0 (z)
            "test   %[pred], %[pred];"
            // copy dst to r14
            "cmovz  %%r12, %%r14;"
            // copy src to r14
            "cmovnz %%r13, %%r14;"
            // store to dst
            "mov    %%r14, (%[dst], %%r15, 8);"
            "inc    %%r15;"
            // loop back!
            "jmp    loop_start%=;"

            "out%=:"
            // restore registers
            "pop    %%r12;"
            "pop    %%r13;"
            "pop    %%r14;"
            "pop    %%r15;"
            :
            : [dst] "r" (dst),
              [src] "r" (src),
              [cnt] "r" (cnt),
              [pred] "r" (is_valid_copy)
            : "memory");
    return size;
}

size_t __attribute__ ((noinline))
o_memcpy_byte(uint64_t is_valid_copy, void *dst, void *src, size_t size) {
    asm volatile(
            // save registers; r15 = loop cnt, r14 = temp reg
            "push   %%r15;"
            "push   %%r14;"
            "push   %%r13;"   // src
            "push   %%r12;"   // dst

            // r15 is loop cnt
            "xor    %%r15, %%r15;"

            // loop head
            "loop_start%=:"
            "cmp    %%r15, %[cnt];"
            // loop exit condition
            "je out%=;"
            "movb   (%[dst], %%r15, 1), %%r12b;"
            "movb   (%[src], %%r15, 1), %%r13b;"

            // copy src (r13) if 1 (nz), copy dst (r12) if 0 (z)
            "test   %[pred], %[pred];"
            // copy dst to r14
            "cmovz  %%r12, %%r14;"
            // copy src to r14
            "cmovnz %%r13, %%r14;"
            // store to dst
            "movb   %%r14b, (%[dst], %%r15, 1);"
            "inc    %%r15;"
            // loop back!
            "jmp    loop_start%=;"

            "out%=:"
            // restore registers
            "pop    %%r12;"
            "pop    %%r13;"
            "pop    %%r14;"
            "pop    %%r15;"
            :
            : [dst] "r" (dst),
              [src] "r" (src),
              [cnt] "r" (size),
              [pred] "r" (is_valid_copy)
            : "memory");
    return size;
}

size_t __attribute__ ((noinline))
o_memset_byte(uint64_t is_valid_set, void *dst, uint8_t val, size_t size) {
    asm volatile(
            // save registers; r15 = loop cnt, r14 = temp reg
            "push   %%r15;"
            "push   %%r14;"
            "push   %%r13;"   // val
            "push   %%r12;"   // dst

            // r15 is loop cnt
            "xor    %%r15, %%r15;"

            // loop head
            "loop_start%=:"
            "cmp    %%r15, %[cnt];"
            // loop exit condition
            "je out%=;"
            "movb   (%[dst], %%r15, 1), %%r12b;"
            "movb   %[val], %%r13b;"

            // copy val (r13) if 1 (nz), copy dst (r12) if 0 (z)
            "test   %[pred], %[pred];"
            // copy dst to r14
            "cmovz  %%r12, %%r14;"
            // copy val to r14
            "cmovnz %%r13, %%r14;"
            // store to dst
            "movb   %%r14b, (%[dst], %%r15, 1);"
            "inc    %%r15;"
            // loop back!
            "jmp    loop_start%=;"

            "out%=:"
            // restore registers
            "pop    %%r12;"
            "pop    %%r13;"
            "pop    %%r14;"
            "pop    %%r15;"
            :
            : [dst] "r" (dst),
              [val] "r" (val),
              [cnt] "r" (size),
              [pred] "r" (is_valid_set)
            : "memory");
    return size;
}

size_t __attribute__ ((noinline))
o_memset_8(uint64_t is_valid_set, void *dst, uint64_t val, size_t size) {
    size_t cnt = (size >> 3);
    asm volatile(
            // save registers; r15 = loop cnt, r14 = temp reg
            "push   %%r15;"
            "push   %%r14;"
            "push   %%r13;"   // val
            "push   %%r12;"   // dst

            // r15 is loop cnt
            "xor    %%r15, %%r15;"

            // loop head
            "loop_start%=:"
            "cmp    %%r15, %[cnt];"
            // loop exit condition
            "je out%=;"
            "movq   (%[dst], %%r15, 8), %%r12;"
            "movq   %[val], %%r13;"

            // copy val (r13) if 1 (nz), copy dst (r12) if 0 (z)
            "test   %[pred], %[pred];"
            // copy dst to r14
            "cmovz  %%r12, %%r14;"
            // copy val to r14
            "cmovnz %%r13, %%r14;"
            // store to dst
            "movq   %%r14, (%[dst], %%r15, 8);"
            "inc    %%r15;"
            // loop back!
            "jmp    loop_start%=;"

            "out%=:"
            // restore registers
            "pop    %%r12;"
            "pop    %%r13;"
            "pop    %%r14;"
            "pop    %%r15;"
            :
            : [dst] "r" (dst),
              [val] "r" (val),
              [cnt] "r" (cnt),
              [pred] "r" (is_valid_set)
            : "memory");
    return size;

}
#endif


// C++ end
#if defined(__cplusplus)
}
#endif

// include guard
#endif
