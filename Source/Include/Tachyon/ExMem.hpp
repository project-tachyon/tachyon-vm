#pragma once

#include <stddef.h>

namespace Tachyon {
	/* allocates writable memory (R/W) */
	void * AllocateCodeMemory(size_t CodeSize);
	/* sets the page permissions to executable and read-only (R/X) */
	bool ProtectCodeMemory(void * Base, size_t CodeSize);
};