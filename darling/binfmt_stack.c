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

#if defined(GEN_64BIT)
#define FUNCTION_NAME setup_stack64
#define user_long_t unsigned long
#elif defined(GEN_32BIT)
#define FUNCTION_NAME setup_stack32
#define user_long_t unsigned int
#else
#error See above
#endif

int FUNCTION_NAME(struct linux_binprm* bprm, struct load_results* lr)
{
	int err = 0;
	// unsigned char rand_bytes[16];
	char *executable_path, *executable_buf;
	user_long_t __user* argv;
	user_long_t __user* envp;
	user_long_t __user* applep;
	user_long_t __user* sp;
	char __user* exepath_user;
	size_t exepath_len;
	char __user* kernfd_user;
	char kernfd[12];
	char __user* applep_contents[3];
	char* env_value = NULL;
	char* vchroot_path = NULL;

	// Produce executable_path=... for applep
	executable_buf = kmalloc(4096, GFP_KERNEL);
	if (!executable_buf)
		return -ENOMEM;

	executable_path = d_path(&bprm->file->f_path, executable_buf, 4095);
	if (IS_ERR(executable_path))
	{
		err = -ENAMETOOLONG;
		goto out;
	}

	task_t task = darling_task_get_current();

	if (task)
		vchroot_path = task_copy_vchroot_path(task);

	if (vchroot_path != NULL)
	{
		const size_t vchroot_len = strlen(vchroot_path);
		exepath_len = strlen(executable_path);

		if (strncmp(executable_path, vchroot_path, vchroot_len) == 0)
		{
			memmove(executable_buf, executable_path + vchroot_len, exepath_len - vchroot_len + 1);
		}
		else
		{
			static const char SYSTEM_ROOT[] = "/Volumes/SystemRoot";

			memcpy(executable_buf, SYSTEM_ROOT, sizeof(SYSTEM_ROOT) - 1);
			memmove(executable_buf + sizeof(SYSTEM_ROOT) - 1, executable_path, exepath_len + 1);
		}
		executable_path = executable_buf;
	}

	// printk(KERN_NOTICE "Stack top: %p\n", bprm->p);
	exepath_len = strlen(executable_path);
	sp = (user_long_t*) (bprm->p & ~(sizeof(user_long_t)-1));
	sp -= bprm->argc + bprm->envc + 6 + exepath_len + sizeof(kernfd)/4;
	exepath_user = (char __user*) bprm->p - exepath_len - sizeof(EXECUTABLE_PATH);

	if (!find_extend_vma(current->mm, (unsigned long) sp))
	{
		err = -EFAULT;
		goto out;
	}

	snprintf(kernfd, sizeof(kernfd), "kernfd=%d", lr->kernfd);
	kernfd_user = exepath_user - sizeof(kernfd);
	if (copy_to_user(kernfd_user, kernfd, sizeof(kernfd)))
	{
		err = -EFAULT;
		goto out;
	}

	if (copy_to_user(exepath_user, EXECUTABLE_PATH, sizeof(EXECUTABLE_PATH)-1))
	{
		err = -EFAULT;
		goto out;
	}
	if (copy_to_user(exepath_user + sizeof(EXECUTABLE_PATH)-1, executable_path, exepath_len + 1))
	{
		err = -EFAULT;
		goto out;
	}
	applep_contents[0] = exepath_user;
	applep_contents[1] = kernfd_user;
	applep_contents[2] = NULL;

	kfree(executable_buf);
	executable_buf = NULL;
	bprm->p = (unsigned long) sp;

	// XXX: skip this for static executables, but we don't support them anyway...
	if (__put_user((user_long_t) lr->mh, sp++))
	{
		err = -EFAULT;
		goto out;
	}
	if (__put_user((user_long_t) bprm->argc, sp++))
	{
		err = -EFAULT;
		goto out;
	}

	unsigned long p = current->mm->arg_start;
	int argc = bprm->argc;

	argv = sp;
	envp = argv + argc + 1;

	// Fill in argv pointers
	while (argc--)
	{
		if (__put_user((user_long_t) p, argv++))
		{
			err = -EFAULT;
			goto out;
		}

		size_t len = strnlen_user((void __user*) p, MAX_ARG_STRLEN);
		if (!len || len > MAX_ARG_STRLEN)
		{
			err = -EINVAL;
			goto out;
		}

		p += len;
	}
	if (__put_user((user_long_t) 0, argv++))
	{
		err = -EFAULT;
		goto out;
	}
	current->mm->arg_end = current->mm->env_start = p;

	// Fill in envp pointers
	int envc = bprm->envc;
	env_value = (char*) kmalloc(MAX_ARG_STRLEN, GFP_KERNEL);

	while (envc--)
	{
		size_t len = strnlen_user((void __user*) p, MAX_ARG_STRLEN);
		if (!len || len > MAX_ARG_STRLEN)
		{
			err = -EINVAL;
			goto out;
		}

		if (copy_from_user(env_value, (void __user*) p, len) == 0)
		{
			// Don't pass this special env var down the the userland
			if (strncmp(env_value, "__mldr_bprefs=", 14) == 0)
			{
				p += len;
				continue;
			}
		}

		if (__put_user((user_long_t) p, envp++))
		{
			err = -EFAULT;
			goto out;
		}

		p += len;
	}
	if (__put_user((user_long_t) 0, envp++))
	{
		err = -EFAULT;
		goto out;
	}
	current->mm->env_end = p;
	applep = envp; // envp is now at the end of env pointers

	int i;
	for (i = 0; i < sizeof(applep_contents)/sizeof(applep_contents[0]); i++)
	{
		if (__put_user((user_long_t)(unsigned long) applep_contents[i], applep++))
		{
			err = -EFAULT;
			goto out;
		}
	}

	// get_random_bytes(rand_bytes, sizeof(rand_bytes));

	// TODO: produce stack_guard, e.g. stack_guard=0xcdd5c48c061b00fd (must contain 00 somewhere!)
	// TODO: produce malloc_entropy, e.g. malloc_entropy=0x9536cc569d9595cf,0x831942e402da316b
	// TODO: produce main_stack?
	
out:
	if (vchroot_path)
		kfree(vchroot_path);
	if (executable_buf)
		kfree(executable_buf);
	if (env_value)
		kfree(env_value);
	return err;
}

#undef FUNCTION_NAME
#undef user_long_t

