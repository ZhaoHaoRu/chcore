/*
 * Copyright (c) 2022 Institute of Parallel And Distributed Systems (IPADS)
 * ChCore-Lab is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan
 * PSL v1. You may obtain a copy of Mulan PSL v1 at:
 *     http://license.coscl.org.cn/MulanPSL
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v1 for more details.
 */

#include <common/util.h>
#include <common/vars.h>
#include <common/macro.h>
#include <common/types.h>
#include <common/errno.h>
#include <lib/printk.h>
#include <mm/kmalloc.h>
#include <mm/mm.h>
#include <arch/mmu.h>

#include <arch/mm/page_table.h>

extern void set_ttbr0_el1(paddr_t);

void set_page_table(paddr_t pgtbl)
{
        set_ttbr0_el1(pgtbl);
}

#define USER_PTE 0
/*
 * the 3rd arg means the kind of PTE.
 */
static int set_pte_flags(pte_t *entry, vmr_prop_t flags, int kind)
{
        // Only consider USER PTE now.
        BUG_ON(kind != USER_PTE);

        /*
         * Current access permission (AP) setting:
         * Mapped pages are always readable (No considering XOM).
         * EL1 can directly access EL0 (No restriction like SMAP
         * as ChCore is a microkernel).
         */
        if (flags & VMR_WRITE)
                entry->l3_page.AP = AARCH64_MMU_ATTR_PAGE_AP_HIGH_RW_EL0_RW;
        else
                entry->l3_page.AP = AARCH64_MMU_ATTR_PAGE_AP_HIGH_RO_EL0_RO;

        if (flags & VMR_EXEC)
                entry->l3_page.UXN = AARCH64_MMU_ATTR_PAGE_UX;
        else
                entry->l3_page.UXN = AARCH64_MMU_ATTR_PAGE_UXN;

        // EL1 cannot directly execute EL0 accessiable region.
        entry->l3_page.PXN = AARCH64_MMU_ATTR_PAGE_PXN;
        // Set AF (access flag) in advance.
        entry->l3_page.AF = AARCH64_MMU_ATTR_PAGE_AF_ACCESSED;
        // Mark the mapping as not global
        entry->l3_page.nG = 1;
        // Mark the mappint as inner sharable
        entry->l3_page.SH = INNER_SHAREABLE;
        // Set the memory type
        if (flags & VMR_DEVICE) {
                entry->l3_page.attr_index = DEVICE_MEMORY;
                entry->l3_page.SH = 0;
        } else if (flags & VMR_NOCACHE) {
                entry->l3_page.attr_index = NORMAL_MEMORY_NOCACHE;
        } else {
                entry->l3_page.attr_index = NORMAL_MEMORY;
        }

        return 0;
}

#define GET_PADDR_IN_PTE(entry) \
        (((u64)entry->table.next_table_addr) << PAGE_SHIFT)
#define GET_NEXT_PTP(entry) phys_to_virt(GET_PADDR_IN_PTE(entry))

#define NORMAL_PTP (0)
#define BLOCK_PTP  (1)

/*
 * Find next page table page for the "va".
 *
 * cur_ptp: current page table page
 * level:   current ptp level
 *
 * next_ptp: returns "next_ptp"
 * pte     : returns "pte" (points to next_ptp) in "cur_ptp"
 *
 * alloc: if true, allocate a ptp when missing
 *
 */
static int get_next_ptp(ptp_t *cur_ptp, u32 level, vaddr_t va, ptp_t **next_ptp,
                        pte_t **pte, bool alloc)
{
        u32 index = 0;
        pte_t *entry;

        if (cur_ptp == NULL)
                return -ENOMAPPING;

        switch (level) {
        case 0:
                index = GET_L0_INDEX(va);
                break;
        case 1:
                index = GET_L1_INDEX(va);
                break;
        case 2:
                index = GET_L2_INDEX(va);
                break;
        case 3:
                index = GET_L3_INDEX(va);
                break;
        default:
                BUG_ON(1);
        }

        entry = &(cur_ptp->ent[index]);
        if (IS_PTE_INVALID(entry->pte)) {
                if (alloc == false) {
                        return -ENOMAPPING;
                } else {
                        /* alloc a new page table page */
                        ptp_t *new_ptp;
                        paddr_t new_ptp_paddr;
                        pte_t new_pte_val;

                        /* alloc a single physical page as a new page table page  */
                        /* LAB 2 TODO 3 BEGIN
                         * Hint: use get_pages to allocate a new page table page
                         *       set the attr `is_valid`, `is_table` and `next_table_addr` of new pte
                         */
                        new_ptp = (ptp_t*)get_pages(0);

                        memset(new_ptp, 0, PAGE_SIZE);
                        // clear the pte
                        new_pte_val.pte = 0;
                        new_pte_val.table.is_valid = 1;
                        new_pte_val.table.is_table = 1;

                        ///@note using physical address
                        new_ptp_paddr = virt_to_phys((vaddr_t)new_ptp);
                        new_pte_val.table.next_table_addr = (new_ptp_paddr >> PAGE_SHIFT);
                        entry->pte = new_pte_val.pte;

                        /* LAB 2 TODO 3 END */
                }
        }

        *next_ptp = (ptp_t *)GET_NEXT_PTP(entry);
        *pte = entry;
        if (IS_PTE_TABLE(entry->pte))
                return NORMAL_PTP;
        else
                return BLOCK_PTP;
}

void free_page_table(void *pgtbl)
{
        ptp_t *l0_ptp, *l1_ptp, *l2_ptp, *l3_ptp;
        pte_t *l0_pte, *l1_pte, *l2_pte;
        int i, j, k;

        if (pgtbl == NULL) {
                kwarn("%s: input arg is NULL.\n", __func__);
                return;
        }

        /* L0 page table */
        l0_ptp = (ptp_t *)pgtbl;

        /* Interate each entry in the l0 page table*/
        for (i = 0; i < PTP_ENTRIES; ++i) {
                l0_pte = &l0_ptp->ent[i];
                if (IS_PTE_INVALID(l0_pte->pte) || !IS_PTE_TABLE(l0_pte->pte))
                        continue;
                l1_ptp = (ptp_t *)GET_NEXT_PTP(l0_pte);

                /* Interate each entry in the l1 page table*/
                for (j = 0; j < PTP_ENTRIES; ++j) {
                        l1_pte = &l1_ptp->ent[j];
                        if (IS_PTE_INVALID(l1_pte->pte)
                            || !IS_PTE_TABLE(l1_pte->pte))
                                continue;
                        l2_ptp = (ptp_t *)GET_NEXT_PTP(l1_pte);

                        /* Interate each entry in the l2 page table*/
                        for (k = 0; k < PTP_ENTRIES; ++k) {
                                l2_pte = &l2_ptp->ent[k];
                                if (IS_PTE_INVALID(l2_pte->pte)
                                    || !IS_PTE_TABLE(l2_pte->pte))
                                        continue;
                                l3_ptp = (ptp_t *)GET_NEXT_PTP(l2_pte);
                                /* Free the l3 page table page */
                                free_pages(l3_ptp);
                        }

                        /* Free the l2 page table page */
                        free_pages(l2_ptp);
                }

                /* Free the l1 page table page */
                free_pages(l1_ptp);
        }

        free_pages(l0_ptp);
}

/*
 * Translate a va to pa, and get its pte for the flags
 */
// pgtbl: ptr for the first level page table(pgd) virtual address
int query_in_pgtbl(void *pgtbl, vaddr_t va, paddr_t *pa, pte_t **entry)
{
        /* LAB 2 TODO 3 BEGIN */
        /*
         * Hint: Walk through each level of page table using `get_next_ptp`,
         * return the pa and pte until a L0/L1 block or page, return
         * `-ENOMAPPING` if the va is not mapped.
         */
        ptp_t *cur_ptp = (ptp_t*)pgtbl;
        ptp_t *next_ptp = NULL;
        pte_t *pte = NULL;

        // check l0
        u32 level = 0;
        int ret = get_next_ptp(cur_ptp, level, va, &next_ptp, &pte, 0);
        if (ret == -ENOMAPPING) {
                return ret;
        }

        // check l1 & l2
        for (level = 1; level <= 2; ++level) {
                cur_ptp = next_ptp;
                ret = get_next_ptp(cur_ptp, level, va, &next_ptp, &pte, 0);
                if (ret == -ENOMAPPING) {
                        return ret;
                } else if (ret == BLOCK_PTP) {
                        *entry = pte;
                        // block base physical address + offset
                        if (pte != NULL) {
                                if (level == 1) {
                                        *pa = (u64)((u64)(pte->l1_block.pfn) << L1_INDEX_SHIFT) + GET_VA_OFFSET_L1(va);
//                                        kdebug("the level 1 pa: %lx, pfn: %llx, pfn: %llx, offset: %lx\n", pa, pte->l1_block.pfn, ((u64)pte->l1_block.pfn) << L1_INDEX_SHIFT, GET_VA_OFFSET_L1(va));
                                } else {
                                        *pa = (u64)((u64)(pte->l2_block.pfn) << L2_INDEX_SHIFT) + GET_VA_OFFSET_L2(va);
                                }

                        }
                        return 0;
                }
        }

        // check l3
        int entry_index = GET_L3_INDEX(va);
        cur_ptp = next_ptp;
        pte = &(cur_ptp->ent[entry_index]);
        if (IS_PTE_INVALID(pte->pte)) {
                return -ENOMAPPING;
        }

        *entry = pte;
        *pa = (u64)(pte->l3_page.pfn << PAGE_SHIFT) + GET_VA_OFFSET_L3(va);
        return 0;
        /* LAB 2 TODO 3 END */
}

int map_range_in_pgtbl(void *pgtbl, vaddr_t va, paddr_t pa, size_t len,
                       vmr_prop_t flags)
{
        /* LAB 2 TODO 3 BEGIN */
        /*
         * Hint: Walk through each level of page table using `get_next_ptp`,
         * create new page table page if necessary, fill in the final level
         * pte with the help of `set_pte_flags`. Iterate until all pages are
         * mapped.
         */
        ptp_t *cur_ptp = NULL;
        pte_t *pte = NULL;
        int ret = 0, entry_index = 0;
        long length = len;

        // map an address range into page
        // va: for finding entry in page tables
        // pa: for pfn in pte
        while (length > 0) {
                // find the proper page entry in l3
                cur_ptp = (ptp_t*)pgtbl;
                for (int i = 0; i < 3; ++i) {
                        ret = get_next_ptp(cur_ptp, i, va, &cur_ptp, &pte, 1);
                        if (ret == -ENOMAPPING) {
                                kwarn("map_range_in_pgtbl: can not find the proper page\n");
                                return -ENOMAPPING;
                        }
                }

                entry_index = GET_L3_INDEX(va);
                while (entry_index < PTP_ENTRIES && length > 0) {
                        pte_t new_entry;
                        new_entry.pte = 0;
                        new_entry.l3_page.is_valid = 1;
                        new_entry.l3_page.is_page = 1;
                        new_entry.l3_page.pfn = (pa >> PAGE_SHIFT);

                        set_pte_flags(&new_entry, flags, USER_PTE);
                        cur_ptp->ent[entry_index] = new_entry;

//                        kdebug("in map the virtual address: %lu, the physical address: %lu\n", va, pa);

                        length -= PAGE_SIZE;
                        va += PAGE_SIZE;
                        pa += PAGE_SIZE;
                        ++entry_index;
                }
        }
        return 0;
        /* LAB 2 TODO 3 END */
}

int unmap_range_in_pgtbl(void *pgtbl, vaddr_t va, size_t len)
{
        /* LAB 2 TODO 3 BEGIN */
        /*
         * Hint: Walk through each level of page table using `get_next_ptp`,
         * mark the final level pte as invalid. Iterate until all pages are
         * unmapped.
         */
        BUG_ON(pgtbl == NULL);
        ptp_t *cur_ptp = NULL;
        pte_t *pte = NULL;
        int ret = 0, entry_index = 0, i = 0;
        size_t skip_length = 0;
        long length = len;

        while (length > 0) {
                cur_ptp = (ptp_t*)pgtbl;
                for (i = 0; i < 3; ++i) {

                        ret = get_next_ptp(cur_ptp, i, va, &cur_ptp, &pte, 0);
                        if (ret == -ENOMAPPING) {
                                kwarn("unmap_range_in_pgtbl: the page not exist\n");
                                if (i == 0) {
                                     skip_length = L0_PER_ENTRY_PAGES * PAGE_SIZE;
                                } else if (i == 1) {
                                     skip_length = L1_PER_ENTRY_PAGES * PAGE_SIZE;
                                } else if (i == 2) {
                                     skip_length = L2_PER_ENTRY_PAGES * PAGE_SIZE;
                                }
                                length = skip_length >= length ? 0 : length - skip_length;
                                if (length > 0) {
                                     va += skip_length;
                                }
                                break;
                        }
                }

                // don't have appropriate l3 entry
                if (i != 3) {
                        continue;
                }

                entry_index = GET_L3_INDEX(va);
                while (entry_index < PTP_ENTRIES && len > 0) {
                        cur_ptp->ent[entry_index].l3_page.is_valid = 0;
                        va += PAGE_SIZE;
                        length -= PAGE_SIZE;
                        ++entry_index;
                }

        }
        return 0;
        /* LAB 2 TODO 3 END */
}

int map_range_in_pgtbl_huge(void *pgtbl, vaddr_t va, paddr_t pa, size_t len,
                            vmr_prop_t flags)
{
        /* LAB 2 TODO 4 BEGIN */
        BUG_ON(pgtbl == NULL);
        ptp_t *cur_ptp = NULL;
        pte_t *pte = NULL, new_entry;
        int ret = 0, entry_index = 0, i = 0;
        long length = len, mapped_length = 0;


        while (length > 0) {
                cur_ptp = (ptp_t*)pgtbl;
                kdebug("map_range_in_pgtbl_huge the length: %ld\n", length);
                for (i = 0; i < 3 && length > 0; ++i) {
//                        kdebug("get next ptp va: %lx\n", va);
                        ret = get_next_ptp(cur_ptp, i, va, &cur_ptp, &pte, 1);
//                        kdebug("finish get_next_ptp");
                        if (ret == -ENOMAPPING) {
                                kwarn("map_range_in_pgtbl_huge: can not find the proper page\n");
                                return -ENOMAPPING;
                        }

                        if (i == 0) {
                                mapped_length = L1_PER_ENTRY_PAGES * PAGE_SIZE;
                        } else if (i == 1) {
                                mapped_length = L2_PER_ENTRY_PAGES * PAGE_SIZE;
                        }

                        // map huge page
                        if ((i == 1 || i == 0) && length > mapped_length) {
                                new_entry.pte = 0;
                                if (i == 0) {
                                        new_entry.l1_block.is_valid = 1;
                                        new_entry.l1_block.is_table = 0;
                                        new_entry.l1_block.pfn = (pa >> L1_INDEX_SHIFT);

                                        set_pte_flags(&new_entry, flags, USER_PTE);

                                        cur_ptp->ent[GET_L1_INDEX(va)] = new_entry;
                                        length -= mapped_length;
                                        va += mapped_length;
                                        pa += mapped_length;
                                        break;
                                } else if (i == 1) {
                                        new_entry.l2_block.is_valid = 1;
                                        new_entry.l1_block.is_table = 0;
                                        new_entry.l2_block.pfn =
                                                (pa >> L2_INDEX_SHIFT);
                                        set_pte_flags(&new_entry, flags, USER_PTE);

                                        cur_ptp->ent[GET_L2_INDEX(va)] = new_entry;
                                        length -= mapped_length;
                                        va += mapped_length;
                                        pa += mapped_length;
                                        break;
                                }
                        }
                }

//                kdebug("the current length: %d", len);
                // already map the huge page
                if (i != 3) {
                        continue;
                }

                entry_index = GET_L3_INDEX(va);
                while (entry_index < PTP_ENTRIES && len > 0) {
                        pte_t new_entry;
                        new_entry.pte = 0;
                        new_entry.l3_page.is_valid = 1;
                        new_entry.l3_page.is_page = 1;
                        new_entry.l3_page.pfn = (pa >> PAGE_SHIFT);

                        set_pte_flags(&new_entry, flags, USER_PTE);
                        cur_ptp->ent[entry_index] = new_entry;

                        //                        kdebug("in map the virtual address: %lu, the physical address: %lu\n", va, pa);

                        length -= PAGE_SIZE;
                        va += PAGE_SIZE;
                        pa += PAGE_SIZE;
                        ++entry_index;
                }
        }
        return 0;
        /* LAB 2 TODO 4 END */
}

int unmap_range_in_pgtbl_huge(void *pgtbl, vaddr_t va, size_t len)
{
        /* LAB 2 TODO 4 BEGIN */
        BUG_ON(pgtbl == NULL);
        ptp_t *cur_ptp = NULL;
        pte_t *pte = NULL;
        int ret, entry_index = 0, i = 0, mapped_length = 0;
        size_t skip_length = 0;

        while (len > 0) {
                cur_ptp = (ptp_t*)pgtbl;
                for (i = 0; i < 3 && len > 0; ++i) {
                        ret = get_next_ptp(cur_ptp, i, va, &cur_ptp, &pte, 0);

                        // don't have such map
                        if (ret == -ENOMAPPING) {
                                kwarn("unmap_range_in_pgtbl: the page not exist\n");
                                if (i == 0) {
                                     skip_length = L0_PER_ENTRY_PAGES * PAGE_SIZE;
                                } else if (i == 1) {
                                     skip_length = L1_PER_ENTRY_PAGES * PAGE_SIZE;
                                } else if (i == 2) {
                                     skip_length = L2_PER_ENTRY_PAGES * PAGE_SIZE;
                                }
                                len = skip_length >= len ? 0 : len - skip_length;
                                if (len != 0) {
                                     va += skip_length;
                                }
                                break;
                        }

                        // check a huge page
                       if (i == 0) {
                             mapped_length = L1_PER_ENTRY_PAGES * PAGE_SIZE;
                             if (len > mapped_length) {
                                     cur_ptp->ent[GET_L1_INDEX(va)].l1_block.is_valid = 0;
                                     va += mapped_length;
                                     len -= mapped_length;
                                     break;
                             }
                       } else if (i == 1) {
                             mapped_length = L2_PER_ENTRY_PAGES * PAGE_SIZE;
                             if (len > mapped_length) {
                                     cur_ptp->ent[GET_L2_INDEX(va)].l2_block.is_valid = 0;
                                     va += mapped_length;
                                     len -= mapped_length;
                                     break;
                             }
                       }
                }

                // already unmap huge pages
                if (i != 3) {
                        continue;
                }

                entry_index = GET_L3_INDEX(va);
                while (entry_index < PTP_ENTRIES && len > 0) {
                        cur_ptp->ent[entry_index].l3_page.is_valid = 0;
                        va += PAGE_SIZE;
                        len -= PAGE_SIZE;
                        ++entry_index;
                }
        }
        return 0;
        /* LAB 2 TODO 4 END */
}

#ifdef CHCORE_KERNEL_TEST
#include <mm/buddy.h>
#include <lab.h>
void lab2_test_page_table(void)
{
        vmr_prop_t flags = VMR_READ | VMR_WRITE;
        {
                bool ok = true;
                void *pgtbl = get_pages(0);
                memset(pgtbl, 0, PAGE_SIZE);
                paddr_t pa;
                pte_t *pte;
                int ret;

                ret = map_range_in_pgtbl(
                        pgtbl, 0x1001000, 0x1000, PAGE_SIZE, flags);
                lab_assert(ret == 0);

                ret = query_in_pgtbl(pgtbl, 0x1001000, &pa, &pte);
                lab_assert(ret == 0 && pa == 0x1000);
                lab_assert(pte && pte->l3_page.is_valid && pte->l3_page.is_page
                           && pte->l3_page.SH == INNER_SHAREABLE);
                ret = query_in_pgtbl(pgtbl, 0x1001050, &pa, &pte);
                lab_assert(ret == 0 && pa == 0x1050);

                ret = unmap_range_in_pgtbl(pgtbl, 0x1001000, PAGE_SIZE);
                lab_assert(ret == 0);
                ret = query_in_pgtbl(pgtbl, 0x1001000, &pa, &pte);
                lab_assert(ret == -ENOMAPPING);

                free_page_table(pgtbl);
                lab_check(ok, "Map & unmap one page");
        }
        {
                bool ok = true;
                void *pgtbl = get_pages(0);
                memset(pgtbl, 0, PAGE_SIZE);
                paddr_t pa;
                pte_t *pte;
                int ret;
                size_t nr_pages = 10;
                size_t len = PAGE_SIZE * nr_pages;

                ret = map_range_in_pgtbl(pgtbl, 0x1001000, 0x1000, len, flags);
                kdebug("multiple map_range_in_pgtbl ret: %d\n", ret);
                lab_assert(ret == 0);
                ret = map_range_in_pgtbl(
                        pgtbl, 0x1001000 + len, 0x1000 + len, len, flags);
                kdebug("multiple map_range_in_pgtbl twice ret: %d\n", ret);
                lab_assert(ret == 0);

                for (int i = 0; i < nr_pages * 2; i++) {
                        ret = query_in_pgtbl(
                                pgtbl, 0x1001050 + i * PAGE_SIZE, &pa, &pte);
                        lab_assert(ret == 0 && pa == 0x1050 + i * PAGE_SIZE);
                        lab_assert(pte && pte->l3_page.is_valid
                                   && pte->l3_page.is_page);
                        kdebug("multiple query_in_pgtbl time %d ret: %d\n", i, ret);
                }

                ret = unmap_range_in_pgtbl(pgtbl, 0x1001000, len);
                kdebug("multiple unmap range in pgtbl ret: %d\n", ret);
                lab_assert(ret == 0);
                ret = unmap_range_in_pgtbl(pgtbl, 0x1001000 + len, len);
                kdebug("multiple unmap range in pgtbl twice ret: %d\n", ret);
                lab_assert(ret == 0);

                for (int i = 0; i < nr_pages * 2; i++) {
                        ret = query_in_pgtbl(
                                pgtbl, 0x1001050 + i * PAGE_SIZE, &pa, &pte);
                        kdebug("multiple query_in_pgtbl after free time %d ret: %d\n", i, ret);
                        lab_assert(ret == -ENOMAPPING);
                }

                free_page_table(pgtbl);
                lab_check(ok, "Map & unmap multiple pages");
        }
        {
                bool ok = true;
                void *pgtbl = get_pages(0);
                memset(pgtbl, 0, PAGE_SIZE);
                paddr_t pa;
                pte_t *pte;
                int ret;
                /* 1GB + 4MB + 40KB */
                size_t len = (1 << 30) + (4 << 20) + 10 * PAGE_SIZE;

                ret = map_range_in_pgtbl(
                        pgtbl, 0x100000000, 0x100000000, len, flags);
                lab_assert(ret == 0);
                ret = map_range_in_pgtbl(pgtbl,
                                         0x100000000 + len,
                                         0x100000000 + len,
                                         len,
                                         flags);
                lab_assert(ret == 0);

                for (vaddr_t va = 0x100000000; va < 0x100000000 + len * 2;
                     va += 5 * PAGE_SIZE + 0x100) {
                        ret = query_in_pgtbl(pgtbl, va, &pa, &pte);
                        lab_assert(ret == 0 && pa == va);
                }

                ret = unmap_range_in_pgtbl(pgtbl, 0x100000000, len);
                lab_assert(ret == 0);
                ret = unmap_range_in_pgtbl(pgtbl, 0x100000000 + len, len);
                lab_assert(ret == 0);

                for (vaddr_t va = 0x100000000; va < 0x100000000 + len;
                     va += 5 * PAGE_SIZE + 0x100) {
                        ret = query_in_pgtbl(pgtbl, va, &pa, &pte);
                        lab_assert(ret == -ENOMAPPING);
                }

                free_page_table(pgtbl);
                lab_check(ok, "Map & unmap huge range");
        }
        {
                kdebug("begin the last test\n");
                bool ok = true;
                void *pgtbl = get_pages(0);
                memset(pgtbl, 0, PAGE_SIZE);
                paddr_t pa;
                pte_t *pte;
                int ret;
                /* 1GB + 4MB + 40KB */
                size_t len = (1 << 30) + (4 << 20) + 10 * PAGE_SIZE;
                size_t free_mem, used_mem;

                free_mem = get_free_mem_size_from_buddy(&global_mem[0]);
                kdebug("begin test map_range_in_pgtbl_huge\n");
                ret = map_range_in_pgtbl_huge(
                        pgtbl, 0x100000000, 0x100000000, len, flags);
//                ret = map_range_in_pgtbl(
//                        pgtbl, 0x100000000, 0x100000000, len, flags);
                kdebug("map_range_in_pgtbl_huge ret: %d\n", ret);
                lab_assert(ret == 0);
                used_mem =
                        free_mem - get_free_mem_size_from_buddy(&global_mem[0]);
                kdebug("after map_range_in_pgtbl_huge used_mem: %d\n", used_mem);
                lab_assert(used_mem < PAGE_SIZE * 8);

                for (vaddr_t va = 0x100000000; va < 0x100000000 + len;
                     va += 5 * PAGE_SIZE + 0x100) {
                        ret = query_in_pgtbl(pgtbl, va, &pa, &pte);
                        kdebug("query_in_pgtbl_huge ret: %d, pa: %lx, va: %lx\n", ret, pa, va);
                        lab_assert(ret == 0 && pa == va);
                }

               ret = unmap_range_in_pgtbl_huge(pgtbl, 0x100000000, len);
//                ret = unmap_range_in_pgtbl(pgtbl, 0x100000000, len);
                kdebug("unmap range huge in pgtbl ret: %d\n", ret);
                lab_assert(ret == 0);

                for (vaddr_t va = 0x100000000; va < 0x100000000 + len;
                     va += 5 * PAGE_SIZE + 0x100) {
                        ret = query_in_pgtbl(pgtbl, va, &pa, &pte);
//                        kdebug("query_in_pgtbl_huge ret: %d\n", ret);
                        lab_assert(ret == -ENOMAPPING);
                }

                free_page_table(pgtbl);
                lab_check(ok, "Map & unmap with huge page support");
        }
        printk("[TEST] Page table tests finished\n");
}
#endif /* CHCORE_KERNEL_TEST */
