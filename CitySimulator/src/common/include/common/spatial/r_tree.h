#pragma once

#include <common/stdafx.h>
#include <common/math/bounding_box.h>

namespace tjs::common {
	/**
	 * @brief Minimal R-tree with linear split and logN query complexity.
	 */
	template<typename T, size_t NODE_CAPACITY = 4>
	class RTree {
	public:
		void insert(const BoundingBox& box, const T& value) {
			if (!_root) {
				_root = std::make_unique<Node>();
				_root->leaf = true;
			}
			auto newNode = insert(_root.get(), box, value);
			if (newNode) {
				auto newRoot = std::make_unique<Node>();
				newRoot->leaf = false;
				newRoot->children.push_back(std::move(_root));
				newRoot->children.push_back(std::move(newNode));
				recompute_box(*newRoot);
				_root = std::move(newRoot);
			}
		}

		template<typename OutputIt>
		void query(const BoundingBox& box, OutputIt out) const {
			if (!_root) {
				return;
			}
			query_node(*_root, box, out);
		}

		size_t size() const { return _size; }

	private:
		struct Entry {
			BoundingBox box;
			T value;
		};

		struct Node {
			bool leaf = true;
			BoundingBox box { std::numeric_limits<double>::max(),
				std::numeric_limits<double>::max(),
				std::numeric_limits<double>::lowest(),
				std::numeric_limits<double>::lowest() };
			std::vector<Entry> entries;                  // when leaf
			std::vector<std::unique_ptr<Node>> children; // when internal
		};

		std::unique_ptr<Node> _root;
		size_t _size = 0;

		static void expand(BoundingBox& dst, const BoundingBox& src) {
			if (dst.min_x > dst.max_x) {
				dst = src;
				return;
			}
			dst = combine(dst, src);
		}

		static void recompute_box(Node& node) {
			BoundingBox b { std::numeric_limits<double>::max(),
				std::numeric_limits<double>::max(),
				std::numeric_limits<double>::lowest(),
				std::numeric_limits<double>::lowest() };
			if (node.leaf) {
				for (const auto& e : node.entries) {
					expand(b, e.box);
				}
			} else {
				for (const auto& c : node.children) {
					expand(b, c->box);
				}
			}
			node.box = b;
		}

		static Node* choose_subtree(Node& node, const BoundingBox& box) {
			Node* best = nullptr;
			double bestIncrease = std::numeric_limits<double>::max();
			for (auto& child : node.children) {
				double oldArea = area(child->box);
				BoundingBox expanded = combine(child->box, box);
				double inc = area(expanded) - oldArea;
				if (inc < bestIncrease) {
					bestIncrease = inc;
					best = child.get();
				}
			}
			return best;
		}

		std::unique_ptr<Node> insert(Node* node, const BoundingBox& box, const T& value) {
			if (node->leaf) {
				node->entries.push_back({ box, value });
				expand(node->box, box);
				++_size;
				if (node->entries.size() > NODE_CAPACITY) {
					return split_leaf(*node);
				}
				return nullptr;
			} else {
				Node* child = choose_subtree(*node, box);
				auto newChild = insert(child, box, value);
				expand(node->box, box);
				if (newChild) {
					node->children.push_back(std::move(newChild));
					if (node->children.size() > NODE_CAPACITY) {
						return split_internal(*node);
					}
					recompute_box(*node);
				}
				return nullptr;
			}
		}

		std::unique_ptr<Node> split_leaf(Node& node) {
			auto newNode = std::make_unique<Node>();
			newNode->leaf = true;
			auto& entries = node.entries;
			std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
				return (a.box.min_x + a.box.max_x) < (b.box.min_x + b.box.max_x);
			});
			size_t half = entries.size() / 2;
			newNode->entries.assign(entries.begin() + half, entries.end());
			entries.erase(entries.begin() + half, entries.end());
			recompute_box(node);
			recompute_box(*newNode);
			return newNode;
		}

		std::unique_ptr<Node> split_internal(Node& node) {
			auto newNode = std::make_unique<Node>();
			newNode->leaf = false;
			auto& children = node.children;
			std::sort(children.begin(), children.end(), [](const std::unique_ptr<Node>& a, const std::unique_ptr<Node>& b) {
				return (a->box.min_x + a->box.max_x) < (b->box.min_x + b->box.max_x);
			});
			size_t half = children.size() / 2;
			newNode->children.assign(std::make_move_iterator(children.begin() + half),
				std::make_move_iterator(children.end()));
			children.erase(children.begin() + half, children.end());
			recompute_box(node);
			recompute_box(*newNode);
			return newNode;
		}

		template<typename OutputIt>
		void query_node(const Node& node, const BoundingBox& box, OutputIt out) const {
			if (!intersect(node.box, box)) {
				return;
			}
			if (node.leaf) {
				for (const auto& e : node.entries) {
					if (intersect(e.box, box)) {
						*out++ = e.value;
					}
				}
			} else {
				for (const auto& c : node.children) {
					query_node(*c, box, out);
				}
			}
		}
	};

} // namespace tjs::common
