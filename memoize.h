#ifndef _MEMOIZE_H_
#define _MEMOIZE_H_

#include "common.h"

namespace dp14 {

template <typename T, typename U, typename... Args>
class memoize_registrator;

template <typename T, typename... Args>
class memoize
{
public:
	using key_cache = size_t;
	using key_impl = size_t;
	using registrator_function = std::function<std::shared_ptr<T>(Args...)>;
	using cache_container = std::unordered_map<key_cache, std::weak_ptr<T>>;
	using registrator_container = std::unordered_map<key_impl, registrator_function>;
	using cache_iterator = typename cache_container::const_iterator;
	template <typename U>
	using registrator = memoize_registrator<T, U, Args...>;

	static typename T::memoize& instance()
	{
		static typename T::memoize memoize;
		return memoize;
	}

	template <typename U,
				class = typename std::enable_if<
					(has_key<U>::value)
				>::type
			>
	key_impl get_key(int=0) const
	{
		return std::hash<std::string>()(U::KEY());
	}

	template <typename U,
				class = typename std::enable_if<
					(!has_key<U>::value)
				>::type
			>
	key_impl get_key(long=0) const
	{
		return std::hash<U>()();
	}

	inline key_cache get_base_hash(const key_impl& keyimpl, Args&&... data) const
	{
		return keyimpl ^ dp14::hash<T>()(std::forward<Args>(data)...);
	}

	template <typename U, typename F>
	void register_type(F&& value)
	{
		_map_registrators[get_key<U>()] = std::forward<F>(value);
	}

	inline bool exists(const std::string& keyimpl_str, Args&&... data) const
	{
		key_impl keyimpl = std::hash<std::string>()(keyimpl_str);
		return exists(keyimpl, std::forward<Args>(data)...);
	}

	template <typename U>
	inline bool exists(Args&&... data) const
	{
		return exists(get_key<U>(), std::forward<Args>(data)...);
	}

	std::shared_ptr<T> get(const std::string& keyimpl_str, Args&&... data) const
	{
		key_impl keyimpl = std::hash<std::string>()(keyimpl_str);
		key_cache key = get_base_hash(keyimpl, std::forward<Args>(data)...);
		return get(keyimpl, key, std::forward<Args>(data)...);
	}

	template <typename U>
	inline std::shared_ptr<U> get(Args&&... data) const
	{
		return std::dynamic_pointer_cast<U>(get(get_key<U>(), std::forward<Args>(data)...));
	}

protected:
	std::shared_ptr<T> get(const key_impl& keyimpl, key_cache key, Args&&... data) const
	{
		cache_iterator it = _exists(keyimpl, std::forward<Args>(data)...);
		if (it != _map_cache.end())
		{
			return it->second.lock();
		}

		auto itc = _map_registrators.find(keyimpl);
		if (itc == _map_registrators.end())
		{
			std::cout << "Can't found key in map: " << key << std::endl;
			throw std::exception();
		}

		std::shared_ptr<T> new_product = (itc->second)(std::forward<Args>(data)...);
		_map_cache[key] = std::weak_ptr<T>(new_product);
		return new_product;
	}

	std::shared_ptr<T> get(const key_impl& keyimpl, Args&&... data) const
	{
		key_cache key = get_base_hash(keyimpl, std::forward<Args>(data)...);
		return get(keyimpl, key, std::forward<Args>(data)...);
	}

	bool exists(const key_impl& keyimpl, Args&&... data) const
	{
		return _exists(keyimpl, std::forward<Args>(data)...) != _map_cache.end();
	}

	cache_iterator _exists(const key_impl& keyimpl, Args&&... data) const
	{
		key_cache key = get_base_hash(keyimpl, std::forward<Args>(data)...);
		cache_iterator it = _map_cache.find(key);
		cache_iterator ite = _map_cache.end();
		if (it != _map_cache.end())
		{
			// pointer cached can be dangled
			if (!it->second.expired())
			{
				return it;
			}
		}
		return ite;
	}

protected:
	registrator_container _map_registrators;
	mutable cache_container _map_cache;
};

template <typename T, typename U, typename... Args>
class memoize_registrator
{
public:
	explicit memoize_registrator() { register_to_singleton(make_int_sequence<sizeof...(Args)>{}); }

	explicit memoize_registrator(memoize<T, Args...>& memoize)
	{
		register_in_a_memoize(memoize, make_int_sequence<sizeof...(Args)>{});
	}

	static std::shared_ptr<T> get(Args&&... data)
	{
		return std::make_shared<U>(std::forward<Args>(data)...);
	}

protected:
	template <int... Is>
	void register_to_singleton(int_sequence<Is...>)
	{
		T::memoize::instance().template register_type<U>(
			std::bind(&memoize_registrator<T, U, Args...>::get, placeholder_template<Is>{}...));
	}

	template <int... Is>
	void register_in_a_memoize(memoize<T, Args...>& memoize, int_sequence<Is...>)
	{
		memoize.template register_type<U>(
			std::bind(&memoize_registrator<T, U, Args...>::get, placeholder_template<Is>{}...));
	}
};

}

#endif
