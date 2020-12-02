#include <linux/pagemap.h>
#include <asm/cacheflush.h>

/*
 * Access another process' address space as given in mm.  If non-NULL, use the
 * given task for page fault accounting.
 */
static int __access_remote_vm_darling(struct task_struct *tsk, struct mm_struct *mm,
                unsigned long addr, void *buf, int len, unsigned int gup_flags)
{
        struct vm_area_struct *vma;
        void *old_buf = buf;
        int write = gup_flags & FOLL_WRITE;

        #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,8,0)
        down_read(&mm->mmap_lock);
        #else
        down_read(&mm->mmap_sem);
        #endif
        /* ignore errors, just check how much was successfully transferred */
        while (len) {
                int bytes, ret, offset;
                void *maddr;
                struct page *page = NULL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,9,0)
                ret = get_user_pages_remote(mm, addr, 1,
                                gup_flags, &page, &vma, NULL);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0)
                ret = get_user_pages_remote(tsk, mm, addr, 1,
                                gup_flags, &page, &vma, NULL);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
                ret = get_user_pages_remote(tsk, mm, addr, 1,
                                gup_flags, &page, &vma);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0)
                ret = get_user_pages_remote(tsk, mm, addr, 1,
                                write, 1, &page, &vma);
#else
                ret = get_user_pages(tsk, mm, addr, 1,
                                write, 1, &page, &vma);
#endif
                if (ret <= 0) {
#ifndef CONFIG_HAVE_IOREMAP_PROT
                        break;
#else
                        /*
                         * Check if this is a VM_IO | VM_PFNMAP VMA, which
                         * we can access using slightly different code.
                         */
                        vma = find_vma(mm, addr);
                        if (!vma || vma->vm_start > addr)
                                break;
                        if (vma->vm_ops && vma->vm_ops->access)
                                ret = vma->vm_ops->access(vma, addr, buf,
                                                          len, write);
                        if (ret <= 0)
                                break;
                        bytes = ret;
#endif
                } else {
                        bytes = len;
                        offset = addr & (PAGE_SIZE-1);
                        if (bytes > PAGE_SIZE-offset)
                                bytes = PAGE_SIZE-offset;

                        maddr = kmap(page);
                        if (write) {
                                copy_to_user_page(vma, page, addr,
                                                  maddr + offset, buf, bytes);
                                set_page_dirty_lock(page);
                        } else {
                                copy_from_user_page(vma, page, addr,
                                                    buf, maddr + offset, bytes);
                        }
                        kunmap(page);
                        put_page(page);
                }
                len -= bytes;
                buf += bytes;
                addr += bytes;
        }
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,8,0)
        up_read(&mm->mmap_lock);
        #else
        up_read(&mm->mmap_sem);
        #endif

        return buf - old_buf;
}


/*
 * Access another process' address space.
 * Source/target buffer must be kernel space,
 * Do not walk the page table directly, use get_user_pages
 */
static int __access_process_vm(struct task_struct *tsk, unsigned long addr,
                void *buf, int len, unsigned int gup_flags)
{
        struct mm_struct *mm;
        int ret;

        mm = get_task_mm(tsk);
        if (!mm)
                return 0;

        ret = __access_remote_vm_darling(tsk, mm, addr, buf, len, gup_flags);

        mmput(mm);

        return ret;
}


