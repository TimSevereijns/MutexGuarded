#pragma once

#include <iostream>
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

   namespace tag
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
   struct mutex_traits_impl<MutexType, detail::tag::unique>
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
   struct mutex_traits_impl<MutexType, detail::tag::shared> :
      detail::mutex_traits_impl<MutexType, detail::tag::unique>
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
   struct mutex_traits_impl<MutexType, detail::tag::unique_and_timed> :
      detail::mutex_traits_impl<MutexType, detail::tag::unique>
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
   struct mutex_traits_impl<MutexType, detail::tag::shared_and_timed> :
      detail::mutex_traits_impl<MutexType, detail::tag::unique_and_timed>,
      detail::mutex_traits_impl<MutexType, detail::tag::shared>
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
      using type = detail::tag::unique;
   };

   template<>
   struct mutex_tagger<true, true, false>
   {
      using type = detail::tag::shared;
   };

   template<>
   struct mutex_tagger<true, false, true>
   {
      using type = detail::tag::unique_and_timed;
   };

   template<>
   struct mutex_tagger<true, true, true>
   {
      using type = detail::tag::shared_and_timed;
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

   /**
   * Function mapping:
   *
   *     Lock()   -> Lock()
   *     Unlock() -> Unlock()
   */
   template<typename MutexTraits>
   struct unique_lock_policy
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
   *     Lock()   -> lock_shared()
   *     Unlock() -> Unlock_shared()
   */
   template<typename MutexTraits>
   struct shared_lock_policy
   {
      template<typename MutexType>
      static void lock(MutexType& mutex)
      {
         static_assert(
            detail::supports_shared_locking<MutexType>::value,
            "The SharedLockPolicy expects to operate on a mutex that supports the SharedMutex "
            "Concept.");

         MutexTraits::lock_shared(mutex);
      }

      template<typename MutexType>
      static void unlock(MutexType& mutex)
      {
         static_assert(
            detail::supports_shared_locking<MutexType>::value,
            "The SharedLockPolicy expects to operate on a mutex that supports the SharedMutex "
            "Concept.");

         MutexTraits::unlock_shared(mutex);
      }
   };

   /**
   * Function mapping:
   *
   *     Lock()   -> TryLockFor()
   *     Unlock() -> Unlock()
   */
   template<typename MutexTraits>
   struct timed_unique_lock_policy
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

         MutexTraits::try_lock_for(mutex, timeout);
      }

      template<typename MutexType>
      static void unlock(MutexType& mutex)
      {
         static_assert(
            detail::supports_timed_locking<MutexType>::value,
            "The SharedTimedLockPolicy expects to operate on a mutex that supports the TimedMutex "
            "Concept.");

         MutexTraits::unlock(mutex);
      }
   };

   /**
   * Function mapping:
   *
   *     Lock()   -> TryLockFor()
   *     Unlock() -> Unlock()
   */
   template<typename MutexTraits>
   struct timed_shared_lock_policy
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

         MutexTraits::try_lock_shared_for(mutex, timeout);
      }

      template<typename MutexType>
      static void unlock(MutexType& mutex)
      {
         static_assert(
            detail::supports_timed_locking<MutexType>::value,
            "The SharedTimedLockPolicy expects to operate on a mutex that supports the TimedMutex "
            "Concept.");

         MutexTraits::unlock_shared(mutex);
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

   lock_proxy(const ParentType* parent) : m_parent{ parent }
   {
      LockPolicyType::lock(parent->m_mutex);
   }

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

   mutable ParentType* m_parent;
};

template<typename DataType, typename MutexType>
class mutex_guarded;

template<
   typename SubclassType,
   typename MutexType,
   typename DataType,
   typename TagType>
class mutex_guarded_impl
{
};

template<
   typename SubclassType,
   typename DataType,
   typename MutexType>
class mutex_guarded_impl<SubclassType, DataType, MutexType, detail::tag::unique>
{
public:

   using mutex_traits = detail::mutex_traits<MutexType>;
   using unique_lock_policy = detail::unique_lock_policy<mutex_traits>;
   using unique_lock_proxy = lock_proxy<SubclassType, unique_lock_policy>;

   auto operator->() -> unique_lock_proxy
   {
      return { static_cast<SubclassType*>(this) };
   }

   auto operator->() const -> const unique_lock_proxy
   {
      return { static_cast<const SubclassType*>(this) };
   }

   auto lock() -> unique_lock_proxy
   {
      return { static_cast<SubclassType*>(this) };
   }

   auto lock() const -> const unique_lock_proxy
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
};

template<
   typename SubclassType,
   typename DataType,
   typename MutexType>
class mutex_guarded_impl<SubclassType, DataType, MutexType, detail::tag::unique_and_timed>
{
public:

   using mutex_traits = detail::mutex_traits<MutexType>;

   using timed_lock_policy = detail::timed_unique_lock_policy<mutex_traits>;
   using timed_lock_proxy = lock_proxy<SubclassType, timed_lock_policy>;

   using unique_lock_policy = detail::unique_lock_policy<mutex_traits>;
   using unique_lock_proxy = lock_proxy<SubclassType, unique_lock_policy>;

   template<typename ChronoType>
   auto try_lock_for(const ChronoType& timeout) -> timed_lock_proxy
   {
      return { static_cast<SubclassType*>(this), timeout };
   }

   template<typename ChronoType>
   auto try_lock_for(const ChronoType& timeout) const -> const timed_lock_proxy
   {
      return { static_cast<SubclassType*>(this), timeout };
   }
};

template<
   typename SubclassType,
   typename DataType,
   typename MutexType>
class mutex_guarded_impl<SubclassType, DataType, MutexType, detail::tag::shared>
{
public:

   using mutex_traits = detail::mutex_traits<MutexType>;

   using shared_lock_policy = detail::shared_lock_policy<mutex_traits>;
   using shared_lock_proxy = lock_proxy<SubclassType, shared_lock_policy>;

   using unique_lock_policy = detail::unique_lock_policy<mutex_traits>;
   using unique_lock_proxy = lock_proxy<SubclassType, unique_lock_policy>;

   auto write_lock() -> unique_lock_proxy
   {
      return { static_cast<SubclassType*>(this) };
   }

   auto write_lock() const -> const unique_lock_proxy
   {
      return { static_cast<SubclassType*>(this) };
   }

   auto read_lock() -> shared_lock_proxy
   {
      return { static_cast<SubclassType*>(this) };
   }

   auto read_lock() const -> const shared_lock_proxy
   {
      return { static_cast<SubclassType*>(this) };
   }

   template<typename CallableType>
   auto with_write_lock_held(CallableType&& callable)
      -> decltype(std::declval<CallableType>().operator()(std::declval<DataType&>()))
   {
      const auto scopedGuard = write_lock();
      return callable(this->m_data);
   }

   template<typename CallableType>
   auto with_read_lock_held(CallableType&& callable)
      -> decltype(std::declval<CallableType>().operator()(std::declval<const DataType&>()))
   {
      const auto scopedGuard = read_lock();
      return callable(this->m_data);
   }
};

namespace detail
{
   template<
      typename DataType,
      typename MutexType>
   using mutex_guarded_base = mutex_guarded_impl<
      mutex_guarded<DataType, MutexType>,
      DataType,
      MutexType,
      typename detail::mutex_traits<MutexType>::tag_type>;
}

template<
   typename DataType,
   typename MutexType = std::mutex>
class mutex_guarded : public detail::mutex_guarded_base<DataType, MutexType>
{
   template <typename S, typename M>
   friend class lock_proxy;

   template<typename G, typename D, typename M, typename T>
   friend class mutex_guarded_impl;

   using mutex_traits = detail::mutex_traits<MutexType>;
   using unique_lock_policy = detail::unique_lock_policy<mutex_traits>;
   using unique_lock_proxy = lock_proxy<mutex_guarded<DataType, MutexType>, unique_lock_policy>;

public:

   using value_type = DataType;
   using reference = value_type&;
   using const_reference = const value_type&;

   mutex_guarded() = default;

   mutex_guarded(DataType data)
      : m_data{ std::move(data) }
   {
   }

   mutex_guarded(mutex_guarded<DataType, MutexType>& other)
   {
      unique_lock_proxy guard{ this };
      m_data = other.m_data;
   }

   mutex_guarded<DataType, MutexType>& operator=(mutex_guarded<DataType, MutexType>& other)
   {
      unique_lock_proxy guard{ this };
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
