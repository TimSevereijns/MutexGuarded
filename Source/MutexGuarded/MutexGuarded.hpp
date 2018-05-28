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
         decltype(std::declval<MutexType>().try_lock_for()),
         decltype(std::declval<MutexType>().try_lock_until())>> : std::true_type
   {
   };
}

namespace detail
{
   enum struct MutexLevel
   {
      UNIQUE,
      SHARED,
      UNIQUE_AND_TIMED,
      SHARED_AND_TIMED
   };
}

namespace detail
{
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
      static void lock(MutexType& mutex)
         noexcept(std::is_nothrow_invocable_v<decltype(&MutexType::lock), MutexType>)
      {
         mutex.lock();
      }

      static bool try_lock(MutexType& mutex)
         noexcept(std::is_nothrow_invocable_v<decltype(&MutexType::try_lock), MutexType>)
      {
         return mutex.try_lock();
      }

      static void unlock(MutexType& mutex)
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
      static void lock_shared(MutexType& mutex)
         noexcept(std::is_nothrow_invocable_v<decltype(&MutexType::lock_shared), MutexType>)
      {
         mutex.lock_shared();
      }

      static bool try_lock_shared(MutexType& mutex)
         noexcept(std::is_nothrow_invocable_v<decltype(&MutexType::try_lock_shared), MutexType>)
      {
         return mutex.try_lock_shared();
      }

      static void unlock_shared(MutexType& mutex)
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
      static void try_lock_for(MutexType& mutex, ChronoType& timeout)
         noexcept(std::is_nothrow_invocable_v<
            decltype(&MutexType::try_lock_for), ChronoType,
            MutexType>)
      {
         mutex.try_lock_for(timeout);
      }

      template<typename ChronoType>
      static bool try_lock_until(MutexType& mutex, ChronoType& timeout)
         noexcept(std::is_nothrow_invocable_v<
            decltype(&MutexType::try_lock_until), ChronoType,
            MutexType>)
      {
         return mutex.try_lock_until(timeout);
      }
   };

   /**
   * @brief Partial specialization for shared, timed lock traits.
   */
   template<typename MutexType>
   struct MutexTraitsImpl<MutexType, MutexLevel::SHARED_AND_TIMED> :
      MutexTraitsImpl<MutexType, MutexLevel::UNIQUE_AND_TIMED>
   {
      template<typename ChronoType>
      static void try_lock_shared_for(MutexType& mutex, ChronoType& timeout)
         noexcept(std::is_nothrow_invocable_v<
            decltype(&MutexType::try_lock_shared_for), ChronoType,
            MutexType>)
      {
         mutex.try_lock_shared_for(timeout);
      }

      template<typename ChronoType>
      static bool try_lock_shared_until(MutexType& mutex, ChronoType& timeout)
         noexcept(std::is_nothrow_invocable_v<
            decltype(&MutexType::try_lock_shared_until), ChronoType,
            MutexType>)
      {
         return mutex.try_lock_shared_until(timeout);
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
}

namespace detail
{
   /**
   * Function mapping:
   *
   *     lock()   -> lock()
   *     unlock() -> unlock()
   */
   template<typename MutexTraits>
   struct UniqueLockPolicy
   {
      template<typename MutexType>
      static void lock(MutexType& mutex)
      {
         MutexTraits::lock(mutex);
      }

      template<typename MutexType>
      static void unlock(MutexType& mutex)
      {
         MutexTraits::unlock(mutex);
      }
   };

   /**
   * Function mapping:
   *
   *     lock()   -> lock_shared()
   *     unlock() -> unlock_shared()
   */
   template<typename MutexTraits>
   struct SharedLockPolicy
   {
      template<typename MutexType>
      static void lock(MutexType& mutex)
      {
         static_assert(
            detail::SupportsSharedLocking<MutexType>::value,
            "The SharedLockPolicy expects to operate on a mutex that supports the SharedMutex "
            "Concept.");

         MutexTraits::lock_shared(mutex);
      }

      template<typename MutexType>
      static void unlock(MutexType& mutex)
      {
         static_assert(
            detail::SupportsSharedLocking<MutexType>::value,
            "The SharedLockPolicy expects to operate on a mutex that supports the SharedMutex "
            "Concept.");

         MutexTraits::unlock_shared(mutex);
      }
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
      std::cout << "Locking..." << std::endl;

      LockPolicyType::lock(parent->m_mutex);
   }

   ~LockProxy() noexcept
   {
      std::cout << "Unlocking..." << std::endl;

      LockPolicyType::unlock(m_parent->m_mutex);
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

// @todo Need specialization for the timed lock proxy...

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

   auto write_lock() -> UniqueLockProxy
   {
      return { this };
   }

   auto write_lock() const -> const UniqueLockProxy
   {
      return { this };
   }

   auto read_lock() -> SharedLockProxy
   {
      return { this };
   }

   auto read_lock() const -> const SharedLockProxy
   {
      return { this };
   }

private:

   typename MutexTraits::MutexType m_mutex;
   DataType m_data;
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

   auto lock() -> UniqueLockProxy
   {
      return { this };
   }

   auto lock() const -> const UniqueLockProxy
   {
      return { this };
   }

private:

   typename MutexTraits::MutexType m_mutex;
   DataType m_data;
};

template<
   typename DataType,
   typename MutexType = std::shared_mutex>
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

   MutexGuarded(MutexGuarded<DataType, MutexType>&& other) = default;
   MutexGuarded<DataType, MutexType>& operator=(MutexGuarded<DataType, MutexType>&& other) = default;
};
