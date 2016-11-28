/*
 * NewMadeleine
 * Copyright (C) 2014-2016 (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULA R PURPOSE.  See the GNU
 * General Public License for more details.
 */

/** @file nm_data.c High-level data manipulation through iterators
 */

#include <nm_private.h>

#include <alloca.h>
#include <setjmp.h>
#include <ucontext.h>

PADICO_MODULE_HOOK(NewMad_Core);


/* ********************************************************* */
/* ** contiguous data */

struct nm_data_contiguous_generator_s
{
  int done; /**< whether block is processed (boolean) */
};
static void nm_data_contiguous_generator(const struct nm_data_s*p_data, void*_generator)
{
  struct nm_data_contiguous_generator_s*p_generator = _generator;
  p_generator->done = 0;
}
static struct nm_data_chunk_s nm_data_contiguous_next(const struct nm_data_s*p_data, void*_generator)
{
  const struct nm_data_contiguous_s*p_contiguous = nm_data_contiguous_content(p_data);
  struct nm_data_contiguous_generator_s*p_generator = _generator;
  if(p_generator->done)
    {
      return NM_DATA_CHUNK_NULL;
    }
  else
    {
      const struct nm_data_chunk_s chunk = { .ptr = p_contiguous->ptr, .len = p_contiguous->len };
      p_generator->done = 1;
      return chunk;
    }
}
static void nm_data_contiguous_traversal(const void*_content, nm_data_apply_t apply, void*_context)
{
  const struct nm_data_contiguous_s*p_contiguous = _content;
  void*ptr = p_contiguous->ptr;
  const nm_len_t len = p_contiguous->len;
  (*apply)(ptr, len, _context);
}
const struct nm_data_ops_s nm_data_ops_contiguous =
  {
    .p_traversal = &nm_data_contiguous_traversal,
    .p_generator = &nm_data_contiguous_generator,
    .p_next      = &nm_data_contiguous_next
  };

/* ********************************************************* */
/* ** iovec data */

struct nm_data_iov_generator_s
{
  int i; /**< current index in v */
};
static void nm_data_iov_generator(const struct nm_data_s*p_data, void*_generator)
{
  struct nm_data_iov_generator_s*p_generator = _generator;
  p_generator->i = 0;
}
static struct nm_data_chunk_s nm_data_iov_next(const struct nm_data_s*p_data, void*_generator)
{
  const struct nm_data_iov_s*p_iov = nm_data_iov_content(p_data);
  struct nm_data_iov_generator_s*p_generator = _generator;
  const struct iovec*v = &p_iov->v[p_generator->i];
  const int n = p_iov->n;
  if(p_generator->i >= n)
    {
      return NM_DATA_CHUNK_NULL;
    }
  else
    {
      const struct nm_data_chunk_s chunk = { .ptr = v->iov_base, .len = v->iov_len };
      p_generator->i++;
      return chunk;
    }
}
static void nm_data_iov_traversal(const void*_content, nm_data_apply_t apply, void*_context)
{
  const struct nm_data_iov_s*p_iov = _content;
  const struct iovec*const v = p_iov->v;
  const int n = p_iov->n;
  int i;
  for(i = 0; i < n; i++)
    {
      (*apply)(v[i].iov_base, v[i].iov_len, _context);
    }
}
const struct nm_data_ops_s nm_data_ops_iov =
  {
    .p_traversal = &nm_data_iov_traversal,
    .p_generator = &nm_data_iov_generator,
    .p_next      = &nm_data_iov_next
  };

/* ********************************************************* */

/** filter function application on aggregated contiguous chunks
 */
struct nm_data_aggregator_s
{
  void*chunk_ptr;         /**< pointer on current chunk begin */
  nm_len_t chunk_len;     /**< length of current chunk beeing processed */
  nm_data_apply_t apply;  /**< composed function to apply to chunk */
  void*_context;          /**< context for composed apply function */
};
static void nm_data_aggregator_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_aggregator_s*p_context = _context;
  if((ptr != NULL) && (ptr == p_context->chunk_ptr + p_context->chunk_len))
    {
      /* contiguous with current chunk */
      p_context->chunk_len += len;
    }
  else
    {
      /* not contiguous; flush prev chunk, set current block as current chunk */
      if(p_context->chunk_len != NM_LEN_UNDEFINED)
	{
	  (*p_context->apply)(p_context->chunk_ptr, p_context->chunk_len, p_context->_context);
	}
      p_context->chunk_ptr = ptr;
      p_context->chunk_len = len;
    }
}
void nm_data_aggregator_traversal(const struct nm_data_s*p_data, nm_data_apply_t apply, void*_context)
{
  const struct nm_data_properties_s*p_props = nm_data_properties_get(p_data);
  if(p_props->is_contig)
    {
      void*base_ptr = nm_data_baseptr_get(p_data);
      (*apply)(base_ptr, p_props->size, _context);
    }
  else
    {
      struct nm_data_aggregator_s context = { .chunk_ptr = NULL, .chunk_len = NM_LEN_UNDEFINED, .apply = apply, ._context = _context };
      nm_data_traversal_apply(p_data, &nm_data_aggregator_apply, &context);
      if(context.chunk_len != NM_LEN_UNDEFINED)
	{
	  /* flush last pending chunk */
	  (*context.apply)(context.chunk_ptr, context.chunk_len, context._context);
	}
      else
	{
	  /* zero-length data */
	  (*context.apply)(NULL, 0, context._context);
	}
    }
 }

/* ********************************************************* */

/** filter function application to a delimited sub-set of data
 */
struct nm_data_chunk_extractor_s
{
  nm_len_t chunk_offset; /**< offset for begin of copy at destination */
  nm_len_t chunk_len;    /**< length to copy */
  nm_len_t done;         /**< offset done so far at destination */
  nm_data_apply_t apply; /**< composed function to apply to chunk */
  void*_context;         /**< context for composed apply function */
};
static void nm_data_chunk_extractor_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_chunk_extractor_s*p_context = _context;
  const nm_len_t chunk_offset = p_context->chunk_offset;
  const nm_len_t chunk_len = p_context->chunk_len;
  if( (!(p_context->done + len <= p_context->chunk_offset))                   /* data before chunk- do nothing */
      &&
      (!(p_context->done >= p_context->chunk_offset + p_context->chunk_len))) /* data after chunk- do nothing */
    {
      /* data in chunk */
      const nm_len_t block_offset = (p_context->done < chunk_offset) ? (chunk_offset - p_context->done) : 0;
      const nm_len_t block_len = (chunk_offset + chunk_len > p_context->done + len) ? 
	(len - block_offset) : (chunk_offset + chunk_len - p_context->done - block_offset);
      (*p_context->apply)(ptr + block_offset, block_len, p_context->_context);
    }
  p_context->done += len;
}
/** apply function to only a given chunk of data */
void nm_data_chunk_extractor_traversal(const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len,
				       nm_data_apply_t apply, void*_context)
{
  if(chunk_len != 0)
    {
      if((chunk_offset == 0) && (chunk_len == nm_data_size(p_data)))
	{
	  nm_data_traversal_apply(p_data, apply, _context);
	}
	else
	  {
	    struct nm_data_chunk_extractor_s chunk_extractor = 
	      { .chunk_offset = chunk_offset, .chunk_len = chunk_len, .done = 0,
		.apply = apply, ._context = _context };
	    nm_data_traversal_apply(p_data, &nm_data_chunk_extractor_apply, &chunk_extractor);
	  }
    }
  else
    {
      (*apply)(NULL, 0, _context);
    }
}

/* ********************************************************* */
/* ** generic generator, using traversal */
/* use with care, complexity is n^2 */

struct nm_data_generic_traversal_s
{
  const int target;
  int done;
  struct nm_data_chunk_s chunk;
};
static void nm_data_generic_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_generic_traversal_s*p_generic = _context;
  if(p_generic->target == p_generic->done)
    {
      p_generic->chunk.ptr = ptr;
      p_generic->chunk.len = len;
    }
  p_generic->done++;
}
struct nm_data_generic_generator_s
{
  int i;
};
void nm_data_generic_generator(const struct nm_data_s*p_data, void*_generator)
{
  struct nm_data_generic_generator_s*p_generator = _generator;
  p_generator->i = 0;
}
struct nm_data_chunk_s nm_data_generic_next(const struct nm_data_s*p_data, void*_generator)
{
  struct nm_data_generic_generator_s*p_generator = _generator;
  struct nm_data_generic_traversal_s generic_traversal =
    {
      .target = p_generator->i,
      .done   = 0,
      .chunk  = (struct nm_data_chunk_s){.ptr = NULL, .len = 0 }
    };
  nm_data_traversal_apply(p_data, &nm_data_generic_apply, &generic_traversal);
  p_generator->i++;
  return generic_traversal.chunk;
}

/* ********************************************************* */
/* ** coroutine generator, using traversal */

#define NM_DATA_COROUTINE_STACK (256 * 1024)

struct nm_data_coroutine_traversal_s
{
  struct nm_data_chunk_s chunk;
  jmp_buf caller_context;
  jmp_buf traversal_context;
  ucontext_t generator_context;
};
static void nm_data_coroutine_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_coroutine_traversal_s*p_coroutine = _context;
  /*  fprintf(stderr, "# coroutine_apply- len = %d; ptr = %p\n", len, ptr); */
  p_coroutine->chunk.ptr = ptr;
  p_coroutine->chunk.len = len;
  if(_setjmp(p_coroutine->traversal_context))
    return;
  _longjmp(p_coroutine->caller_context, 1);
}
static void nm_data_coroutine_trampoline(const struct nm_data_s*p_data, struct nm_data_coroutine_traversal_s*p_coroutine)
{
  /* fprintf(stderr, "# coroutine_trampoline-\n");  */
  if(_setjmp(p_coroutine->traversal_context) == 0)
    {
      /* init */
      _longjmp(p_coroutine->caller_context, 1);
    }
  else
    {
      /* back from longjmp- perform traversal */
      /*   fprintf(stderr, "# coroutine_trampoline- back from longjmp\n"); */
      nm_data_traversal_apply(p_data, &nm_data_coroutine_apply, p_coroutine);
      p_coroutine->chunk = NM_DATA_CHUNK_NULL;
      if(_setjmp(p_coroutine->traversal_context))
	{
	  fprintf(stderr, "# nm_data_coroutine_trampoline- ### after traversal ###\n");
	  abort();
	}
      _longjmp(p_coroutine->caller_context, 1);
    }
}
struct nm_data_coroutine_generator_s
{
  struct nm_data_coroutine_traversal_s*p_coroutine;
  void*stack;
};
void nm_data_coroutine_generator(const struct nm_data_s*p_data, void*_generator)
{
  struct nm_data_coroutine_generator_s*p_generator = _generator;
  /* fprintf(stderr, "# coroutine_generator- generator = %p; data size = %d\n", p_generator, nm_data_size(p_data)); */
  p_generator->p_coroutine = malloc(sizeof(struct nm_data_coroutine_traversal_s));
  getcontext(&p_generator->p_coroutine->generator_context);
  p_generator->stack = malloc(NM_DATA_COROUTINE_STACK);
  p_generator->p_coroutine->generator_context.uc_stack.ss_sp = p_generator->stack;
  p_generator->p_coroutine->generator_context.uc_stack.ss_size = NM_DATA_COROUTINE_STACK;
  makecontext(&p_generator->p_coroutine->generator_context, (void*)&nm_data_coroutine_trampoline, 2, p_data, p_generator->p_coroutine);
#ifdef __ia64__
#error setjmp/longjmp not supported on Itanium. TODO- write a version based on switchcontext()
#endif /* __ia64__ */
  if(_setjmp(p_generator->p_coroutine->caller_context) == 0)
    {
      setcontext(&p_generator->p_coroutine->generator_context);
      fprintf(stderr, "# ### after trampoline\n");
    }
  else
    {
      /* back from longjmp */
      /* fprintf(stderr, "# coroutine_generator- generator = %p; back from longjmp\n", p_generator);  */
    }
}
struct nm_data_chunk_s nm_data_coroutine_next(const struct nm_data_s*p_data, void*_generator)
{
  struct nm_data_coroutine_generator_s*p_generator = _generator;
  /*  fprintf(stderr, "# coroutine_next- generator = %p\n", p_generator); */
  if(_setjmp(p_generator->p_coroutine->caller_context) == 0)
    {
      /* init */
      _longjmp(p_generator->p_coroutine->traversal_context, 1);
    }
  return p_generator->p_coroutine->chunk;
}

void nm_data_coroutine_generator_destroy(const struct nm_data_s*p_data, void*_generator)
{
  struct nm_data_coroutine_generator_s*p_generator = _generator;
  free(p_generator->stack);
  free(p_generator->p_coroutine);
}


/* ********************************************************* */
/* ** sliced generator */

void nm_data_slicer_generator_init(nm_data_slicer_t*p_slicer, const struct nm_data_s*p_data)
{
#ifdef DEBUG
  p_slicer->done = 0;
#endif /* DEBUG */
  p_slicer->p_data = p_data;
  p_slicer->pending_chunk = (struct nm_data_chunk_s){ .ptr = NULL, .len = 0 };
  nm_data_generator_init(p_data, &p_slicer->generator);
}

void nm_data_slicer_generator_forward(nm_data_slicer_t*p_slicer, nm_len_t offset)
{
  struct nm_data_chunk_s chunk = p_slicer->pending_chunk;
  const struct nm_data_s*p_data = p_slicer->p_data;
  while(offset > 0)
    {
      if(chunk.len == 0)
	chunk = nm_data_generator_next(p_data, &p_slicer->generator);
      const nm_len_t chunk_len = (chunk.len > offset) ? offset : chunk.len;
      chunk.ptr += chunk_len;
      chunk.len -= chunk_len;
      offset    -= chunk_len;
    }
  p_slicer->pending_chunk = chunk;
#ifdef DEBUG
  p_slicer->done += offset;
  assert(p_slicer->done <= nm_data_size(p_slicer->p_data));
#endif /* DEBUG */
}

void nm_data_slicer_generator_copy_from(nm_data_slicer_t*p_slicer, void*dest_ptr, nm_len_t slice_len)
{
  struct nm_data_chunk_s chunk = p_slicer->pending_chunk;
  const struct nm_data_s*const p_data = p_slicer->p_data;
  while(slice_len > 0)
    {
      if(chunk.len == 0)
	chunk = nm_data_generator_next(p_data, &p_slicer->generator);
      const nm_len_t chunk_len = (chunk.len > slice_len) ? slice_len : chunk.len;
      memcpy(dest_ptr, chunk.ptr, chunk_len);
      dest_ptr += chunk_len;
      chunk.ptr += chunk_len;
      chunk.len -= chunk_len;
      slice_len -= chunk_len;
#ifdef DEBUG
      p_slicer->done += chunk_len;
      assert(p_slicer->done <= nm_data_size(p_slicer->p_data));
#endif /* DEBUG */
    }
  p_slicer->pending_chunk = chunk;
}

void nm_data_slicer_generator_copy_to(nm_data_slicer_t*p_slicer, const void*src_ptr, nm_len_t slice_len)
{
  struct nm_data_chunk_s chunk = p_slicer->pending_chunk;
  const struct nm_data_s*const p_data = p_slicer->p_data;
  while(slice_len > 0)
    {
      if(chunk.len == 0)
	chunk = nm_data_generator_next(p_data, &p_slicer->generator);
      const nm_len_t chunk_len = (chunk.len > slice_len) ? slice_len : chunk.len;
      memcpy(chunk.ptr, src_ptr, chunk_len);
      src_ptr   += chunk_len;
      chunk.ptr += chunk_len;
      chunk.len -= chunk_len;
      slice_len -= chunk_len;
#ifdef DEBUG
      p_slicer->done += chunk_len;
      assert(p_slicer->done <= nm_data_size(p_slicer->p_data));
#endif /* DEBUG */
    }
  p_slicer->pending_chunk = chunk;
}

void nm_data_slicer_generator_destroy(nm_data_slicer_t*p_slicer)
{
  const struct nm_data_s*p_data = p_slicer->p_data;
  nm_data_generator_destroy(p_data, &p_slicer->generator);
}

/* ********************************************************* */
/* ** alternate coroutine-based slicer */

struct nm_data_slicer_coroutine_s
{
  enum nm_data_slicer_coroutine_op_e
    {
      NM_SLICER_OP_NONE = 0,
      NM_SLICER_OP_FORWARD,
      NM_SLICER_OP_COPY_FROM,
      NM_SLICER_OP_COPY_TO
    } op;
  const struct nm_data_s*p_data;
  void*ptr;                     /**< pointer to copy from/to contiguous data */
  volatile nm_len_t slice_len;  /**< length of the current slice */
  jmp_buf trampoline_context;   /**< initial context of trampoline function */
  jmp_buf caller_context;       /**< context to go back to the application stack */
  jmp_buf traversal_context;    /**< context of the coroutine for data traversal */
  ucontext_t init_context;
  void*stack;
  nm_len_t done;                /**< length of data processed so far */
};
PUK_LFQUEUE_TYPE(nm_data_slicer_coroutine, struct nm_data_slicer_coroutine_s*, NULL, 8);
static struct nm_data_slicer_coroutine_lfqueue_s nm_data_slicer_coroutine_cache = PUK_LFQUEUE_INITIALIZER(NULL);

static void nm_data_slicer_coroutine_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_slicer_coroutine_s*p_coroutine = _context;
  nm_len_t chunk_len = 0;
 restart:
  chunk_len = (len > p_coroutine->slice_len) ? p_coroutine->slice_len : len;
  if(p_coroutine->op == NM_SLICER_OP_COPY_FROM)
    {
      memcpy(p_coroutine->ptr, ptr, chunk_len);
    }
  else if(p_coroutine->op == NM_SLICER_OP_COPY_TO)
    {
      memcpy(ptr, p_coroutine->ptr, chunk_len);
    }
  else if(p_coroutine->op == NM_SLICER_OP_FORWARD)
    {
      /* do nothing */
    }
  else
    {
      fprintf(stderr, "# nmad: FATAL- wrong state %d for data slicer.\n", p_coroutine->op);
      abort();
    }
  p_coroutine->slice_len -= chunk_len;
  p_coroutine->ptr       += chunk_len;
#ifdef DEBUG
  p_coroutine->done      += chunk_len;
#endif
  if(p_coroutine->slice_len == 0)
    {
      /* slice is done- switch context */
      if(_setjmp(p_coroutine->traversal_context) == 0)
	{
	  _longjmp(p_coroutine->caller_context, 1);
	}
      if(len != chunk_len)
	{
	  /* process remainder */
	  ptr += chunk_len;
	  len -= chunk_len;
	  goto restart;
	}
    }
}
static void nm_data_slicer_coroutine_trampoline(struct nm_data_slicer_coroutine_s*p_coroutine)
{
  if(_setjmp(p_coroutine->trampoline_context) == 0)
    {
      /* back to coroutine alloc */
      _longjmp(p_coroutine->caller_context, 1);
    }
  else
    {
      if(_setjmp(p_coroutine->traversal_context) == 0)
	{
	  /* back to slicer init */
	  _longjmp(p_coroutine->caller_context, 1);
	}
      else
	{
	  /* back from longjmp- perform traversal */
	  nm_data_traversal_apply(p_coroutine->p_data, &nm_data_slicer_coroutine_apply, p_coroutine);
	  fprintf(stderr, "# slicer_coroutine_trampoline- ### after traversal ###\n");
	  abort();
	}
    }
}
struct nm_data_slicer_coroutine_s*nm_data_slicer_coroutine_alloc(void)
{
  struct nm_data_slicer_coroutine_s*p_coroutine = nm_data_slicer_coroutine_lfqueue_dequeue(&nm_data_slicer_coroutine_cache);
  if(p_coroutine == NULL)
    {
      /* cache miss */
      p_coroutine = malloc(sizeof(struct nm_data_slicer_coroutine_s));
      /* create new coroutine context */
      getcontext(&p_coroutine->init_context);
      p_coroutine->stack = malloc(NM_DATA_COROUTINE_STACK);
      p_coroutine->init_context.uc_stack.ss_sp = p_coroutine->stack;
      p_coroutine->init_context.uc_stack.ss_size = NM_DATA_COROUTINE_STACK;
      makecontext(&p_coroutine->init_context, (void*)&nm_data_slicer_coroutine_trampoline, 1, p_coroutine);
      if(_setjmp(p_coroutine->caller_context) == 0)
	{
	  setcontext(&p_coroutine->init_context);
	  fprintf(stderr, "# nmad: nm_data_slicer_coroutine_alloc()- internal error: after trampoline ### \n");
	  abort();
	}
    }
  /* init coroutine */
#ifdef DEBUG
  p_coroutine->done = 0;
#endif
  p_coroutine->op = NM_SLICER_OP_NONE;
  p_coroutine->ptr = NULL;
  p_coroutine->slice_len = 0;
  return p_coroutine;
}
void nm_data_slicer_coroutine_init(nm_data_slicer_t*p_slicer, const struct nm_data_s*p_data)
{
  p_slicer->p_data = p_data;
  p_slicer->p_coroutine = nm_data_slicer_coroutine_alloc();
  p_slicer->p_coroutine->p_data = p_data;
  if(_setjmp(p_slicer->p_coroutine->caller_context) == 0)
    {
      /* jump to trampoline once to initialize traversal_context */
      _longjmp(p_slicer->p_coroutine->trampoline_context, 1);
      fprintf(stderr, "# nmad: nm_data_slicer_coroutine_init()- internal error: after trampoline ### \n");
      abort();
    }
}

static inline void nm_data_slicer_coroutine_op(nm_data_slicer_t*p_slicer, void*ptr, nm_len_t len, enum nm_data_slicer_coroutine_op_e op)
{
  struct nm_data_slicer_coroutine_s*p_coroutine = p_slicer->p_coroutine;
  assert(p_coroutine->slice_len == 0);
  assert(p_coroutine->op == NM_SLICER_OP_NONE);
  p_coroutine->op = op;
  p_coroutine->ptr = ptr;
  p_coroutine->slice_len = len;
  if(_setjmp(p_coroutine->caller_context) == 0)
    {
      _longjmp(p_coroutine->traversal_context, 1);
    }
  assert(p_coroutine->slice_len == 0);
  p_coroutine->op = NM_SLICER_OP_NONE;
}

void nm_data_slicer_coroutine_copy_from(nm_data_slicer_t*p_slicer, void*dest_ptr, nm_len_t slice_len)
{
  nm_data_slicer_coroutine_op(p_slicer, dest_ptr, slice_len, NM_SLICER_OP_COPY_FROM);
}

void nm_data_slicer_coroutine_copy_to(nm_data_slicer_t*p_slicer, const void*src_ptr, nm_len_t slice_len)
{
  nm_data_slicer_coroutine_op(p_slicer, (void*)src_ptr, slice_len, NM_SLICER_OP_COPY_TO);
}

void nm_data_slicer_coroutine_forward(nm_data_slicer_t*p_slicer, nm_len_t offset)
{
  nm_data_slicer_coroutine_op(p_slicer, NULL, offset, NM_SLICER_OP_FORWARD);
}

void nm_data_slicer_coroutine_destroy(nm_data_slicer_t*p_slicer)
{
  struct nm_data_slicer_coroutine_s*p_coroutine = p_slicer->p_coroutine;
  p_slicer->p_coroutine = NULL;
  p_slicer->p_data = NULL;
  p_coroutine->p_data = NULL;
  p_coroutine->op = NM_SLICER_OP_NONE;
  p_coroutine->ptr = NULL;
  int rc = nm_data_slicer_coroutine_lfqueue_enqueue(&nm_data_slicer_coroutine_cache, p_coroutine);
  if(rc)
    {
      /* cache full- free coroutine */
      free(p_coroutine->stack);
      free(p_coroutine);
    }
}



/* ********************************************************* */


/** compute various data properties
 */
struct nm_data_properties_context_s
{
  void*blockend; /**< end of previous block*/
  struct nm_data_properties_s props;
};
static void nm_data_properties_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_properties_context_s*p_context = _context;
  p_context->props.size += len;
  p_context->props.blocks += 1;
  if(p_context->props.is_contig)
    {
      if((p_context->blockend != NULL) && (ptr != p_context->blockend))
	p_context->props.is_contig = 0;
      p_context->blockend = ptr + len;
    }
}

void nm_data_default_properties_compute(struct nm_data_s*p_data)
{
  assert(p_data->props.blocks == -1);
  struct nm_data_properties_context_s context = { .blockend = NULL,
						  .props = { .size = 0, .blocks = 0, .is_contig = 1 }  };
  nm_data_traversal_apply(p_data, &nm_data_properties_apply, &context);
  p_data->props = context.props;
}

struct nm_data_baseptr_context_s
{
  void*base_ptr; /**< base pointer, NULL if not known yet */
};
static void nm_data_baseptr_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_baseptr_context_s*p_context = _context;
  if(p_context->base_ptr == NULL)
    p_context->base_ptr = ptr;
}
void*nm_data_baseptr_get(const struct nm_data_s*p_data)
{
#ifdef DEBUG
  const struct nm_data_properties_s*p_props = nm_data_properties_get(p_data);
  assert(p_props->is_contig);
#endif /* DEBUG */
  struct nm_data_baseptr_context_s context = { .base_ptr = NULL };
  nm_data_traversal_apply(p_data, &nm_data_baseptr_apply, &context);
  return context.base_ptr;
}

/* ********************************************************* */

/** copy data from network buffer (contiguous) to user layout
 */
struct nm_data_copy_to_s
{
  const void*ptr; /**< source buffer (contiguous) */
};
static void nm_data_copy_to_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_copy_to_s*p_context = _context;
  memcpy(ptr, p_context->ptr, len);
  p_context->ptr += len;
}
/** copy chunk of data to user layout */
void nm_data_copy_to(const struct nm_data_s*p_data, nm_len_t offset, nm_len_t len, const void*srcbuf)
{
  if(len > 0)
    {
      struct nm_data_copy_to_s copy = { .ptr = srcbuf };
      nm_data_chunk_extractor_traversal(p_data, offset, len, &nm_data_copy_to_apply, &copy);
    }
}

/* ********************************************************* */

/** copy data from user layout to network buffer (contiguous)
 */
struct nm_data_copy_from_s
{
  void*ptr; /**< dest buffer (contiguous) */
};
static void nm_data_copy_from_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_copy_from_s*p_context = _context;
  memcpy(p_context->ptr, ptr, len);
  p_context->ptr += len;
}

void nm_data_copy_from(const struct nm_data_s*p_data, nm_len_t offset, nm_len_t len, void*destbuf)
{
  if(len > 0)
    {
      struct nm_data_copy_from_s copy = { .ptr = destbuf };
      nm_data_chunk_extractor_traversal(p_data, offset, len, &nm_data_copy_from_apply, &copy);
    }
}

/* ********************************************************* */

/** pack iterator-based data to pw with global header
 */
struct nm_data_pkt_packer_s
{
  struct nm_pkt_wrap_s*p_pw; /**< the pw to fill */
  nm_len_t skip;
  uint16_t*p_prev_len;     /**< previous block */
};
static void nm_data_pkt_pack_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_pkt_packer_s*p_context = _context;
  struct nm_pkt_wrap_s*p_pw = p_context->p_pw;
  struct iovec*v0 = &p_pw->v[0];
  if(len < NM_DATA_IOV_THRESHOLD)
    {
      /* small data in header */
      assert(len <= UINT16_MAX);
      if((p_context->p_prev_len != NULL) && (*p_context->p_prev_len + len < NM_DATA_IOV_THRESHOLD))
	{
	  *p_context->p_prev_len += len;
	  memcpy(v0->iov_base + v0->iov_len, ptr, len);
	  v0->iov_len += len;
	  p_pw->length += len;
	}
      else
	{
	  uint16_t*p_len = v0->iov_base + v0->iov_len;
	  *p_len = len;
	  memcpy(v0->iov_base + v0->iov_len + sizeof(uint16_t), ptr, len);
	  v0->iov_len += len + sizeof(uint16_t);
	  p_pw->length += len + sizeof(uint16_t);
	  p_context->p_prev_len = p_len;
	}
    }
  else
    {
      /* data in iovec */
      struct nm_header_pkt_data_chunk_s*p_chunk_header = v0->iov_base + v0->iov_len;
      assert(len <= UINT16_MAX);
      p_chunk_header->len = len;
      if(p_context->skip == 0)
	{
	  nm_len_t skip = 0;
	  int i;
	  for(i = 1; i < p_pw->v_nb; i++)
	    {
	      skip += p_pw->v[i].iov_len;
	    }
	  p_context->skip = skip;
	}
      assert(p_context->skip < UINT16_MAX);
      struct iovec*v = nm_pw_grow_iovec(p_context->p_pw);
      v->iov_base = ptr;
      v->iov_len = len;
      p_chunk_header->skip = p_context->skip;
      p_pw->v[0].iov_len += sizeof(struct nm_header_pkt_data_chunk_s);
      p_pw->length += len + sizeof(struct nm_header_pkt_data_chunk_s);
      p_context->skip += len;
      p_context->p_prev_len = NULL;
    }
}

void nm_data_pkt_pack(struct nm_pkt_wrap_s*p_pw, nm_core_tag_t tag, nm_seq_t seq,
		      const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len, uint8_t flags)
{
  struct iovec*v0 = &p_pw->v[0];
  struct nm_header_pkt_data_s*h = v0->iov_base + v0->iov_len;
  nm_header_init_pkt_data(h, tag, seq, flags, chunk_len, chunk_offset);
  v0->iov_len  += NM_HEADER_PKT_DATA_SIZE;
  p_pw->length += NM_HEADER_PKT_DATA_SIZE;
  struct nm_data_pkt_packer_s packer = { .p_pw = p_pw, .skip = 0, .p_prev_len = NULL };
  nm_data_chunk_extractor_traversal(p_data, chunk_offset, chunk_len, &nm_data_pkt_pack_apply, &packer);
  v0 = &p_pw->v[0]; /* pw->v may have moved- update */
  assert(v0->iov_len <= UINT16_MAX);
  h->hlen = (v0->iov_base + v0->iov_len) - (void*)h;
}


/* ********************************************************* */

/** Unpack data from packed pkt to nm_data
*/
struct nm_data_pkt_unpacker_s
{
  const struct nm_header_pkt_data_s*const h;
  const void*ptr;     /**< current source chunk in buffer */
  const void*rem_buf; /**< pointer to remainder from previous block */
  uint16_t rem_len;   /**< length of remainder */
  const void*v1_base; /**< v0 + skip, base of iov-based fragments */
};
static void nm_data_pkt_unpack_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_data_pkt_unpacker_s*p_context = _context;
  const uint16_t hlen = p_context->h->hlen;
  const void*rbuffer  = p_context->rem_buf;
  uint16_t rlen       = p_context->rem_len;
  const void*rptr     = p_context->ptr;
  while(len > 0)
    {
      if(rbuffer == NULL)
	{
	  /* load next block */
	  const uint16_t*p_len = rptr;
	  rlen = *p_len;
	  if((rptr - (void*)p_context->h) >= hlen)
	    {
	      p_context->rem_buf = NULL;
	      return;
	    }
	  else if(rlen < NM_DATA_IOV_THRESHOLD)
	    {
	      rbuffer = rptr + 2;
	      rptr += rlen + sizeof(uint16_t);
	    }	  
	  else
	    {
	      const uint16_t*p_skip = rptr + 2;
	      rbuffer = p_context->v1_base + *p_skip;
	      rptr += 2 * sizeof(uint16_t);
	    }
	}
      uint16_t blen = rlen;
      const void*const bbuffer = rbuffer;
      if(blen > len)
	{
	  /* consume len bytes, and truncate block */
	  blen = len;
	  rbuffer += len;
	  rlen -= len;
	}
      else
	{
	  /* consume block */
	  rbuffer = NULL;
	}
      memcpy(ptr, bbuffer, blen);
      ptr += blen;
      len -= blen;
    }
  p_context->rem_buf = rbuffer;
  p_context->rem_len = rlen;
  p_context->ptr = rptr;
}

/** unpack from pkt format to data format */
void nm_data_pkt_unpack(const struct nm_data_s*p_data, const struct nm_header_pkt_data_s*h, const struct nm_pkt_wrap_s*p_pw,
			nm_len_t chunk_offset, nm_len_t chunk_len)
{
  struct nm_data_pkt_unpacker_s data_unpacker =
    { .h = h, .ptr = h + 1, .v1_base = p_pw->v[0].iov_base + nm_header_global_v0len(p_pw), .rem_buf = NULL };
  nm_data_chunk_extractor_traversal(p_data, chunk_offset, chunk_len, &nm_data_pkt_unpack_apply, &data_unpacker);
}
