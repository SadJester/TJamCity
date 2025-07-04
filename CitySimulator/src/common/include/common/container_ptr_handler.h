#pragma once
namespace tjs::common {

	template<typename C>
	concept IndexableContainer =
		requires(C c, std::size_t i) {
			typename C::value_type;
			{ c[i] } -> std::same_as<typename C::value_type&>;
		};

	/**
	 * @brief A pointer-like handle to an element in an indexable container.
	 * 
	 * This utility wraps an index and a pointer to the container, allowing access
	 * to the container element via dereference (`*`) and member access (`->`) operators.
	 * 
	 * It is useful when:
	 * - You need a lightweight, indirect reference to an element without storing a raw pointer.
	 * - The container may be moved or resized, and you want to avoid pointer invalidation.
	 * - You work with index-based references (e.g., in graph structures or simulation models).
	 * 
	 * @tparam _Container An indexable container type (must define `value_type` and support `operator[]`).
	 */
	template<IndexableContainer _Container>
	struct ContainerPtrHolder {
		using ValueType = _Container::value_type;

		ContainerPtrHolder(_Container& c, size_t id_)
			: _container(c)
			, _id(id_) {
		}

		const ValueType& operator*() const { return _container[_id]; }
		const ValueType* operator->() const {
			const ValueType* ptr = &_container[_id];
			return ptr;
		}

		ValueType& operator*() { return _container[_id]; }
		ValueType* operator->() { return &_container[_id]; }

		void set_id(size_t id) { _id = id; }
		size_t get_id() const { return _id; }

	private:
		_Container& _container;
		size_t _id = 0;
	};
} // namespace tjs::common
