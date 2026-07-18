#include <switch.h>
#include <io_util.h>

extern size_t __nx_heap_size;

void mono_nx_fakemmap_init(intptr_t memory_start, intptr_t memory_end);

// Heap init code for libnx, except we steal half the memory from newlib for the custom mono fake mmap allocator.
void __libnx_initheap(void)
{
    void*  addr;
    size_t size = 0;
    size_t mem_available = 0, mem_used = 0;

    if (envHasHeapOverride()) {
        addr = envGetHeapOverrideAddr();
        size = envGetHeapOverrideSize();
    }
    else {
        if (__nx_heap_size==0) {
            svcGetInfo(&mem_available, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
            svcGetInfo(&mem_used, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);
            if (mem_available > mem_used+0x200000)
                size = (mem_available - mem_used - 0x200000) & ~0x1FFFFF;
            if (size==0)
                size = 0x2000000*16;
        }
        else {
            size = __nx_heap_size;
        }

        Result rc = svcSetHeapSize(&addr, size);

        if (R_FAILED(rc))
            diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_HeapAllocFailed));
    }

    // Newlib
    extern char* fake_heap_start;
    extern char* fake_heap_end;

	// Give libnx half the heap and mono the other half, Align the mono side to 4MB because we know that's the biggest granularity it needs.
	size_t newlib_size = size / 2;
	intptr_t mono_heap_start = (((intptr_t)addr + newlib_size) + (0x400000 - 1)) & ~(0x400000 - 1);
	intptr_t mono_heap_end   = (intptr_t)addr + size;

    fake_heap_start = (char*)addr;
    fake_heap_end   = (char*)mono_heap_start;

	// Heap must be ready by the time we call this
	mono_nx_fakemmap_init(mono_heap_start, mono_heap_end);

    io_debugf("libnx heap: %p-%p (%zu bytes)", 
		fake_heap_start, fake_heap_end, (size_t)(fake_heap_end - fake_heap_start));
	io_debugf("mono heap: %p-%p (%zu bytes)", 
		(void*)mono_heap_start, (void*)mono_heap_end, (size_t)(mono_heap_end - mono_heap_start));	
}