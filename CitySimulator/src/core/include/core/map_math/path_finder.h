#pragma once

namespace tjs::core {
	struct RoadNetwork;
	struct Node;
	struct Edge_Contract;
} // namespace tjs::core

namespace tjs::core::algo {
	class PathFinder {
	public:
		static std::deque<Node*> find_path(const RoadNetwork& network, uint64_t source, uint64_t target);
		static std::deque<Node*> find_path_a_star(const RoadNetwork& network, Node* source, Node* target);
		static std::unordered_set<Node*> reachable_nodes(const RoadNetwork& network, Node* source);

	private:
		// Вспомогательная функция для проверки возможности перехода через shortcut
		static bool can_traverse_shortcut(
			const RoadNetwork& network,
			uint64_t from, uint64_t to,
			const Edge_Contract& shortcut);

	private:
		// Вспомогательные функции для проверок
		static bool is_geometry_valid(
			const RoadNetwork& network,
			uint64_t from, uint64_t to,
			const Edge_Contract& shortcut) {
			// Проверка на корректность геометрии shortcut-ребра
			// Можно добавить проверку на пересечение с другими объектами
			// Можно добавить проверку на прямолинейность
			return true;
		}

		static bool is_time_valid(const RoadNetwork& network,
			uint64_t from, uint64_t to,
			const Edge_Contract& shortcut) {
			// Проверка временных ограничений (если они есть)
			// Например, проверка на время суток для определенных дорог
			return true;
		}

		static bool is_transport_valid(const RoadNetwork& network,
			uint64_t from, uint64_t to,
			const Edge_Contract& shortcut) {
			// Проверка транспортных ограничений
			// Например, проверка на допустимый тип транспорта
			return true;
		}

		static bool is_obstacle_free(const RoadNetwork& network,
			uint64_t from, uint64_t to,
			const Edge_Contract& shortcut) {
			// Проверка на отсутствие физических препятствий
			// Например, проверка на наличие закрытых дорог
			return true;
		}
	};

} // namespace tjs::core::algo
