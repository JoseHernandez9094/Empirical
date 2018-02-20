#ifndef EMP_PLOT_EMP
#define EMP_PLOT_EMP

#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "flow.h"
#include "meta/type_traits.h"

namespace emp {
  namespace tools {

    struct value_tag {};

    template <typename V1, typename V2>
    struct ValuesEq
      : std::is_same<typename V1::attr_type, typename V2::attr_type> {};

    template <typename T>
    struct IsAttributeValue : std::is_base_of<value_tag, T> {};

    template <typename T>
    struct IsMergeable : std::false_type {};

    template <typename...>
    class Attrs;

    namespace __impl_has_attr {
      template <typename Pack, typename Attr>
      struct HasAttr {
        template <typename T>
        static constexpr std::true_type hasAttr(
          const typename Attr::template value_type<T>&) {
          return {};
        }
        static constexpr std::false_type hasAttr(...) { return {}; }

        using type = decltype(hasAttr(std::declval<Pack>()));
      };
    }  // namespace __impl_has_attr

    template <typename Pack, typename Attr>
    struct HasAttr : __impl_has_attr::HasAttr<Pack, Attr>::type {};

    namespace __impl_attr_base {
      template <typename Attr, template <typename> class Value>
      struct AttrBase {
        using attr_type = Attr;
        template <typename T>
        using value_type = Value<T>;

        /// Given an attribute pack, getValue will extract just this attribute
        template <class T>
        static constexpr decltype(auto) getValue(value_type<T>& value) {
          return value;
        }

        template <class T>
        static constexpr decltype(auto) getValue(const value_type<T>& value) {
          return value;
        }

        template <class T>
        static constexpr decltype(auto) getValue(value_type<T>&& value) {
          return value;
        }

        /// Given an attribute pack, get(pack) will extract the value of this
        /// attribute in that pack
        template <class T>
        static constexpr const T& get(const value_type<T>& target) {
          return target.get();
        }

        template <class T>
        static constexpr T& get(value_type<T>& target) {
          return target.get();
        }

        template <class T>
        static constexpr T&& get(value_type<T>&& target) {
          return std::move(target).get();
        }

        template <class T>
        static constexpr const T&& get(const value_type<T>&& target) {
          return std::move(target).get();
        }

        // -- callOrGet --
        private:
        // Handle the case when get(target) is callable
        template <class Pack, class... U>
        static constexpr decltype(auto) __impl_callOrGet(
          const std::true_type& isCallable, Pack&& pack, U&&... args) {
          return get(std::forward<Pack>(pack))(std::forward<U>(args)...);
        }

        // Handle the case when get(target) is not callable
        template <class T, class... U>
        static constexpr decltype(auto) __impl_callOrGet(
          const std::false_type& isCallable, T&& target, U&&... args) {
          return get(std::forward<T>(target));
        }

        public:
        /// Given an attribute pack, callOrGet(pack, args...) will attempt to
        /// return get(pack)(args...). If this fails to compile, because the
        /// value of this attribute is not callable in the given pack, then just
        /// the value of this attribute will be returned, get(pack).
        template <class V, class... U>
        static constexpr decltype(auto) callOrGet(V&& target, U&&... args) {
          using ValueOfTargetType = decltype(get(std::forward<V>(target)));

          return __impl_callOrGet(
            // Check if the target attribute value is invocable
            emp::is_invocable<ValueOfTargetType,
                              decltype(std::forward<U>(args))...>{},
            std::forward<V>(target), std::forward<U>(args)...);
        }
        // -- getOrElse --
        private:
        template <class Pack, class F, class... U>
        static constexpr decltype(auto) __impl_getOrElse(const std::true_type&,
                                                         Pack&& pack, F&&,
                                                         U&&...) {
          return get(std::forward<Pack>(pack));
        }

        template <class Pack, class F, class... U>
        static constexpr decltype(auto) __impl_getOrElse(const std::false_type&,
                                                         Pack&&,
                                                         F&& defaultFunction,
                                                         U&&... args) {
          return std::forward<F>(defaultFunction)(std::forward<U>(args)...);
        }

        public:
        template <class Pack, class F, class... U>
        static constexpr decltype(auto) getOrElse(Pack&& pack,
                                                  F&& defaultFunction,
                                                  U&&... args) {
          return __impl_getOrElse(
            HasAttr<std::decay_t<Pack>, attr_type>{}, std::forward<Pack>(pack),
            std::forward<F>(defaultFunction), std::forward<U>(args)...);
        }

        private:
        // Utility class used by getOr which simply remembers and returns a
        // value. @todo: when we switch to c++17, we can replace this with a
        // constexpr closure
        template <class D>
        struct Default {
          D value;
          constexpr decltype(auto) operator()() const {
            return std::forward<D>(value);
          }
        };

        public:
        template <class Pack, class D>
        static constexpr decltype(auto) getOr(Pack&& pack, D&& defaultValue) {
          return getOrElse(std::forward<Pack>(pack),
                           Default<D&&>{std::forward<D>(defaultValue)});
        }

        private:
        // Utility class used by getOrGetIn which simply remembers a attribute
        // pack.
        template <typename...>
        struct FallbackHandler;

        template <typename Fallback>
        struct FallbackHandler<Fallback> {
          Fallback fallback;
          template <typename F>
          constexpr FallbackHandler(F&& fallback)
            : fallback(std::forward<F>(fallback)) {}

          constexpr decltype(auto) operator()() const {
            return get(std::forward<Fallback>(fallback));
          }
        };

        template <typename Fallback0, typename Fallback1, typename... Fallbacks>
        struct FallbackHandler<Fallback0, Fallback1, Fallbacks...> {
          Fallback0 fallback;
          FallbackHandler<Fallback1, Fallbacks...> fallbacks;
          template <typename F0, typename F1, typename... F>
          constexpr FallbackHandler(F0&& fallback0, F1&& fallback1,
                                    F&&... fallbacks)
            : fallback(std::forward<F0>(fallback0)),
              fallbacks(std::forward<F1>(fallback1),
                        std::forward<F>(fallbacks)...) {}

          constexpr decltype(auto) operator()() && {
            return getOrElse(std::forward<Fallback0>(fallback),
                             std::move(fallbacks));
          }
        };

        public:
        template <class... Fallbacks>
        static constexpr decltype(auto) getOrGetIn(Fallbacks&&... fallbacks) {
          return FallbackHandler<Fallbacks&&...>{
            std::forward<Fallbacks>(fallbacks)...}();
        }

        template <class T, class V>
        static constexpr void set(value_type<T>& target, V&& value) {
          get(target) = std::forward<V>(value);
        }
        template <class T>
        static constexpr value_type<std::decay_t<T>> value(T&& value) {
          return {std::forward<T>(value)};
        }
      };
    };  // namespace __impl_attr_base

#define DEFINE_ATTR(NAME, _name)                                             \
  template <class T>                                                         \
  struct NAME##Value;                                                        \
  struct NAME : emp::tools::__impl_attr_base::AttrBase<NAME, NAME##Value> {  \
    static constexpr auto name = #_name;                                     \
  };                                                                         \
  constexpr const char* NAME::name;                                          \
  template <class T>                                                         \
  struct NAME##Value : emp::tools::value_tag {                               \
    static constexpr auto name = #_name;                                     \
    using attr_type = NAME;                                                  \
    using attrs_type = emp::tools::Attrs<NAME##Value<T>>;                    \
    using value_type = T;                                                    \
    T _name;                                                                 \
    NAME##Value() = delete;                                                  \
    constexpr NAME##Value(const T& value) : _name(value) {}                  \
    constexpr NAME##Value(T&& value) : _name(std::move(value)) {}            \
    constexpr NAME##Value(const NAME##Value& other) : _name(other._name) {}  \
    constexpr NAME##Value(NAME##Value&& other)                               \
      : _name(std::move(other._name)) {}                                     \
    constexpr NAME##Value& operator=(const T& value) {                       \
      _name = value;                                                         \
      return *this;                                                          \
    }                                                                        \
    constexpr NAME##Value& operator=(T&& value) {                            \
      _name = std::move(value);                                              \
      return *this;                                                          \
    }                                                                        \
    constexpr NAME##Value& operator=(const NAME##Value& other) {             \
      if (this != &other) _name = other._name;                               \
      return *this;                                                          \
    }                                                                        \
    constexpr NAME##Value& operator=(NAME##Value&& other) {                  \
      if (this != &other) _name = std::move(other._name);                    \
      return *this;                                                          \
    }                                                                        \
    constexpr T& get() & { return _name; }                                   \
    constexpr T&& get() && { return std::move(_name); }                      \
    constexpr const T& get() const & { return _name; }                       \
    constexpr const T& get() const && { return std::move(_name); }           \
  };                                                                         \
  template <typename T>                                                      \
  constexpr const char* NAME##Value<T>::name;                                \
  template <typename T>                                                      \
  struct emp::tools::IsMergeable<NAME##Value<T>> : std::true_type {};        \
  template <class T>                                                         \
  constexpr auto _name(T&& value) {                                          \
    return NAME::value(std::forward<T>(value));                              \
  }                                                                          \
  template <class A, class B>                                                \
  constexpr bool operator==(const NAME##Value<A>& a,                         \
                            const NAME##Value<B>& b) {                       \
    return NAME::get(a) == NAME::get(b);                                     \
  }                                                                          \
  template <class T>                                                         \
  std::ostream& operator<<(std::ostream& out, const NAME##Value<T>& value) { \
    return out << "\"" #_name "\": " << value._name << std::endl;            \
  }

    namespace __attrs_impl {

      template <class... T>
      class AttrsParent : public T... {
        protected:
        struct args_tag {};
        struct copy_tag {};
        struct move_tag {};

        template <class U>
        struct __attrs_impl_constructor_detector {
          private:
          template <class... X>
          static copy_tag detect(const Attrs<X...>&) {
            return {};
          };

          template <class... X>
          static move_tag detect(Attrs<X...>&&) {
            return {};
          };

          static args_tag detect(...) { return {}; }

          public:
          using type = decltype(detect(std::declval<U>()));
        };

        template <class U>
        using constructor_detector =
          typename __attrs_impl_constructor_detector<U>::type;

        template <class... U>
        constexpr AttrsParent(const args_tag&, U&&... args)
          : T{std::forward<U>(args)}... {}

        template <class... U>
        constexpr AttrsParent(const copy_tag&, const Attrs<U...>& other)
          : T{T::attr_type::get(other)}... {}
        template <class... U>
        constexpr AttrsParent(const move_tag&, Attrs<U...>&& other)
          : T{T::attr_type::get(std::move(other))}... {}
      };

      // This struct exists so that we can pass around Attrs<U...> without it
      // taking up any memory or having any side effects from construction.
      template <class>
      struct wrapper {};

    }  // namespace __attrs_impl

    template <class D>
    class ListTransform {
      constexpr D* self() { return static_cast<D*>(this); }
      constexpr const D* self() const { return static_cast<const D*>(this); }
      template <class... U>
      constexpr decltype(auto) callDerived(U&&... args) {
        return (*self())(std::forward<U>(args)...);
      }
      template <class... U>
      constexpr decltype(auto) callDerived(U&&... args) const {
        return (*self())(std::forward<U>(args)...);
      }

      public:
      constexpr ListTransform() = default;
      constexpr ListTransform(const ListTransform&) = default;
      constexpr ListTransform(ListTransform&&) = default;

      template <class InputIter, class OutputIter>
      void apply(InputIter&& begin, InputIter&& end,
                 OutputIter&& target) const {
        std::transform(begin, end, target, [this](auto&& d) {
          return callDerived(std::forward<decltype(d)>(d));
        });
      }

      template <class DataIter, class PrevIter>
      auto apply(DataIter dbegin, DataIter dend, PrevIter pbegin,
                 PrevIter pend) const {
        std::vector<decltype(pbegin->update(callDerived(*dbegin)))> transformed;
        for (; dbegin != dend && pbegin != pend; ++pbegin, ++dbegin) {
          transformed.push_back(pbegin->update(callDerived(*dbegin)));
        }
        return transformed;
      }

      template <class Iter>
      auto apply(Iter begin, Iter end) const {
        std::vector<decltype(callDerived(*begin))> transformed;
        for (; begin != end; ++begin) {
          transformed.push_back(callDerived(*begin));
        }
        return transformed;
      }

      template <class U>
      auto apply(const std::vector<U>& data) const {
        std::vector<decltype(callDerived(data[0]))> transformed;
        transformed.reserve(data.size());

        for (auto& d : data) {
          transformed.push_back(callDerived(d));
        }

        return transformed;
      }
    };

    template <typename... T>
    constexpr Attrs<std::decay_t<T>...> attrs(T&&... props);

    template <typename... T>
    class Attrs : public Joinable<Attrs<T...>>,
                  public ListTransform<Attrs<T...>>,
                  public __attrs_impl::AttrsParent<T...> {
      public:
      using attrs_type = Attrs;
      // This is one of the really nasty parts. The problem is that we really
      // want two constructors
      //
      // (1) template<class... U> Attrs(U&& ... attrs),
      // which initializes all the members of the attribute pack and
      //
      // (2) template<class... U> Attrs(const Attrs<U...>& other),
      // which copies the data out of the other attribute pack and into this
      // one.
      //
      // Sadly, C++ can't tell the difference between these two signatures,
      // and will only call the first because of how c++ decides the order to
      // resolve overloads in. In ordered to ensure that the correct
      // constructor is called, we calculate which constructor should be
      // called, and then pass the result into AttrsParent, which has
      // unambiguously overloaded constructors which handle each case
      template <class U0, class... U>
      constexpr Attrs(U0&& arg, U&&... args)
        : __attrs_impl::AttrsParent<T...>(
            // This will resolve to one of several marker structs which will
            // select the correct constructor
            typename __attrs_impl::AttrsParent<
              T...>::template constructor_detector<U0>{},
            // The arguments are then forwarded as normal
            std::forward<U0>(arg), std::forward<U>(args)...) {}

      private:
      template <bool Last, typename A>
      struct GetAttr;
      template <typename A>
      struct GetAttr<false, A> {
        template <typename S, typename... U>
        static constexpr decltype(auto) get(S&& self, U&&... args) {
          using attr_type = typename A::attr_type;
          return attr_type::value(
            attr_type::callOrGet(std::forward<S>(self), args...));
        }
      };

      template <typename A>
      struct GetAttr<true, A> {
        template <typename S, typename... U>
        static constexpr decltype(auto) get(S&& self, U&&... args) {
          using attr_type = typename A::attr_type;
          return attr_type::value(attr_type::callOrGet(
            std::forward<S>(self), std::forward<U>(args)...));
        }
      };

      template <typename S, typename... U>
      static constexpr decltype(auto) call(S&& self, U&&... args) {
        constexpr size_t last = sizeof...(T) - 1;
        return attrs(
          GetAttr<variadic_index_of_v<T, Attrs> == last,
                  T>::template get<S, U...>(std::forward<S>(self),
                                            std::forward<U>(args)...)...);
      }

      public:
      template <class... U>
      constexpr decltype(auto) operator()(U&&... args) & {
        return call(*this, std::forward<U>(args)...);
      }

      template <class... U>
      constexpr decltype(auto) operator()(U&&... args) && {
        return call(std::move(*this), std::forward<U>(args)...);
      }

      template <class... U>
      constexpr decltype(auto) operator()(U&&... args) const & {
        return call(*this, std::forward<U>(args)...);
      }

      private:
      template <class O>
      constexpr bool eq(const O& other,
                        const __attrs_impl::wrapper<Attrs<>>&) const {
        return true;
      }

      template <class O, class U0, class... U>
      constexpr bool eq(const O& other,
                        const __attrs_impl::wrapper<Attrs<U0, U...>>&) const {
        return U0::attr_type::get(*this) == U0::attr_type::get(other) &&
               eq(other, __attrs_impl::wrapper<Attrs<U...>>{});
      }

      public:
      /// Attribute packs support equals and not equals, but are not
      /// orderable, as the members are not considered to be ordered from the
      /// user's perspective.
      template <class... U>
      constexpr bool operator==(const Attrs<U...>& other) const {
        static_assert(sizeof...(T) == sizeof...(U),
                      "Cannot compare attribute packs with different members");
        return eq(other, __attrs_impl::wrapper<Attrs<U...>>{});
      }

      private:
      template <typename F, typename I, typename U0, typename... U>
      constexpr decltype(auto) __impl_reduce(
        F&& callback, I&& init,
        const __attrs_impl::wrapper<Attrs<U0, U...>>&) const {
        return std::forward<F>(callback)(std::forward<I>(init), U0::name,
                                         U0::attr_type::get(*this));
      }

      template <typename F, typename I, typename U0, typename U1, typename... U>
      constexpr decltype(auto) __impl_reduce(
        F&& callback, I&& init,
        const __attrs_impl::wrapper<Attrs<U0, U1, U...>>&) const {
        using new_init_t = decltype(
          callback(std::forward<I>(init), U0::name, U0::attr_type::get(*this)));
        return __impl_reduce(
          std::forward<F>(callback),
          std::forward<new_init_t>(callback(std::forward<I>(init), U0::name,
                                            U0::attr_type::get(*this))),
          __attrs_impl::wrapper<Attrs<U1, U...>>{});
      }

      public:
      template <typename F, typename I>
      constexpr decltype(auto) reduce(F&& callback, I&& init) const {
        return __impl_reduce(std::forward<F>(callback), std::forward<I>(init),
                             __attrs_impl::wrapper<Attrs>{});
      };

      private:
      template <typename S, typename F, typename U0>
      static constexpr void __impl_foreach(
        S&& self, F&& callback, const __attrs_impl::wrapper<Attrs<U0>>&) {
        std::forward<F>(callback)(U0::name,
                                  U0::attr_type::get(std::forward<S>(self)));
      }

      template <typename S, typename F, typename U0, typename U1, typename... U>
      static constexpr void __impl_foreach(
        S&& self, F&& callback,
        const __attrs_impl::wrapper<Attrs<U0, U1, U...>>&) {
        callback(U0::name, U0::attr_type::get(self));

        __impl_foreach(std::forward<S>(self), std::forward<F>(callback),
                       __attrs_impl::wrapper<Attrs<U1, U...>>{});
      }

      template <typename S, typename F>
      static constexpr void __impl_foreach(S&& self, F&& callback) {
        __impl_foreach(std::forward<S>(self), std::forward<F>(callback),
                       __attrs_impl::wrapper<Attrs>{});
      };

      public:
      template <typename F>
      constexpr void foreach (F&& callback) & {
        __impl_foreach(*this, std::forward<F>(callback));
      }

      template <typename F>
      constexpr void foreach (F&& callback) const & {
        __impl_foreach(*this, std::forward<F>(callback));
      }

      template <typename F>
      constexpr void foreach (F&& callback) && {
        __impl_foreach(std::move(*this), std::forward<F>(callback));
      }

      template <typename F>
      constexpr void foreach (F&& callback) const && {
        __impl_foreach(std::move(*this), std::forward<F>(callback));
      }

      template <typename O>
      constexpr decltype(auto) set(O&& other) & {
        return merge(*this, std::forward<O>(other));
      }

      template <typename O>
      constexpr decltype(auto) set(O&& other) const & {
        return merge(*this, std::forward<O>(other));
      }

      template <typename O>
      constexpr decltype(auto) set(O&& other) && {
        return merge(std::move(*this), std::forward<O>(other));
      }

      template <typename O>
      constexpr decltype(auto) set(O&& other) const && {
        return merge(std::move(*this), std::forward<O>(other));
      }
    };

    template <typename... T>
    struct IsMergeable<Attrs<T...>> : std::true_type {};

    ///  An alternative syntax for creating attribute packs. Takes any number
    ///  of attributes and returns a pack containing each of those attributes.
    template <class... T>
    constexpr Attrs<std::decay_t<T>...> attrs(T&&... props) {
      return {std::forward<T>(props)...};
    };

    namespace __attrs_impl {
      template <typename S, typename P, typename... U>
      static constexpr decltype(auto) __impl_merge(
        S&& self, P&& other, const wrapper<Attrs<U...>>&) {
        // use the getters to extract all the data
        return attrs(U::attr_type::value(U::attr_type::getOrGetIn(
          std::forward<P>(other), std::forward<S>(self)))...);
      }
    }  // namespace __attrs_impl

    /// Creates a new attribute pack which has all the attributes of this
    /// pack and another pack. Values will be taken from other other pack
    /// preferentially.

    template <typename A>
    constexpr decltype(auto) merge(A&& a) {
      return std::forward<A>(a);
    }

    template <typename A0, typename A1>
    constexpr decltype(auto) merge(A0&& a0, A1&& a1) {
      using self_attrs_type = typename std::decay_t<A0>::attrs_type;
      using other_attrs_type = typename std::decay_t<A1>::attrs_type;

      return __attrs_impl::__impl_merge(
        std::forward<A0>(a0), std::forward<A1>(a1),
        __attrs_impl::wrapper<
          variadic_union_t<self_attrs_type, other_attrs_type>>{});
    }

    template <typename A0, typename A1, typename A2, typename... A>
    constexpr decltype(auto) merge(A0&& a0, A1&& a1, A2&& a2, A&&... packs) {
      using self_attrs_type = typename std::decay_t<A0>::attrs_type;
      using other_attrs_type = typename std::decay_t<A1>::attrs_type;

      return merge(__attrs_impl::__impl_merge(
                     std::forward<A0>(a0), std::forward<A1>(a1),
                     __attrs_impl::wrapper<
                       variadic_union_t<self_attrs_type, other_attrs_type>>{}),
                   std::forward<A2>(a2), std::forward<A>(packs)...);
    }

    template <class From, class To,
              class = std::enable_if_t<IsMergeable<std::decay_t<From>>::value &&
                                       IsMergeable<std::decay_t<To>>::value>>
    constexpr auto operator+(From&& from, To&& to) {
      return merge(std::forward<From>(from), std::forward<To>(to));
    }

    namespace __attrs_impl {
      template <class...>
      struct print_attrs_tag {};

      template <class... T, class H>
      void printAttrs(std::ostream& out, const Attrs<T...>& attrs,
                      const print_attrs_tag<H>&) {
        out << '"' << H::name << "\": " << H::attr_type::get(attrs);
      }
      template <class... T, class H0, class H1, class... U>
      void printAttrs(std::ostream& out, const Attrs<T...>& attrs,
                      const print_attrs_tag<H0, H1, U...>&) {
        out << '"' << H0::name << "\": " << H0::attr_type::get(attrs) << ", ";
        printAttrs(out, attrs, print_attrs_tag<H1, U...>{});
      }
    }  // namespace __attrs_impl

    template <class H, class... T>
    std::ostream& operator<<(std::ostream& out, const Attrs<H, T...>& attrs) {
      out << "{ ";
      __attrs_impl::printAttrs(out, attrs,
                               __attrs_impl::print_attrs_tag<H, T...>{});
      return out << " }";
    }

    std::ostream& operator<<(std::ostream& out, const Attrs<>& attrs) {
      return out << "{ }";
    }
  }  // namespace tools
}  // namespace emp

#endif  // EMP_PLOT_EMP
