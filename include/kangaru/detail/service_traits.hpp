#ifndef KGR_KANGARU_INCLUDE_KANGARU_DETAIL_SERVICE_TRAITS_HPP
#define KGR_KANGARU_INCLUDE_KANGARU_DETAIL_SERVICE_TRAITS_HPP

#include "traits.hpp"
#include "single.hpp"
#include "injected.hpp"
#include "invoke.hpp"
#include "container_service.hpp"

#ifdef _MSC_VER
#ifndef __clang__
#define KGR_KANGARU_MSVC_NO_AUTOCALL_MAP_CHECK
#endif // !__clang__
#endif // _MSC_VER

#if _MSC_VER == 1900
#ifndef __clang__
#define KGR_KANGARU_MSVC_NO_DEPENDENT_TEMPLATE_KEYWORD
#endif // !__clang__
#endif // _MSC_VER

namespace kgr {
namespace detail {

template<typename T>
using is_container_service = std::is_base_of<ContainerServiceTag, T>;

template<typename F, typename... Args>
struct is_construct_invokable_helper {
private:
	template<typename...>
	static std::false_type test_helper(...);
	
	template<typename C, typename... As, std::size_t... S, int_t<
		decltype(std::declval<C>()(std::declval<function_argument_t<S, C>>()..., std::declval<As>()...)),
		enable_if_t<is_service<injected_argument_t<S, C>>::value>...> = 0>
	static std::true_type test_helper(seq<S...>);
	
	template<typename C, typename... As>
	static decltype(test_helper<C, As...>(tuple_seq_minus<function_arguments_t<C>, sizeof...(As)>{})) test(int);
	
	template<typename...>
	static std::false_type test(...);
	
public:
	using type = decltype(test<F, Args...>(0));
};

template<typename F, typename... Args>
using is_construct_invokable = typename is_construct_invokable_helper<F, Args...>::type;

template<typename, typename, typename, typename = void>
struct has_callable_template_construct : std::false_type {};

template<typename T, typename... TArgs, typename... Args>
struct has_callable_template_construct<
	T, meta_list<TArgs...>, meta_list<Args...>,
	enable_if_t<is_construct_invokable<decltype(&T::template construct<TArgs...>), Args...>::value>
> : std::true_type {};

template<typename, typename, typename, typename = void>
struct get_template_construct_helper;

template<typename T, typename... Args>
struct get_template_construct_helper<
	T, meta_list<>, meta_list<Args...>,
	enable_if_t<!has_callable_template_construct<T, meta_list<>, meta_list<Args...>>::value>
> {};

template<typename T, typename Head, typename... Tail, typename... Args>
struct get_template_construct_helper<
	T, meta_list<Head, Tail...>, meta_list<Args...>,
	enable_if_t<!has_callable_template_construct<T, meta_list<Head, Tail...>, meta_list<Args...>>::value>
> : get_template_construct_helper<T, meta_list<Tail...>, meta_list<Args...>> {};

template<typename T, typename... TArgs, typename... Args>
struct get_template_construct_helper<
	T, meta_list<TArgs...>, meta_list<Args...>,
	enable_if_t<has_callable_template_construct<T, meta_list<TArgs...>, meta_list<Args...>>::value>
> : std::integral_constant<decltype(&T::template construct<TArgs...>), &T::template construct<TArgs...>> {};

template<typename T, typename... Args>
using get_template_construct = get_template_construct_helper<T, meta_list<Args...>, meta_list<Args...>>;

template<typename, typename, typename = void>
struct template_construct_exist : std::false_type {};

template<typename T, typename... Args>
struct template_construct_exist<T, meta_list<Args...>, void_t<decltype(&T::template construct<Args...>)>> : std::true_type {};

template<typename, typename, typename = void>
struct get_any_template_construct_helper;

template<typename T, typename Head, typename... Tail>
struct get_any_template_construct_helper<
	T, meta_list<Head, Tail...>,
	enable_if_t<!template_construct_exist<T, meta_list<Head, Tail...>>::value>
> : get_any_template_construct_helper<T, meta_list<Tail...>> {};

template<typename T, typename... Args>
struct get_any_template_construct_helper<
	T, meta_list<Args...>,
	enable_if_t<template_construct_exist<T, meta_list<Args...>>::value>
> : std::integral_constant<decltype(&T::template construct<Args...>), &T::template construct<Args...>> {};

template<typename T, typename... Args>
using get_any_template_construct = get_any_template_construct_helper<T, meta_list<Args...>>;

template<typename, typename, typename = void>
struct has_any_template_construct_helper : std::false_type {};

template<typename T, typename... Args>
struct has_any_template_construct_helper<T, meta_list<Args...>, void_t<typename get_any_template_construct<T, Args...>::value_type>> : std::true_type {};

template<typename T, typename... Args>
using has_any_template_construct = has_any_template_construct_helper<T, meta_list<Args...>>;

template<typename, typename, typename = void>
struct has_template_construct_helper : std::false_type {};

template<typename T, typename... Args>
struct has_template_construct_helper<T, meta_list<Args...>, void_t<typename get_template_construct<T, Args...>::value_type>> : std::true_type {};

template<typename T, typename... Args>
using has_template_construct = has_template_construct_helper<T, meta_list<Args...>>;

template<bool, typename T, typename... Args>
struct construct_function_helper {
private:
	template<typename U>
	struct get_construct {
		using type = std::integral_constant<decltype(&U::construct), &U::construct>;
	};
	
	template<typename U, typename... As, enable_if_t<
		has_construct<U>::value &&
		!has_template_construct<U, As...>::value, int> = 0>
	static get_construct<U> test();
	
	template<typename U, typename... As, enable_if_t<has_template_construct<U, As...>::value, int> = 0>
	static get_template_construct<U, As...> test();
	
	using inner_type = decltype(test<T, Args...>());
	
public:
	using type = typename inner_type::type;
};

template<typename T, typename... Args>
struct construct_function_helper<false, T, Args...> {};

template<typename T, typename... Args>
struct has_any_construct {
	constexpr static bool value = has_any_template_construct<T, Args...>::value || has_construct<T>::value;
};

template<typename T, typename... Args>
using has_valid_construct = std::integral_constant<bool, has_template_construct<T, Args...>::value || has_construct<T>::value>;

template<typename T, typename... Args>
using construct_function = typename construct_function_helper<has_valid_construct<T, Args...>::value, T, Args...>::type;

template<typename T, typename... Args>
using construct_function_t = typename construct_function<T, Args...>::value_type;

template<typename T, typename... Args>
using construct_result_seq = tuple_seq<function_result_t<construct_function_t<T, Args...>>>;

template<typename, typename, typename = void>
struct is_construct_function_callable_helper : std::false_type {};

template<typename T, typename... Args>
struct is_construct_function_callable_helper<T, meta_list<Args...>, enable_if_t<is_construct_invokable<construct_function_t<T, Args...>, Args...>::value>> : std::true_type {};

template<typename T, typename... Args>
struct is_construct_function_callable_helper<T, meta_list<Args...>, enable_if_t<is_container_service<T>::value || is_abstract_service<T>::value>> : std::true_type {};

template<typename T, typename... Args>
using is_construct_function_callable = is_construct_function_callable_helper<T, meta_list<Args...>>;

template<template<typename...> class Trait, typename T, typename... Args>
struct dependency_trait_helper {
	template<typename U, std::size_t I, typename... As>
	struct expand {
		using type = Trait<injected_argument_t<I, construct_function_t<U, As...>>>;
	};

	template<typename U, typename... As, std::size_t... S, int_t<
		enable_if_t<dependency_trait_helper<Trait, injected_argument_t<S, construct_function_t<U, As...>>>::type::value>...,
		enable_if_t<expand<U, S, As...>::type::value>...> = 0>
	static std::true_type test(seq<S...>);
	
	template<typename U, typename... As, enable_if_t<!has_any_construct<U, As... >::value, int> = 0>
	static std::true_type test_helper(int);
	
	template<typename...>
	static std::false_type test(...);
	
	template<typename...>
	static std::false_type test_helper(...);
	
	template<typename U, typename... As, enable_if_t<has_any_construct<U, As...>::value, int> = 0, int_t<construct_function_t<U, As...>> = 0>
	static decltype(test<U, As...>(tuple_seq_minus<function_arguments_t<construct_function_t<U, As...>>, sizeof...(As)>{})) test_helper(int);
	
public:
	using type = decltype(test_helper<T, Args...>(0));
};

template<template<typename...> class Trait, typename T, typename... Args>
using dependency_trait = typename dependency_trait_helper<Trait, T, Args...>::type;

template<typename T, typename... Args>
struct is_service_constructible_helper {
private:
	template<typename U, typename... As, std::size_t F, std::size_t... S>
	static is_service_instantiable<T,
		tuple_element_t<F, function_result_t<construct_function_t<U, As...>>>,
		tuple_element_t<S, function_result_t<construct_function_t<U, As...>>>...> test(seq<F, S...>);

	// This overload is needed for msvc.
	// Or else it will try to call the one just above with a 0 as S for strange reason.
	template<typename U, typename... As, int_t<construct_function_t<U, As...>> = 0>
	static is_service_instantiable<T> test(seq<>);
	
	template<typename U, typename... As, enable_if_t<!has_any_construct<U, As... >::value, int> = 0>
	static std::true_type test_helper(int);
	
	template<typename...>
	static std::false_type test(...);
	
	template<typename...>
	static std::false_type test_helper(...);
	
	// The enable if is required here or else the function call will be ambiguous on visual studio.
	template<typename U, typename... As, enable_if_t<has_any_construct<U, As... >::value, int> = 0>
	static decltype(test<U, As...>(tuple_seq<function_result_t<construct_function_t<U, As...>>>{})) test_helper(int);
	
public:
	using type = decltype(test_helper<T, Args...>(0));
};

template<typename T, typename... Args>
using is_service_constructible = typename is_service_constructible_helper<T, Args...>::type;

template<typename T>
struct is_override_service_helper {
private:
	template<typename...>
	static std::false_type test(...);
	
	template<typename U, std::size_t... S, int_t<enable_if_t<is_service<meta_list_element_t<S, parent_types<U>>>::value>...> = 0>
	static std::true_type test(seq<S...>);
	
public:
	using type = decltype(test<T>(tuple_seq<parent_types<T>>{}));
};

template<typename T>
using is_override_service = typename is_override_service_helper<T>::type;

template<typename T>
struct is_override_convertible_helper {
private:
	// This is a workaround for msvc. Expansion in very complex expression
	// leaves the compiler without clues about what's going on.
	template<std::size_t I, typename U>
	struct expander {
		using type = is_explicitly_convertible<ServiceType<U>, ServiceType<meta_list_element_t<I, parent_types<U>>>>;
	};

	template<typename...>
	static std::false_type test(...);

	template<typename...>
	static std::false_type test_helper(...);
	
	template<typename U, std::size_t... S, int_t<enable_if_t<expander<S, U>::type::value>...> = 0>
	static std::true_type test_helper(seq<S...>);
	
	template<typename U, std::size_t... S, int_t<enable_if_t<is_service<meta_list_element_t<S, parent_types<U>>>::value>...> = 0>
	static decltype(test_helper<U>(seq<S...>{})) test(seq<S...>);
	
public:
	using type = decltype(test<T>(tuple_seq<parent_types<T>>{}));
};

template<typename T>
using is_override_convertible = typename is_override_convertible_helper<T>::type;

template<typename T>
struct is_override_services_helper {
private:
	// This is a workaround for msvc. Expansion in very complex expression
	// leaves the compiler without clues about what's going on.
	template<std::size_t I, typename U>
	struct expander {
		using type = is_service<meta_list_element_t<I, parent_types<U>>>;
	};
	template<typename...>
	static std::false_type test(...);

	template<typename...>
	static std::false_type test_helper(...);
	
	template<typename U, std::size_t... S, int_t<enable_if_t<expander<S, U>::type::value>...> = 0>
	static std::true_type test_helper(seq<S...>);
	
	template<typename U, std::size_t... S, int_t<enable_if_t<is_service<meta_list_element_t<S, parent_types<U>>>::value>...> = 0>
	static decltype(test_helper<U>(seq<S...>{})) test(seq<S...>);
	
public:
	using type = decltype(test<T>(tuple_seq<parent_types<T>>{}));
};

template<typename T>
using is_override_services = typename is_override_services_helper<T>::type;

template<typename T>
struct is_override_not_final_helper {
private:
	// This is a workaround for msvc. Expansion in very complex expression
	// leaves the compiler without clues about what's going on.
	template<std::size_t I, typename U>
	struct expander {
		using type = is_final_service<meta_list_element_t<I, parent_types<U>>>;
	};
	
	template<typename...>
	static std::false_type test(...);
	
	template<typename U, std::size_t... S, int_t<enable_if_t<!expander<S, U>::type::value>...> = 0>
	static std::true_type test(seq<S...>);
	
public:
	using type = decltype(test<T>(tuple_seq<parent_types<T>>{}));
};

/*
 * Alias for is_override_virtual_helper
 */
template<typename T>
using is_override_not_final = typename is_override_not_final_helper<T>::type;

template<typename T>
using is_abstract_not_final = std::integral_constant<bool,
	!is_abstract_service<T>::value || !is_final_service<T>::value
>;

template<typename T, typename... Args>
using is_single_no_args = std::integral_constant<bool,
	!is_single<T>::value || meta_list_empty<meta_list<Args...>>::value
>;

template<typename T>
struct is_default_overrides_abstract_helper {
private:
	template<typename>
	static std::false_type test(...);
	
	template<typename U, enable_if_t<has_default<U>::value && is_abstract_service<U>::value, int> = 0>
	static is_overriden_by<U, default_type<U>> test(int);
	
	template<typename U, enable_if_t<!has_default<U>::value, int> = 0>
	static std::true_type test(int);
	
public:
	using type = decltype(test<T>(0));
};

template<typename T>
using is_default_overrides_abstract = typename is_default_overrides_abstract_helper<T>::type;

template<typename T>
struct is_default_convertible_helper {
private:
	template<typename>
	static std::false_type test(...);
	
	template<typename U, enable_if_t<has_default<U>::value, int> = 0>
	static is_explicitly_convertible<ServiceType<default_type<U>>, ServiceType<U>> test(int);
	
	template<typename U, enable_if_t<!has_default<U>::value, int> = 0>
	static std::true_type test(int);
	
public:
	using type = decltype(test<T>(0));
};

template<typename T>
using is_default_convertible = typename is_default_convertible_helper<T>::type;

template<typename T>
using is_default_service_valid = std::integral_constant<bool,
	(is_abstract_service<T>::value || !has_default<T>::value) &&
	is_default_overrides_abstract<T>::value &&
	is_default_convertible<T>::value
>;

template<typename T, typename... Args>
using service_check = std::integral_constant<bool, 
	is_service<T>::value &&
	is_service_constructible<T, Args...>::value &&
	is_construct_function_callable<T, Args...>::value &&
	is_default_service_valid<T>::value &&
	is_override_convertible<T>::value &&
	is_override_services<T>::value &&
	is_override_not_final<T>::value &&
	is_abstract_not_final<T>::value
>;

template<typename T, typename... Args>
using dependency_check = std::integral_constant<bool, 
	dependency_trait<is_service, T, Args...>::value &&
	dependency_trait<is_service_constructible, T, Args...>::value &&
	dependency_trait<is_construct_function_callable, T, Args...>::value &&
	dependency_trait<is_default_service_valid, T, Args...>::value &&
	dependency_trait<is_override_convertible, T, Args...>::value &&
	dependency_trait<is_override_services, T, Args...>::value &&
	dependency_trait<is_override_not_final, T, Args...>::value &&
	dependency_trait<is_abstract_not_final, T, Args...>::value
>;

template<template<typename> class Map, typename T, typename P, typename... Args>
struct is_pointer_invokable_helper {
private:
	template<typename U, typename V, typename... As, std::size_t... S, int_t<
		decltype((std::declval<U>().*std::declval<V>())(std::declval<ServiceType<service_map_t<Map, function_argument_t<S, V>>>>()..., std::declval<As>()...))> = 0>
	static std::true_type test(seq<S...>);
	
	template<typename...>
	static std::false_type test(...);
	
public:
	using type = decltype(test<T, P, Args...>(tuple_seq_minus<function_arguments_t<P>, sizeof...(Args)>{}));
};

template<template<typename> class Map, typename T, typename P, typename... Args>
struct is_pointer_invokable : is_pointer_invokable_helper<Map, T, P, Args...>::type {};

template<template<typename> class, typename, typename, typename, typename = void>
struct has_callable_template_call : std::false_type {};

template<template<typename> class Map, typename T, typename... TArgs, typename... Args>
struct has_callable_template_call<
	Map, T, meta_list<TArgs...>, meta_list<Args...>,
	enable_if_t<is_pointer_invokable<Map, T,

#ifdef KGR_KANGARU_MSVC_NO_DEPENDENT_TEMPLATE_KEYWORD
	decltype(exact(&T::operator()<TArgs...>)),
#else
	decltype(exact(&T::template operator()<TArgs...>)),
#endif // KGR_KANGARU_MSVC_NO_DEPENDENT_TEMPLATE_KEYWORD
	
	Args...>::value>
> : std::true_type {};

template<template<typename> class, typename, typename, typename, typename = void>
struct get_template_call_helper;

template<template<typename> class Map, typename T, typename... Args>
struct get_template_call_helper<
	Map, T, meta_list<>, meta_list<Args...>,
	enable_if_t<!has_callable_template_call<Map, T, meta_list<>, meta_list<Args...>>::value>
> {};

template<template<typename> class Map, typename T, typename Head, typename... Tail, typename... Args>
struct get_template_call_helper<
	Map, T, meta_list<Head, Tail...>, meta_list<Args...>,
	enable_if_t<!has_callable_template_call<Map, T, meta_list<Head, Tail...>, meta_list<Args...>>::value>
> : get_template_call_helper<Map, T, meta_list<Tail...>, meta_list<Args...>> {};

template<template<typename> class Map, typename T, typename... TArgs, typename... Args>
struct get_template_call_helper<
	Map, T, meta_list<TArgs...>, meta_list<Args...>,
	enable_if_t<has_callable_template_call<Map, T, meta_list<TArgs...>, meta_list<Args...>>::value>
> {
#ifdef KGR_KANGARU_MSVC_NO_DEPENDENT_TEMPLATE_KEYWORD
	using type = decltype(exact(&T::operator()<TArgs...>));
#else
	using type = decltype(exact(&T::template operator()<TArgs...>));
#endif // KGR_KANGARU_MSVC_NO_DEPENDENT_TEMPLATE_KEYWORD
};

template<template<typename> class Map, typename T, typename... Args>
using get_template_call = get_template_call_helper<Map, T, meta_list<Args...>, meta_list<Args...>>;

template<template<typename> class, typename, typename, typename = void>
struct has_template_call_operator : std::false_type {};

template<template<typename> class Map, typename T, typename... Args>
struct has_template_call_operator<Map, T, meta_list<Args...>, void_t<typename get_template_call<Map, T, Args...>::type>> : std::true_type {};

template<template<typename> class Map, typename, typename, typename = void>
struct invoke_function_helper {};

template<template<typename> class Map, typename T, typename... Args>
struct invoke_function_helper<Map, T, meta_list<Args...>, enable_if_t<has_call_operator<T>::value && !has_template_call_operator<Map, T, meta_list<Args...>>::value>> {
	using type = decltype(&T::operator());
	using return_type = function_result_t<type>;
	using argument_types = function_arguments_t<type>;
	template<std::size_t n> using argument_type = meta_list_element_t<n, argument_types>;
};

template<template<typename> class Map, typename T, typename... Args>
struct invoke_function_helper<Map, T, meta_list<Args...>, void_t<function_result_t<T>, enable_if_t<!has_call_operator<T>::value && !has_template_call_operator<Map, T, meta_list<Args...>>::value>>> {
	using type = T;
	using return_type = function_result_t<type>;
	using argument_types = function_arguments_t<type>;
	template<std::size_t n> using argument_type = meta_list_element_t<n, argument_types>;
};

template<template<typename> class Map, typename T, typename... Args>
struct invoke_function_helper<
	Map, T, meta_list<Args...>,
	enable_if_t<!has_call_operator<T>::value && has_template_call_operator<Map, T, meta_list<Args...>>::value>
> {
	using type = typename get_template_call<Map, T, Args...>::type;
	using return_type = function_result_t<type>;
	using argument_types = function_arguments_t<type>;
	template<std::size_t n> using argument_type = meta_list_element_t<n, argument_types>;
};

template<template<typename> class Map, typename T, typename... Args>
struct invoke_function : invoke_function_helper<Map, T, meta_list<Args...>> {};

template<template<typename> class Map, typename T, typename... Args>
using invoke_function_t = typename invoke_function<Map, T, Args...>::type;

template<template<typename> class Map, typename T, typename... Args>
using invoke_function_arguments_t = typename invoke_function<Map, T, Args...>::argument_types;

template<std::size_t n, template<typename> class Map, typename T, typename... Args>
using invoke_function_argument_t = typename invoke_function<Map, T, Args...>::template argument_type<n>;

template<template<typename> class Map, typename T, typename... Args>
using invoke_function_result_t = typename invoke_function<Map, T, Args...>::return_type;

template<template<typename> class Map, typename T, typename... Args>
struct is_invokable_helper {
private:
	template<std::size_t I, typename U>
	struct expand {
		using type = invoke_function_argument_t<I, Map, U, Args...>;
	};

	// This sub trait is for visual studio
	// The constraint can be simplified because every argument are simple template argument
	template<typename U, typename... As>
	struct call_test {
	private:
		template<typename...>
		static std::false_type test(...);

		template<typename V, typename... A2s, int_t<decltype(std::declval<V>()(std::declval<A2s>()...))> = 0>
		static std::true_type test(int);

		using type = decltype(test<U, As...>(0));
		
	public:
		static constexpr bool value = type::value;
	};
	
	template<typename U>
	using map_t = service_map_t<Map, U>;

	template<typename U, typename... As, std::size_t... S, enable_if_t<call_test<U, ServiceType<map_t<typename expand<S, U>::type>>..., As...>::value, int> = 0>
	static std::true_type test_helper(seq<S...>);
	
	template<typename...>
	static std::false_type test_helper(...);
	
	template<typename U, typename... As>
	static decltype(test_helper<U, As...>(tuple_seq_minus<invoke_function_arguments_t<Map, U, As...>, sizeof...(Args)>{})) test(int);
	
	template<typename...>
	static std::false_type test(...);
	
public:
	using type = decltype(test<T, Args...>(0));
};

template<template<typename> class Map, typename T, typename... Args>
struct is_invokable : is_invokable_helper<Map, T, Args...>::type {};

template<template<typename...> class Trait, typename T>
struct autocall_trait_helper {
private:
	template<typename U, std::size_t I>
	struct expand {
		using type = Trait<U, meta_list_element_t<I, typename U::Autocall>>;
	};

	template<typename>
	static std::false_type test_helper(...);
	
	template<typename U, std::size_t... S, int_t<
		enable_if_t<expand<U, S>::type::value>...> = 0>
	static std::true_type test_helper(seq<S...>);
	
	template<typename>
	static std::true_type test(...);
	
	template<typename U>
	static decltype(test_helper<U>(detail::tuple_seq<typename U::Autocall>{})) test(int);
	
public:
	using type = decltype(test<T>(0));
};

template<template<typename...> class Trait, typename T>
using autocall_trait = typename autocall_trait_helper<Trait, T>::type;

template<typename T, typename F>
struct is_autocall_entry_map_complete_helper {
#ifndef KGR_KANGARU_MSVC_NO_AUTOCALL_MAP_CHECK
private:
	template<typename U, typename C, enable_if_t<is_invoke_call<C>::value, int> = 0>
	static std::true_type test(...);
	
	template<typename U, typename C, enable_if_t<!is_invoke_call<C>::value, int> = 0>
	static std::false_type test(...);
	
	template<typename>
	static std::false_type test_helper(...);
	
	template<template<typename> class Map, typename U, typename C, std::size_t... S, int_t<
		service_map_t<Map, meta_list_element_t<S, function_arguments_t<typename C::value_type>>>...> = 0>
	static std::true_type test_helper(seq<S...>);
	
	template<typename U, typename C>
	static decltype(test_helper<U::template Map, U, C>(tuple_seq<function_arguments_t<typename C::value_type>>{})) test(int);
	
public:
	using type = decltype(test<T, F>(0));
#else 
	using type = std::true_type;
#endif
};

template<typename T, typename F>
using is_autocall_entry_map_complete = typename is_autocall_entry_map_complete_helper<T, F>::type;

template<typename T, typename F>
struct is_autocall_entry_valid_helper {
private:
	// This is workaround for clang. Clang will not interpret the name of the class itself
	// as a valid template template parameter. using this alias, it forces it to use the name as a template.
	template<typename U, typename C>
	using self_t = typename is_autocall_entry_valid_helper<U, C>::type;
	
	struct invoke_method_condition {
		template<typename U, typename C, std::size_t I>
		using type = std::integral_constant<bool,
			service_check<meta_list_element_t<I, autocall_arguments_t<U, C>>>::value &&
			dependency_check<meta_list_element_t<I, autocall_arguments_t<U, C>>>::value>;
	};

	struct invoke_call_condition {
		template<typename U, typename C, std::size_t I>
		using type = is_invoke_call<C>;
	};

	template<typename U, typename C, std::size_t I>
	struct expander {
		using type = typename std::conditional<is_autocall_entry_map_complete<U, C>::value, invoke_method_condition, invoke_call_condition>::type::template type<U, C, I>::type;
	};
	
	template<typename...>
	static std::false_type test(...);
	
	template<typename>
	static std::false_type test_helper(...);
	
	template<typename U, typename C, std::size_t... S, int_t<
		enable_if_t<expander<U, C, S>::type::value>...> = 0>
	static std::true_type test_helper(seq<S...>);
	
	template<typename U, typename C, enable_if_t<is_valid_autocall_function<U, C>::value, int> = 0>
	static decltype(test_helper<U, C>(tuple_seq<function_arguments_t<typename C::value_type>>{})) test(int);
	
public:
	using type = decltype(test<T, F>(0));
};

template<typename T, typename F>
using is_autocall_entry_valid = typename is_autocall_entry_valid_helper<T, F>::type;

template<typename T>
using is_autocall_valid = autocall_trait<is_autocall_entry_valid, T>;

template<typename T, typename... Args>
struct is_service_valid : std::integral_constant<bool,
	is_single_no_args<T, Args...>::value &&
	service_check<T, Args...>::value &&
	dependency_check<T, Args...>::value &&
	is_autocall_valid<T>::value &&
	dependency_trait<is_autocall_valid, T, Args...>::value
> {};

template<template<typename> class Map, typename T, typename... Args>
struct is_invoke_service_valid_helper {
private:
	template<typename U, std::size_t I>
	struct expander {
		using type = std::integral_constant<bool, is_service_valid<service_map_t<Map, invoke_function_argument_t<I, Map, U, Args...>>>::value>;
	};

	template<typename U>
	using map_t = service_map_t<Map, U>;
	
	template<typename U, typename... As, std::size_t... S, int_t<map_t<invoke_function_argument_t<S, Map, U, As...>>..., enable_if_t<expander<U, S>::type::value>...> = 0>
	static std::true_type test_helper(seq<S...>);
	
	template<typename...>
	static std::false_type test_helper(...);
	
	template<typename U, typename... As>
	static decltype(test_helper<U, As...>(tuple_seq_minus<invoke_function_arguments_t<Map, U, As...>, sizeof...(Args)>{})) test(int);
	
	template<typename...>
	static std::false_type test(...);
	
public:
	using type = decltype(test<T, Args...>(0));
};

template<template<typename> class Map, typename T, typename... Args>
struct is_invoke_service_valid : is_invoke_service_valid_helper<Map, T, Args...>::type {};

template<template<typename> class Map, typename T, typename... Args>
using is_invoke_valid = std::integral_constant<bool,
	is_invoke_service_valid<Map, T, Args...>::value &&
	is_invokable<Map, T, Args...>::value
>;

} // namespace detail
} // namespace kgr

#endif // KGR_KANGARU_INCLUDE_KANGARU_DETAIL_SERVICE_TRAITS_HPP
