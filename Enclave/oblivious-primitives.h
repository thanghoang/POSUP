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
	/*
	 * ocmp_eq(uint64_t input1, uint64_t input2);
	 *
	 * returns:
	 *      1           if input1 == input2
	 *      0           if input1 != input2
	 *
	 */
	uint32_t __declspec(noinline)
	ocmp_eq_32(uint32_t input1, uint32_t input2) {
		uint32_t ret_value, one = 1u, zero = 0u;
		__asm {
			mov eax, input1
			cmp eax, input2
			cmove eax, one
			cmovne eax, zero
			mov ret_value, eax
		}
		return ret_value;
	}
	uint64_t __declspec(noinline)
	ocmp_eq(uint64_t input1, uint64_t input2) {
		uint64_t ret_value, one = 1ul, zero = 0ul;
		ret_value = ocmp_eq_32((uint32_t)input1 & 0xffffffff, (uint32_t)input2 & 0xffffffff);
		ret_value &= ocmp_eq_32((uint32_t)(input1 >> 32), (uint32_t)(input2 >> 32));
		return ret_value;
	}

	/*
	 * ocmp_ne(uint64_t input1, uint64_t input2);
	 *
	 * returns:
	 *      1           if input1 != input2
	 *      0           if input1 == input2
	 */
	uint32_t __declspec(noinline)
	ocmp_ne_32(uint32_t input1, uint32_t input2) {
		uint32_t ret_value, one = 1u, zero = 0u;
		__asm {
			mov eax, input1
			cmp eax, input2
			cmove eax, zero
			cmovne eax, one
			mov ret_value, eax
		}
		return ret_value;
	}
	uint64_t __declspec(noinline)
	ocmp_ne(uint64_t input1, uint64_t input2) {
		uint64_t ret_value, one = 1ul, zero = 0ul;
		ret_value = ocmp_ne_32((uint32_t)input1 & 0xffffffff, (uint32_t)input2 & 0xffffffff);
		ret_value |= ocmp_ne_32((uint32_t)(input1 >> 32), (uint32_t)(input2 >> 32));
		return ret_value;
	}

	// Signed comparision
	/*
	 * ocmp_g(int64_t input1, int64_t input2);
	 *
	 * returns:
	 *      1           if input1 >  input2
	 *      0           if input1 <= input2
	 */
	uint32_t __declspec(noinline)
	ocmp_g(int32_t input1, int32_t input2) {
		uint32_t ret_value, one = 1u, zero = 0u;
		__asm {
			mov eax, input1
			cmp eax, input2
			cmovle eax, zero
			cmovg eax, one
			mov ret_value, eax
		}
		return ret_value;
	}

	// Signed comparison
	/*
	 * ocmp_ge(int64_t input1, int64_t input2);
	 *
	 * returns:
	 *      1           if input1 >= input2
	 *      0           if input1 <  input2
	 */
	uint32_t __declspec(noinline)
	ocmp_ge(int32_t input1, int32_t input2) {
		uint32_t ret_value, one = 1u, zero = 0u;
		__asm {
			mov eax, input1
			cmp eax, input2
			cmovl eax, zero
			cmovge eax, one
			mov ret_value, eax
		}
		return ret_value;
	}

	// Unsigned comparison
	/*
	 * ocmp_a(uint64_t input1, uint64_t input2);
	 *
	 * returns:
	 *      1           if input1 >  input2
	 *      0           if input1 <= input2
	 */
	uint32_t __declspec(noinline)
	ocmp_a(uint32_t input1, uint32_t input2) {
		uint32_t ret_value, one = 1u, zero = 0u;
		__asm {
			mov eax, input1
			cmp eax, input2
			cmovbe eax, zero
			cmova eax, one
			mov ret_value, eax
		}
		return ret_value;
	}

	// Unsigned comparison
	/*
	 * ocmp_ae(uint64_t input1, uint64_t input2);
	 *
	 * returns:
	 *      1           if input1 >= input2
	 *      0           if input1 <  input2
	 */
	uint32_t __declspec(noinline)
	ocmp_ae(uint32_t input1, uint32_t input2) {
		uint32_t ret_value, one = 1u, zero = 0u;
		__asm {
			mov eax, input1
			cmp eax, input2
			cmovb eax, zero
			cmovae eax, one
			mov ret_value, eax
		}
		return ret_value;
	}

	/*
	 * o_memcpy_4(uint32_t is_valid_copy, void* dst, void* src, size_t size)
	 *
	 * copy src -> dst if pred == 1
	 * copy src -> src if pred == 0
	 * copy exactly multipe of 4 bytes (not handing if size % 8 != 0)
	 * always returns size.
	 */
	size_t __declspec(noinline)
	o_memcpy_4(uint32_t is_valid_copy, void* dst, void* src, size_t size) {
		size_t cnt = (size >> 2);
		__asm {
			// save registers (a,b,c,d,S,D)
			push   eax;	  // cnt
			push   ebx;   // temp
			push   ecx;   // loop index
			push   edx;   // is_valid_copy
			push   esi;   // src
			push   edi;   // dst

			// push all values
			push is_valid_copy;
			push dst;
			push src;
			push cnt;
			push 0;			// loop idx

			// set registers accordingly
			pop ecx;
			pop eax;		// cnt
			pop esi;		// src
			pop edi;		// dst


		// loop head
		o_memcpy_4_loop_start:
			// loop exit condition
			cmp ecx, eax;
			je o_memcpy_4_loop_out;

			// get is_valid_copy and test
			mov edx, [esp];
			test edx, edx;

			// load from dst
			mov ebx, [edi + ecx * 4];
			// store dest to edx if is_valid_copy == 0
			cmovz  edx, ebx;

			// load from src
			mov ebx, [esi + ecx * 4];
			// store src to edx if is_valid_copy == 1
			cmovnz edx, ebx;

			// store edx to dst
			mov [edi + ecx * 4], edx;

			// increase the loop ctr
			inc ecx;
			jmp o_memcpy_4_loop_start;

		o_memcpy_4_loop_out:
			pop edi;
			pop edi;
			pop esi;
			pop edx;
			pop ecx;
			pop ebx;
			pop eax;
		}
		return size;
	}
	
	/*
	 * o_memcpy_byte(uint32_t is_valid_copy, void* dst, void* src, size_t _size)
	 *
	 * copy src -> dst if pred == 1
	 * copy src -> src if pred == 0
	 * copy by each byte.
	 * always returns size.
	 */
	size_t __declspec(noinline)
	o_memcpy_byte(uint32_t is_valid_copy, void* dst, void* src, size_t _size) {
		__asm {
			// save registers (a,b,c,d,S,D)
			push   eax;	  // cnt
			push   ebx;   // temp
			push   ecx;   // loop index
			push   edx;   // is_valid_copy
			push   esi;   // src
			push   edi;   // dst

			// push all values
			push is_valid_copy;
			push dst;
			push src;
			push _size;
			push 0;			// loop idx

			// set registers accordingly
			pop ecx;
			pop eax;		// cnt
			pop esi;		// src
			pop edi;		// dst


		// loop head
		o_memcpy_byte_loop_start:
			// loop exit condition
			cmp ecx, _size;
			je o_memcpy_byte_loop_out;

			// get is_valid_copy and test
			mov edx, [esp];
			test edx, edx;

			// load from dst
			mov bl, [edi + ecx * 1];
			// store dest to edx if is_valid_copy == 0
			cmovz  edx, ebx;

			// load from src
			mov bl, [esi + ecx * 1];
			// store src to edx if is_valid_copy == 1
			cmovnz edx, ebx;

			// store edx to dst
			mov [edi + ecx * 1], dl;

			// increase the loop ctr
			inc ecx;
			jmp o_memcpy_byte_loop_start;

		o_memcpy_byte_loop_out:
			pop edi;
			pop edi;
			pop esi;
			pop edx;
			pop ecx;
			pop ebx;
			pop eax;
		}
		return _size;
	}
	
	size_t o_memcpy(uint32_t pred, void* dst, void* src, size_t size) {
		size_t mult_size = (size >> 2) << 2;
		size_t remaining_size = size & 3;
		size_t copied = 0;
		if (mult_size != 0) {
			copied += o_memcpy_4(pred, dst, src, mult_size);
		}

		if (remaining_size) {
			uint8_t* c_dst = (uint8_t*)dst;
			uint8_t* c_src = (uint8_t*)src;
			copied += o_memcpy_byte(pred, c_dst + mult_size, c_src + mult_size, remaining_size);
		}
		return copied;
	}

	/*
	 * o_memset_byte(uint64_t pred, void *dst, uint8_t val, size_t size);
	 *
	 * set each byte of dst = val		if pred == 1
	 * set each byte of dst as dst[i]	if pred == 0
	 * always returns size.
	 */
	size_t __declspec(noinline)
	o_memset_byte(uint32_t is_valid_set, void* dst, uint32_t val, size_t _size) {
		__asm {
			// save registers (a,b,c,d,S,D)
			push   eax;	  // cnt
			push   ebx;   // temp
			push   ecx;   // loop index
			push   edx;   // is_valid_copy
			push   esi;   // src
			push   edi;   // dst

			// push all values
			push is_valid_set;
			push dst;
			push val;
			push _size;
			push 0;			// loop idx

			// set registers accordingly
			pop ecx;
			pop eax;		// cnt
			pop esi;		// src
			pop edi;		// dst


		// loop head
		o_memset_byte_loop_start:
			// loop exit condition
			cmp ecx, _size;
			je o_memset_byte_loop_out;

			// get is_valid_copy and test
			mov edx, [esp];
			test edx, edx;

			// load from dst
			mov bl, [edi + ecx * 1];
			// store dest to edx if is_valid_set == 0
			cmovz  edx, ebx;

			// store val to edx if is_valid_set == 1
			cmovnz edx, esi;

			// store edx to dst
			mov[edi + ecx * 1], dl;

			// increase the loop ctr
			inc ecx;
			jmp o_memset_byte_loop_start;

		o_memset_byte_loop_out:
			pop edi;
			pop edi;
			pop esi;
			pop edx;
			pop ecx;
			pop ebx;
			pop eax;
		}
		return _size;
	}

	/*
	 * o_memset_4(uint32_t is_valid_set, void* dst, uint32_t val, size_t size
	 *
	 * set each byte of dst = val		if pred == 1
	 * set each byte of dst as dst[i]	if pred == 0
	 * set 8byte value per each iteration.
	 * always returns size.
	 */
	size_t __declspec(noinline)
	o_memset_4(uint32_t is_valid_set, void* dst, uint32_t val, size_t size) {
		size_t cnt = (size >> 2);
		__asm {
			// save registers (a,b,c,d,S,D)
			push   eax;	  // cnt
			push   ebx;   // temp
			push   ecx;   // loop index
			push   edx;   // is_valid_copy
			push   esi;   // src
			push   edi;   // dst

			// push all values
			push is_valid_set;
			push dst;
			push val;
			push cnt;
			push 0;			// loop idx

			// set registers accordingly
			pop ecx;
			pop eax;		// cnt
			pop esi;		// src
			pop edi;		// dst


		// loop head
		o_memset_4_loop_start:
			// loop exit condition
			cmp ecx, eax;
			je o_memset_4_loop_out;

			// get is_valid_copy and test
			mov edx, [esp];
			test edx, edx;

			// load from dst
			mov ebx, [edi + ecx * 4];
			// store dest to edx if is_valid_set == 0
			cmovz  edx, ebx;

			// store val to edx if is_valid_set == 1
			cmovnz edx, esi;

			// store edx to dst
			mov [edi + ecx * 4], edx;

			// increase the loop ctr
			inc ecx;
			jmp o_memset_4_loop_start;

		o_memset_4_loop_out:
			pop edi;
			pop edi;
			pop esi;
			pop edx;
			pop ecx;
			pop ebx;
			pop eax;
		}
		return size;

	}
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
	uint64_t __attribute__((noinline))
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
	uint64_t __attribute__((noinline))
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
	uint64_t __attribute__((noinline))
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
	uint64_t __attribute__((noinline))
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
	uint64_t __attribute__((noinline))
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
	uint64_t __attribute__((noinline))
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
	 * copy src -> src if pred == 0
	 * copy exactly multipe of 8 bytes (not handing if size % 8 != 0)
	 * always returns size.
	 */
	size_t __attribute__((noinline))
		o_memcpy_8(uint64_t is_valid_copy, void* dst, void* src, size_t size) {
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

	/*
	 * o_memcpy_byte(uint64_t pred, void *dst, void *src, size_t size);
	 *
	 * copy src -> dst if pred == 1
	 * copy src -> src if pred == 0
	 * copy by each byte.
	 * always returns size.
	 */
	size_t __attribute__((noinline))
		o_memcpy_byte(uint64_t is_valid_copy, void* dst, void* src, size_t size) {
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

	/*
	 * o_memset_byte(uint64_t pred, void *dst, uint8_t val, size_t size);
	 *
	 * set each byte of dst = val		if pred == 1
	 * set each byte of dst as dst[i]	if pred == 0
	 * always returns size.
	 */
	size_t __attribute__((noinline))
		o_memset_byte(uint64_t is_valid_set, void* dst, uint8_t val, size_t size) {
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

	/*
	 * o_memset_8(uint64_t pred, void *dst, uint64_t val, size_t size);
	 *
	 * set each byte of dst = val		if pred == 1
	 * set each byte of dst as dst[i]	if pred == 0
	 * set 8byte value per each iteration.
	 * always returns size.
	 */
	size_t __attribute__((noinline))
		o_memset_8(uint64_t is_valid_set, void* dst, uint64_t val, size_t size) {
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