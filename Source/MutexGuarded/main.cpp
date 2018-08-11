#include <iostream>
#include <mutex>
#include <shared_mutex>

#include <boost/thread/recursive_mutex.hpp>

#include "MutexGuarded.hpp"
#include "BoostTraits.hpp"

int main()
{
   {
      mutex_guarded<std::string> str{ "Testing a std::mutex." };
      std::cout << str.lock()->length() << '\n';
   }

   std::cout << '\n';

   {
      mutex_guarded<std::string> data{ "Testing a std::mutex." };
      const int result = data.with_lock_held([] (decltype(data)::value_type& value) noexcept
      {
         std::cout << value.length() << '\n';
         return 42;
      });
   }

   std::cout << '\n';

   {
      mutex_guarded<std::string, std::shared_mutex> str{ "Testing a std::shared_mutex." };
      std::cout << str.write_lock()->length() << '\n';
   }

   std::cout << '\n';

   {
      mutex_guarded<std::string, std::timed_mutex> str{ "Testing a std::timed_mutex." };
      std::cout << str.try_lock_for(std::chrono::seconds{ 1 })->length() << '\n';
   }

   std::cout << '\n';

   {
      mutex_guarded<std::string, boost::recursive_mutex> str{ "Testing a boost recursive mutex." };
      std::cout << str.lock()->length() << '\n';
   }

   std::cout << '\n';

   {
      mutex_guarded<std::string, boost::recursive_timed_mutex> str{ "Testing a boost recursive, timed mutex." };
      std::cout << str.try_lock_for(boost::chrono::seconds{ 1 })->length() << '\n';
   }

   return 0;
}
