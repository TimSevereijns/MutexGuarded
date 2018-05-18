#include <iostream>
#include <shared_mutex>

#include "MutexGuarded.hpp"

int main()
{
   {
      MutexGuarded<std::string, std::shared_mutex> str{ "This is a test." };
      std::cout << str.write_lock()->length() << '\n';
   }

   std::cout << '\n';

   {
      MutexGuarded<std::string, std::mutex> str{ "This is another test." };
      std::cout << str.lock()->length() << '\n';
   }

   return 0;
}
