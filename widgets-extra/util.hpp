/*
 * util.hpp
 *
 *  Created on: 23 de out de 2016
 *      Author: felipe
 */

#ifndef WIDGETS_EXTRA_UTIL_HPP_
#define WIDGETS_EXTRA_UTIL_HPP_

#define abstract =0
#define null NULL
#define extends :

#define foreach(TYPE, ELEMENT, COLLECTION_TYPE, COLLECTION)\
for(COLLECTION_TYPE::iterator ELEMENT##MACRO_TEMP_IT = (COLLECTION).begin(); ELEMENT##MACRO_TEMP_IT != (COLLECTION).end(); ++ELEMENT##MACRO_TEMP_IT)\
for(bool ELEMENT##MACRO_TEMP_B = true; ELEMENT##MACRO_TEMP_B;)\
for(TYPE ELEMENT = *(ELEMENT##MACRO_TEMP_IT); ELEMENT##MACRO_TEMP_B; ELEMENT##MACRO_TEMP_B = false)

#define const_foreach(TYPE, ELEMENT, COLLECTION_TYPE, COLLECTION)\
for(COLLECTION_TYPE::const_iterator ELEMENT##MACRO_TEMP_IT = (COLLECTION).begin(); ELEMENT##MACRO_TEMP_IT != (COLLECTION).end(); ++ELEMENT##MACRO_TEMP_IT)\
for(bool ELEMENT##MACRO_TEMP_B = true; ELEMENT##MACRO_TEMP_B;)\
for(TYPE ELEMENT = *(ELEMENT##MACRO_TEMP_IT); ELEMENT##MACRO_TEMP_B; ELEMENT##MACRO_TEMP_B = false)

#include <string>
#include <stdexcept>
#include <sstream>

template <typename StringCastableType>
std::string stringfy_by_cast(StringCastableType value)
{
	return (std::string) value;
}

template <typename T>
T parse(const std::string& str)
{
	std::stringstream convertor(str);
	T value;
	convertor >> value;
	if(convertor.fail())
		throw std::invalid_argument(std::string("Failed to convert ").append(str));
	return value;
}

template <typename T>
bool parseable(const std::string& str)
{
	std::stringstream convertor(str);
	T value;
	convertor >> value;
	return not convertor.fail();
}

std::string& replace_all(std::string* const s, const std::string& from, const std::string& to)
{
	std::string& str = *s;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substd::string of 'from'
    }
    return str;
}

std::string replace_all(const std::string& str, const std::string& from, const std::string& to)
{
	std::string copy = std::string(str);
	return replace_all(&copy, from, to);
}

#include <algorithm>

template<typename CollectionType, typename Type>
int index_of(CollectionType& collection, const Type& element)
{
	typename CollectionType::iterator it = std::find(collection.begin(), collection.end(), element);
	if(it == collection.end()) return -1;
	else return it - collection.begin();
}

#include <vector>
template<typename ListenerType, typename CallbackType=void(*)()>
struct ListenerManager
{
	/// A pointer to a std::vector of Listener's pointers. Initialized with NULL, but instantiated if any Listener is added through addListener().
	std::vector<ListenerType*>* listeners;

	/// (Optional) A function pointer to a callback. Initialized with NULL.
	CallbackType callback;

	ListenerManager() : listeners(NULL), callback(NULL) {}

	/// Adds a listener to this manager's list of listeners. Advised to be used if usage of [] operator is wanted to access listeners.
	void addListener(ListenerType* listener)
	{
		if(listeners == NULL)
			listeners = new std::vector<ListenerType*>(); //instantiate on demand
		if(std::find(listeners->begin(), listeners->end(), listener) == listeners->end())
			listeners->push_back(listener);
	}

	/// Removes an added Listener. If none were added or the given listener were not added, nothing is changed.
	void removeListener(ListenerType* listener)
	{
		if(listeners == NULL) return;
		typename std::vector<ListenerType*>::iterator it = std::find(listeners->begin(), listeners->end(), listener);
		if(it != listeners->end()) //found it
			listeners->erase(it);
	}

	/// Returns the listeners count.
	unsigned size()
	{
		if(listeners == NULL) return 0;
		else return listeners->size();
	}

	/// Used to access or iterate over the listeners.
	ListenerType* operator[](unsigned index)
	{
		if(listeners == NULL) return NULL;
		return listeners->at(index);
	}
};

#endif /* WIDGETS_EXTRA_UTIL_HPP_ */
