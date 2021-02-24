/* so_util.c -- utils to load and hook .so modules
 *
 * Copyright (C) 2021 Andy Nguyen, fgsfds
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <switch.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <elf.h>

#include "config.h"
#include "so_util.h"
#include "util.h"
#include "error.h"
#include "elf.h"

void *text_base, *text_virtbase;
size_t text_size;

void *data_base, *data_virtbase;
size_t data_size;

static void *load_base, *load_virtbase;
static size_t load_size;
static VirtmemReservation *load_memrv;

static void *so_base;

static Elf64_Ehdr *elf_hdr;
static Elf64_Phdr *prog_hdr;
static Elf64_Shdr *sec_hdr;
static Elf64_Sym *syms;
static int num_syms;

static char *shstrtab;
static char *dynstrtab;

void hook_thumb(uintptr_t addr, uintptr_t dst) {
  if (addr == 0)
    return;
  addr &= ~1;
  if (addr & 2) {
    uint16_t nop = 0xbf00;
    memcpy((void *)addr, &nop, sizeof(nop));
    addr += 2;
  }
  uint32_t hook[2];
  hook[0] = 0xf000f8df; // LDR PC, [PC]
  hook[1] = dst;
  memcpy((void *)addr, hook, sizeof(hook));
}

void hook_arm(uintptr_t addr, uintptr_t dst) {
  if (addr == 0)
    return;
  uint32_t hook[2];
  hook[0] = 0xe51ff004; // LDR PC, [PC, #-0x4]
  hook[1] = dst;
  memcpy((void *)addr, hook, sizeof(hook));
}

void hook_arm64(uintptr_t addr, uintptr_t dst) {
  if (addr == 0)
    return;
  uint32_t *hook = (uint32_t *)addr;
  hook[0] = 0x58000051u; // LDR X17, #0x8
  hook[1] = 0xd61f0220u; // BR X17
  *(uint64_t *)(hook + 2) = dst;
}

void so_flush_caches(void) {
  armDCacheFlush(load_virtbase, load_size);
  armICacheInvalidate(load_virtbase, load_size);
}

void so_free_temp(void) {
  free(so_base);
  so_base = NULL;
}

void so_finalize(void) {
  Result rc = 0;

  // map the entire thing as code memory
  rc = svcMapProcessCodeMemory(envGetOwnProcessHandle(), (u64)load_virtbase, (u64)load_base, load_size);
  if (R_FAILED(rc)) fatal_error("Error: svcMapProcessCodeMemory failed:\n%08x", rc);

  // map code sections as R+X
  const u64 text_asize = ALIGN_MEM(text_size, 0x1000); // align to page
  rc = svcSetProcessMemoryPermission(envGetOwnProcessHandle(), (u64)text_virtbase, text_asize, Perm_Rx);
  if (R_FAILED(rc)) fatal_error("Error: could not map %u bytes of RX memory at %p:\n%08x", text_asize, text_virtbase, rc);

  // map the rest as R+W
  const u64 rest_asize = load_size - text_asize;
  const uintptr_t rest_virtbase = (uintptr_t)text_virtbase + text_asize;
  rc = svcSetProcessMemoryPermission(envGetOwnProcessHandle(), rest_virtbase, rest_asize, Perm_Rw);
  if (R_FAILED(rc)) fatal_error("Error: could not map %u bytes of RW memory at %p (%p) (2):\n%08x", rest_asize, data_virtbase, rest_virtbase, rc);
}

int so_load(const char *filename, void *base, size_t max_size) {
  int res = 0;
  size_t so_size = 0;
  int text_segno = -1;
  int data_segno = -1;

  FILE *fd = fopen(filename, "rb");
  if (fd == NULL)
    return -1;

  fseek(fd, 0, SEEK_END);
  so_size = ftell(fd);
  fseek(fd, 0, SEEK_SET);

  so_base = malloc(so_size);
  if (!so_base) {
    fclose(fd);
    return -2;
  }

  fread(so_base, so_size, 1, fd);
  fclose(fd);

  if (memcmp(so_base, ELFMAG, SELFMAG) != 0) {
    res = -1;
    goto err_free_so;
  }

  elf_hdr = (Elf64_Ehdr *)so_base;
  prog_hdr = (Elf64_Phdr *)((uintptr_t)so_base + elf_hdr->e_phoff);
  sec_hdr = (Elf64_Shdr *)((uintptr_t)so_base + elf_hdr->e_shoff);
  shstrtab = (char *)((uintptr_t)so_base + sec_hdr[elf_hdr->e_shstrndx].sh_offset);

  // calculate total size of the LOAD segments
  for (int i = 0; i < elf_hdr->e_phnum; i++) {
    if (prog_hdr[i].p_type == PT_LOAD) {
      const size_t prog_size = ALIGN_MEM(prog_hdr[i].p_memsz, prog_hdr[i].p_align);
      // get the segment numbers of text and data segments
      if ((prog_hdr[i].p_flags & PF_X) == PF_X) {
        text_segno = i;
      } else {
        // assume data has to be after text
        if (text_segno < 0)
          goto err_free_so;
        data_segno = i;
        // since data is after text, total program size = last_data_offset + last_data_aligned_size
        load_size = prog_hdr[i].p_vaddr + prog_size;
      }
    }
  }

  // align total size to page size
  load_size = ALIGN_MEM(load_size, 0x1000);
  if (load_size > max_size) {
    res = -3;
    goto err_free_so;
  }

  // allocate space for all load segments (align to page size)
  // TODO: find out a way to allocate memory that doesn't fuck with the heap
  load_base = base;
  if (!load_base) goto err_free_so;
  memset(load_base, 0, load_size);

  // reserve virtual memory space for the entire LOAD zone while we're fucking with the ELF
  virtmemLock();
  load_virtbase = virtmemFindCodeMemory(load_size, 0x1000);
  load_memrv = virtmemAddReservation(load_virtbase, load_size);
  virtmemUnlock();

  // copy segments to where they belong

  // text
  text_size = prog_hdr[text_segno].p_memsz;
  text_virtbase = (void *)(prog_hdr[text_segno].p_vaddr + (Elf64_Addr)load_virtbase);
  text_base =     (void *)(prog_hdr[text_segno].p_vaddr + (Elf64_Addr)load_base    );
  prog_hdr[text_segno].p_vaddr = (Elf64_Addr)text_virtbase;
  memcpy(text_base, (void *)((uintptr_t)so_base + prog_hdr[text_segno].p_offset), prog_hdr[text_segno].p_filesz);

  // data
  data_size = prog_hdr[data_segno].p_memsz;
  data_virtbase = (void *)(prog_hdr[data_segno].p_vaddr + (Elf64_Addr)load_virtbase);
  data_base     = (void *)(prog_hdr[data_segno].p_vaddr + (Elf64_Addr)load_base    );
  prog_hdr[data_segno].p_vaddr = (Elf64_Addr)data_virtbase;
  memcpy(data_base, (void *)((uintptr_t)so_base + prog_hdr[data_segno].p_offset), prog_hdr[data_segno].p_filesz);

  syms = NULL;
  dynstrtab = NULL;

  for (int i = 0; i < elf_hdr->e_shnum; i++) {
    char *sh_name = shstrtab + sec_hdr[i].sh_name;
    if (strcmp(sh_name, ".dynsym") == 0) {
      syms = (Elf64_Sym *)((uintptr_t)text_base + sec_hdr[i].sh_addr);
      num_syms = sec_hdr[i].sh_size / sizeof(Elf64_Sym);
    } else if (strcmp(sh_name, ".dynstr") == 0) {
      dynstrtab = (char *)((uintptr_t)text_base + sec_hdr[i].sh_addr);
    }
  }

  if (syms == NULL || dynstrtab == NULL) {
    res = -2;
    goto err_free_load;
  }

  return 0;

err_free_load:
  virtmemLock();
  virtmemRemoveReservation(load_memrv);
  virtmemUnlock();
  free(load_base);
err_free_so:
  free(so_base);

  return res;
}

int so_relocate(void) {
  for (int i = 0; i < elf_hdr->e_shnum; i++) {
    char *sh_name = shstrtab + sec_hdr[i].sh_name;
    if (strcmp(sh_name, ".rela.dyn") == 0 || strcmp(sh_name, ".rela.plt") == 0) {
      Elf64_Rela *rels = (Elf64_Rela *)((uintptr_t)text_base + sec_hdr[i].sh_addr);
      for (int j = 0; j < sec_hdr[i].sh_size / sizeof(Elf64_Rela); j++) {
        uintptr_t *ptr = (uintptr_t *)((uintptr_t)text_base + rels[j].r_offset);
        Elf64_Sym *sym = &syms[ELF64_R_SYM(rels[j].r_info)];

        int type = ELF64_R_TYPE(rels[j].r_info);
        switch (type) {
          case R_AARCH64_ABS64:
            // FIXME: = or += ?
            *ptr = (uintptr_t)text_virtbase + sym->st_value + rels[j].r_addend;
            break;

          case R_AARCH64_RELATIVE:
            // sometimes the value of r_addend is also at *ptr
            *ptr = (uintptr_t)text_virtbase + rels[j].r_addend;
            break;

          case R_AARCH64_GLOB_DAT:
          case R_AARCH64_JUMP_SLOT:
          {
            if (sym->st_shndx != SHN_UNDEF)
              *ptr = (uintptr_t)text_virtbase + sym->st_value + rels[j].r_addend;
            break;
          }

          default:
            fatal_error("Error: unknown relocation type:\n%x\n", type);
            break;
        }
      }
    }
  }

  return 0;
}

int so_resolve(DynLibFunction *funcs, int num_funcs, int taint_missing_imports) {
  for (int i = 0; i < elf_hdr->e_shnum; i++) {
    char *sh_name = shstrtab + sec_hdr[i].sh_name;
    if (strcmp(sh_name, ".rela.dyn") == 0 || strcmp(sh_name, ".rela.plt") == 0) {
      Elf64_Rela *rels = (Elf64_Rela *)((uintptr_t)text_base + sec_hdr[i].sh_addr);
      for (int j = 0; j < sec_hdr[i].sh_size / sizeof(Elf64_Rela); j++) {
        uintptr_t *ptr = (uintptr_t *)((uintptr_t)text_base + rels[j].r_offset);
        Elf64_Sym *sym = &syms[ELF64_R_SYM(rels[j].r_info)];

        int type = ELF64_R_TYPE(rels[j].r_info);
        switch (type) {
          case R_AARCH64_GLOB_DAT:
          case R_AARCH64_JUMP_SLOT:
          {
            if (sym->st_shndx == SHN_UNDEF) {
              // make it crash for debugging
              if (taint_missing_imports)
                *ptr = rels[j].r_offset;

              char *name = dynstrtab + sym->st_name;
              for (int k = 0; k < num_funcs; k++) {
                if (strcmp(name, funcs[k].symbol) == 0) {
                  *ptr = funcs[k].func;
                  break;
                }
              }
            }

            break;
          }

          default:
            break;
        }
      }
    }
  }

  return 0;
}

void so_execute_init_array(void) {
  for (int i = 0; i < elf_hdr->e_shnum; i++) {
    char *sh_name = shstrtab + sec_hdr[i].sh_name;
    if (strcmp(sh_name, ".init_array") == 0) {
      int (** init_array)() = (void *)((uintptr_t)text_virtbase + sec_hdr[i].sh_addr);
      for (int j = 0; j < sec_hdr[i].sh_size / 8; j++) {
        if (init_array[j] != 0)
          init_array[j]();
      }
    }
  }
}

uintptr_t so_find_addr(const char *symbol) {
  for (int i = 0; i < num_syms; i++) {
    char *name = dynstrtab + syms[i].st_name;
    if (strcmp(name, symbol) == 0)
      return (uintptr_t)text_base + syms[i].st_value;
  }

  fatal_error("Error: could not find symbol:\n%s\n", symbol);
  return 0;
}

uintptr_t so_find_rel_addr(const char *symbol) {
  for (int i = 0; i < elf_hdr->e_shnum; i++) {
    char *sh_name = shstrtab + sec_hdr[i].sh_name;
    if (strcmp(sh_name, ".rela.dyn") == 0 || strcmp(sh_name, ".rela.plt") == 0) {
      Elf64_Rela *rels = (Elf64_Rela *)((uintptr_t)text_base + sec_hdr[i].sh_addr);
      for (int j = 0; j < sec_hdr[i].sh_size / sizeof(Elf64_Rela); j++) {
        Elf64_Sym *sym = &syms[ELF64_R_SYM(rels[j].r_info)];

        int type = ELF64_R_TYPE(rels[j].r_info);
        if (type == R_AARCH64_GLOB_DAT || type == R_AARCH64_JUMP_SLOT) {
          char *name = dynstrtab + sym->st_name;
          if (strcmp(name, symbol) == 0)
            return (uintptr_t)text_base + rels[j].r_offset;
        }
      }
    }
  }

  fatal_error("Error: could not find symbol:\n%s\n", symbol);
  return 0;
}

uintptr_t so_find_addr_rx(const char *symbol) {
  for (int i = 0; i < num_syms; i++) {
    char *name = dynstrtab + syms[i].st_name;
    if (strcmp(name, symbol) == 0)
      return (uintptr_t)text_virtbase + syms[i].st_value;
  }

  fatal_error("Error: could not find symbol:\n%s\n", symbol);
  return 0;
}

DynLibFunction *so_find_import(DynLibFunction *funcs, int num_funcs, const char *name) {
  for (int i = 0; i < num_funcs; ++i)
    if (!strcmp(funcs[i].symbol, name))
      return &funcs[i];
  return NULL;
}

int so_unload(void) {
  if (load_base == NULL)
    return -1;

  if (so_base) {
    // someone forgot to free the temp data
    so_free_temp();
  }

  // remap text as RW
  const u64 text_asize = ALIGN_MEM(text_size, 0x1000); // align to page
  svcSetProcessMemoryPermission(envGetOwnProcessHandle(), (u64)text_virtbase, text_asize, Perm_Rw);
  // unmap everything
  svcUnmapProcessCodeMemory(envGetOwnProcessHandle(), (u64)load_virtbase, (u64)load_base, load_size);

  // release virtual address range
  virtmemLock();
  virtmemRemoveReservation(load_memrv);
  virtmemUnlock();

  return 0;
}
