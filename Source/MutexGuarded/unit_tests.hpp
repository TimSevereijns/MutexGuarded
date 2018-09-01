#pragma once

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "mutex_guarded.hpp"

#include <iostream>

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
   class wrapped_mutex<MutexType, detail::mutex_category::unique>
   {
   public:

      wrapped_mutex()
      {
         global::reset_tracker();
      }

      auto lock() -> decltype(std::declval<MutexType>().lock())
      {
         //std::cout << "Locking...\n";

         global::tracker.was_locked = true;
         m_mutex.lock();
      }

      auto try_lock() -> decltype(std::declval<MutexType>().try_lock())
      {
         m_mutex.try_lock();
      }

      auto unlock() -> decltype(std::declval<MutexType>().unlock())
      {
         //std::cout << "Unlocking...\n";

         global::tracker.was_unlocked = true;
         m_mutex.unlock();
      }

   private:

      mutable MutexType m_mutex;
   };

   template<typename MutexType>
   class wrapped_mutex<MutexType, detail::mutex_category::shared> :
      public wrapped_mutex<MutexType, detail::mutex_category::unique>
   {
   public:

      wrapped_mutex()
      {
         global::reset_tracker();
      }

      auto lock_shared() -> decltype(std::declval<MutexType>().lock_shared())
      {
         //std::cout << "Locking shared...\n";

         global::tracker.was_locked = true;
         m_mutex.lock_shared();
      }

      auto try_lock_shared() -> decltype(std::declval<MutexType>().try_lock_shared())
      {
         global::tracker.was_locked = true;
         return m_mutex.try_lock_shared();
      }

      auto unlock_shared() -> decltype(std::declval<MutexType>().unlock_shared())
      {
         //std::cout << "Unlocking shared...\n";

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

   SECTION("Locking capabilities of boost::shared_mutex", "[Boost]")
   {
      REQUIRE(detail::supports_unique_locking<boost::shared_mutex>::value == true);
      REQUIRE(detail::supports_shared_locking<boost::shared_mutex>::value == true);
      REQUIRE(detail::supports_timed_locking<boost::shared_mutex>::value == false);
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
         lock_proxy<mutex_guarded<std::string>, detail::mutex_category::unique>::value_type,
         std::string>);

      REQUIRE(std::is_same_v<
         lock_proxy<mutex_guarded<std::string>, detail::mutex_category::unique>::pointer,
         std::string*>);

      REQUIRE(std::is_same_v<
         lock_proxy<mutex_guarded<std::string>, detail::mutex_category::unique>::const_pointer,
         const std::string*>);

      REQUIRE(std::is_same_v<
         lock_proxy<mutex_guarded<std::string>, detail::mutex_category::unique>::reference,
         std::string&>);

      REQUIRE(std::is_same_v<
         lock_proxy<mutex_guarded<std::string>, detail::mutex_category::unique>::const_reference,
         const std::string&>);
   }
}

TEST_CASE("Guarded with a std::mutex", "[Std]")
{
   const std::string sample = "Testing a std::mutex.";

   using mutex_type = detail::wrapped_mutex<std::mutex, detail::mutex_category::unique>;
   const mutex_guarded<std::string, mutex_type> data{ sample };

   SECTION("Locking")
   {
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      REQUIRE(data.lock().is_valid());
      REQUIRE(*data.lock() == sample);
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
         //std::cout << "   In lambda...\n";

         return value.length();
      });

      REQUIRE(length == sample.length());
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }
}

TEST_CASE("Guarded with a boost::recursive_mutex", "[Boost]")
{
   const std::string sample = "Testing a boost::recursive_mutex.";

   using mutex_type = detail::wrapped_mutex<boost::recursive_mutex, detail::mutex_category::unique>;
   mutex_guarded<std::string, mutex_type> data{ sample };

   SECTION("Locking")
   {
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      REQUIRE(data.lock().is_valid());
      REQUIRE(*data.lock() == sample);
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
         //std::cout << "   In lambda...\n";

         return value.length();
      });

      REQUIRE(length == sample.length());
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }
}

TEST_CASE("Guarded with a boost::shared_mutex", "[Boost]")
{
   const std::string sample = "Testing a boost::shared_mutex.";

   using mutex_type = detail::wrapped_mutex<boost::shared_mutex, detail::mutex_category::shared>;
   mutex_guarded<std::string, mutex_type> data{ sample };

   SECTION("Read Locking")
   {
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      REQUIRE(data.read_lock().is_valid());
      REQUIRE(*data.read_lock() == sample);
      REQUIRE(data.read_lock()->length() == sample.length());

      REQUIRE(detail::global::tracker.was_locked == true);
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }

   SECTION("Write Locking")
   {
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      REQUIRE(data.write_lock().is_valid());
      REQUIRE(data.write_lock()->length() == sample.length());

      REQUIRE(detail::global::tracker.was_locked == true);
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }

   SECTION("Reading data using a lambda")
   {
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      const std::size_t length = data.with_read_lock_held([] (const std::string& value) noexcept
      {
         REQUIRE(detail::global::tracker.was_locked == true);
         //std::cout << "   In lambda...\n";

         return value.length();
      });

      REQUIRE(length == sample.length());
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }

   SECTION("Writing data using a lambda")
   {
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      const std::string anotherString = "Something else";

      const std::size_t length = data.with_write_lock_held([&] (std::string& value) noexcept
      {
         REQUIRE(detail::global::tracker.was_locked == true);
         //std::cout << "   In lambda...\n";

         value = anotherString;
         return value.length();
      });

      REQUIRE(length == anotherString.length());
      REQUIRE(*data.read_lock() == anotherString);

      REQUIRE(detail::global::tracker.was_unlocked == true);
   }
}

TEST_CASE("Moves and Copies")
{
   const std::string sample = "Testing a std::mutex.";

   using mutex_type = detail::wrapped_mutex<std::mutex, detail::mutex_category::unique>;
   mutex_guarded<std::string, mutex_type> data{ sample };

   SECTION("Copy-assignment")
   {
      auto copy = data;

      // Making a copy should lock the source object:
      REQUIRE(detail::global::tracker.was_locked == true);
      REQUIRE(detail::global::tracker.was_unlocked == true);

      detail::global::reset_tracker();

      REQUIRE(copy.lock()->length() == sample.length());

      REQUIRE(detail::global::tracker.was_locked == true);
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }

   SECTION("Move-assignment")
   {
      const auto copy = std::move(data);

      // Moving should not require locking:
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      REQUIRE(copy.lock()->length() == sample.length());

      REQUIRE(detail::global::tracker.was_locked == true);
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }
}

TEST_CASE("Const-Correctness")
{
   const std::string sample = "Some sample data.";

   SECTION("Const-ref to mutable mutex_guarded<..., std::mutex>")
   {
      using mutex_type = detail::wrapped_mutex<std::mutex, detail::mutex_category::unique>;
      mutex_guarded<std::string, mutex_type> data{ sample };

      const auto& copy = data;

      REQUIRE(copy.lock().is_valid());
      REQUIRE(copy.lock()->length() == sample.length());
   }

   SECTION("Const-ref to mutable mutex_guarded<..., boost::shared_mutex>")
   {
      using mutex_type = detail::wrapped_mutex<boost::shared_mutex, detail::mutex_category::shared>;
      mutex_guarded<std::string, mutex_type> data{ sample };

      const auto& copy = data;

      REQUIRE(copy.read_lock().is_valid());
      REQUIRE(copy.read_lock()->length() == sample.length());

      // @note Grabbing a write lock on a const-reference to a mutex_guarded<...> instance
      // is obviously non-sensical; use a read lock instead!
   }
}
