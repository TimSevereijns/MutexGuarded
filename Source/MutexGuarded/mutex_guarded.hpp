#pragma once

#include <mutex>
#include <type_traits>

#include "future_std.hpp"

namespace detail
{
   template<
      typename,
      typename = void>
   struct supports_unique_locking : std::false_type
   {
   };

   template<typename MutexType>
   struct supports_unique_locking<
      MutexType,
      future_std::std17::void_t<
         decltype(std::declval<MutexType>().lock()),
         decltype(std::declval<MutexType>().unlock())>> : std::true_type
   {
   };

   template<
      typename,
      typename = void>
   struct supports_shared_locking : std::false_type
   {
   };

   template<typename MutexType>
   struct supports_shared_locking<
      MutexType,
      future_std::std17::void_t<
         decltype(std::declval<MutexType>().lock_shared()),
         decltype(std::declval<MutexType>().try_lock_shared()),
         decltype(std::declval<MutexType>().unlock_shared())>> : std::true_type
   {
   };

   template<
      typename,
      typename = void>
   struct supports_timed_locking : std::false_type
   {
   };

   template<typename MutexType>
   struct supports_timed_locking<
      MutexType,
      future_std::std17::void_t<
         decltype(std::declval<MutexType>().try_lock_for(std::declval<std::chrono::seconds>()))>>
      : std::true_type
   {
   };

   namespace mutex_category
   {
      struct unique;
      struct shared;
      struct unique_and_timed;
      struct shared_and_timed;
   }

   template<
      typename MutexType,
      typename TagType>
   struct mutex_traits_impl
   {
   };

   /**
   * @brief Partial specialization for exclusive lock traits.
   */
   template<typename MutexType>
   struct mutex_traits_impl<MutexType, detail::mutex_category::unique>
   {
      static void lock(MutexType& mutex)
      {
         mutex.lock();
      }

      static bool try_lock(MutexType& mutex)
      {
         return mutex.try_lock();
      }

      static void unlock(MutexType& mutex)
      {
         mutex.unlock();
      }
   };

   /**
   * @brief Partial specialization for shared lock traits.
   */
   template<typename MutexType>
   struct mutex_traits_impl<MutexType, detail::mutex_category::shared> :
      detail::mutex_traits_impl<MutexType, detail::mutex_category::unique>
   {
      static void lock_shared(MutexType& mutex)
      {
         mutex.lock_shared();
      }

      static bool try_lock_shared(MutexType& mutex)
      {
         return mutex.try_lock_shared();
      }

      static void unlock_shared(MutexType& mutex)
      {
         mutex.unlock_shared();
      }
   };

   /**
   * @brief Partial specialization for exclusive, timed lock traits.
   */
   template<typename MutexType>
   struct mutex_traits_impl<MutexType, detail::mutex_category::unique_and_timed> :
      detail::mutex_traits_impl<MutexType, detail::mutex_category::unique>
   {
      template<typename ChronoType>
      static bool try_lock_for(MutexType& mutex, const ChronoType& timeout)
      {
         return mutex.try_lock_for(timeout);
      }

      template<typename DurationType>
      static bool try_lock_until(MutexType& mutex, const DurationType& duration)
      {
         return mutex.try_lock_until(duration);
      }
   };

   /**
   * @brief Partial specialization for shared, timed lock traits.
   */
   template<typename MutexType>
   struct mutex_traits_impl<MutexType, detail::mutex_category::shared_and_timed> :
      detail::mutex_traits_impl<MutexType, detail::mutex_category::unique_and_timed>,
      detail::mutex_traits_impl<MutexType, detail::mutex_category::shared>
   {
      template<typename ChronoType>
      static void try_lock_shared_for(MutexType& mutex, const ChronoType& timeout)
      {
         mutex.try_lock_shared_for(timeout);
      }

      template<typename DurationType>
      static bool try_lock_shared_until(MutexType& mutex, const DurationType& duration)
      {
         return mutex.try_lock_shared_until(duration);
      }
   };

   template<
      bool SupportsUniqueLocking,
      bool SupportsSharedLocking,
      bool SupportsTimedLocking>
   struct mutex_tagger
   {
   };

   template<>
   struct mutex_tagger<true, false, false>
   {
      using type = detail::mutex_category::unique;
   };

   template<>
   struct mutex_tagger<true, true, false>
   {
      using type = detail::mutex_category::shared;
   };

   template<>
   struct mutex_tagger<true, false, true>
   {
      using type = detail::mutex_category::unique_and_timed;
   };

   template<>
   struct mutex_tagger<true, true, true>
   {
      using type = detail::mutex_category::shared_and_timed;
   };

   /**
   * @brief Mutex traits, as derived from the detected functionality of the mutex.
   */
   template<typename MutexType>
   struct mutex_traits : mutex_traits_impl<
      MutexType,
      typename mutex_tagger<
         detail::supports_unique_locking<MutexType>::value,
         detail::supports_shared_locking<MutexType>::value,
         detail::supports_timed_locking<MutexType>::value>::type>
   {
      using tag_type = typename mutex_tagger<
         detail::supports_unique_locking<MutexType>::value,
         detail::supports_shared_locking<MutexType>::value,
         detail::supports_timed_locking<MutexType>::value>::type;

      using mutex_type = MutexType;
   };

   template<template<typename...> class MutexTraits>
   struct policy_base
   {
   };

   /**
   * Function mapping:
   *
   *     Lock()   -> Lock()
   *     Unlock() -> Unlock()
   */
   struct unique_lock_policy : policy_base<detail::mutex_traits>
   {
      template<typename MutexType>
      static void lock(MutexType& mutex)
      {
         detail::mutex_traits<MutexType>::lock(mutex);
      }

      template<typename MutexType>
      static void unlock(MutexType& mutex)
      {
         detail::mutex_traits<MutexType>::unlock(mutex);
      }
   };

   /**
   * Function mapping:
   *
   *     Lock()   -> lock_shared()
   *     Unlock() -> Unlock_shared()
   */
   struct shared_lock_policy : policy_base<detail::mutex_traits>
   {
      template<typename MutexType>
      static void lock(MutexType& mutex)
      {
         static_assert(
            detail::supports_shared_locking<MutexType>::value,
            "The SharedLockPolicy expects to operate on a mutex that supports the SharedMutex "
            "Concept.");

         detail::mutex_traits<MutexType>::lock_shared(mutex);
      }

      template<typename MutexType>
      static void unlock(MutexType& mutex)
      {
         static_assert(
            detail::supports_shared_locking<MutexType>::value,
            "The SharedLockPolicy expects to operate on a mutex that supports the SharedMutex "
            "Concept.");

         detail::mutex_traits<MutexType>::unlock_shared(mutex);
      }
   };

   /**
   * Function mapping:
   *
   *     Lock()   -> TryLockFor()
   *     Unlock() -> Unlock()
   */
   struct timed_unique_lock_policy : policy_base<detail::mutex_traits>
   {
      template<
         typename MutexType,
         typename ChronoType>
      static void lock(MutexType& mutex, ChronoType& timeout)
      {
         static_assert(
            detail::supports_timed_locking<MutexType>::value,
            "The SharedTimedLockPolicy expects to operate on a mutex that supports the TimedMutex "
            "Concept.");

         detail::mutex_traits<MutexType>::try_lock_for(mutex, timeout);
      }

      template<typename MutexType>
      static void unlock(MutexType& mutex)
      {
         static_assert(
            detail::supports_timed_locking<MutexType>::value,
            "The SharedTimedLockPolicy expects to operate on a mutex that supports the TimedMutex "
            "Concept.");

         detail::mutex_traits<MutexType>::unlock(mutex);
      }
   };

   /**
   * Function mapping:
   *
   *     Lock()   -> TryLockFor()
   *     Unlock() -> Unlock()
   */
   struct timed_shared_lock_policy : policy_base<detail::mutex_traits>
   {
      template<
         typename MutexType,
         typename ChronoType>
      static void lock(MutexType& mutex, ChronoType& timeout)
      {
         static_assert(
            detail::supports_timed_locking<MutexType>::value,
            "The SharedTimedLockPolicy expects to operate on a mutex that supports the TimedMutex "
            "Concept.");

         detail::mutex_traits<MutexType>::try_lock_shared_for(mutex, timeout);
      }

      template<typename MutexType>
      static void unlock(MutexType& mutex)
      {
         static_assert(
            detail::supports_timed_locking<MutexType>::value,
            "The SharedTimedLockPolicy expects to operate on a mutex that supports the TimedMutex "
            "Concept.");

         detail::mutex_traits<MutexType>::unlock_shared(mutex);
      }
   };
}

template<
   typename ParentType,
   typename LockPolicyType>
class lock_proxy
{
public:

   using value_type = typename ParentType::value_type;

   using pointer = value_type*;
   using const_pointer = const value_type*;

   using reference = value_type&;
   using const_reference = const value_type&;

   lock_proxy(ParentType* parent) : m_parent{ parent }
   {
      LockPolicyType::lock(parent->m_mutex);
   }

   template<typename ChronoType>
   lock_proxy(ParentType* parent, const ChronoType& timeout) : m_parent{ parent }
   {
      LockPolicyType::lock(parent->m_mutex, timeout);
   }

   ~lock_proxy() noexcept
   {
      LockPolicyType::unlock(m_parent->m_mutex);
   }

   auto operator->() noexcept -> pointer
   {
      return &m_parent->m_data;
   }

   auto operator->() const noexcept -> const_pointer
   {
      return &m_parent->m_data;
   }

   auto operator*() noexcept -> reference
   {
      return m_parent->m_data;
   }

   auto operator*() const noexcept -> const_reference
   {
      return m_parent->m_data;
   }

private:

   mutable typename std::conditional<
      std::is_const<ParentType>::value,
      typename std::add_pointer<const ParentType>::type,
      typename std::add_pointer<ParentType>::type>::type m_parent;
};

template<
   typename SubclassType,
   typename DataType,
   typename TagType>
class mutex_guarded_impl
{
};

template<
   typename SubclassType,
   typename DataType>
class mutex_guarded_impl<SubclassType, DataType, detail::mutex_category::unique>
{
public:

   using unique_lock_policy = detail::unique_lock_policy;
   using unique_lock_proxy = lock_proxy<SubclassType, unique_lock_policy>;
   using const_unique_lock_proxy = const lock_proxy<const SubclassType, unique_lock_policy>;

   auto lock() -> unique_lock_proxy
   {
      return { static_cast<SubclassType*>(this) };
   }

   auto lock() const -> const_unique_lock_proxy
   {
      return { static_cast<const SubclassType*>(this) };
   }

   template<typename CallableType>
   auto with_lock_held(CallableType&& callable)
      -> decltype(std::declval<CallableType>().operator()(std::declval<DataType&>()))
   {
      const auto scopedGuard = lock();
      return callable(static_cast<SubclassType*>(this)->m_data);
   }

   template<typename CallableType>
   auto with_lock_held(CallableType&& callable) const
      -> decltype(std::declval<CallableType>().operator()(std::declval<const DataType&>()))
   {
      const auto scopedGuard = lock();
      return callable(static_cast<const SubclassType*>(this)->m_data);
   }
};

template<
   typename SubclassType,
   typename DataType>
class mutex_guarded_impl<SubclassType, DataType, detail::mutex_category::unique_and_timed>
{
public:

   using timed_lock_policy = detail::timed_unique_lock_policy;
   using timed_lock_proxy = lock_proxy<SubclassType, timed_lock_policy>;
   using const_timed_lock_proxy = const lock_proxy<const SubclassType, timed_lock_policy>;

   using unique_lock_policy = detail::unique_lock_policy;
   using unique_lock_proxy = lock_proxy<SubclassType, unique_lock_policy>;
   using const_unique_lock_proxy = const lock_proxy<const SubclassType, unique_lock_policy>;

   template<typename ChronoType>
   auto try_lock_for(const ChronoType& timeout) -> timed_lock_proxy
   {
      return { static_cast<SubclassType*>(this), timeout };
   }

   template<typename ChronoType>
   auto try_lock_for(const ChronoType& timeout) const -> const_timed_lock_proxy
   {
      return { static_cast<const SubclassType*>(this), timeout };
   }
};

template<
   typename SubclassType,
   typename DataType>
class mutex_guarded_impl<SubclassType, DataType, detail::mutex_category::shared>
{
public:

   using shared_lock_policy = detail::shared_lock_policy;
   using shared_lock_proxy = lock_proxy<SubclassType, detail::shared_lock_policy>;
   using const_shared_lock_proxy = const lock_proxy<const SubclassType, detail::shared_lock_policy>;

   using unique_lock_policy = detail::unique_lock_policy;
   using unique_lock_proxy = lock_proxy<SubclassType, unique_lock_policy>;
   using const_unique_lock_proxy = const lock_proxy<const SubclassType, unique_lock_policy>;

   auto write_lock() -> unique_lock_proxy
   {
      return { static_cast<SubclassType*>(this) };
   }

   auto write_lock() const -> const_unique_lock_proxy
   {
      return { static_cast<const SubclassType*>(this) };
   }

   auto read_lock() -> shared_lock_proxy
   {
      return { static_cast<SubclassType*>(this) };
   }

   auto read_lock() const -> const_shared_lock_proxy
   {
      return { static_cast<const SubclassType*>(this) };
   }

   template<typename CallableType>
   auto with_write_lock_held(CallableType&& callable)
      -> decltype(std::declval<CallableType>().operator()(std::declval<DataType&>()))
   {
      const auto scopedGuard = write_lock();
      return callable(static_cast<SubclassType*>(this)->m_data);
   }

   template<typename CallableType>
   auto with_read_lock_held(CallableType&& callable) const
      -> decltype(std::declval<CallableType>().operator()(std::declval<const DataType&>()))
   {
      const auto scopedGuard = read_lock();
      return callable(static_cast<const SubclassType*>(this)->m_data);
   }
};

template<typename DataType, typename MutexType>
class mutex_guarded;

namespace detail
{
   template<
      typename DataType,
      typename MutexType>
   using mutex_guarded_base = mutex_guarded_impl<
      mutex_guarded<DataType, MutexType>,
      DataType,
      typename detail::mutex_traits<MutexType>::tag_type>;
}

template<
   typename DataType,
   typename MutexType = std::mutex>
class mutex_guarded : public detail::mutex_guarded_base<DataType, MutexType>
{
   template <typename S, typename M>
   friend class lock_proxy;

   template<typename G, typename D, typename T>
   friend class mutex_guarded_impl;

   using unique_lock_proxy = lock_proxy<
      mutex_guarded<DataType, MutexType>,
      detail::unique_lock_policy>;

public:

   using value_type = DataType;
   using reference = value_type&;
   using const_reference = const value_type&;

   mutex_guarded() = default;
   ~mutex_guarded() noexcept = default;

   mutex_guarded(DataType data)
      : m_data{ std::move(data) }
   {
   }

   mutex_guarded(mutex_guarded<DataType, MutexType>& other)
   {
      unique_lock_proxy guard{ &other };
      m_data = other.m_data;
   }

   mutex_guarded<DataType, MutexType>& operator=(mutex_guarded<DataType, MutexType>& other)
   {
      unique_lock_proxy guard{ &other };
      m_data = other.m_data;
   };

   mutex_guarded(mutex_guarded<DataType, MutexType>&& other)
      : m_data{ std::move(other.m_data) }
   {
   }

   mutex_guarded<DataType, MutexType>& operator=(mutex_guarded<DataType, MutexType>&& other)
   {
      m_data = std::move(other.m_data);
   }

private:

   mutable MutexType m_mutex;
   DataType m_data;
};
