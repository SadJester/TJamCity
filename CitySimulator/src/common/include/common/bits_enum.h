#pragma once

namespace tjs::common {

	template<typename _Enum, typename _EnumDivision = uint16_t, typename bit_size = uint16_t>
	class BitMaskValue {
	public:
		static void set_info(bit_size& container, _Enum flag, _EnumDivision division_mask) {
			container = (container & ~static_cast<bit_size>(division_mask)) | (static_cast<bit_size>(flag) & static_cast<bit_size>(division_mask));
		}

		static bit_size get_info(const bit_size& container, _EnumDivision division_mask) {
			return container & static_cast<bit_size>(division_mask);
		}

		static void clear_info(bit_size& container, _EnumDivision division_mask) {
			container &= ~static_cast<bit_size>(division_mask);
		}

		static void remove_info(bit_size& container, _Enum flag, _EnumDivision division_mask) {
			container &= ~(static_cast<bit_size>(flag) & static_cast<bit_size>(division_mask));
		}

		static void overwrite_info(bit_size& container, _Enum flag, _EnumDivision division_mask) {
			container = (container & ~static_cast<bit_size>(division_mask)) | (static_cast<bit_size>(flag) & static_cast<bit_size>(division_mask));
		}

		static bool has_info(const bit_size& container, _Enum flag) {
			return (container & static_cast<bit_size>(flag)) == static_cast<bit_size>(flag);
		}
	};

} // namespace tjs::common
