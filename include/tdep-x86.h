/* libunwind - a platform-independent unwind library
   Copyright (C) 2002-2004 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

#ifndef TDEP_X86_H
#define TDEP_X86_H

/* Target-dependent definitions that are internal to libunwind but need
   to be shared with target-independent code.  */

#include <stdlib.h>
#include <libunwind.h>

#include "elf32.h"
#include "dwarf.h"

struct unw_addr_space
  {
    struct unw_accessors acc;
    unw_caching_policy_t caching_policy;
    uint32_t cache_generation;
    unw_word_t dyn_generation;		/* see dyn-common.h */
    unw_word_t dyn_info_list_addr;	/* (cached) dyn_info_list_addr */
   };

struct cursor
  {
    struct dwarf_cursor dwarf;		/* must be first */

    /* Format of sigcontext structure and address at which it is
       stored: */
    enum
      {
	X86_SCF_NONE,			/* no signal frame encountered */
	X86_SCF_LINUX_SIGFRAME,		/* classic x86 sigcontext */
	X86_SCF_LINUX_RT_SIGFRAME	/* POSIX ucontext_t */
      }
    sigcontext_format;
    unw_word_t sigcontext_addr;
  };

#define DWARF_GET_LOC(l)	((l).val)

#ifdef UNW_LOCAL_ONLY
# define DWARF_NULL_LOC		DWARF_LOC (0, 0)
# define DWARF_IS_NULL_LOC(l)	(DWARF_GET_LOC (l) == 0)
# define DWARF_LOC(r, t)	((dwarf_loc_t) { .val = (r) })
# define DWARF_REG_LOC(c,r)	(DWARF_LOC((unw_word_t)			     \
				 tdep_uc_addr((c)->as_arg, (r)), 0))
# define DWARF_MEM_LOC(c,m)	DWARF_LOC ((m), 0)
# define DWARF_FPREG_LOC(c,r)	(DWARF_LOC((unw_word_t)			     \
				 tdep_uc_addr((c)->as_arg, (r)), 0))

static inline int
dwarf_getfp (struct dwarf_cursor *c, dwarf_loc_t loc, unw_fpreg_t *val)
{
  if (!DWARF_GET_LOC (loc))
    return -1;
  *val = *(unw_fpreg_t *) DWARF_GET_LOC (loc);
  return 0;
}

static inline int
dwarf_putfp (struct dwarf_cursor *c, dwarf_loc_t loc, unw_fpreg_t val)
{
  if (!DWARF_GET_LOC (loc))
    return -1;
  *(unw_fpreg_t *) DWARF_GET_LOC (loc) = val;
  return 0;
}

static inline int
dwarf_get (struct dwarf_cursor *c, dwarf_loc_t loc, unw_word_t *val)
{
  if (!DWARF_GET_LOC (loc))
    return -1;
  *val = *(unw_word_t *) DWARF_GET_LOC (loc);
  return 0;
}

static inline int
dwarf_put (struct dwarf_cursor *c, dwarf_loc_t loc, unw_word_t val)
{
  if (!DWARF_GET_LOC (loc))
    return -1;
  *(unw_word_t *) DWARF_GET_LOC (loc) = val;
  return 0;
}

#else /* !UNW_LOCAL_ONLY */
# define DWARF_LOC_TYPE_FP	(1 << 0)
# define DWARF_LOC_TYPE_REG	(1 << 1)
# define DWARF_NULL_LOC		DWARF_LOC (0, 0)
# define DWARF_IS_NULL_LOC(l)						\
		({ dwarf_loc_t _l = (l); _l.val == 0 && _l.type == 0; })
# define DWARF_LOC(r, t)	((dwarf_loc_t) { .val = (r), .type = (t) })
# define DWARF_IS_REG_LOC(l)	(((l).type & DWARF_LOC_TYPE_REG) != 0)
# define DWARF_IS_FP_LOC(l)	(((l).type & DWARF_LOC_TYPE_FP) != 0)
# define DWARF_REG_LOC(c,r)	DWARF_LOC((r), DWARF_LOC_TYPE_REG)
# define DWARF_MEM_LOC(c,m)	DWARF_LOC ((m), 0)
# define DWARF_FPREG_LOC(c,r)	DWARF_LOC((r), (DWARF_LOC_TYPE_REG	\
						| DWARF_LOC_TYPE_FP))

static inline int
dwarf_getfp (struct dwarf_cursor *c, dwarf_loc_t loc, unw_fpreg_t *val)
{
  unw_word_t addr;
  int ret;

  if (DWARF_IS_NULL_LOC (loc))
    return -UNW_EBADREG;

  if (DWARF_IS_REG_LOC (loc))
    return (*c->as->acc.access_fpreg) (c->as, DWARF_GET_LOC (loc),
				       val, 0, c->as_arg);

  addr = DWARF_GET_LOC (loc);
  if ((ret = (*c->as->acc.access_mem) (c->as, addr + 0, (unw_word_t *) &val,
				       0, c->as_arg)) < 0)
    return ret;

  return (*c->as->acc.access_mem) (c->as, addr + 4, (unw_word_t *) &val + 1, 0,
				   c->as_arg);
}

static inline int
dwarf_putfp (struct dwarf_cursor *c, dwarf_loc_t loc, unw_fpreg_t val)
{
  unw_word_t addr;
  int ret;

  if (DWARF_IS_NULL_LOC (loc))
    return -UNW_EBADREG;

  if (DWARF_IS_REG_LOC (loc))
    return (*c->as->acc.access_fpreg) (c->as, DWARF_GET_LOC (loc),
				       &val, 1, c->as_arg);

  addr = DWARF_GET_LOC (loc);
  if ((ret = (*c->as->acc.access_mem) (c->as, addr + 0, (unw_word_t *) &val,
				       1, c->as_arg)) < 0)
    return ret;

  return (*c->as->acc.access_mem) (c->as, addr + 4, (unw_word_t *) &val + 1,
				   1, c->as_arg);
}

static inline int
dwarf_get (struct dwarf_cursor *c, dwarf_loc_t loc, unw_word_t *val)
{
  if (DWARF_IS_NULL_LOC (loc))
    return -UNW_EBADREG;

  /* If a code-generator were to save a value of type unw_word_t in a
     floating-point register, we would have to support this case.  I
     suppose it could happen with MMX registers, but does it really
     happen?  */
  assert (!DWARF_IS_FP_LOC (loc));

  if (DWARF_IS_REG_LOC (loc))
    return (*c->as->acc.access_reg) (c->as, DWARF_GET_LOC (loc), val,
				     0, c->as_arg);
  else
    return (*c->as->acc.access_mem) (c->as, DWARF_GET_LOC (loc), val,
				     0, c->as_arg);
}

static inline int
dwarf_put (struct dwarf_cursor *c, dwarf_loc_t loc, unw_word_t val)
{
  if (DWARF_IS_NULL_LOC (loc))
    return -UNW_EBADREG;

  /* If a code-generator were to save a value of type unw_word_t in a
     floating-point register, we would have to support this case.  I
     suppose it could happen with MMX registers, but does it really
     happen?  */
  assert (!DWARF_IS_FP_LOC (loc));

  if (DWARF_IS_REG_LOC (loc))
    return (*c->as->acc.access_reg) (c->as, DWARF_GET_LOC (loc), &val,
				     1, c->as_arg);
  else
    return (*c->as->acc.access_mem) (c->as, DWARF_GET_LOC (loc), &val,
				     1, c->as_arg);
}

#endif /* !UNW_LOCAL_ONLY */

#define tdep_needs_initialization	UNW_OBJ(needs_initialization)
#define tdep_init			UNW_OBJ(init)
/* Platforms that support UNW_INFO_FORMAT_TABLE need to define
   tdep_search_unwind_table.  */
#define tdep_search_unwind_table	dwarf_search_unwind_table
#define tdep_uc_addr			UNW_ARCH_OBJ(uc_addr)
#define tdep_get_elf_image		UNW_ARCH_OBJ(get_elf_image)
#define tdep_access_reg			UNW_OBJ(access_reg)
#define tdep_access_fpreg		UNW_OBJ(access_fpreg)

#ifdef UNW_LOCAL_ONLY
# define tdep_find_proc_info(c,ip,n)				\
	dwarf_find_proc_info((c)->as, (ip), &(c)->pi, (n),	\
				       (c)->as_arg)
# define tdep_put_unwind_info(c,pi)				\
	dwarf_put_unwind_info((c)->as, (pi), (c)->as_arg)
#else
# define tdep_find_proc_info(c,ip,n)					\
	(*(c)->as->acc.find_proc_info)((c)->as, (ip), &(c)->pi, (n),	\
				       (c)->as_arg)
# define tdep_put_unwind_info(c,pi)					\
	(*(c)->as->acc.put_unwind_info)((c)->as, (pi), (c)->as_arg)
#endif

#define tdep_get_as(c)			((c)->dwarf.as)
#define tdep_get_as_arg(c)		((c)->dwarf.as_arg)
#define tdep_get_ip(c)			((c)->dwarf.ip)
#define tdep_big_endian(as)		0

extern int tdep_needs_initialization;

extern void tdep_init (void);
extern int tdep_search_unwind_table (unw_addr_space_t as, unw_word_t ip,
				     unw_dyn_info_t *di, unw_proc_info_t *pi,
				     int need_unwind_info, void *arg);
extern void *tdep_uc_addr (ucontext_t *uc, int reg);
extern int tdep_get_elf_image (struct elf_image *ei, pid_t pid, unw_word_t ip,
			       unsigned long *segbase, unsigned long *mapoff);
extern int tdep_access_reg (struct cursor *c, unw_regnum_t reg,
			    unw_word_t *valp, int write);
extern int tdep_access_fpreg (struct cursor *c, unw_regnum_t reg,
			      unw_fpreg_t *valp, int write);

#endif /* TDEP_X86_H */
