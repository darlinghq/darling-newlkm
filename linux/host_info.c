/*
 * Darling Mach Linux Kernel Module
 * Copyright (C) 2015-2017 Lubos Dolezel
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

#include <duct/duct.h>
#include <linux/string.h>
#include <linux/cpumask.h>
#include <linux/mm.h>
#include <duct/duct_pre_xnu.h>
#include <mach/mach_types.h>
#include <duct/duct_post_xnu.h>

kern_return_t darling_host_info(host_flavor_t flavor, host_info_t host_info_out, mach_msg_type_number_t* host_info_outCnt)
{
	switch (flavor)
	{
		case HOST_BASIC_INFO:
		{
			struct host_basic_info* hinfo;
			struct sysinfo i;

			hinfo = (struct host_basic_info*) host_info_out;

			if (*host_info_outCnt < HOST_BASIC_INFO_OLD_COUNT)
				return KERN_FAILURE;
			
			memset(host_info_out, 0, *host_info_outCnt * sizeof(int));

			si_meminfo(&i);
			hinfo->memory_size = i.totalram * PAGE_SIZE;
			hinfo->max_cpus = num_possible_cpus();
			hinfo->avail_cpus = num_online_cpus();
#if defined(__i386__)
			hinfo->cpu_type = CPU_TYPE_I386;
			hinfo->cpu_subtype = CPU_SUBTYPE_I386_ALL;
#elif defined(__x86_64__)
			if (!test_thread_flag(TIF_IA32))
			{
				hinfo->cpu_type = CPU_TYPE_I386;
				hinfo->cpu_subtype = CPU_SUBTYPE_X86_64_ALL;
			}
			else
			{
				hinfo->cpu_type = CPU_TYPE_I386;
				hinfo->cpu_subtype = CPU_SUBTYPE_I386_ALL;
			}
#else
			hinfo->cpu_type = 0;
			hinfo->cpu_subtype = 0;
#endif
			hinfo->cpu_threadtype = 0;

			if (*host_info_outCnt <= HOST_BASIC_INFO_COUNT)
			{
				int pos, last_phys_id = -1;
				
				*host_info_outCnt = HOST_BASIC_INFO_COUNT;
				hinfo->max_mem = i.totalram * PAGE_SIZE;
				hinfo->logical_cpu = 0;
				hinfo->physical_cpu = 0;
				
				for (pos = 0;; pos++)
				{
					pos = cpumask_next(pos - 1, cpu_online_mask);
					if (pos >= nr_cpu_ids)
						break;
					
#if defined(__i386__) || defined(__x86_64__)
					{
						struct cpuinfo_x86* cpu;
						cpu = &cpu_data(pos);
						
						hinfo->logical_cpu++;
						
						if (cpu->phys_proc_id == last_phys_id)
							continue;
						
						last_phys_id = cpu->phys_proc_id;
						hinfo->physical_cpu += cpu->booted_cores;
					}
#endif
				}
				
				hinfo->logical_cpu_max = hinfo->logical_cpu;
				hinfo->physical_cpu_max = hinfo->physical_cpu;
			}
			else
				*host_info_outCnt = HOST_BASIC_INFO_OLD_COUNT;
			
			return KERN_SUCCESS;
		}
		//case HOST_PROCESSOR_SLOTS:
		//	break;
		case HOST_SCHED_INFO:
			break;
		case HOST_PRIORITY_INFO:
			if (*host_info_outCnt < HOST_PRIORITY_INFO_COUNT)
				return KERN_FAILURE;
			
			// Dummy data - called from pthread_init())
			memset(host_info_out, 0, sizeof(host_priority_info_data_t));
			*host_info_outCnt = HOST_PRIORITY_INFO_COUNT;
			
			return KERN_SUCCESS;
		default:
			return KERN_NOT_SUPPORTED;
	}
	return KERN_NOT_SUPPORTED;
}

