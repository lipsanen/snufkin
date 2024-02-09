#pragma once

// The actual threadtools header is a mess, so only implement the fast mutex
// This is a dummy implementation. Source is single-threaded for everything anyway so all accesses to the mutex
// always succeed and nothing is checked. The layout is kept the same so the game doesnt crash fiddling with random areas in memory.

#include <cstdint>

namespace tier0 {
	class CThreadFastMutex {
	public:
		CThreadFastMutex() : m_ownerID(0), m_depth(0) {}

	private:
		bool TryLockInline(const uint32_t threadId) volatile { return true; }

		bool TryLock(const uint32_t threadId) volatile { return TryLockInline(threadId); }

		void Lock(const uint32_t threadId, unsigned nSpinSleepTime) volatile;

	public:
		bool TryLock() volatile { return true; }

		void Lock(unsigned int nSpinSleepTime = 0) volatile {}

		void Unlock() volatile {}

		bool TryLock() const volatile { return (const_cast<CThreadFastMutex*>(this))->TryLock(); }
		void Lock(unsigned nSpinSleepTime = 1) const volatile {
			(const_cast<CThreadFastMutex*>(this))->Lock(nSpinSleepTime);
		}
		void Unlock() const volatile { (const_cast<CThreadFastMutex*>(this))->Unlock(); }
		// To match regular CThreadMutex:
		bool AssertOwnedByCurrentThread() { return true; }
		void SetTrace(bool) {}

		uint32_t GetOwnerId() const { return m_ownerID; }
		int GetDepth() const { return m_depth; }

	private:
		volatile uint32_t m_ownerID;
		int m_depth;
	};
} // namespace tier0

using namespace tier0;
