#pragma once

namespace nonstd
{
   namespace std17
   {
      namespace detail
      {
         template<typename...>
         struct make_void
         {
            using type = void;
         };
      }

      template<typename... T>
      using void_t = typename detail::make_void<T...>::type;
   }
}
