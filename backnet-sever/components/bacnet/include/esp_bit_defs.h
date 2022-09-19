/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once


#define BIT63    (0x80000000ULL << 32)
#define BIT62    (0x40000000ULL << 32)
#define BIT61    (0x20000000ULL << 32)
#define BIT60    (0x10000000ULL << 32)
#define BIT59    (0x08000000ULL << 32)
#define BIT58    (0x04000000ULL << 32)
#define BIT57    (0x02000000ULL << 32)
#define BIT56    (0x01000000ULL << 32)
#define BIT55    (0x00800000ULL << 32)
#define BIT54    (0x00400000ULL << 32)
#define BIT53    (0x00200000ULL << 32)
#define BIT52    (0x00100000ULL << 32)
#define BIT51    (0x00080000ULL << 32)
#define BIT50    (0x00040000ULL << 32)
#define BIT49    (0x00020000ULL << 32)
#define BIT48    (0x00010000ULL << 32)
#define BIT47    (0x00008000ULL << 32)
#define BIT46    (0x00004000ULL << 32)
#define BIT45    (0x00002000ULL << 32)
#define BIT44    (0x00001000ULL << 32)
#define BIT43    (0x00000800ULL << 32)
#define BIT42    (0x00000400ULL << 32)
#define BIT41    (0x00000200ULL << 32)
#define BIT40    (0x00000100ULL << 32)
#define BIT39    (0x00000080ULL << 32)
#define BIT38    (0x00000040ULL << 32)
#define BIT37    (0x00000020ULL << 32)
#define BIT36    (0x00000010ULL << 32)
#define BIT35    (0x00000008ULL << 32)
#define BIT34    (0x00000004ULL << 32)
#define BIT33    (0x00000002ULL << 32)
#define BIT32    (0x00000001ULL << 32)

#ifndef __ASSEMBLER__
#ifndef BIT
#define BIT(nr)                 (1UL << (nr))
#endif
#ifndef BIT64
#define BIT64(nr)               (1ULL << (nr))
#endif
#else
#ifndef BIT
#define BIT(nr)                 (1 << (nr))
#endif
#endif
