#include "unit_tests.hpp"

//#include <iostream>
//#include <mutex>
//#include <shared_mutex>
//
//#include <boost/thread/recursive_mutex.hpp>
//
//#include "mutex_guarded.hpp"
//#include "boost_mutex_traits.hpp"

//int main()
//{
//   std::cout << '\n';
//
//   {
//      mutex_guarded<std::string, std::timed_mutex> str{ "Testing a std::timed_mutex." };
//      std::cout << str.try_lock_for(std::chrono::seconds{ 1 })->length() << '\n';
//   }
//
//   std::cout << '\n';
//
//   {
//      mutex_guarded<std::string, boost::recursive_timed_mutex> str{ "Testing a boost recursive, timed mutex." };
//      std::cout << str.try_lock_for(boost::chrono::seconds{ 1 })->length() << '\n';
//   }
//
//   return 0;
//}
