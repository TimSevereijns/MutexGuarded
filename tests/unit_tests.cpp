#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include <shared_mutex>

#include <mutex_guarded.h>

namespace detail
{
struct lock_tracker
{
    bool was_locked = false;
    bool was_unlocked = false;
} tracker;

void reset_tracker() noexcept
{
    tracker.was_locked = false;
    tracker.was_unlocked = false;
}

template <typename MutexType> class wrapped_unique_mutex
{
  public:
    wrapped_unique_mutex()
    {
        reset_tracker();
    }

    void lock()
    {
        m_mutex.lock();
        tracker.was_locked = true;
    }

    bool try_lock()
    {
        const auto successfully_locked = m_mutex.try_lock();
        tracker.was_locked = successfully_locked;
        return successfully_locked;
    }

    void unlock()
    {
        m_mutex.unlock();
        tracker.was_unlocked = true;
    }

  private:
    mutable MutexType m_mutex;
};

template <typename MutexType> class wrapped_shared_mutex
{
  public:
    wrapped_shared_mutex()
    {
        reset_tracker();
    }

    void lock()
    {
        m_mutex.lock();
        tracker.was_locked = true;
    }

    bool try_lock()
    {
        const auto successfully_locked = m_mutex.try_lock();
        tracker.was_locked = successfully_locked;
        return successfully_locked;
    }

    void unlock()
    {
        m_mutex.unlock();
        tracker.was_unlocked = true;
    }

    void lock_shared()
    {
        m_mutex.lock_shared();
        tracker.was_locked = true;
    }

    bool try_lock_shared()
    {
        const auto successfully_locked = m_mutex.try_lock_shared();
        tracker.was_locked = successfully_locked;
        return successfully_locked;
    }

    void unlock_shared()
    {
        m_mutex.unlock_shared();
        tracker.was_unlocked = true;
    }

  private:
    mutable MutexType m_mutex;
};

template <typename MutexType, typename ShouldStartLocked = std::false_type>
class wrapped_unique_and_timed_mutex
{
  public:
    wrapped_unique_and_timed_mutex()
    {
        if (ShouldStartLocked::value) {
            m_mutex.lock();
        }

        reset_tracker();
    }

    void lock()
    {
        m_mutex.lock();
        tracker.was_locked = true;
    }

    bool try_lock()
    {
        const auto successfully_locked = m_mutex.try_lock();
        tracker.was_locked = successfully_locked;
        return successfully_locked;
    }

    void unlock()
    {
        m_mutex.unlock();
        tracker.was_unlocked = true;
    }

    template <class Rep, class Period>
    bool try_lock_for(const std::chrono::duration<Rep, Period>& timeout)
    {
        const auto successfully_locked = m_mutex.try_lock_for(timeout);
        tracker.was_locked = successfully_locked;
        return successfully_locked;
    }

  private:
    mutable MutexType m_mutex;
};

template <typename MutexType, typename ShouldStartLocked = std::false_type>
class wrapped_shared_and_timed_mutex
{
  public:
    wrapped_shared_and_timed_mutex()
    {
        if (ShouldStartLocked::value) {
            m_mutex.lock();
        }

        reset_tracker();
    }

    void lock()
    {
        m_mutex.lock();
        tracker.was_locked = true;
    }

    bool try_lock()
    {
        const auto successfully_locked = m_mutex.try_lock();
        tracker.was_locked = successfully_locked;
        return successfully_locked;
    }

    void unlock()
    {
        m_mutex.unlock();
        tracker.was_unlocked = true;
    }

    bool try_lock_shared()
    {
        const auto successfully_locked = m_mutex.try_lock_shared();
        tracker.was_locked = successfully_locked;
        return successfully_locked;
    }

    void lock_shared()
    {
        m_mutex.lock_shared();
        tracker.was_locked = true;
    }

    template <class Rep, class Period>
    bool try_lock_for(const std::chrono::duration<Rep, Period>& timeout)
    {
        const auto successfully_locked = m_mutex.try_lock_for(timeout);
        tracker.was_locked = successfully_locked;
        return successfully_locked;
    }

    template <class Rep, class Period>
    bool try_lock_shared_for(const std::chrono::duration<Rep, Period>& timeout)
    {
        const auto successfully_locked = m_mutex.try_lock_shared_for(timeout);
        tracker.was_locked = successfully_locked;
        return successfully_locked;
    }

    void unlock_shared()
    {
        m_mutex.unlock_shared();
        tracker.was_unlocked = true;
    }

  private:
    mutable MutexType m_mutex;
};
} // namespace detail

TEST_CASE("Trait Detection")
{
    SECTION("Locking capabilities of std::mutex", "[Std]")
    {
        STATIC_REQUIRE(detail::traits::is_mutex<std::mutex>::value == true);
        STATIC_REQUIRE(detail::traits::is_shared_mutex<std::mutex>::value == false);
        STATIC_REQUIRE(detail::traits::is_timed_mutex<std::mutex>::value == false);
        STATIC_REQUIRE(detail::traits::is_timed_shared_mutex<std::mutex>::value == false);
    }

    SECTION("Locking capabilities of std::shared_timed_mutex", "[Std]")
    {
        STATIC_REQUIRE(detail::traits::is_mutex<std::shared_timed_mutex>::value == true);
        STATIC_REQUIRE(detail::traits::is_shared_mutex<std::shared_timed_mutex>::value == true);
        STATIC_REQUIRE(detail::traits::is_timed_mutex<std::shared_timed_mutex>::value == true);
        STATIC_REQUIRE(
            detail::traits::is_timed_shared_mutex<std::shared_timed_mutex>::value == true);
    }

    SECTION("Locking capabilities of boost::recursive_mutex", "[Boost]")
    {
        STATIC_REQUIRE(detail::traits::is_mutex<boost::recursive_mutex>::value == true);
        STATIC_REQUIRE(detail::traits::is_shared_mutex<boost::recursive_mutex>::value == false);
        STATIC_REQUIRE(detail::traits::is_timed_mutex<boost::recursive_mutex>::value == false);
        STATIC_REQUIRE(
            detail::traits::is_timed_shared_mutex<boost::recursive_mutex>::value == false);
    }

    SECTION("Locking capabilities of boost::shared_mutex", "[Boost]")
    {
        STATIC_REQUIRE(detail::traits::is_mutex<boost::shared_mutex>::value == true);
        STATIC_REQUIRE(detail::traits::is_shared_mutex<boost::shared_mutex>::value == true);
        STATIC_REQUIRE(detail::traits::is_timed_mutex<boost::shared_mutex>::value == false);
        STATIC_REQUIRE(detail::traits::is_timed_shared_mutex<boost::shared_mutex>::value == false);
    }
}

TEST_CASE("Simple sanity checks")
{
    SECTION("Verifying type-definitions")
    {
        STATIC_REQUIRE(std::is_same_v<mutex_guarded<std::string>::value_type, std::string>);

        STATIC_REQUIRE(std::is_same_v<mutex_guarded<std::string>::reference, std::string&>);

        STATIC_REQUIRE(
            std::is_same_v<mutex_guarded<std::string>::const_reference, const std::string&>);

        STATIC_REQUIRE(std::is_same_v<mutex_guarded<std::string>::mutex_type, std::mutex>);

        STATIC_REQUIRE(
            std::is_same_v<lock_proxy<mutex_guarded<std::string>>::value_type, std::string>);

        STATIC_REQUIRE(
            std::is_same_v<lock_proxy<mutex_guarded<std::string>>::pointer, std::string*>);

        STATIC_REQUIRE(std::is_same_v<
                       lock_proxy<mutex_guarded<std::string>>::const_pointer, const std::string*>);

        STATIC_REQUIRE(
            std::is_same_v<lock_proxy<mutex_guarded<std::string>>::reference, std::string&>);

        STATIC_REQUIRE(
            std::is_same_v<
                lock_proxy<mutex_guarded<std::string>>::const_reference, const std::string&>);
    }
}

TEST_CASE("Guarded with a std::mutex", "[Std]")
{
    const std::string sample = "Testing a std::mutex.";

    using mutex_type = detail::wrapped_unique_mutex<std::mutex>;
    const mutex_guarded<std::string, mutex_type> data{ sample };

    SECTION("Locking")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        REQUIRE(data.lock().is_locked());
        REQUIRE(*data.lock() == sample);
        REQUIRE(data.lock()->length() == sample.length());

        REQUIRE(detail::tracker.was_locked == true);
        REQUIRE(detail::tracker.was_unlocked == true);
    }

    SECTION("Data access using a lambda")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        const std::size_t length = data.with_lock_held([](const std::string& value) noexcept {
            REQUIRE(detail::tracker.was_locked == true);

            return value.length();
        });

        REQUIRE(length == sample.length());
        REQUIRE(detail::tracker.was_unlocked == true);
    }
}

TEST_CASE("Guarded with a boost::recursive_mutex", "[Boost]")
{
    const std::string sample = "Testing a boost::recursive_mutex.";

    using mutex_type = detail::wrapped_unique_mutex<boost::recursive_mutex>;

    mutex_guarded<std::string, mutex_type> data{ sample };

    SECTION("Locking")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        REQUIRE(data.lock().is_locked());
        REQUIRE(*data.lock() == sample);
        REQUIRE(data.lock()->length() == sample.length());

        REQUIRE(detail::tracker.was_locked == true);
        REQUIRE(detail::tracker.was_unlocked == true);
    }

    SECTION("Data access using a lambda")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        const std::size_t length = data.with_lock_held([](const std::string& value) noexcept {
            REQUIRE(detail::tracker.was_locked == true);

            return value.length();
        });

        REQUIRE(length == sample.length());
        REQUIRE(detail::tracker.was_unlocked == true);
    }
}

TEST_CASE("Guarded with a boost::shared_mutex", "[Boost]")
{
    const std::string sample = "Testing a boost::shared_mutex.";

    using mutex_type = detail::wrapped_shared_mutex<boost::shared_mutex>;
    mutex_guarded<std::string, mutex_type> data{ sample };

    SECTION("Read Locking")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        REQUIRE(data.read_lock().is_locked());
        REQUIRE(*data.read_lock() == sample);
        REQUIRE(data.read_lock()->length() == sample.length());

        REQUIRE(detail::tracker.was_locked == true);
        REQUIRE(detail::tracker.was_unlocked == true);
    }

    SECTION("Write Locking")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        REQUIRE(data.write_lock().is_locked());
        REQUIRE(data.write_lock()->length() == sample.length());

        REQUIRE(detail::tracker.was_locked == true);
        REQUIRE(detail::tracker.was_unlocked == true);
    }

    SECTION("Reading data using a lambda")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        const std::size_t length = data.with_read_lock_held([](const std::string& value) noexcept {
            REQUIRE(detail::tracker.was_locked == true);

            return value.length();
        });

        REQUIRE(length == sample.length());
        REQUIRE(detail::tracker.was_unlocked == true);
    }

    SECTION("Writing data using a lambda")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        const std::string anotherString = "Something else";

        const std::size_t length = data.with_write_lock_held([&](std::string & value) noexcept {
            REQUIRE(detail::tracker.was_locked == true);

            value = anotherString;
            return value.length();
        });

        REQUIRE(length == anotherString.length());
        REQUIRE(*data.read_lock() == anotherString);

        REQUIRE(detail::tracker.was_unlocked == true);
    }
}

TEST_CASE("Moves and Copies", "[Std]")
{
    const std::string sample = "Testing a std::mutex.";

    using mutex_type = detail::wrapped_unique_mutex<std::mutex>;
    mutex_guarded<std::string, mutex_type> data{ sample };

    SECTION("Copy-assignment")
    {
        auto copy = data;

        // Making a copy should require lock acquisition:
        REQUIRE(detail::tracker.was_locked == true);
        REQUIRE(detail::tracker.was_unlocked == true);

        detail::reset_tracker();

        REQUIRE(copy.lock()->length() == sample.length());

        REQUIRE(detail::tracker.was_locked == true);
        REQUIRE(detail::tracker.was_unlocked == true);
    }

    SECTION("Move-assignment")
    {
        const auto copy = std::move(data);

        // For moves, we assume we're dealing with true r-values, meaning that
        // lock acquisition shouldn't be necessary:
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        REQUIRE(copy.lock()->length() == sample.length());

        // Accessing the data should, of course, require lock acquisition:
        REQUIRE(detail::tracker.was_locked == true);
        REQUIRE(detail::tracker.was_unlocked == true);
    }
}

TEST_CASE("Const-Correctness")
{
    const std::string sample = "Some sample data.";

    SECTION("Const-ref to mutable mutex_guarded<..., std::mutex>")
    {
        using mutex_type = detail::wrapped_unique_mutex<std::mutex>;
        mutex_guarded<std::string, mutex_type> data{ sample };

        const auto& copy = data;

        REQUIRE(copy.lock().is_locked());
        REQUIRE(copy.lock()->length() == sample.length());
    }

    SECTION("Const-ref to mutable mutex_guarded<..., boost::shared_mutex>")
    {
        using mutex_type = detail::wrapped_shared_mutex<boost::shared_mutex>;
        mutex_guarded<std::string, mutex_type> data{ sample };

        const auto& copy = data;

        REQUIRE(copy.read_lock().is_locked());
        REQUIRE(copy.read_lock()->length() == sample.length());

        // @note Grabbing a write lock on a const-reference to a mutex_guarded<...> instance
        // is obviously non-sensical; use a read lock instead!
    }
}

TEST_CASE("Unique Timed Mutex without Contention")
{
    const std::string sample = "Testing a std::timed_mutex.";

    using start_locked = std::false_type;
    using mutex_type = detail::wrapped_unique_and_timed_mutex<std::timed_mutex, start_locked>;

    mutex_guarded<std::string, mutex_type> data{ sample };

    SECTION("Basic non-timed locking")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        REQUIRE(data.lock().is_locked());
        REQUIRE(*data.lock() == sample);
        REQUIRE(data.lock()->length() == sample.length());

        REQUIRE(detail::tracker.was_locked == true);
        REQUIRE(detail::tracker.was_unlocked == true);
    }

    SECTION("Basic timed locking")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        constexpr auto timeout = std::chrono::milliseconds{ 10 };

        REQUIRE(data.try_lock_for(timeout).is_locked());
        REQUIRE(*data.try_lock_for(timeout) == sample);
        REQUIRE(data.try_lock_for(timeout)->length() == sample.length());

        REQUIRE(detail::tracker.was_locked == true);
        REQUIRE(detail::tracker.was_unlocked == true);
    }

    SECTION("Writing data using a lambda that returns something")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        auto wasLambdaInvoked = false;

        const std::optional<std::size_t> length = data.try_with_lock_held_for(
            std::chrono::milliseconds{ 10 }, [&](const std::string& value) noexcept {
                REQUIRE(detail::tracker.was_locked == true);

                wasLambdaInvoked = true;
                return value.length();
            });

        REQUIRE(length.has_value());
        REQUIRE(wasLambdaInvoked == true);
        REQUIRE(detail::tracker.was_unlocked == true);
    }

    SECTION("Writing data using a lambda that returns nothing")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        auto wasLambdaInvoked = false;

        const std::string anotherString = "Something else";

        const auto wasLocked = data.try_with_lock_held_for(
            std::chrono::milliseconds{ 10 }, [&](std::string & value) noexcept {
                REQUIRE(detail::tracker.was_locked == true);

                wasLambdaInvoked = true;
                value = anotherString;
            });

        REQUIRE(wasLocked);
        REQUIRE(wasLambdaInvoked == true);
        REQUIRE(detail::tracker.was_unlocked == true);

        REQUIRE(*data.lock() == anotherString);
    }
}

TEST_CASE("Unique Timed Mutex with Contention")
{
    const std::string sample = "Testing a std::timed_mutex.";

    using start_locked = std::true_type;
    using mutex_type = detail::wrapped_unique_and_timed_mutex<std::timed_mutex, start_locked>;
    mutex_guarded<std::string, mutex_type> data{ sample };

    SECTION("Basic timed locking")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        constexpr auto timeout = std::chrono::milliseconds{ 10 };

        REQUIRE(data.try_lock_for(timeout).is_locked() == false);

        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);
    }

    SECTION("Writing data using a lambda that returns something")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        auto wasLambdaInvoked = false;

        const std::optional<std::size_t> length = data.try_with_lock_held_for(
            std::chrono::milliseconds{ 10 }, [&](const std::string& value) noexcept {
                REQUIRE(detail::tracker.was_locked == false);

                wasLambdaInvoked = true;
                return value.length();
            });

        REQUIRE(length.has_value() == false);
        REQUIRE(wasLambdaInvoked == false);
        REQUIRE(detail::tracker.was_unlocked == false);
    }

    SECTION("Writing data using a lambda that returns nothing")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        auto wasLambdaInvoked = false;

        const std::string anotherString = "Something else";

        const auto wasLocked = data.try_with_lock_held_for(
            std::chrono::milliseconds{ 10 }, [&](std::string & value) noexcept {
                wasLambdaInvoked = true;
                value = anotherString;
            });

        REQUIRE(wasLocked == false);
        REQUIRE(wasLambdaInvoked == false);
        REQUIRE(detail::tracker.was_unlocked == false);
    }
}

TEST_CASE("Shared Timed Mutex without Contention")
{
    const std::string sample = "Testing a std::shared_timed_mutex.";

    using start_locked = std::false_type;
    using mutex_type =
        detail::wrapped_shared_and_timed_mutex<std::shared_timed_mutex, start_locked>;

    mutex_guarded<std::string, mutex_type> data{ sample };

    STATIC_REQUIRE(std::is_same<
                   detail::mutex_traits<mutex_type>::category_type,
                   detail::mutex_category::shared_and_timed>::value);

    SECTION("Basic timed locking")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        constexpr auto timeout = std::chrono::milliseconds{ 10 };

        REQUIRE(data.try_read_lock_for(timeout).is_locked() == true);

        REQUIRE(detail::tracker.was_locked == true);
        REQUIRE(detail::tracker.was_unlocked == true);
    }

    SECTION("Writing data using a lambda that returns something")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        auto wasLambdaInvoked = false;

        const std::optional<std::size_t> length = data.try_with_read_lock_held_for(
            std::chrono::milliseconds{ 10 }, [&](const std::string& value) noexcept {
                REQUIRE(detail::tracker.was_locked == true);

                wasLambdaInvoked = true;
                return value.length();
            });

        REQUIRE(length.has_value() == true);
        REQUIRE(wasLambdaInvoked == true);
        REQUIRE(detail::tracker.was_unlocked == true);
    }

    SECTION("Writing data using a lambda that returns nothing")
    {
        REQUIRE(detail::tracker.was_locked == false);
        REQUIRE(detail::tracker.was_unlocked == false);

        auto wasLambdaInvoked = false;

        const std::string anotherString = "Something else";

        const auto wasLocked = data.try_with_write_lock_held_for(
            std::chrono::milliseconds{ 10 }, [&](std::string & value) noexcept {
                wasLambdaInvoked = true;
                value = anotherString;
            });

        REQUIRE(wasLocked == true);
        REQUIRE(wasLambdaInvoked == true);
        REQUIRE(detail::tracker.was_unlocked == true);
    }
}
