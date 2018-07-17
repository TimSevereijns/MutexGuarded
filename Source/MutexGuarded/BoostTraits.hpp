#pragma once

#include "MutexGuarded.hpp"

#include <boost/chrono/chrono.hpp>

namespace detail
{
   template<>
   struct SupportsTimedLocking<
      boost::recursive_timed_mutex,
      std::void_t<
         decltype(std::declval<boost::recursive_timed_mutex>().try_lock_for(std::declval<boost::chrono::seconds>()))>>
      : std::true_type
   {
   };
}
