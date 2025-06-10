#pragma once

namespace tjs {
	class Application;
} // namespace tjs

namespace tjs::core::model {
	class IDataModel {
	public:
		virtual ~IDataModel() = default;
	};

	template<typename T>
	concept HasStaticGetType = requires(T) {
		{ T::get_type() } -> std::same_as<std::type_index>;
	};

	template<typename T>
	concept DataModelType = std::default_initializable<T> && std::is_base_of_v<IDataModel, T> && HasStaticGetType<T>;

	class DataModelStore {
	public:
		template<typename T>
			requires core::model::DataModelType<T>
		T* get_model() {
			auto it = _models.find(T::get_type());
			return it != _models.end() ? static_cast<T*>(it->second.get()) : nullptr;
		}

		template<typename T>
			requires core::model::DataModelType<T>
		void add_model() {
			_models[T::get_type()] = std::make_unique<T>();
		}

	private:
		std::unordered_map<std::type_index, std::unique_ptr<core::model::IDataModel>> _models;
	};

	void setup_models(Application& app);
} // namespace tjs::core::model
