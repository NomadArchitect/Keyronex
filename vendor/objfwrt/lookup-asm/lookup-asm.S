/*
 * Copyright (c) 2008-2023 Jonathan Schleifer <js@nil.im>
 *
 * All rights reserved.
 *
 * This file is part of ObjFW. It may be distributed under the terms of the
 * Q Public License 1.0, which can be found in the file LICENSE.QPL included in
 * the packaging of this file.
 *
 * Alternatively, it may be distributed under the terms of the GNU General
 * Public License, either version 2 or 3, which can be found in the file
 * LICENSE.GPLv2 or LICENSE.GPLv3 respectively included in the packaging of this
 * file.
 */

#include "config.h"

#include "platform.h"

#if defined(OF_ELF)
# if defined(OF_AMD64)
#  include "lookup-asm-amd64-elf.S"
# elif defined(OF_X86)
#  include "lookup-asm-x86-elf.S"
# elif defined(OF_ARM64)
#  include "lookup-asm-arm64-elf.S"
# elif defined(OF_ARM)
#  include "lookup-asm-arm-elf.S"
# elif defined(OF_POWERPC64)
#  include "lookup-asm-powerpc64-elf.S"
# elif defined(OF_POWERPC)
#  include "lookup-asm-powerpc-elf.S"
# elif defined(OF_MIPS64_N64)
#  include "lookup-asm-mips64-n64-elf.S"
# elif defined(OF_MIPS)
#  include "lookup-asm-mips-elf.S"
# elif defined(OF_SPARC64)
#  include "lookup-asm-sparc64-elf.S"
# elif defined(OF_SPARC)
#  include "lookup-asm-sparc-elf.S"
# endif
#elif defined(OF_MACH_O)
# if defined(OF_AMD64)
#  include "lookup-asm-amd64-macho.S"
# endif
#elif defined(OF_WINDOWS)
# if defined(OF_AMD64)
#  include "lookup-asm-amd64-win64.S"
# elif defined(OF_X86)
#  include "lookup-asm-x86-win32.S"
# endif
#endif
