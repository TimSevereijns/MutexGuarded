#pragma once

#include <chrono>
#include <mutex>
#include <optional>
#include <type_traits>

namespace detail
{
namespace traits
{
template <typename, typename = void> struct is_mutex : std::false_type
{
};

template <typename MutexType>
struct is_mutex<
    MutexType,
    std::void_t<
        decltype(std::declval<MutexType>().lock()), decltype(std::declval<MutexType>().unlock())>>
    : std::true_type
{
};

template <typename, typename = void> struct is_shared_mutex : std::false_type
{
};

template <typename MutexType>
struct is_shared_mutex<
    MutexType, std::void_t<
                   decltype(std::declval<MutexType>().lock_shared()),
                   decltype(std::declval<MutexType>().try_lock_shared()),
                   decltype(std::declval<MutexType>().unlock_shared())>> : std::true_type
{
};

template <typename, typename = void> struct is_timed_shared_mutex : std::false_type
{
};

template <typename MutexType>
struct is_timed_shared_mutex<
    MutexType, std::void_t<decltype(std::declval<MutexType>().try_lock_shared_for(
                   std::declval<std::chrono::duration<int>>()))>> : std::true_type
{
};

template <typename, typename = void> struct is_timed_mutex : std::false_type
{
};

template <typename MutexType>
struct is_timed_mutex<
    MutexType, std::void_t<decltype(std::declval<MutexType>().try_lock_for(
                   std::declval<std::chrono::seconds>()))>> : std::true_type
{
};
} // namespace traits

namespace mutex_category
{
struct unique;
struct shared;
struct unique_and_timed;
struct shared_and_timed;
} // namespace mutex_category

/**
 * @brief Base class for lock traits.
 */
template <typename MutexType, typename TagType> struct mutex_traits_impl
{
};

/**
 * @brief Partial specialization for exclusive lock traits.
 */
template <typename MutexType> struct mutex_traits_impl<MutexType, detail::mutex_category::unique>
{
    static void lock(MutexType& mutex)
    {
        mutex.lock();
    }

    [[nodiscard]] static bool try_lock(MutexType& mutex)
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
template <typename MutexType>
struct mutex_traits_impl<MutexType, detail::mutex_category::shared>
    : detail::mutex_traits_impl<MutexType, detail::mutex_category::unique>
{
    static void lock_shared(MutexType& mutex)
    {
        mutex.lock_shared();
    }

    [[nodiscard]] static bool try_lock_shared(MutexType& mutex)
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
template <typename MutexType>
struct mutex_traits_impl<MutexType, detail::mutex_category::unique_and_timed>
    : detail::mutex_traits_impl<MutexType, detail::mutex_category::unique>
{
    template <typename ChronoType>
    [[nodiscard]] static bool try_lock_for(MutexType& mutex, const ChronoType& timeout)
    {
        return mutex.try_lock_for(timeout);
    }

    template <typename DurationType>
    [[nodiscard]] static bool try_lock_until(MutexType& mutex, const DurationType& duration)
    {
        return mutex.try_lock_until(duration);
    }
};

/**
 * @brief Partial specialization for shared, timed lock traits.
 */
template <typename MutexType>
struct mutex_traits_impl<MutexType, detail::mutex_category::shared_and_timed>
    : detail::mutex_traits_impl<MutexType, detail::mutex_category::unique_and_timed>,
      detail::mutex_traits_impl<MutexType, detail::mutex_category::shared>
{
    template <typename ChronoType>
    [[nodiscard]] static bool try_lock_shared_for(MutexType& mutex, const ChronoType& timeout)
    {
        return mutex.try_lock_shared_for(timeout);
    }

    template <typename DurationType>
    [[nodiscard]] static bool try_lock_shared_until(MutexType& mutex, const DurationType& duration)
    {
        return mutex.try_lock_shared_until(duration);
    }
};

template <bool IsMutex, bool IsSharedMutex, bool IsTimedMutex, bool IsSharedTimedMutex>
struct mutex_tagger
{
};

template <> struct mutex_tagger<true, false, false, false>
{
    using type = detail::mutex_category::unique;
};

template <> struct mutex_tagger<true, true, false, false>
{
    using type = detail::mutex_category::shared;
};

template <> struct mutex_tagger<true, false, true, false>
{
    using type = detail::mutex_category::unique_and_timed;
};

template <> struct mutex_tagger<true, true, true, true>
{
    using type = detail::mutex_category::shared_and_timed;
};

template <typename MutexType>
using detect_mutex_category = typename mutex_tagger<
    detail::traits::is_mutex<MutexType>::value, detail::traits::is_shared_mutex<MutexType>::value,
    detail::traits::is_timed_mutex<MutexType>::value,
    detail::traits::is_timed_shared_mutex<MutexType>::value>::type;

/**
 * @brief Mutex traits, as derived from the detected functionality of the mutex.
 */
template <typename MutexType>
struct mutex_traits : mutex_traits_impl<MutexType, detect_mutex_category<MutexType>>
{
    using category_type = detect_mutex_category<MutexType>;

    using mutex_type = MutexType;
};

/**
 * @brief A locking policy targeted at mutexes that comply with the Mutex concept.
 *
 * Function mapping:
 *
 *     lock()   --> lock()
 *     unlock() --> unlock()
 */
struct unique_lock_policy
{
    template <typename MutexType> static void lock(MutexType& mutex)
    {
        detail::mutex_traits<MutexType>::lock(mutex);
    }

    template <typename MutexType> static void unlock(MutexType& mutex)
    {
        detail::mutex_traits<MutexType>::unlock(mutex);
    }
};

/**
 * @brief A locking policy targeted at mutexes that comply with the SharedMutex concept.
 *
 * Function mapping:
 *
 *     lock()   --> lock_shared()
 *     unlock() --> unlock_shared()
 */
struct shared_lock_policy
{
    template <typename MutexType> static void lock(MutexType& mutex)
    {
        static_assert(
            detail::traits::is_shared_mutex<MutexType>::value,
            "The shared_lock_policy expects to operate on a mutex that supports the SharedMutex "
            "Concept.");

        detail::mutex_traits<MutexType>::lock_shared(mutex);
    }

    template <typename MutexType> static void unlock(MutexType& mutex)
    {
        static_assert(
            detail::traits::is_shared_mutex<MutexType>::value,
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
 *     lock()   --> try_lock_for()
 *     unlock() --> unlock()
 */
struct timed_unique_lock_policy
{
    template <typename MutexType, typename ChronoType>
    static bool lock(MutexType& mutex, ChronoType& timeout)
    {
        static_assert(
            detail::traits::is_timed_mutex<MutexType>::value,
            "The timed_unique_lock_policy expects to operate on a mutex that supports the "
            "TimedMutex Concept.");

        return detail::mutex_traits<MutexType>::try_lock_for(mutex, timeout);
    }

    template <typename MutexType> static void unlock(MutexType& mutex)
    {
        static_assert(
            detail::traits::is_timed_mutex<MutexType>::value,
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
 *     lock()   --> try_lock_shared_for()
 *     unlock() --> unlock_shared()
 */
struct timed_shared_lock_policy
{
    template <typename MutexType, typename ChronoType>
    static bool lock(MutexType& mutex, ChronoType& timeout)
    {
        static_assert(
            detail::traits::is_timed_mutex<MutexType>::value &&
                detail::traits::is_shared_mutex<MutexType>::value,
            "The timed_shared_lock_policy expects to operate on a mutex that supports the "
            "SharedTimedMutex Concept.");

        return detail::mutex_traits<MutexType>::try_lock_shared_for(mutex, timeout);
    }

    template <typename MutexType> static void unlock(MutexType& mutex)
    {
        static_assert(
            detail::traits::is_timed_mutex<MutexType>::value &&
                detail::traits::is_shared_mutex<MutexType>::value,
            "The timed_shared_lock_policy expects to operate on a mutex that supports the "
            "SharedTimedMutex Concept.");

        detail::mutex_traits<MutexType>::unlock_shared(mutex);
    }
};
} // namespace detail

/**
 * @brief A RAII proxy that allows the guarded data to be accessed only after the associated mutex
 * has been locked.
 */
template <
    typename BaseType,
    typename LockPolicyType =
        typename detail::mutex_traits<typename BaseType::mutex_type>::category_type>
class lock_proxy
{
  public:
    using value_type = typename BaseType::value_type;

    using pointer = value_type*;
    using const_pointer = const value_type*;

    using reference = value_type&;
    using const_reference = const value_type&;

    lock_proxy(BaseType* base) : m_base{ base }
    {
        assert(base);

        LockPolicyType::lock(m_base->m_mutex);
    }

    template <typename ChronoType> lock_proxy(BaseType* base, const ChronoType& timeout)
    {
        assert(base);

        const auto wasLocked = LockPolicyType::lock(base->m_mutex, timeout);
        m_base = wasLocked ? base : nullptr;
    }

    ~lock_proxy() noexcept
    {
        if (m_base) {
            LockPolicyType::unlock(m_base->m_mutex);
        }
    }

    [[nodiscard]] auto is_locked() const -> bool
    {
        return m_base != nullptr;
    }

    [[nodiscard]] auto operator-> () noexcept -> pointer
    {
        return &m_base->m_data;
    }

    [[nodiscard]] auto operator-> () const noexcept -> const_pointer
    {
        return &m_base->m_data;
    }

    [[nodiscard]] auto operator*() noexcept -> reference
    {
        return m_base->m_data;
    }

    [[nodiscard]] auto operator*() const noexcept -> const_reference
    {
        return m_base->m_data;
    }

  private:
    std::conditional_t<
        std::is_const_v<BaseType>, std::add_pointer_t<const BaseType>, std::add_pointer_t<BaseType>>
        m_base = nullptr;
};

namespace detail
{
template <typename DerivedType, typename DataType, typename TagType> class mutex_guarded_impl
{
};

/**
 * @brief Specialization that provides the functionality to lock and unlock a mutex that supports
 * the Mutex concept.
 */
template <typename DerivedType, typename DataType>
class mutex_guarded_impl<DerivedType, DataType, detail::mutex_category::unique>
{
  public:
    using unique_lock_proxy = lock_proxy<DerivedType, detail::unique_lock_policy>;

    using const_unique_lock_proxy = const lock_proxy<const DerivedType, detail::unique_lock_policy>;

    /**
     * @brief Returns a proxy class that will automatically lock and unlock the underlying mutex.
     *
     * @returns An RAII proxy.
     */
    [[nodiscard]] auto lock() -> unique_lock_proxy
    {
        return { static_cast<DerivedType*>(this) };
    }

    /**
     * @brief Returns a proxy class that will automatically lock and unlock the underlying mutex.
     *
     * @returns An RAII proxy.
     */
    [[nodiscard]] auto lock() const -> const_unique_lock_proxy
    {
        return { static_cast<const DerivedType*>(this) };
    }

    /**
     * @brief Locks the underlying mutex, and then executes the passed in functor with the lock
     * held.
     *
     * @param[in] callable            A callable type like a lambda, std::function, etc.
     *                                This callable type must take its input parameter
     *                                by reference; avoid taking input by value.
     *
     * @returns The result of invoking the functor, provided that the functor returns something.
     */
    template <typename CallableType>
    [[nodiscard]] auto with_lock_held(CallableType&& callable)
        -> decltype(callable(std::declval<DataType&>()))
    {
        const auto scopedGuard = lock();
        return callable(static_cast<DerivedType*>(this)->m_data);
    }

    /**
     * @brief Locks the underlying mutex, and then executes the passed in functor with the lock
     * held.
     *
     * @param[in] callable            A callable type like a lambda, std::function, etc.
     *                                This callable type should take its input parameter
     *                                by const reference. Failure to do so will result in
     *                                compilation failure.
     *
     * @returns The result of invoking the functor, provided that the functor returns something.
     */
    template <typename CallableType>
    [[nodiscard]] auto with_lock_held(CallableType&& callable) const
        -> decltype(callable(std::declval<const DataType&>()))
    {
        const auto scopedGuard = lock();
        return callable(static_cast<const DerivedType*>(this)->m_data);
    }
};

/**
 * @brief Specialization that provides the functionality to lock and unlock a mutex that supports
 * the TimedMutex concept.
 */
template <typename DerivedType, typename DataType>
class mutex_guarded_impl<DerivedType, DataType, detail::mutex_category::unique_and_timed>
{
  public:
    using timed_lock_proxy = lock_proxy<DerivedType, detail::timed_unique_lock_policy>;

    using const_timed_lock_proxy =
        const lock_proxy<const DerivedType, detail::timed_unique_lock_policy>;

    using unique_lock_proxy = lock_proxy<DerivedType, detail::unique_lock_policy>;

    using const_unique_lock_proxy = const lock_proxy<const DerivedType, detail::unique_lock_policy>;

    /**
     * @brief Returns a proxy class that will automatically lock and unlock the underlying mutex.
     *
     * @returns An RAII proxy.
     */
    [[nodiscard]] auto lock() -> unique_lock_proxy
    {
        return { static_cast<DerivedType*>(this) };
    }

    /**
     * @brief Returns a proxy class that will automatically lock and unlock the underlying mutex.
     *
     * @returns An RAII proxy.
     */
    [[nodiscard]] auto lock() const -> const_unique_lock_proxy
    {
        return { static_cast<const DerivedType*>(this) };
    }

    /**
     * @brief Returns a proxy class that will automatically lock and unlock the underlying mutex
     * using the specified timeout.
     *
     * @returns An RAII proxy.
     */
    template <typename ChronoType> auto try_lock_for(const ChronoType& timeout) -> timed_lock_proxy
    {
        return { static_cast<DerivedType*>(this), timeout };
    }

    /**
     * @brief Returns a proxy class that will automatically lock and unlock the underlying mutex
     * using the specified timeout.
     *
     * @returns An RAII proxy.
     */
    template <typename ChronoType>
    [[nodiscard]] auto try_lock_for(const ChronoType& timeout) const -> const_timed_lock_proxy
    {
        return { static_cast<const DerivedType*>(this), timeout };
    }

    /**
     * @brief Executes the functor only if the mutex can be locked before the timer expires.
     *
     * This function will be enabled if the functor's return type is void.
     *
     * @param[in] timeout             The length of time to wait before abandoning the lock
     *                                attempt.
     * @param[in] callable            The functor to be invoked once the underlying mutex has
     *                                been locked.
     *
     * @returns True if a lock was aqcuired on the mutex; false otherwise.
     */
    template <typename ChronoType, typename CallableType>
    [[nodiscard]] auto try_with_lock_held_for(const ChronoType& timeout, CallableType&& callable)
        -> std::enable_if_t<
            std::is_same_v<decltype(callable(std::declval<DataType&>())), void>, bool>
    {
        const auto proxy = try_lock_for(timeout);
        if (proxy.is_locked()) {
            callable(static_cast<DerivedType*>(this)->m_data);
            return true;
        }

        return false;
    }

    /**
     * @brief Executes the functor only if the mutex can be locked before the timer expires.
     *
     * This function will be enabled if the functor's return type is not void.
     *
     * @param[in] timeout             The length of time to wait before abandoning the lock
     *                                attempt.
     * @param[in] callable            The functor to be invoked once the underlying mutex has
     *                                been locked.
     *
     * @returns An optional containing the result of invoking the functor if a lock on the
     * mutex was obtained. If the lock could not be obtained, an empty optional is returned
     * instead.
     */
    template <typename ChronoType, typename CallableType>
    [[nodiscard]] auto try_with_lock_held_for(const ChronoType& timeout, CallableType&& callable)
        -> std::enable_if_t<
            !std::is_same_v<decltype(callable(std::declval<DataType&>())), void>,
            std::optional<decltype(callable(std::declval<DataType&>()))>>
    {
        const auto proxy = try_lock_for(timeout);
        if (proxy.is_locked()) {
            return callable(static_cast<DerivedType*>(this)->m_data);
        }

        return {};
    }
};

/**
 * @brief Specialization that provides the functionality to lock and unlock a mutex that supports
 * the SharedMutex concept.
 */
template <typename DerivedType, typename DataType>
class mutex_guarded_impl<DerivedType, DataType, detail::mutex_category::shared>
{
  public:
    using shared_lock_proxy = const lock_proxy<const DerivedType, detail::shared_lock_policy>;

    using unique_lock_proxy = lock_proxy<DerivedType, detail::unique_lock_policy>;

    /**
     * @brief Returns a proxy class that will automatically acquire and release an exclusive
     * lock on the underlying mutex.
     *
     * @returns An RAII proxy.
     */
    [[nodiscard]] auto write_lock() -> unique_lock_proxy
    {
        return { static_cast<DerivedType*>(this) };
    }

    /**
     * @brief Returns a proxy class that will automatically acquire and release a shared
     * lock on the underlying mutex.
     *
     * @returns An RAII proxy.
     */
    [[nodiscard]] auto read_lock() const -> shared_lock_proxy
    {
        return { static_cast<const DerivedType*>(this) };
    }

    /**
     * @brief Grabs an exclusive lock on the underlying mutex, and then executes the passed in
     * functor with the lock held.
     *
     * @param[in] callable            A callable type like a lambda, std::function, etc.
     *                                This callable type must take its input parameter
     *                                by reference; avoid taking input by value.
     *
     * @returns The result of invoking the functor, provided that the functor returns something.
     */
    template <typename CallableType>
    [[nodiscard]] auto with_write_lock_held(CallableType&& callable)
        -> decltype(callable(std::declval<DataType&>()))
    {
        const auto scopedGuard = write_lock();
        return callable(static_cast<DerivedType*>(this)->m_data);
    }

    /**
     * @brief Grabs a shared lock on the underlying mutex, and then executes the passed in
     * functor with the lock held.
     *
     * @param[in] callable            A callable type like a lambda, std::function, etc.
     *                                This callable type should take its input parameter
     *                                by const reference. Failure to do so will result in
     *                                compilation failure.
     *
     * @returns The result of invoking the functor, provided that the functor returns something.
     */
    template <typename CallableType>
    [[nodiscard]] auto with_read_lock_held(CallableType&& callable) const
        -> decltype(callable(std::declval<const DataType&>()))
    {
        const auto scopedGuard = read_lock();
        return callable(static_cast<const DerivedType*>(this)->m_data);
    }
};

/**
 * @brief Specialization that provides the functionality to lock and unlock a mutex that supports
 * the SharedTimedMutex concept.
 */
template <typename DerivedType, typename DataType>
class mutex_guarded_impl<DerivedType, DataType, detail::mutex_category::shared_and_timed>
{
  public:
    using unique_lock_proxy = lock_proxy<DerivedType, detail::unique_lock_policy>;

    using shared_lock_proxy = const lock_proxy<const DerivedType, detail::shared_lock_policy>;

    using timed_unique_lock_proxy = lock_proxy<DerivedType, detail::timed_unique_lock_policy>;

    using timed_shared_lock_proxy =
        const lock_proxy<const DerivedType, detail::timed_shared_lock_policy>;

    /**
     * @brief Returns a proxy class that will manage the acquisition and release of an exclusive
     * lock on the underlying mutex.
     *
     * @returns An RAII proxy.
     */
    [[nodiscard]] auto write_lock() -> unique_lock_proxy
    {
        return { static_cast<DerivedType*>(this) };
    }

    /**
     * @brief Returns a proxy class that will manage the acquisition and release of a shared
     * lock on the underlying mutex.
     *
     * @returns An RAII proxy.
     */
    [[nodiscard]] auto read_lock() const -> shared_lock_proxy
    {
        return { static_cast<const DerivedType*>(this) };
    }

    /**
     * @brief Returns a proxy class that will manage the acquisition and release of an exclusive
     * lock on the underlying mutex.
     *
     * The validity of the resulting proxy should be checked to see if the lock was acquired
     * before the timer expired.
     *
     * @param[in] timeout             The length of time to wait before abandoning the lock
     *                                attempt.
     *
     * @returns An RAII proxy.
     */
    template <typename ChronoType>
    [[nodiscard]] auto try_write_lock_for(const ChronoType& timeout) -> timed_unique_lock_proxy
    {
        return { static_cast<DerivedType*>(this), timeout };
    }

    /**
     * @brief Returns a proxy class that will manage the acquisition and release of a shared
     * lock on the underlying mutex.
     *
     * The validity of the resulting proxy should be checked to see if the lock was acquired
     * before the timer expired.
     *
     * @param[in] timeout             The length of time to wait before abandoning the lock
     *                                attempt.
     *
     * @returns An RAII proxy.
     */
    template <typename ChronoType>
    [[nodiscard]] auto try_read_lock_for(const ChronoType& timeout) const -> timed_shared_lock_proxy
    {
        return { static_cast<const DerivedType*>(this), timeout };
    }

    /**
     * @brief Executes the functor only if the mutex can be locked before the timer expires.
     *
     * This function will be enabled if the functor's return type is void.
     *
     * @param[in] timeout             The length of time to wait before abandoning the lock
     *                                attempt.
     * @param[in] callable            The functor to be invoked once the underlying mutex has
     *                                been locked.
     *
     * @returns True if a lock was aqcuired on the mutex; false otherwise.
     */
    template <typename ChronoType, typename CallableType>
    [[nodiscard]] auto
    try_with_write_lock_held_for(const ChronoType& timeout, CallableType&& callable)
        -> std::enable_if_t<
            std::is_same_v<decltype(callable(std::declval<DataType&>())), void>, bool>
    {
        const auto proxy = try_write_lock_for(timeout);
        if (proxy.is_locked()) {
            callable(static_cast<DerivedType*>(this)->m_data);
            return true;
        }

        return false;
    }

    /**
     * @brief Executes the functor only if the mutex can be locked before the timer expires.
     *
     * This function will be enabled if the functor's return type is not void.
     *
     * @param[in] timeout             The length of time to wait before abandoning the lock
     *                                attempt.
     * @param[in] callable            The functor to be invoked once the underlying mutex has
     *                                been locked.
     *
     * @returns An optional containing the result of invoking the functor if a lock on the
     * mutex was obtained. If the lock could not be obtained, an empty optional is returned
     * instead.
     */
    template <typename ChronoType, typename CallableType>
    [[nodiscard]] auto
    try_with_write_lock_held_for(const ChronoType& timeout, CallableType&& callable)
        -> std::enable_if_t<
            !std::is_same_v<decltype(callable(std::declval<DataType&>())), void>,
            std::optional<decltype(callable(std::declval<DataType&>()))>>
    {
        const auto proxy = try_write_lock_for(timeout);
        if (proxy.is_locked()) {
            return callable(static_cast<DerivedType*>(this)->m_data);
        }

        return {};
    }

    /**
     * @brief Executes the functor only if the mutex can be locked before the timer expires.
     *
     * This function will be enabled if the functor's return type is void.
     *
     * @param[in] timeout             The length of time to wait before abandoning the lock
     *                                attempt.
     * @param[in] callable            The functor to be invoked once the underlying mutex has
     *                                been locked.
     *
     * @returns True if a lock was aqcuired on the mutex; false otherwise.
     */
    template <typename ChronoType, typename CallableType>
    [[nodiscard]] auto
    try_with_read_lock_held_for(const ChronoType& timeout, CallableType&& callable) const
        -> std::enable_if_t<
            std::is_same_v<decltype(callable(std::declval<const DataType&>())), void>, bool>
    {
        const auto proxy = try_write_lock_for(timeout);
        if (proxy.is_locked()) {
            callable(static_cast<const DerivedType*>(this)->m_data);
            return true;
        }

        return false;
    }

    /**
     * @brief Executes the functor only if the mutex can be locked before the timer expires.
     *
     * This function will be enabled if the functor's return type is not void.
     *
     * @param[in] timeout             The length of time to wait before abandoning the lock
     *                                attempt.
     * @param[in] callable            The functor to be invoked once the underlying mutex has
     *                                been locked.
     *
     * @returns An optional containing the result of invoking the functor if a lock on the
     * mutex was obtained. If the lock could not be obtained, an empty optional is returned
     * instead.
     */
    template <typename ChronoType, typename CallableType>
    [[nodiscard]] auto
    try_with_read_lock_held_for(const ChronoType& timeout, CallableType&& callable) const
        -> std::enable_if_t<
            !std::is_same_v<decltype(callable(std::declval<const DataType&>())), void>,
            std::optional<decltype(callable(std::declval<const DataType&>()))>>
    {
        const auto proxy = try_read_lock_for(timeout);
        if (proxy.is_locked()) {
            return callable(static_cast<const DerivedType*>(this)->m_data);
        }

        return {};
    }
};
} // namespace detail

template <typename DataType, typename MutexType> class mutex_guarded;

namespace detail
{
template <typename DataType, typename MutexType>
using mutex_guarded_base = detail::mutex_guarded_impl<
    mutex_guarded<DataType, MutexType>, DataType,
    typename detail::mutex_traits<MutexType>::category_type>;
}

/**
 * @brief A light-weight wrapper that ensures that all reads and writes from and to the supplied
 * data type are guarded by a mutex.
 *
 * This class uses the Curiously Recurring Template Pattern (CRTP) and template metaprogramming to
 * statically inherit functionality appropriate to the specified mutex. See the various
 * `detail::mutex_guard_impl<...>` classes for further documentation.
 */
template <typename DataType, typename MutexType = std::mutex>
class mutex_guarded : public detail::mutex_guarded_base<DataType, MutexType>
{
    static_assert(
        detail::traits::is_mutex<MutexType>::value, "The MutexType must support the Mutex concept");

    template <typename P, typename L> friend class lock_proxy;

    template <typename S, typename D, typename T> friend class detail::mutex_guarded_impl;

  public:
    using value_type = DataType;
    using reference = value_type&;
    using const_reference = const value_type&;
    using mutex_type = MutexType;

    mutex_guarded() = default;
    ~mutex_guarded() noexcept = default;

    mutex_guarded(DataType data) : m_data{ std::move(data) }
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
    }

    mutex_guarded(mutex_guarded<DataType, MutexType>&& other) : m_data{ std::move(other.m_data) }
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
