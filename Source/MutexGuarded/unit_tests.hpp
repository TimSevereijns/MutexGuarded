#pragma once

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include "mutex_guarded.hpp"

#include <shared_mutex>

#include <boost/thread/recursive_mutex.hpp>

namespace detail
{
   namespace global
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
   }

   template<
      typename MutexType,
      typename TagType>
   class wrapped_mutex
   {
   };

   template<typename MutexType>
   class wrapped_mutex<MutexType, detail::tag::unique>
   {
   public:

      wrapped_mutex() = default;

      auto lock() -> decltype(std::declval<MutexType>().lock())
      {
         global::tracker.was_locked = true;
         m_mutex.lock();
      }

      auto try_lock() -> decltype(std::declval<MutexType>().try_lock())
      {
         m_mutex.try_lock();
      }

      auto unlock() -> decltype(std::declval<MutexType>().unlock())
      {
         global::tracker.was_unlocked = true;
         m_mutex.unlock();
      }

   private:

      mutable MutexType m_mutex;
   };

   template<typename MutexType>
   class wrapped_mutex<MutexType, detail::tag::shared>
   {
   public:

      wrapped_mutex() = default;

      auto lock_shared() -> decltype(std::declval<MutexType>().lock_shared())
      {
         global::tracker.was_locked = true;
         m_mutex.lock_shared();
      }

      auto try_lock_shared() -> decltype(std::declval<MutexType>().try_lock_shared())
      {
         m_mutex.try_lock_shared();
      }

      auto unlock_shared() -> decltype(std::declval<MutexType>().unlock_shared())
      {
         global::tracker.was_unlocked = true;
         m_mutex.unlock_shared();
      }

   private:

      mutable MutexType m_mutex;
   };
}

TEST_CASE("Trait Detection")
{
   SECTION("Locking capabilities of std::mutex", "[Std]")
   {
      REQUIRE(detail::supports_unique_locking<std::mutex>::value == true);
      REQUIRE(detail::supports_shared_locking<std::mutex>::value == false);
      REQUIRE(detail::supports_timed_locking<std::mutex>::value == false);
   }

   SECTION("Locking capabilities of boost::recursive_mutex", "[Boost]")
   {
      REQUIRE(detail::supports_unique_locking<boost::recursive_mutex>::value == true);
      REQUIRE(detail::supports_shared_locking<boost::recursive_mutex>::value == false);
      REQUIRE(detail::supports_timed_locking<boost::recursive_mutex>::value == false);
   }

   SECTION("Unique locking should always be possible")
   {
      // @todo Use SFINAE to detect lock() functionality on all mutex categories.
   }
}

TEST_CASE("Simple sanity checks")
{
   SECTION("Verifying type-definition")
   {
      REQUIRE(std::is_same_v<
         mutex_guarded<std::string>::value_type,
         std::string>);

      REQUIRE(std::is_same_v<
         mutex_guarded<std::string>::reference,
         std::string&>);

      REQUIRE(std::is_same_v<
         mutex_guarded<std::string>::const_reference,
         const std::string&>);

      REQUIRE(std::is_same_v<
         lock_proxy<mutex_guarded<std::string>, detail::tag::unique>::value_type,
         std::string>);

      REQUIRE(std::is_same_v<
         lock_proxy<mutex_guarded<std::string>, detail::tag::unique>::pointer,
         std::string*>);

      REQUIRE(std::is_same_v<
         lock_proxy<mutex_guarded<std::string>, detail::tag::unique>::const_pointer,
         const std::string*>);

      REQUIRE(std::is_same_v<
         lock_proxy<mutex_guarded<std::string>, detail::tag::unique>::reference,
         std::string&>);

      REQUIRE(std::is_same_v<
         lock_proxy<mutex_guarded<std::string>, detail::tag::unique>::const_reference,
         const std::string&>);
   }
}

TEST_CASE("Guarded with a std::mutex", "[Std]")
{
   const std::string sample = "Testing a std::mutex.";

   using mutex_type = detail::wrapped_mutex<std::mutex, detail::tag::unique>;
   mutex_guarded<std::string, mutex_type> data{ sample };

   detail::global::reset_tracker();

   SECTION("Locking")
   {
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      REQUIRE(*data.lock() == sample);

      REQUIRE(detail::global::tracker.was_locked == true);
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }

   SECTION("Simple data access")
   {
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      REQUIRE(data.lock()->length() == sample.length());

      REQUIRE(detail::global::tracker.was_locked == true);
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }

   SECTION("Data access using a lambda")
   {
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      const std::size_t length = data.with_lock_held([] (const std::string& value) noexcept
      {
         REQUIRE(detail::global::tracker.was_locked == true);

         return value.length();
      });

      REQUIRE(length == sample.length());
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }
}

TEST_CASE("Guarded with a boost::recursive_mutex", "[Boost]")
{
   const std::string sample = "Testing a boost::recursive_mutex.";

   using mutex_type = detail::wrapped_mutex<boost::recursive_mutex, detail::tag::unique>;
   mutex_guarded<std::string, mutex_type> data{ sample };

   detail::global::reset_tracker();

   SECTION("Locking")
   {
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      REQUIRE(*data.lock() == sample);

      REQUIRE(detail::global::tracker.was_locked == true);
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }

   SECTION("Simple data access")
   {
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      REQUIRE(data.lock()->length() == sample.length());

      REQUIRE(detail::global::tracker.was_locked == true);
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }

   SECTION("Data access using a lambda")
   {
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      const std::size_t length = data.with_lock_held([] (const std::string& value) noexcept
      {
         REQUIRE(detail::global::tracker.was_locked == true);

         return value.length();
      });

      REQUIRE(length == sample.length());
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }
}

TEST_CASE("Moves and Copies")
{
   const std::string sample = "Testing a std::mutex.";

   using mutex_type = detail::wrapped_mutex<std::mutex, detail::tag::unique>;
   mutex_guarded<std::string, mutex_type> data{ sample };

   detail::global::reset_tracker();

   SECTION("Copy-assignment")
   {
      auto copy = data;

      REQUIRE(detail::global::tracker.was_locked == true);
      REQUIRE(detail::global::tracker.was_unlocked == true);

      detail::global::reset_tracker();

      REQUIRE(copy.lock()->length() == sample.length());

      REQUIRE(detail::global::tracker.was_locked == true);
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }

   SECTION("Move-assignment")
   {
      auto copy = std::move(data);

      // Moving should not require locking:
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      REQUIRE(copy.lock()->length() == sample.length());

      REQUIRE(detail::global::tracker.was_locked == true);
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }
}
