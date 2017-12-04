#ifndef KGR_KANGARU_INCLUDE_KANGARU_CONTAINER_HPP
#define KGR_KANGARU_INCLUDE_KANGARU_CONTAINER_HPP

#include "detail/traits.hpp"
#include "detail/service_traits.hpp"
#include "detail/utils.hpp"
#include "detail/container_service.hpp"
#include "detail/invoke.hpp"
#include "detail/single.hpp"
#include "detail/injected.hpp"
#include "detail/error.hpp"
#include "predicate.hpp"

#include <unordered_map>
#include <memory>
#include <type_traits>
#include <tuple>
#include <algorithm>
#include <vector>

namespace kgr {

struct Container final {
private:
	template<typename Condition, typename T = int> using enable_if = detail::enable_if_t<Condition::value, T>;
	template<typename Condition, typename T = int> using disable_if = detail::enable_if_t<!Condition::value, T>;
	template<typename T> using instance_ptr = std::unique_ptr<T, void(*)(void*)>;
	using instance_cont = std::vector<instance_ptr<void>>;
	using service_cont = std::unordered_map<type_id_t, void*>;
	template<typename T> using contained_service_t = typename std::conditional<
		detail::is_single<T>::value,
		instance_ptr<detail::SingleInjected<T>>,
		detail::Injected<T>>::type;
	
	template<typename T>
	static void deleter(void* i) {
		delete static_cast<T*>(i);
	}
	
	template<typename T, typename... Args>
	static instance_ptr<detail::SingleInjected<T>> make_instance_ptr(Args&&... args) {
		return instance_ptr<detail::SingleInjected<T>>{
			new detail::SingleInjected<T>{std::forward<Args>(args)...},
			&Container::deleter<detail::BaseInjected<T>>
		};
	}
	
	template<typename T, typename C, typename... Args>
	static instance_ptr<detail::BaseInjected<C>> make_override_ptr(Args&&... args) {
		return instance_ptr<detail::ServiceOverride<T, C>>{
			new detail::ServiceOverride<T, C>{std::forward<Args>(args)...},
			&Container::deleter<detail::BaseInjected<C>>
		};
	}
	
public:
	explicit Container() = default;
	Container(const Container &) = delete;
	Container& operator=(const Container &) = delete;
	Container(Container&&) = default;
	Container& operator=(Container&&) = default;
	~Container() = default;
	
	/*
	 * This function construct a service definition with the provided arguments.
	 * It also saves it in the container.
	 * It returns void.
	 * This function only works with single services.
	 * This version of the function will call functions in autocall.
	 */
	template<typename T, typename... Args,
		enable_if<detail::is_single<T>> = 0,
		enable_if<detail::is_service<T>> = 0,
		enable_if<detail::is_service_instantiable<T, Args...>> = 0>
	void instance(Args&&... args) {
		autocall(save_instance<T>(make_contained_service<T>(std::forward<Args>(args)...)).get());
	}
	
	/*
	 * This function construct a service definition with the provided arguments.
	 * It also saves it in the container.
	 * It returns void.
	 * This function only works with single services.
	 * This version of the function will not call functions in autocall.
	 */
	template<typename T, typename... Args,
		enable_if<detail::is_single<T>> = 0,
		enable_if<detail::is_service<T>> = 0,
		enable_if<detail::is_service_instantiable<T, Args...>> = 0>
	void instance(detail::no_autocall_t, Args&&... args) {
		save_instance<T>(make_contained_service<T>(std::forward<Args>(args)...));
	}
	
	/*
	 * This function returns the service given by serivce definition T.
	 * T must be a valid service and must be constructible with arguments passed as parameters
	 * In case of a non-single service, it takes additional arguments to be sent to the T::construct function.
	 * T must not be a polymorphic type.
	 */
	template<typename T, typename... Args, enable_if<detail::is_service_valid<T, Args...>> = 0>
	ServiceType<T> service(Args&&... args) {
		return definition<T>(std::forward<Args>(args)...).forward();
	}
	
	/*
	 * The following two overloads are called in a case where the service is invalid,
	 * or is called when provided arguments don't match the constructor.
	 * In GCC, a diagnostic is provided.
	 */
	template<typename T, enable_if<std::is_default_constructible<detail::ServiceError<T>>> = 0>
	detail::Sink service(detail::ServiceError<T> = {}) = delete;
	
	template<typename T, typename... Args>
	detail::Sink service(detail::ServiceError<T, detail::identity_t<Args>...>, Args&&...) = delete;
	
	/*
	 * This function returns the result of the callable object of type U.
	 * Args are additional arguments to be sent to the function after services arguments.
	 * This function will deduce arguments from the function signature.
	 */
	template<template<typename> class Map = AdlMap, typename U, typename... Args, enable_if<detail::is_invoke_valid<Map, detail::decay_t<U>, Args...>> = 0>
	detail::invoke_function_result_t<Map, detail::decay_t<U>, Args...> invoke(U&& function, Args&&... args) {
		return invoke_helper<Map>(
			detail::tuple_seq_minus<detail::invoke_function_arguments_t<Map, detail::decay_t<U>, Args...>, sizeof...(Args)>{},
			std::forward<U>(function),
			std::forward<Args>(args)...
		);
	}
	
	/*
	 * This function returns the result of the callable object of type U.
	 * This version of the function is called when `function` is not invokable to provide diagnostic.
	 */
	template<template<typename> class Map>
	detail::Sink invoke(detail::NotInvokableError = {}, ...) = delete;
	
	/*
	 * This function returns the result of the callable object of type U.
	 * It will call the function with the sevices listed in the `Services` parameter pack.
	 * It will call it in a equivalent expression of `std::declval<U>()(std::declval<ServiceType<Services>>()..., std::declval<Args>()...)`
	 */
	template<typename... Services, typename U, typename... Args, detail::int_t<
		enable_if<detail::is_service_valid<Services>>...,
		detail::enable_if_t<(sizeof...(Services) > 0)>> = 0>
	auto invoke(U&& function, Args&&... args) -> decltype(std::declval<U>()(std::declval<ServiceType<Services>>()..., std::declval<Args>()...)) {
		return std::forward<U>(function)(service<Services>()..., std::forward<Args>(args)...);
	}
	
	/*
	 * This oveload is called when one of the services to be injected is invalid.
	 * It will provide diagnostic on GCC.
	 */
	template<typename... Services>
	detail::Sink invoke(detail::NotInvokableError = {}, ...) = delete;
	
	/*
	 * This function clears this container.
	 * Every single services are invalidated after calling this function.
	 */
	inline void clear() {
		_instances.clear();
		_services.clear();
	}
	
	/*
	 * This function fork the container into a new container.
	 * The new container will have the copied state of the first container.
	 * Construction of new services within the new container will not affect the original one.
	 * The new container must exist within the lifetime of the original container.
	 * 
	 * It takes a predicate type as template argument.
	 * The default predicate is kgr::All.
	 * 
	 * This version of the function takes a predicate that is default constructible.
	 * It will call fork() with a predicate as parameter.
	 */
	template<typename Predicate = All, enable_if<std::is_default_constructible<Predicate>> = 0>
	Container fork() const {
		return fork(Predicate{});
	}
	
	/*
	 * This function fork the container into a new container.
	 * The new container will have the copied state of the first container.
	 * Construction of new services within the new container will not affect the original one.
	 * The new container must exist within the lifetime of the original container.
	 * 
	 * It takes a predicate as argument.
	 */
	template<typename Predicate>
	Container fork(Predicate&& predicate) const {
		Container c;
		
		c._services.reserve(_services.size());
		
		std::copy_if(
			_services.begin(), _services.end(),
			std::inserter(c._services, c._services.begin()),
			[&predicate](service_cont::const_reference i){
				// We don't forward the predicate here, we use it many times.
				return predicate(i.first);
			}
		);
		
		return c;
	}
	
	/*
	 * This function merges a container with another.
	 * The receiving container will prefer it's own instances in a case of conflicts.
	 */
	inline void merge(Container&& other) {
		_instances.insert(_instances.end(), std::make_move_iterator(other._instances.begin()), std::make_move_iterator(other._instances.end()));
		_services.insert(other._services.begin(), other._services.end());
	}
	
	/*
	 * This function return true if the container contains the service T.
	 * T nust be a single service.
	 */
	template<typename T, enable_if<detail::is_service<T>> = 0, enable_if<detail::is_single<T>> = 0>
	bool contains() const {
		return _services.find(type_id<T>) != _services.end();
	}
	
private:
	///////////////////////
	//   save instance   //
	///////////////////////
	
	/*
	 * This function will create a new instance and save it.
	 * It also returns a reference to the constructed service.
	 */
	template<typename T, typename... Args, enable_if<detail::is_single<T>> = 0, disable_if<detail::is_abstract_service<T>> = 0>
	detail::BaseInjected<T>& save_new_instance(Args&&... args) {
		auto& service = save_instance<T>(make_service_instance<T>(std::forward<Args>(args)...));
		
		autocall(service.get());
		
		return service;
	}
	
	/*
	 * This function is a specialization of save_new_instance for abstract classes.
	 * Since you cannot construct an abstract class, this function always throw.
	 */
	template<typename T, typename... Args,
		enable_if<detail::is_single<T>> = 0,
		enable_if<detail::is_abstract_service<T>> = 0,
		disable_if<detail::has_default<T>> = 0>
	detail::BaseInjected<T>& save_new_instance(Args&&...) {
		throw std::out_of_range{"No instance found for the requested abstract service"};
	}
	
	/*
	 * This function is a specialization of save_new_instance for abstract classes.
	 * Since that abstract service has a default service specified, we can contruct that one.
	 */
	template<typename T, typename... Args,
		enable_if<detail::is_single<T>> = 0,
		enable_if<detail::is_abstract_service<T>> = 0,
		enable_if<detail::has_default<T>> = 0>
	detail::BaseInjected<T>& save_new_instance(Args&&...) {
		save_new_instance<detail::default_type<T>>();
		
		// The static assert is still required here, if other checks fails and allow
		// a call to this function where the default service don't overrides T, it would be UB.
		static_assert(
			detail::is_overriden_by<T, detail::default_type<T>>::value,
			"The default service type of an abstract service must override that abstract serivce."
		);
		
		// This could be faster if we had access to instance of override services.
		return *static_cast<detail::BaseInjected<T>*>(_services[type_id<T>]);
	}
	
	/*
	 * This function will save the instance sent as arguments.
	 * It receive a service that overrides.
	 * It will save the instance and will save overrides if any.
	 */
	template<typename T>
	detail::SingleInjected<T>& save_instance(contained_service_t<T> service) {
		return save_instance<T>(detail::tuple_seq<detail::parent_types<T>>{}, std::move(service));
	}
	
	/*
	 * This function saves the instance and it's overrides if any.
	 * It starts the iteration that save each overrides and the service itself.
	 */
	template<typename T, std::size_t... S>
	detail::SingleInjected<T>& save_instance(detail::seq<S...>, contained_service_t<T> service) {
		return save_instance_helper<T, detail::meta_list_element_t<S, detail::parent_types<T>>...>(std::move(service));
	}
	
	/*
	 * This function save the instance into the container.
	 * It also returns a reference to the instance inside the container.
	 */
	template<typename T>
	detail::SingleInjected<T>& save_instance_helper(contained_service_t<T> service) {
		auto& serviceRef = *service;
		instance_ptr<detail::BaseInjected<T>> injectedTypeService = std::move(service);
		
		_services[type_id<T>] = injectedTypeService.get();
		_instances.emplace_back(std::move(injectedTypeService));
		
		return serviceRef;
	}
	
	/*
	 * This function is the iteration that saves overrides to the container.
	 * It will call itself until there is no override left.
	 * It will class save_instance_helper with one template argument as it's last iteration.
	 */
	template<typename T, typename Override, typename... Others>
	detail::SingleInjected<T>& save_instance_helper(contained_service_t<T> service) {
		auto overrideService = make_override_ptr<T, Override>(service->get());
		
		static_assert(
			std::is_same<instance_ptr<detail::BaseInjected<Override>>, decltype(overrideService)>::value,
			"the override service must be the type instance_ptr<detail::BaseInjected<Override>>"
		);
		
		static_assert(!detail::is_final_service<Override>::value,
			"A final service cannot be overriden"
		);
		
		_services[type_id<Override>] = overrideService.get();
		_instances.emplace_back(std::move(overrideService));
		
		return save_instance_helper<T, Others...>(std::move(service));
	}
	
	///////////////////////
	//      service      //
	///////////////////////
	
	/*
	 * This function call service using the service map.
	 * This function is called when the service map `Map` is valid for a given `T`
	 */
	template<template<typename> class Map, typename T, enable_if<detail::is_complete_map<Map, T>> = 0>
	auto mapped_service() -> decltype(service<detail::service_map_t<Map, T>>()) {
		return service<detail::service_map_t<Map, T>>();
	}
	
	///////////////////////
	//    definition     //
	///////////////////////
	
	/*
	 * This function returns a service definition.
	 * This version of this function create the service each time it is called.
	 */
	template<typename T, typename... Args, disable_if<detail::is_single<T>> = 0, disable_if<detail::is_container_service<T>> = 0>
	detail::Injected<T> definition(Args&&... args) {
		auto service = make_service_instance<T>(std::forward<Args>(args)...);
		
		autocall(service.get());
		
		return service;
	}
	
	/*
	 * This function returns a service definition.
	 * This version of this function is specific to a container service.
	 */
	template<typename T, enable_if<detail::is_container_service<T>> = 0>
	detail::Injected<T> definition() {
		return detail::Injected<T>{T{*this}};
	}
	
	/*
	 * This function returns a service definition.
	 * This version of this function create the service if it was not created before.
	 */
	template<typename T, enable_if<detail::is_single<T>> = 0, disable_if<detail::is_container_service<T>> = 0>
	detail::BaseInjected<T>& definition() {
		if (auto service = _services[type_id<T>]) {
			return *static_cast<detail::BaseInjected<T>*>(service);
		} else {
			return save_new_instance<T>();
		}
	}
	
	///////////////////////
	//   make instance   //
	///////////////////////
	
	/*
	 * This function creates an instance of a service.
	 * It forward the work to make_service_instance_helper with an integer sequence.
	 */
	template<typename T, typename... Args>
	contained_service_t<T> make_service_instance(Args&&... args) {
		return make_service_instance_helper<T>(detail::construct_result_seq<T, Args...>{}, std::forward<Args>(args)...);
	}
	
	/*
	 * This function is the helper for make_service_instance.
	 * It construct the service using the values returned by construct.
	 * It forward it's work to make_contained_service.
	 */
	template<typename T, typename... Args, std::size_t... S>
	contained_service_t<T> make_service_instance_helper(detail::seq<S...>, Args&&... args) {
		auto constructArgs = invoke_raw(detail::construct_function<T, Args...>::value, std::forward<Args>(args)...);
		
		// This line is used to shut unused-variable warning, since S can be empty.
		static_cast<void>(constructArgs);
		
		return make_contained_service<T>(std::forward<detail::tuple_element_t<S, decltype(constructArgs)>>(std::get<S>(constructArgs))...);
	}
	
	/*
	 * This function create a service with the received arguments.
	 * It creating it in the right type for the container to contain it in it's container.
	 */
	template<typename T, typename... Args,
		enable_if<detail::is_single<T>> = 0,
		enable_if<detail::is_someway_constructible<T, in_place_t, Args...>> = 0>
	contained_service_t<T> make_contained_service(Args&&... args) {
		return make_instance_ptr<T>(in_place, std::forward<Args>(args)...);
	}
	
	/*
	 * This function create a service with the received arguments.
	 * It creating it in the right type for the container return it and inject it without overhead.
	 */
	template<typename T, typename... Args,
		disable_if<detail::is_single<T>> = 0,
		enable_if<detail::is_someway_constructible<T, in_place_t, Args...>> = 0>
	contained_service_t<T> make_contained_service(Args&&... args) {
		return detail::Injected<T>{in_place, std::forward<Args>(args)...};
	}
	
	/*
	 * This function create a service with the received arguments.
	 * It creating it in the right type for the container to contain it in it's container.
	 * This version of the function is called when the service definition has no valid constructor.
	 * It will try to call an emplace function that construct the service in a lazy way.
	 */
	template<typename T, typename... Args,
		enable_if<detail::is_single<T>> = 0,
		disable_if<detail::is_someway_constructible<T, in_place_t, Args...>> = 0,
		enable_if<detail::is_emplaceable<T, Args...>> = 0>
	contained_service_t<T> make_contained_service(Args&&... args) {
		auto service = make_instance_ptr<T>();
		
		service->get().emplace(std::forward<Args>(args)...);
		
		return service;
	}
	
	/*
	 * This function create a service with the received arguments.
	 * It creating it in the right type for the container return it and inject it without overhead.
	 * This version of the function is called when the service definition has no valid constructor.
	 * It will try to call an emplace function that construct the service in a lazy way.
	 */
	template<typename T, typename... Args,
		disable_if<detail::is_single<T>> = 0,
		disable_if<detail::is_someway_constructible<T, in_place_t, Args...>> = 0,
		enable_if<detail::is_emplaceable<T, Args...>> = 0>
	contained_service_t<T> make_contained_service(Args&&... args) {
		detail::Injected<T> service;
		
		service.get().emplace(std::forward<Args>(args)...);
		
		return service;
	}
	
	///////////////////////
	//      invoke       //
	///////////////////////
	
	/*
	 * This function is an helper for the public invoke function.
	 * It unpacks arguments of the function with an integer sequence.
	 */
	template<template<typename> class Map, typename U, typename... Args, std::size_t... S>
	detail::invoke_function_result_t<Map, detail::decay_t<U>, Args...> invoke_helper(detail::seq<S...>, U&& function, Args&&... args) {
		return std::forward<U>(function)(
			mapped_service<Map, detail::invoke_function_argument_t<S, Map, detail::decay_t<U>, Args...>>()...,
			std::forward<Args>(args)...
		);
	}
	
	/*
	 * This function is the same as invoke but it sends service definitions instead of the service itself.
	 * It is called with some autocall function and the make_service_instance function.
	 */
	template<typename U, typename... Args>
	detail::function_result_t<detail::decay_t<U>> invoke_raw(U&& function, Args&&... args) {
		return invoke_raw_helper(
			detail::tuple_seq_minus<detail::function_arguments_t<detail::decay_t<U>>, sizeof...(Args)>{},
			std::forward<U>(function),
			std::forward<Args>(args)...
		);
	}
	
	/*
	 * This function is an helper of the invoke_raw function.
	 * It unpacks arguments of the function U with an integer sequence.
	 */
	template<typename U, typename... Args, std::size_t... S>
	detail::function_result_t<detail::decay_t<U>> invoke_raw_helper(detail::seq<S...>, U&& function, Args&&... args) {
		return std::forward<U>(function)(definition<detail::injected_argument_t<S, detail::decay_t<U>>>()..., std::forward<Args>(args)...);
	}
	
	///////////////////////
	//     autocall      //
	///////////////////////
	
	/*
	 * This function starts the iteration (autocall_helper).
	 */
	template<typename T, enable_if<detail::has_autocall<detail::decay_t<T>>> = 0>
	void autocall(T&& service) {
		autocall(detail::tuple_seq<typename detail::decay_t<T>::Autocall>{}, std::forward<T>(service));
	}
	
	/*
	 * This function is the iteration for autocall.
	 */
	template<typename T, std::size_t... S, enable_if<detail::has_autocall<detail::decay_t<T>>> = 0>
	void autocall(detail::seq<S...>, T&& service) {
		using U = detail::decay_t<T>;
		using unpack = int[];
		
		(void)unpack{(invoke_autocall(
			detail::function_seq<detail::autocall_nth_function_t<U, S>>{}, std::forward<T>(service), detail::autocall_nth_function<U, S>::value
		), 0)..., 0};
	}
	
	/*
	 * This function is the invoke_autocall that take the method to invoke as parameter.
	 * It invokes the function sent as parameter.
	 */
	template<typename T, typename F, std::size_t... S>
	void invoke_autocall(detail::seq<S...>, T&& service, F&& function) {
		invoke_raw([&service, &function](detail::function_argument_t<S, detail::decay_t<F>>... args) {
			(std::forward<T>(service).*std::forward<F>(function))(std::forward<decltype(args)>(args)...);
		});
	}
	
	/*
	 * This function is called when there is no autocall to do.
	 */
	template<typename T, disable_if<detail::has_autocall<detail::decay_t<T>>> = 0>
	void autocall(T&&) {}
	
	///////////////////////
	//     instances     //
	///////////////////////
	
	instance_cont _instances;
	service_cont _services;
};

}  // namespace kgr

#endif // KGR_KANGARU_INCLUDE_KANGARU_CONTAINER_HPP
