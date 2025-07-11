#pragma once

namespace tjs::common {
	template<typename _T>
	struct WithId {
		WithId() {
			this->_id = WithId<_T>::global_id++;
		}
		int get_id() const {
			return _id;
		}

		static void reset_id() {
			WithId<_T>::global_id = 0;
		}

	private:
		int _id;
		static inline int global_id = 0;
	};
} // namespace tjs::common
