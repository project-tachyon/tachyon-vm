#include <Tachyon/ExMem.hpp>
#include <Common.hpp>
#include <stddef.h>

#if (defined(_WIN32) || defined(_WIN64))
#include <Windows.h>
#else
#include <sys/mman.h>
#endif

void * Tachyon::AllocateCodeMemory(size_t CodeSize) {
#if (defined(_WIN32) || defined(_WIN64))
	/* windows target */
	return VirtualAlloc(nullptr, CodeSize, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
#else
	/* unix target */
	void * Addr = mmap(0, CodeSize, (PROT_WRITE | PROT_READ), (MAP_PRIVATE | MAP_ANON), -1, 0);
	return (Addr == MAP_FAILED) ? NULL : Addr;
#endif
}

bool Tachyon::ProtectCodeMemory(void * Base, size_t CodeSize) {
#if (defined(_WIN32) || defined(_WIN64))
	/* windows target */
	PDWORD __unused PreviousProtection;
	return VirtualProtect(Base, CodeSize, PAGE_EXECUTE, PreviousProtection) ? true : false;
#else
	/* unix target */
	return mprotect(Base, CodeSize, (PROT_EXEC | PROT_READ)) != -1 ? true : false;
#endif
}