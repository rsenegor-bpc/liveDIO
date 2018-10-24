#include "include/kvmem.h"
#include <linux/vmalloc.h>

/*#ifndef virt_to_page
#define virt_to_page(x) MAP_NR(x)
#endif

#ifndef vmalloc_32
#define vmalloc_32(x) vmalloc(x)
#endif */


/* allocate user space mmapable block of memory in the kernel space */
void * rvmalloc(unsigned long size)
{
   void * mem;
   unsigned long adr;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,19)  
   struct page *page;
#else
   unsigned long page;
#endif
   
   // increase the size to make it multiple of the page size
   size = PAGE_ALIGN(size);
   mem=vmalloc_32(size);
   if (mem)
   {
      memset(mem, 0, size); /* Clear the ram out, no junk to the user */
      adr=(unsigned long) mem;

      while (size > 0)
      {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
         page = vmalloc_to_page((void *)adr);
         SetPageReserved(page);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,19)
         page = vmalloc_to_page((void *)adr);
         mem_map_reserve(page);
#else 
   #ifdef MAP_NR
         page = kvirt_to_phys(adr);
         mem_map_reserve(MAP_NR(phys_to_virt(page)));
   #else
         page = kvirt_to_pa(adr);
         mem_map_reserve(virt_to_page(__va(page)));
   #endif  // MAP_NR
#endif  // LINUX_VERSION_CODE
         adr+=PAGE_SIZE;
         size-=PAGE_SIZE;
      }
   }
   return mem;
}

void rvfree(void * mem, unsigned long size)
{
   unsigned long adr;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,19)  
   struct page *page;
#else
   unsigned long page;
#endif


   // increase the size to make it multiple of the page size
   size = PAGE_ALIGN(size);

   if (mem)
   {
      adr=(unsigned long) mem;
      while (size > 0)
      {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
         page = vmalloc_to_page((void *)adr);
         ClearPageReserved(page);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,19)
         page = vmalloc_to_page((void *)adr);
         mem_map_unreserve(page);
#else
   #ifdef MAP_NR
         page = kvirt_to_phys(adr);
         mem_map_unreserve(MAP_NR(phys_to_virt(page)));
   #else
         page = kvirt_to_pa(adr);
         mem_map_unreserve(virt_to_page(__va(page)));
   #endif  // MAP_NR
#endif  // LINUX_VERSION_CODE 

         adr+=PAGE_SIZE;
         size-=PAGE_SIZE;
      }
      vfree(mem);
   }
}

/* this function will map (fragment of) rvmalloc'ed memory area to user space */
int rvmmap(void *mem, unsigned memsize, struct vm_area_struct *vma)
{
   unsigned long pos, size, start=vma->vm_start;
#if LINUX_VERSION_CODE < 0x20300
   unsigned long offset = vma->vm_offset;
#else
   unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
#endif

   memsize = PAGE_ALIGN(memsize);

   /* this is not time critical code, so we check the arguments */
   /* offset HAS to be checked (and is checked)*/
   if ((long)offset<0)
      return -EFAULT;
   size = vma->vm_end - vma->vm_start;
   if (size + offset > memsize)
      return -EFAULT;
   pos = (unsigned long) mem + offset;
   if (pos%PAGE_SIZE || start%PAGE_SIZE || size%PAGE_SIZE)
      return -EFAULT;

   while (size>0)
   {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
      if (remap_pfn_range(vma,start, vmalloc_to_pfn((void*)pos), PAGE_SIZE,
         vma->vm_page_prot ))
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
      if (remap_pfn_range(vma,start,kvirt_to_phys(pos)>>PAGE_SHIFT, PAGE_SIZE,
         vma->vm_page_prot ))
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0) || defined(PD_USING_REDHAT_2420)
      if (remap_page_range(vma,start,kvirt_to_phys(pos), PAGE_SIZE,
         vma->vm_page_prot ))
#else
      if (remap_page_range(start,kvirt_to_phys(pos), PAGE_SIZE,
         vma->vm_page_prot ))
#endif
      {
         /* FIXME: what should we do here to unmap previous pages ?*/
         printk(KERN_ERR "rvmmap failed: vm_start=0x%lx, vm_end=0x%lx, size=0x%lx, pos=0x%lx\n",vma->vm_start,vma->vm_end,size,pos);
         return -EFAULT;
      }
      pos+=PAGE_SIZE;
      start+=PAGE_SIZE;
      size-=PAGE_SIZE;
   }
   return 0;
}
