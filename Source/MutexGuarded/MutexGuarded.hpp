#pragma once

#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <type_traits>

namespace detail
{
   template<
      typename,
      typename = void>
   struct SupportsUniqueLocking : std::false_type
   {
   };

   template<typename MutexType>
   struct SupportsUniqueLocking<
      MutexType,
      std::void_t<
         decltype(std::declval<MutexType>().lock()),
         decltype(std::declval<MutexType>().unlock())>> : std::true_type
   {
   };

   template<
      typename,
      typename = void>
   struct SupportsSharedLocking : std::false_type
   {
   };

   template<typename MutexType>
   struct SupportsSharedLocking<
      MutexType,
      std::void_t<
         decltype(std::declval<MutexType>().lock_shared()),
         decltype(std::declval<MutexType>().try_lock_shared()),
         decltype(std::declval<MutexType>().unlock_shared())>> : std::true_type
   {
   };

   template<
      typename,
      typename = void>
   struct SupportsTimedLocking : std::false_type
   {
   };

   template<typename MutexType>
   struct SupportsTimedLocking<
      MutexType,
      std::void_t<
         decltype(std::declval<MutexType>().try_lock_for(std::declval<std::chrono::seconds>()))>>
      : std::true_type
   {
   };

   enum struct MutexLevel
   {
      UNIQUE,
      SHARED,
      UNIQUE_AND_TIMED,
      SHARED_AND_TIMED
   };

   template<
      typename MutexType,
      MutexLevel>
   struct MutexTraitsImpl
   {
   };

   /**
   * @brief Partial specialization for exclusive lock traits.
   */
   template<typename MutexType>
   struct MutexTraitsImpl<MutexType, MutexLevel::UNIQUE>
   {
      static void Lock(MutexType& mutex)
         noexcept(std::is_nothrow_invocable_v<decltype(&MutexType::lock), MutexType>)
      {
         mutex.lock();
      }

      static bool TryLock(MutexType& mutex)
         noexcept(std::is_nothrow_invocable_v<decltype(&MutexType::try_lock), MutexType>)
      {
         return mutex.try_lock();
      }

      static void Unlock(MutexType& mutex)
         noexcept(std::is_nothrow_invocable_v<decltype(&MutexType::unlock), MutexType>)
      {
         mutex.unlock();
      }
   };

   /**
   * @brief Partial specialization for shared lock traits.
   */
   template<typename MutexType>
   struct MutexTraitsImpl<MutexType, MutexLevel::SHARED> :
      MutexTraitsImpl<MutexType, MutexLevel::UNIQUE>
   {
      static void LockShared(MutexType& mutex)
         noexcept(std::is_nothrow_invocable_v<decltype(&MutexType::lock_shared), MutexType>)
      {
         mutex.lock_shared();
      }

      static bool TryLockShared(MutexType& mutex)
         noexcept(std::is_nothrow_invocable_v<decltype(&MutexType::try_lock_shared), MutexType>)
      {
         return mutex.try_lock_shared();
      }

      static void UnlockShared(MutexType& mutex)
         noexcept(std::is_nothrow_invocable_v<decltype(&MutexType::unlock_shared), MutexType>)
      {
         mutex.unlock_shared();
      }
   };

   /**
   * @brief Partial specialization for exclusive, timed lock traits.
   */
   template<typename MutexType>
   struct MutexTraitsImpl<MutexType, MutexLevel::UNIQUE_AND_TIMED> :
      MutexTraitsImpl<MutexType, MutexLevel::UNIQUE>
   {
      template<typename ChronoType>
      static bool TryLockFor(MutexType& mutex, const ChronoType& timeout)
         //noexcept(decltype(std::declval<MutexType>().try_lock_for(std::declval<ChronoType>())))
      {
         return mutex.try_lock_for(timeout);
      }

      template<typename DurationType>
      static bool TryLockUntil(MutexType& mutex, const DurationType& duration)
         //noexcept(decltype(std::declval<MutexType>().try_lock_shared_for(std::declval<DurationType>())))
      {
         return mutex.try_lock_until(duration);
      }
   };

   /**
   * @brief Partial specialization for shared, timed lock traits.
   */
   template<typename MutexType>
   struct MutexTraitsImpl<MutexType, MutexLevel::SHARED_AND_TIMED> :
      MutexTraitsImpl<MutexType, MutexLevel::UNIQUE_AND_TIMED>,
      MutexTraitsImpl<MutexType, MutexLevel::SHARED>
   {
      template<typename ChronoType>
      static void TryLockSharedFor(MutexType& mutex, const ChronoType& timeout)
         //noexcept(decltype(std::declval<MutexType>().try_lock_shared_for(std::declval<ChronoType>())))
      {
         mutex.try_lock_shared_for(timeout);
      }

      template<typename DurationType>
      static bool TryLockSharedUntil(MutexType& mutex, const DurationType& duration)
         //noexcept(decltype(std::declval<MutexType>().try_lock_until(std::declval<DurationType>())))
      {
         return mutex.try_lock_shared_until(duration);
      }
   };

   template<
      bool SupportsUniqueLocking,
      bool SupportsSharedLocking,
      bool SupportsTimedLocking>
   struct MutexIdentifier
   {
   };

   template<>
   struct MutexIdentifier<true, false, false>
   {
      static constexpr auto value = detail::MutexLevel::UNIQUE;
   };

   template<>
   struct MutexIdentifier<true, true, false>
   {
      static constexpr auto value = detail::MutexLevel::SHARED;
   };

   template<>
   struct MutexIdentifier<true, false, true>
   {
      static constexpr auto value = detail::MutexLevel::UNIQUE_AND_TIMED;
   };

   template<>
   struct MutexIdentifier<true, true, true>
   {
      static constexpr auto value = detail::MutexLevel::SHARED_AND_TIMED;
   };

   /**
   * @brief Mutex traits, as derived from the detected functionality of the mutex.
   */
   template<typename Mutex>
   struct MutexTraits : public MutexTraitsImpl<
      Mutex,
      MutexIdentifier<
         detail::SupportsUniqueLocking<Mutex>::value,
         detail::SupportsSharedLocking<Mutex>::value,
         detail::SupportsTimedLocking<Mutex>::value>::value>
   {
      static constexpr auto Level = MutexIdentifier<
         detail::SupportsUniqueLocking<Mutex>::value,
         detail::SupportsSharedLocking<Mutex>::value,
         detail::SupportsTimedLocking<Mutex>::value>::value;

      using MutexType = Mutex;
   };

   /**
   * Function mapping:
   *
   *     Lock()   -> Lock()
   *     Unlock() -> Unlock()
   */
   template<typename MutexTraits>
   struct UniqueLockPolicy
   {
      using TraitType = MutexTraits;

      template<typename MutexType>
      static void Lock(MutexType& mutex)
      {
         std::cout << "Locking unique mutex..." << std::endl;
         MutexTraits::Lock(mutex);
      }

      template<typename MutexType>
      static void Unlock(MutexType& mutex)
      {
         std::cout << "Unlocking unique mutex..." << std::endl;
         MutexTraits::Unlock(mutex);
      }
   };

   /**
   * Function mapping:
   *
   *     Lock()   -> LockShared()
   *     Unlock() -> UnlockShared()
   */
   template<typename MutexTraits>
   struct SharedLockPolicy
   {
      using TraitType = MutexTraits;

      template<typename MutexType>
      static void Lock(MutexType& mutex)
      {
         static_assert(
            detail::SupportsSharedLocking<MutexType>::value,
            "The SharedLockPolicy expects to operate on a mutex that supports the SharedMutex "
            "Concept.");

         std::cout << "Locking shared mutex..." << std::endl;
         MutexTraits::LockShared(mutex);
      }

      template<typename MutexType>
      static void Unlock(MutexType& mutex)
      {
         static_assert(
            detail::SupportsSharedLocking<MutexType>::value,
            "The SharedLockPolicy expects to operate on a mutex that supports the SharedMutex "
            "Concept.");

         std::cout << "Unlocking shared mutex..." << std::endl;
         MutexTraits::UnlockShared(mutex);
      }
   };

   /**
   * Function mapping:
   *
   *     Lock()   -> TryLockFor()
   *     Unlock() -> Unlock()
   */
   template<typename MutexTraits>
   struct TimedUniqueLockPolicy
   {
      using TraitType = MutexTraits;

      template<
         typename MutexType,
         typename ChronoType>
      static void Lock(MutexType& mutex, ChronoType& timeout)
      {
         static_assert(
            detail::SupportsTimedLocking<MutexType>::value,
            "The SharedTimedLockPolicy expects to operate on a mutex that supports the TimedMutex "
            "Concept.");

         std::cout << "Locking unique, timed mutex..." << std::endl;
         MutexTraits::TryLockFor(mutex, timeout);
      }

      template<typename MutexType>
      static void Unlock(MutexType& mutex)
      {
         static_assert(
            detail::SupportsTimedLocking<MutexType>::value,
            "The SharedTimedLockPolicy expects to operate on a mutex that supports the TimedMutex "
            "Concept.");

         std::cout << "Unlocking unique, timed mutex..." << std::endl;
         MutexTraits::Unlock(mutex);
      }
   };

   /**
   * Function mapping:
   *
   *     Lock()   -> TryLockFor()
   *     Unlock() -> Unlock()
   */
   template<typename MutexTraits>
   struct TimedSharedLockPolicy
   {
      template<
         typename MutexType,
         typename ChronoType>
      static void Lock(MutexType& mutex, ChronoType& timeout)
      {
         static_assert(
            detail::SupportsTimedLocking<MutexType>::value,
            "The SharedTimedLockPolicy expects to operate on a mutex that supports the TimedMutex "
            "Concept.");

         MutexTraits::TryLockSharedFor(mutex, timeout);
      }

      template<typename MutexType>
      static void Unlock(MutexType& mutex)
      {
         static_assert(
            detail::SupportsTimedLocking<MutexType>::value,
            "The SharedTimedLockPolicy expects to operate on a mutex that supports the TimedMutex "
            "Concept.");

         MutexTraits::UnlockShared(mutex);
      }
   };

   template<typename LockPolicyType>
   class ScopedLock
   {
      using MutexType = typename LockPolicyType::TraitType::MutexType;

   public:

      ScopedLock(MutexType& mutex) : m_mutex{ mutex }
      {
         LockPolicyType::Lock(m_mutex);
      }

      ~ScopedLock() noexcept
      {
         LockPolicyType::Unlock(m_mutex);
      }

   private:

      MutexType& m_mutex;
   };
}

template<
   typename ParentType,
   typename LockPolicyType>
class LockProxy
{
public:

   LockProxy(ParentType* parent) : m_parent{ parent }
   {
      LockPolicyType::Lock(parent->m_mutex);
   }

   template<typename ChronoType>
   LockProxy(ParentType* parent, const ChronoType& timeout) : m_parent{ parent }
   {
      LockPolicyType::Lock(parent->m_mutex, timeout);
   }

   ~LockProxy() noexcept
   {
      LockPolicyType::Unlock(m_parent->m_mutex);
   }

   auto* operator->() noexcept
   {
      return &m_parent->m_data;
   }

   const auto* operator->() const noexcept
   {
      return &m_parent->m_data;
   }

private:

   ParentType* m_parent;
};

template<
   typename DataType,
   typename MutexTraits,
   detail::MutexLevel>
class MutexGuardedImpl
{
};

template<
   typename DataType,
   typename MutexTraits>
class MutexGuardedImpl<DataType, MutexTraits, detail::MutexLevel::UNIQUE>
{
   using ThisType = MutexGuardedImpl<DataType, MutexTraits, detail::MutexLevel::UNIQUE>;

   using UniqueLockPolicy = detail::UniqueLockPolicy<MutexTraits>;
   using UniqueLockProxy = LockProxy<ThisType, UniqueLockPolicy>;

   friend class LockProxy<ThisType, UniqueLockPolicy>;

public:

   template<typename ForwardedType>
   MutexGuardedImpl(ForwardedType&& data)
      noexcept(std::is_nothrow_invocable_v<DataType, ForwardedType&&>) :
      m_data{ std::forward<decltype(data)>(data) }
   {
   }

   auto operator->() -> UniqueLockProxy
   {
      return { this };
   }

   auto operator->() const -> const UniqueLockProxy
   {
      return { this };
   }

   auto Lock() -> UniqueLockProxy
   {
      return { this };
   }

   auto Lock() const -> const UniqueLockProxy
   {
      return { this };
   }

   template<typename CallableType>
   auto WithLockHeld(CallableType&& callable)
      noexcept(std::is_nothrow_invocable_v<CallableType, DataType>)
   {
      static_assert(
         std::is_invocable_v<CallableType, DataType&>,
         "The function (or lambda) must taken a DataType& as input.");

      using PolicyType = detail::UniqueLockPolicy<MutexTraits>;
      const detail::ScopedLock<PolicyType> lock{ m_mutex };

      return callable(m_data);
   }

private:

   typename MutexTraits::MutexType m_mutex;
   DataType m_data;
};

template<
   typename DataType,
   typename MutexTraits>
class MutexGuardedImpl<DataType, MutexTraits, detail::MutexLevel::UNIQUE_AND_TIMED>
{
   using ThisType = MutexGuardedImpl<DataType, MutexTraits, detail::MutexLevel::UNIQUE_AND_TIMED>;

   using TimedLockPolicy = detail::TimedUniqueLockPolicy<MutexTraits>;
   using TimedLockProxy = LockProxy<ThisType, TimedLockPolicy>;

   friend class LockProxy<ThisType, TimedLockPolicy>;

public:

   template<typename ForwardedType>
   MutexGuardedImpl(ForwardedType&& data)
      noexcept(std::is_nothrow_invocable_v<DataType, ForwardedType&&>) :
      m_data{ std::forward<decltype(data)>(data) }
   {
   }

   template<typename ChronoType>
   auto TryLockFor(const ChronoType& timeout) -> TimedLockProxy
   {
      return { this, timeout };
   }

   template<typename ChronoType>
   auto TryLockFor(const ChronoType& timeout) const -> const TimedLockProxy
   {
      return { this, timeout };
   }

private:

   mutable typename MutexTraits::MutexType m_mutex;
   DataType m_data;
};

template<
   typename DataType,
   typename MutexTraits>
   class MutexGuardedImpl<DataType, MutexTraits, detail::MutexLevel::SHARED>
{
   using ThisType = MutexGuardedImpl<DataType, MutexTraits, detail::MutexLevel::SHARED>;

   using SharedPolicy = detail::SharedLockPolicy<MutexTraits>;
   using SharedLockProxy = LockProxy<ThisType, SharedPolicy>;

   using UniqueLockPolicy = detail::UniqueLockPolicy<MutexTraits>;
   using UniqueLockProxy = LockProxy<ThisType, UniqueLockPolicy>;

   friend class LockProxy<ThisType, UniqueLockPolicy>;
   friend class LockProxy<ThisType, SharedPolicy>;

public:

   template<typename ForwardedType>
   MutexGuardedImpl(ForwardedType&& data)
      noexcept(std::is_nothrow_invocable_v<DataType, ForwardedType&&>) :
      m_data{ std::forward<decltype(data)>(data) }
   {
   }

   auto WriteLock() -> UniqueLockProxy
   {
      return { this };
   }

   auto WriteLock() const -> const UniqueLockProxy
   {
      return { this };
   }

   auto ReadLock() -> SharedLockProxy
   {
      return { this };
   }

   auto ReadLock() const -> const SharedLockProxy
   {
      return { this };
   }

private:

   typename MutexTraits::MutexType m_mutex;
   DataType m_data;
};

template<
   typename DataType,
   typename MutexType = std::mutex>
class MutexGuarded : public MutexGuardedImpl<
   DataType,
   detail::MutexTraits<MutexType>,
   detail::MutexTraits<MutexType>::Level>
{
   using Base = MutexGuardedImpl<
      DataType,
      detail::MutexTraits<MutexType>,
      detail::MutexTraits<MutexType>::Level>;

public:

   template<typename ForwardedType>
   MutexGuarded(ForwardedType&& data)
      noexcept(std::is_nothrow_invocable_v<DataType, ForwardedType&&>) :
      Base{ std::forward<decltype(data)>(data) }
   {
   }

   MutexGuarded(MutexGuarded<DataType, MutexType>& other) = delete;
   MutexGuarded<DataType, MutexType>& operator=(MutexGuarded<DataType, MutexType>& other) = delete;

   MutexGuarded(MutexGuarded<DataType, MutexType>&& other) = delete;
   MutexGuarded<DataType, MutexType>& operator=(MutexGuarded<DataType, MutexType>&& other) = delete;
};
