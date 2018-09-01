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

   /**
   * @brief Base class for lock traits.
   */
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
      using category_type = typename mutex_tagger<
         detail::supports_unique_locking<MutexType>::value,
         detail::supports_shared_locking<MutexType>::value,
         detail::supports_timed_locking<MutexType>::value>::type;

      using mutex_type = MutexType;
   };

   /**
   * @brief A locking policy targeted at mutexes that comply with the Mutex concept.
   *
   * Function mapping:
   *
   *     lock()   -> lock()
   *     unlock() -> unlock()
   */
   struct unique_lock_policy
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
   * @brief A locking policy targeted at mutexes that comply with the SharedMutex concept.
   *
   * Function mapping:
   *
   *     lock()   -> lock_shared()
   *     unlock() -> unlock_shared()
   */
   struct shared_lock_policy
   {
      template<typename MutexType>
      static void lock(MutexType& mutex)
      {
         static_assert(
            detail::supports_shared_locking<MutexType>::value,
            "The shared_lock_policy expects to operate on a mutex that supports the SharedMutex "
            "Concept.");

         detail::mutex_traits<MutexType>::lock_shared(mutex);
      }

      template<typename MutexType>
      static void unlock(MutexType& mutex)
      {
         static_assert(
            detail::supports_shared_locking<MutexType>::value,
            "The shared_lock_policy expects to operate on a mutex that supports the SharedMutex "
            "Concept.");

         detail::mutex_traits<MutexType>::unlock_shared(mutex);
      }
   };

   /**
   * @brief A locking policy targeted at mutexes that comply with the TimedMutex concept.
   *
   * Function mapping:
   *
   *     lock()   -> try_lock_for()
   *     unlock() -> unlock()
   */
   struct timed_unique_lock_policy
   {
      template<
         typename MutexType,
         typename ChronoType>
      static bool lock(MutexType& mutex, ChronoType& timeout)
      {
         static_assert(
            detail::supports_timed_locking<MutexType>::value,
            "The timed_unique_lock_policy expects to operate on a mutex that supports the "
            "TimedMutex Concept.");

         return detail::mutex_traits<MutexType>::try_lock_for(mutex, timeout);
      }

      template<typename MutexType>
      static void unlock(MutexType& mutex)
      {
         static_assert(
            detail::supports_timed_locking<MutexType>::value,
            "The timed_unique_lock_policy expects to operate on a mutex that supports the "
            "TimedMutex Concept.");

         detail::mutex_traits<MutexType>::unlock(mutex);
      }
   };

   /**
   * @brief A locking policy targeted at mutexes that comply with the SharedTimedMutex concept.
   *
   * Function mapping:
   *
   *     lock()   -> try_lock_shared_for()
   *     unlock() -> unlock_shared()
   */
   struct timed_shared_lock_policy
   {
      template<
         typename MutexType,
         typename ChronoType>
      static bool lock(MutexType& mutex, ChronoType& timeout)
      {
         static_assert(
            detail::supports_timed_locking<MutexType>::value,
            "The timed_shared_lock_policy expects to operate on a mutex that supports the "
            "SharedTimedMutex Concept.");

         return detail::mutex_traits<MutexType>::try_lock_shared_for(mutex, timeout);
      }

      template<typename MutexType>
      static void unlock(MutexType& mutex)
      {
         static_assert(
            detail::supports_timed_locking<MutexType>::value,
            "The timed_shared_lock_policy expects to operate on a mutex that supports the "
            "SharedTimedMutex Concept.");

         detail::mutex_traits<MutexType>::unlock_shared(mutex);
      }
   };
}

/**
* @brief A RAII proxy that allows the guarded data to be accessed only after the associated mutex
* has been locked.
*/
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
      assert(parent);

      LockPolicyType::lock(parent->m_mutex);
   }

   template<typename ChronoType>
   lock_proxy(ParentType* parent, const ChronoType& timeout)
   {
      assert(parent);

      const auto wasLocked = LockPolicyType::lock(parent->m_mutex, timeout);
      m_parent = wasLocked ? parent : nullptr;
   }

   ~lock_proxy() noexcept
   {
      if (m_parent)
      {
         LockPolicyType::unlock(m_parent->m_mutex);
      }
   }

   auto is_valid() const -> bool
   {
      return m_parent != nullptr;
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

   typename std::conditional<
      std::is_const<ParentType>::value,
      typename std::add_pointer<const ParentType>::type,
      typename std::add_pointer<ParentType>::type>::type m_parent = nullptr;
};

template<
   typename SubclassType,
   typename DataType,
   typename TagType>
class mutex_guarded_impl
{
};

/**
* @brief Specialization that provides the functionality to lock and unlock a mutex that supports
* the Mutex concept.
*/
template<
   typename SubclassType,
   typename DataType>
class mutex_guarded_impl<SubclassType, DataType, detail::mutex_category::unique>
{
public:

   using unique_lock_proxy = lock_proxy<
      SubclassType,
      detail::unique_lock_policy>;

   using const_unique_lock_proxy = const lock_proxy<
      const SubclassType,
      detail::unique_lock_policy>;

   /**
   * @brief Returns a proxy class that will automatically lock and unlock the underlying mutex.
   */
   auto lock() -> unique_lock_proxy
   {
      return { static_cast<SubclassType*>(this) };
   }

   /**
   * @overload
   */
   auto lock() const -> const_unique_lock_proxy
   {
      return { static_cast<const SubclassType*>(this) };
   }

   /**
   * @brief Locks the underlying mutex, and then executes the passed in functor with the lock held.
   *
   * @param[in] callable            A callable type like a lambda, std::function, etc.
   *                                This callable type should take its input parameter
   *                                by reference. Taking the input by value is pointless.
   */
   template<typename CallableType>
   auto with_lock_held(CallableType&& callable)
      -> decltype(std::declval<CallableType>().operator()(std::declval<DataType&>()))
   {
      const auto scopedGuard = lock();
      return callable(static_cast<SubclassType*>(this)->m_data);
   }

   /**
   * @brief Locks the underlying mutex, and then executes the passed in functor with the lock held.
   *
   * @param[in] callable            A callable type like a lambda, std::function, etc.
   *                                This callable type should take its input parameter
   *                                by const reference. Taking the input by value is pointless.
   */
   template<typename CallableType>
   auto with_lock_held(CallableType&& callable) const
      -> decltype(std::declval<CallableType>().operator()(std::declval<const DataType&>()))
   {
      const auto scopedGuard = lock();
      return callable(static_cast<const SubclassType*>(this)->m_data);
   }
};

/**
* @brief Specialization that provides the functionality to lock and unlock a mutex that supports
* the TimedMutex concept.
*/
template<
   typename SubclassType,
   typename DataType>
class mutex_guarded_impl<SubclassType, DataType, detail::mutex_category::unique_and_timed>
{
public:

   using timed_lock_proxy = lock_proxy<
      SubclassType,
      detail::timed_unique_lock_policy>;

   using const_timed_lock_proxy = const lock_proxy<
      const SubclassType,
      detail::timed_unique_lock_policy>;

   using unique_lock_proxy = lock_proxy<
      SubclassType,
      detail::unique_lock_policy>;

   using const_unique_lock_proxy = const lock_proxy<
      const SubclassType,
      detail::unique_lock_policy>;

   /**
   * @brief Returns a proxy class that will automatically lock and unlock the underlying mutex.
   */
   auto lock() -> unique_lock_proxy
   {
      return { static_cast<SubclassType*>(this) };
   }

   /**
   * @overload
   */
   auto lock() const -> const_unique_lock_proxy
   {
      return { static_cast<const SubclassType*>(this) };
   }

   /**
   * @brief Returns a proxy class that will automatically lock and unlock the underlying mutex
   * using the specified timeout.
   */
   template<typename ChronoType>
   auto try_lock_for(const ChronoType& timeout) -> timed_lock_proxy
   {
      return { static_cast<SubclassType*>(this), timeout };
   }

   /**
   * @overload
   */
   template<typename ChronoType>
   auto try_lock_for(const ChronoType& timeout) const -> const_timed_lock_proxy
   {
      return { static_cast<const SubclassType*>(this), timeout };
   }
};

/**
* @brief Specialization that provides the functionality to lock and unlock a mutex that supports
* the SharedMutex concept.
*/
template<
   typename SubclassType,
   typename DataType>
class mutex_guarded_impl<SubclassType, DataType, detail::mutex_category::shared>
{
public:

   using shared_lock_proxy = const lock_proxy<
      const SubclassType,
      detail::shared_lock_policy>;

   using unique_lock_proxy = lock_proxy<
      SubclassType,
      detail::unique_lock_policy>;

   /**
   * @brief Returns a proxy class that will automatically acquire and release an exclusive
   * lock on the underlying mutex.
   */
   auto write_lock() -> unique_lock_proxy
   {
      return { static_cast<SubclassType*>(this) };
   }

   /**
   * @brief Returns a proxy class that will automatically acquire and release a shared
   * lock on the underlying mutex.
   */
   auto read_lock() const -> shared_lock_proxy
   {
      return { static_cast<const SubclassType*>(this) };
   }

   /**
   * @brief Grabs an exclusive lock on the underlying mutex, and then executes the passed in
   * functor with the lock held.
   *
   * @param[in] callable            A callable type like a lambda, std::function, etc.
   *                                This callable type should take its input parameter
   *                                by reference. Taking the input by value is pointless.
   */
   template<typename CallableType>
   auto with_write_lock_held(CallableType&& callable)
      -> decltype(std::declval<CallableType>().operator()(std::declval<DataType&>()))
   {
      const auto scopedGuard = write_lock();
      return callable(static_cast<SubclassType*>(this)->m_data);
   }

   /**
   * @brief Grabs a shared lock on the underlying mutex, and then executes the passed in
   * functor with the lock held.
   *
   * @param[in] callable            A callable type like a lambda, std::function, etc.
   *                                This callable type should take its input parameter
   *                                by const reference. Taking the input by value is pointless.
   */
   template<typename CallableType>
   auto with_read_lock_held(CallableType&& callable) const
      -> decltype(std::declval<CallableType>().operator()(std::declval<const DataType&>()))
   {
      const auto scopedGuard = read_lock();
      return callable(static_cast<const SubclassType*>(this)->m_data);
   }
};

/**
* @brief Specialization that provides the functionality to lock and unlock a mutex that supports
* the SharedTimedMutex concept.
*/
template<
   typename SubclassType,
   typename DataType>
class mutex_guarded_impl<SubclassType, DataType, detail::mutex_category::shared_and_timed>
{
public:

   using unique_lock_proxy = lock_proxy<
      SubclassType,
      detail::unique_lock_policy>;

   using shared_lock_proxy = const lock_proxy<
      const SubclassType,
      detail::shared_lock_policy>;

   using timed_unique_lock_proxy = lock_proxy<
      SubclassType,
      detail::timed_unique_lock_policy>;

   using timed_shared_lock_proxy = const lock_proxy<
      const SubclassType,
      detail::timed_shared_lock_policy>;

   /**
   * @brief Returns a proxy class that will automatically acquire and release an exclusive
   * lock on the underlying mutex.
   */
   auto write_lock() -> unique_lock_proxy
   {
      return { static_cast<SubclassType*>(this) };
   }

   /**
   * @brief Returns a proxy class that will automatically acquire and release a shared
   * lock on the underlying mutex.
   */
   auto read_lock() const -> shared_lock_proxy
   {
      return { static_cast<const SubclassType*>(this) };
   }

   /**
   * @brief Returns a proxy class that will automatically lock and unlock the underlying mutex
   * using the specified timeout. Use this function to acquire an exclusive lock.
   */
   template<typename ChronoType>
   auto try_write_lock_for(const ChronoType& timeout) -> timed_unique_lock_proxy
   {
      return { static_cast<SubclassType*>(this), timeout };
   }

   /**
   * @brief Returns a proxy class that will automatically lock and unlock the underlying mutex
   * using the specified timeout. Use this function to acquire a shared lock.
   */
   template<typename ChronoType>
   auto try_read_lock_for(const ChronoType& timeout) const -> timed_shared_lock_proxy
   {
      return { static_cast<const SubclassType*>(this), timeout };
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
      typename detail::mutex_traits<MutexType>::category_type>;
}

/**
* @brief A light-weight wrapper that ensures that all reads and writes from and to the supplied
* data type are guarded by a mutex.
*
* This class uses template metaprogramming to inherit functionality appropriate to the specified
* mutex. See the various `mutex_guard_imp<...>` classes for further documentation.
*/
template<
   typename DataType,
   typename MutexType = std::mutex>
class mutex_guarded : public detail::mutex_guarded_base<DataType, MutexType>
{
   static_assert(
      detail::supports_unique_locking<MutexType>::value,
      "The MutexType must support the Mutex concept");

   template <typename S, typename M>
   friend class lock_proxy;

   template<typename G, typename D, typename T>
   friend class mutex_guarded_impl;

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

   mutex_guarded(const mutex_guarded<DataType, MutexType>& other)
   {
      const std::lock_guard<MutexType> guard{ other.m_mutex };
      m_data = other.m_data;
   }

   mutex_guarded<DataType, MutexType>& operator=(const mutex_guarded<DataType, MutexType>& other)
   {
      const std::lock_guard<MutexType> guard{ other.m_mutex };
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
