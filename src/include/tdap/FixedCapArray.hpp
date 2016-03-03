/*
 * tdap/FixedCapArray.hpp
 *
 * Part of TdAP
 * Time-domain Audio Processing
 * Copyright (C) 2015 Michel Fleur.
 * Source https://bitbucket.org/emmef/tdap
 * Email  tdap@emmef.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TDAP_FIXEDCAPARRAY_HEADER_GUARD
#define TDAP_FIXEDCAPARRAY_HEADER_GUARD

#include <tdap/ArrayTraits.hpp>
#include <tdap/Power2.hpp>

namespace tdap {

template <typename T, size_t CAPACITY>
class FixedCapArray : public ArrayTraits<T, FixedCapArray<T, CAPACITY>>
{
	static_assert(TriviallyCopyable<T>::value, "Type must be trivial to copy, move or destroy and have standard layout");
	static_assert(CAPACITY > 0 && Power2::constant::next(CAPACITY - 1) >= CAPACITY, "Size must be valid");
	static constexpr size_t MAXSIZE = Power2::constant::next(CAPACITY);
	friend class ArrayTraits<T, FixedCapArray<T, CAPACITY>>;

	size_t size_ = 0;
	T data_[MAXSIZE];

	static size_t validSize(size_t size)
	{
		if (size > 0 && size <= MAXSIZE) {
			return size;
		}
		if (size == 0) {
			throw std::invalid_argument("FixedCapArray: size must be positive");
		}
		throw std::invalid_argument("FixedCapArray: size too big");
	}

	size_t _traitGetSize() const { return size_; }
	size_t _traitGetCapacity() const { return MAXSIZE; }

	T& _traitRefAt(size_t i)
	{
		return data_[i];
	}

	const T& _traitRefAt(size_t i) const
	{
		return data_[i];
	}

	T * _traitUnsafeData()
	{
		return &data_[0];
	}

	const T * _traitUnsafeData() const
	{
		return &data_[0];
	}

	T * _traitPlus(size_t i) const
	{
		return data_ + i;
	}

	static constexpr bool _traitHasTrivialAddressing() { return true; }

public:
	FixedCapArray() = default;
	FixedCapArray(size_t size) : size_(validSize(size)) { }

	size_t maxSize() const { return MAXSIZE ; }

	void resize(size_t newSize)
	{
		size_ = validSize(newSize);
	}
};

} /* End of name space tdap */

#endif /* TDAP_FIXEDCAPARRAY_HEADER_GUARD */
