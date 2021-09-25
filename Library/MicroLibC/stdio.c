#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Base.h>
#include <Uefi.h>

#include <Library/MallocLib.h>
#include <Library/MemoryAllocationLib.h>

#include <stdio.h>

#define WT size_t
#define WS (sizeof(WT))

#define ARRAY_SIZE(a)                               \
  ((sizeof(a) / sizeof(*(a))) /                     \
  (size_t)(!(sizeof(a) % sizeof(*(a)))))
  
typedef unsigned uintptr_t;
int vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
  CHAR8 localfmt[100];
  UINTN i;
  UINTN fmtlen = AsciiStrSize(format);

  // copy fmt to our buffer
  CopyMem(localfmt, format, fmtlen);
  localfmt[fmtlen - 1] = 0;

  // fix format
  for (i = 1; localfmt[i]; i++) {
    if (localfmt[i - 1] == '%' && localfmt[i] == 's')
      localfmt[i] = 'a';
  }

  return AsciiVSPrint(str, size, localfmt, ap);
}

int snprintf(char *str, size_t size, const char *format, ...)
{
  int err;

  VA_LIST ap;
  VA_START(ap, format);
  err = vsnprintf(str, size, format, ap);
  VA_END(ap);

  return err;
}

int vprintf(const char *format, va_list ap)
{
  CHAR8 buf[100];

  vsnprintf(buf, ARRAY_SIZE(buf), format, ap);
  DEBUG((DEBUG_ERROR, "%a", buf));
  return 0;
}

int printf(const char *format, ...)
{
  int err;
DEBUG ((EFI_D_INFO, format));
  VA_LIST ap;
  VA_START(ap, format);
  err = vprintf(format, ap);
  VA_END(ap);

  return err;
}

int __printf_chk(const char *format, ...)
{
  int err;

  VA_LIST ap;
  VA_START(ap, format);
  err = vprintf(format, ap);
  VA_END(ap);

  return err;
}

#define CPOOL_HEAD_SIGNATURE SIGNATURE_32('C', 'p', 'h', 'd')

typedef enum {
  ALLOCTYPE_POOL,
  ALLOCTYPE_ALIGNED_PAGES,
  ALLOCTYPE_POOL_RUNTIME,
} ALLOCATION_TYPE;

typedef struct {
  UINT32          Signature;
  UINTN           Size;
  UINTN           Boundary;
  ALLOCATION_TYPE Type;

  UINT8 Data[0];
} CPOOL_HEAD;

VOID *memalign(UINTN Boundary, UINTN Size)
{
  return memalign2(Boundary, Size, FALSE);
}

VOID *memalign2(UINTN Boundary, UINTN Size, BOOLEAN runtime)
{
  VOID *          BaseMemory = NULL;
  CPOOL_HEAD *    Head;
  VOID *          RetVal;
  UINTN           HeadSize;
  UINTN           NodeSize;
  ALLOCATION_TYPE Type = ALLOCTYPE_POOL;

  if (Size == 0) {
    DEBUG((DEBUG_ERROR, "ERROR memalign: Zero Size\n"));
    return NULL;
  }

  if (Boundary == 8 && runtime) {
    Type = ALLOCTYPE_POOL_RUNTIME;
  }
  else if (Boundary == 8) {
    Type = ALLOCTYPE_POOL;
  }
  else {
    Type = ALLOCTYPE_ALIGNED_PAGES;
  }

  HeadSize = ALIGN_VALUE(sizeof(CPOOL_HEAD), Boundary);
  NodeSize = HeadSize + Size;

  DEBUG((DEBUG_POOL, "memalign(%d): NodeSz: %d", Size, NodeSize));

  if (Type == ALLOCTYPE_POOL) {
    BaseMemory = AllocatePool(NodeSize);
  }
  else if (Type == ALLOCTYPE_POOL_RUNTIME) {
    BaseMemory = AllocateRuntimePool(NodeSize);
  }
  else if (Type == ALLOCTYPE_ALIGNED_PAGES) {
    BaseMemory = AllocateAlignedPages(EFI_SIZE_TO_PAGES(NodeSize), Boundary);
    DEBUG((DEBUG_POOL, "Allocated at 0x%llx\n", BaseMemory));
  }
  else {
    ASSERT(FALSE);
  }

  if (BaseMemory == NULL) {
    RetVal = NULL;
    DEBUG((DEBUG_ERROR, "\nERROR memalign: alloc failed\n"));
  }
  else {
    ASSERT(BaseMemory != NULL);
    Head = BaseMemory + HeadSize - sizeof(CPOOL_HEAD);

    // Fill out the pool header
    Head->Signature = CPOOL_HEAD_SIGNATURE;
    Head->Size      = Size;
    Head->Boundary  = Boundary;
    Head->Type      = Type;

    // Return a pointer to the data
    RetVal = (VOID *)Head->Data;
    DEBUG((DEBUG_POOL, " Head: %p, Returns %p\n", Head, RetVal));
  }

  return RetVal;
}

VOID *malloc(UINTN Size) { return memalign(8, Size); }

VOID *malloc_rt(UINTN Size) { return memalign2(8, Size, TRUE); }

VOID *calloc(UINTN Count, UINTN Size)
{
  VOID *Ptr;

  Ptr = malloc(Count * Size);
  if (Ptr)
    SetMem(Ptr, Count * Size, 0);

  return Ptr;
}

VOID free(VOID *Ptr)
{
  CPOOL_HEAD *Head;
  UINTN       HeadSize;
  UINTN       NodeSize;
  VOID *      BaseMemory;

  if (Ptr != NULL) {
    Head = BASE_CR(Ptr, CPOOL_HEAD, Data);
    ASSERT(Head != NULL);
    DEBUG((DEBUG_POOL, "free(%p): Head: %p\n", Ptr, Head));

    if (Head->Signature == CPOOL_HEAD_SIGNATURE) {
      HeadSize   = ALIGN_VALUE(sizeof(CPOOL_HEAD), Head->Boundary);
      NodeSize   = HeadSize + Head->Size;
      BaseMemory = Ptr - HeadSize + 0x4;

      if (Head->Type == ALLOCTYPE_POOL ||
          Head->Type == ALLOCTYPE_POOL_RUNTIME) {
        FreePool(BaseMemory);
      }
      else if (Head->Type == ALLOCTYPE_ALIGNED_PAGES) {
        DEBUG(
            (DEBUG_POOL, "About to free %p, head size 0x%llx\n", BaseMemory,
             HeadSize));
        FreeAlignedPages(BaseMemory, EFI_SIZE_TO_PAGES(NodeSize));
      }
      else {
        ASSERT(FALSE);
      }
    }
    else {
      DEBUG((
          DEBUG_ERROR, "ERROR free(0x%p): Signature is 0x%8X, expected 0x%8X\n",
          Ptr, Head->Signature, CPOOL_HEAD_SIGNATURE));
    }
  }

  DEBUG((DEBUG_POOL, "free Done\n"));
}
void *memmove(void *dest, const void *src, size_t n)
{

CopyMem(dest, src, n);
	return dest;
}

VOID *realloc(VOID *Ptr, UINTN NewSize)
{
  VOID *      RetVal  = NULL;
  CPOOL_HEAD *Head    = NULL;
  UINTN       OldSize = 0;
  UINTN       NumCpy;
  BOOLEAN     IsRuntimePool = FALSE;

  // Find out the size of the OLD memory region
  if (Ptr != NULL) {
    Head = BASE_CR(Ptr, CPOOL_HEAD, Data);
    ASSERT(Head != NULL);
    if (Head->Signature != CPOOL_HEAD_SIGNATURE) {
      DEBUG(
          (DEBUG_ERROR,
           "ERROR realloc(0x%p): Signature is 0x%8X, expected 0x%8X\n", Ptr,
           Head->Signature, CPOOL_HEAD_SIGNATURE));
      return NULL;
    }
    if (Head->Type != ALLOCTYPE_POOL && Head->Type != ALLOCTYPE_POOL_RUNTIME) {
      DEBUG(
          (DEBUG_ERROR,
           "ERROR realloc(0x%p): Only malloc pointers are supported\n", Ptr));
    }
    OldSize       = Head->Size;
    IsRuntimePool = Head->Type == ALLOCTYPE_POOL_RUNTIME;
  }

  // At this point, Ptr is either NULL or a valid pointer to an allocated space

  if (NewSize == OldSize) {
    RetVal = Ptr;
  }
  else if (NewSize > 0) {
    if (IsRuntimePool) {
      RetVal = malloc_rt(NewSize); // Get the NEW memory region for runtime pool
    }
    else {
      RetVal = malloc(NewSize); // Get the NEW memory region
    }

    if (Ptr != NULL) {
      // If there is an OLD region...
      if (RetVal != NULL) {
        // and the NEW region was successfully allocated
        NumCpy = MIN(OldSize, NewSize);
        CopyMem(RetVal, Ptr, NumCpy); // Copy old data to the new region.
        free(Ptr);                    // and reclaim the old region.
      }
    }
  }
  else {
    free(Ptr); // Reclaim the old region.
  }

  DEBUG(
      (DEBUG_POOL, "0x%p = realloc(%p, %d): Head: %p\n", RetVal, Ptr, NewSize,
       Head));

  return RetVal;
}
