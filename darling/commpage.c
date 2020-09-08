/*
 * Darling Mach Linux Kernel Module
 * Copyright (C) 2017 Lubos Dolezel
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <duct/compiler/clang/asm-inline.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/cpumask.h>
#include <linux/mm.h>
#include <asm/processor.h>
#include "commpage.h"

// Include commpage definitions
#include "../osfmk/i386/cpu_capabilities.h"

static const char* SIGNATURE32 = "commpage 32-bit";
static const char* SIGNATURE64 = "commpage 64-bit";

static uint64_t get_cpu_caps(void);

#define CGET(p) (commpage + ((p)-_COMM_PAGE_START_ADDRESS))
#define __cpuid(level, eax, ebx, ecx, edx) \
	eax = level; \
	native_cpuid(&(eax), &(ebx), &(ecx), &(edx))

void* commpage_setup(bool _64bit)
{
	uint8_t* commpage;
	uint64_t* cpu_caps64;
	uint32_t* cpu_caps;
	uint16_t* version;
	char* signature;
	uint64_t my_caps;
	uint8_t *ncpus, *nactivecpus;
	uint8_t *physcpus, *logcpus;

	commpage = vmalloc_user(_64bit ? _COMM_PAGE64_AREA_LENGTH : _COMM_PAGE32_AREA_LENGTH);
	if (!commpage)
		return NULL;

	signature = (char*)CGET(_COMM_PAGE_SIGNATURE);
	version = (uint16_t*)CGET(_COMM_PAGE_VERSION);
	cpu_caps64 = (uint64_t*)CGET(_COMM_PAGE_CPU_CAPABILITIES64);
   	cpu_caps = (uint32_t*)CGET(_COMM_PAGE_CPU_CAPABILITIES);

	strcpy(signature, _64bit ? SIGNATURE64 : SIGNATURE32);
	*version = _COMM_PAGE_THIS_VERSION;

	ncpus = (uint8_t*)CGET(_COMM_PAGE_NCPUS);
	*ncpus = num_online_cpus();

	nactivecpus = (uint8_t*)CGET(_COMM_PAGE_ACTIVE_CPUS);
	*nactivecpus = num_active_cpus();

	// Better imprecise information than no information
	physcpus = (uint8_t*)CGET(_COMM_PAGE_PHYSICAL_CPUS);
	logcpus = (uint8_t*)CGET(_COMM_PAGE_LOGICAL_CPUS);
	*physcpus = *logcpus = *ncpus;

	my_caps = get_cpu_caps();
	if (*ncpus == 1)
		my_caps |= kUP;

	*cpu_caps = (uint32_t) my_caps;
	*cpu_caps64 = my_caps;

	uint64_t* memsize = (uint64_t*)CGET(_COMM_PAGE_MEMORY_SIZE);
	*memsize = get_num_physpages() * PAGE_SIZE;

	return commpage;
}

unsigned long commpage_length(bool _64bit)
{
	return _64bit ? _COMM_PAGE64_AREA_LENGTH : _COMM_PAGE32_AREA_LENGTH;
}

unsigned long commpage_address(bool _64bit)
{
	return _64bit ? _COMM_PAGE64_BASE_ADDRESS : _COMM_PAGE32_BASE_ADDRESS;
}

void commpage_free(void* p)
{
	vfree(p);
}

uint64_t get_cpu_caps(void)
{
	uint64_t caps = 0;

	{
		union cpu_flags1 eax;
		union cpu_flags2 ecx;
		union cpu_flags3 edx;
		uint32_t ebx;

		eax.reg = ecx.reg = edx.reg = 0;
		__cpuid(1, eax.reg, ebx, ecx.reg, edx.reg);

		if (ecx.mmx)
			caps |= kHasMMX;
		if (ecx.sse)
			caps |= kHasSSE;
		if (ecx.sse2)
			caps |= kHasSSE2;
		if (edx.sse3)
			caps |= kHasSSE3;
		if (ecx.ia64)
			caps |= k64Bit;
		if (edx.ssse3)
			caps |= kHasSupplementalSSE3;
		if (edx.sse41)
			caps |= kHasSSE4_1;
		if (edx.sse42)
			caps |= kHasSSE4_2;
		if (edx.aes)
			caps |= kHasAES;
		if (edx.avx)
			caps |= kHasAVX1_0;
		if (edx.rdrnd)
			caps |= kHasRDRAND;
		if (edx.fma)
			caps |= kHasFMA;
		if (edx.f16c)
			caps |= kHasF16C;
	}
	{
		union cpu_flags4 ebx;
		union cpu_flags5 ecx;
		uint32_t edx = 0, eax = 7;

		__cpuid(7, eax, ebx.reg, ecx.reg, edx);

		if (ebx.erms)
			caps |= kHasENFSTRG;
		if (ebx.avx2)
			caps |= kHasAVX2_0;
		if (ebx.bmi1)
			caps |= kHasBMI1;
		if (ebx.bmi2)
			caps |= kHasBMI2;
		if (ebx.rtm)
			caps |= kHasRTM;
		if (ebx.hle)
			caps |= kHasHLE;
		if (ebx.rdseed)
			caps |= kHasRDSEED;
		if (ebx.adx)
			caps |= kHasADX;
		if (ebx.mpx)
			caps |= kHasMPX;
		if (ebx.sgx)
			caps |= kHasSGX;
	}

	return caps;
}

