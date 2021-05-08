// Copyright (c) 2009-2013 The Bitcoin developers
// Copyright (c) 2021 Bean Core www.beancash.org
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "allocators.h"

bool MemoryPageLocker::Lock(const void *addr, size_t len)
{
#ifdef WIN32
    return VirtualLock(const_cast<void*>(addr), len);
#else
    return mlock(addr, len) == 0;
#endif
}

bool MemoryPageLocker::Unlock(const void *addr, size_t len)
{
#ifdef WIN32
    return VirtualUnlock(const_cast<void*>(addr), len);
#else
    return munlock(addr, len) == 0;
#endif
}

LockedPageManager::LockedPageManager() : LockedPageManagerBase<MemoryPageLocker>(GetSystemPageSize())
{
}

