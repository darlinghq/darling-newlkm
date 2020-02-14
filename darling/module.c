#include <linux/module.h>
#include <linux/cred.h>
#include <linux/sched.h>
#include <linux/version.h>

static int group = -1;
extern bool debug_output;

bool in_darling_group(void)
{
	int i;
	const struct cred* cred = current_cred();
	kgid_t grp = KGIDT_INIT(group);

	if (group == -1 || __kuid_val(cred->uid) == 0)
		return true;

	for (i = 0; i < cred->group_info->ngroups; i++)
	{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
		if (gid_eq(cred->group_info->gid[i], grp))
			return true;
#else
		if (gid_eq(GROUP_AT(cred->group_info, i), grp))
			return true;
#endif
	}

	return false;
}

module_param(group, int, 0644);
MODULE_PARM_DESC(group, "Restrict module functionality to given group ID");

module_param(debug_output, bool, 0644);
MODULE_PARM_DESC(debug_output, "Enable debug output");

MODULE_ALIAS("binfmt-feed");
MODULE_ALIAS("binfmt-cafe");
MODULE_ALIAS("binfmt-babe");

MODULE_LICENSE("GPL");

