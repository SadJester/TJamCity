#ifndef TJS_COMMON_SPATIAL_GRID_H
#define TJS_COMMON_SPATIAL_GRID_H

#include <common/stdafx.h>
#include <common/hash_functions.h>
#include <common/spatial/r_tree.h>

namespace tjs::common {

	template<typename CellEntry, typename TreeEntry>
	struct SpatialGrid {
		using GridKey = std::pair<int, int>;
		using EntriesInCell = std::vector<CellEntry*>;

		std::unordered_map<GridKey, EntriesInCell, PairHash> spatialGrid;
		double cellSize = 1.0;

		RTree<TreeEntry*> tree;

		inline GridKey make_key(double x, double y) const {
			return std::make_pair(static_cast<int>(x / cellSize),
				static_cast<int>(y / cellSize));
		}

		inline void add_to_cell(const GridKey& key, CellEntry* entry) {
			auto& cell = spatialGrid[key];
			if (std::ranges::find(cell, entry) == cell.end()) {
				cell.emplace_back(entry);
			}
		}

		template<typename PointLike>
		inline void add_entry(CellEntry* entry, const PointLike& point) {
			add_to_cell(make_key(point.x, point.y), entry);
		}

		inline void add_tree_entry(const BoundingBox& box, TreeEntry* value) {
			tree.insert(box, value);
		}

		inline std::optional<std::reference_wrapper<const EntriesInCell>>
			get_entries_in_cell(GridKey&& key) const {
			auto it = spatialGrid.find(key);
			if (it != spatialGrid.end()) {
				return it->second;
			}
			return std::nullopt;
		}

		inline std::optional<std::reference_wrapper<const EntriesInCell>>
			get_entries_in_cell(double x, double y) const {
			return get_entries_in_cell(make_key(x, y));
		}

		template<typename PointLike>
			requires requires(PointLike p) {
				requires std::is_arithmetic_v<decltype(p.x)>;
				requires std::is_arithmetic_v<decltype(p.y)>;
			}
		inline std::optional<std::reference_wrapper<const EntriesInCell>>
			get_entries_in_cell(const PointLike& point) const {
			return get_entries_in_cell(make_key(point.x, point.y));
		}
	};

} // namespace tjs::common

#endif // TJS_COMMON_SPATIAL_GRID_H
