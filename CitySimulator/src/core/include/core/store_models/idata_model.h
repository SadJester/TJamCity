#pragma once

namespace tjs::core {
	class IStoreEntry {
	public:
		virtual ~IStoreEntry() = default;

		virtual void init() {}
		virtual void release() {}
		virtual void reinit() = 0;
	};

	template<typename T>
	concept HasStaticGetType = requires(T) {
		{ T::get_type() } -> std::same_as<std::type_index>;
	};

	template<typename T>
	concept StoreType = std::is_base_of_v<IStoreEntry, T> && HasStaticGetType<T>;

	template<typename _T>
	class GenericStore {
	public:
		using T = _T;

	public:
		template<typename T>
			requires core::StoreType<T>
		T* get_entry() {
			auto it = _entries.find(T::get_type());
			return it != _entries.end() ? static_cast<T*>(it->second.get()) : nullptr;
		}

		template<typename T, typename... Args>
			requires core::StoreType<T>
		T& create(Args&&... args) {
			static_assert(std::is_constructible_v<T, Args...>,
				"create<T>: T is not constructible from given arguments");

			auto entry_test = get_entry<T>();
			if (entry_test) {
				return *entry_test;
			}

			// perfect-forward all ctor args
			auto entry = std::make_unique<T>(std::forward<Args>(args)...);

			// run post-construction hook immediately if the store is already active
			if (_inited) {
				entry->init();
			}

			// emplace prevents accidental overwrite of an existing entry
			auto [it, inserted] = _entries.emplace(T::get_type(), std::move(entry));
			return static_cast<T&>(*it->second);
		}

		void init() {
			for (auto& entry : _entries) {
				entry.second->init();
			}
			_inited = true;
		}

		void release() {
			for (auto& entry : _entries) {
				entry.second->release();
			}
			_inited = false;
		}

		void reinit() {
			for (auto& entry : _entries) {
				entry.second->reinit();
			}
		}

		const std::unordered_map<std::type_index, std::unique_ptr<T>>& entries() const {
			return _entries;
		}

	private:
		std::unordered_map<std::type_index, std::unique_ptr<T>> _entries;
		bool _inited = false;
	};
} // namespace tjs::core

namespace tjs::core::model {
	class IDataModel : public IStoreEntry {
	public:
		virtual ~IDataModel() = default;
		virtual void reinit() = 0;
	};

	class DataModelStore : public core::GenericStore<IDataModel> {};

} // namespace tjs::core::model
