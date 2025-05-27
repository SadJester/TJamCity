#include <core/stdafx.h>

#include <core/data_layer/world_creator.h>
#include <core/data_layer/world_data.h>
#include <core/map_math/contraction_builder.h>

namespace tjs::core {
	bool WorldCreator::loadOSMData(WorldData& data, std::string_view osmFilename) {
		bool result = false;
		if (osmFilename.ends_with(".osmx")) {
			result = WorldCreator::loadOSMXmlData(data, osmFilename);
		}

		// Prepare data
		for (auto& segment : data.segments()) {
			segment->rebuild_grid();

			auto& road_network = segment->road_network;
			for (auto& [uid, way] : segment->ways) {
				road_network->ways.emplace(uid, way.get());
				for (auto node : way->nodes) {
					road_network->nodes.emplace(node->uid, node);
				}
			}

			algo::ContractionBuilder builder;
			builder.build_contraction_hierarchy(*segment->road_network);
			builder.build_graph(*segment->road_network);
		}

		return result;
	}

	bool WorldCreator::createRandomVehicles(WorldData& data, const SimulationSettings& settings) {
		auto& vehicles = data.vehicles();
		vehicles.clear();

		vehicles.reserve(settings.vehiclesCount);

		// Get all nodes from the road network
		auto& segment = data.segments()[0];

		std::vector<core::Node*> allNodes;
		allNodes.reserve(segment->nodes.size());

		auto& ways = segment->ways;
		// TODO: create with view
		// allNodes = std::views::join(ways | std::ranges::view::transform([](const core::WayInfo& way) {
		//    return way.nodes;
		//})) | std::ranges::to<std::vector>();

		for (auto& way : ways) {
			for (auto node : way.second->nodes) {
				allNodes.push_back(node);
			}
		}

		// Random number generator
		std::random_device rd;
		std::mt19937 gen(rd());

		if (!settings.randomSeed) {
			gen.seed(settings.seedValue);
		}

		std::uniform_int_distribution<> uidDist(1, 10000000);     // Example range for UID
		std::uniform_real_distribution<> speedDist(0.0f, 100.0f); // Example range for speed
		std::uniform_int_distribution<> typeDist(static_cast<int>(VehicleType::SimpleCar), static_cast<int>(VehicleType::FireTrack));

		auto find_way = [&](Node* node) -> core::WayInfo* {
			auto it = std::ranges::find_if(ways, [&](const auto& way) {
				return std::ranges::find(way.second->nodes, node) != way.second->nodes.end();
			});
			if (it != ways.end()) {
				return it->second.get();
			}
			return nullptr;
		};

		// Generate vehicles
		for (size_t i = 0; i < settings.vehiclesCount; ++i) {
			// Randomly select a node for the vehicle's coordinates
			auto nodeIt = std::next(allNodes.begin(), std::uniform_int_distribution<>(0, allNodes.size() - 1)(gen));
			const Coordinates& coordinates = (*nodeIt)->coordinates;

			// Create a vehicle with random attributes and the selected node's coordinates
			Vehicle vehicle;
			vehicle.uid = uidDist(gen);
			vehicle.type = static_cast<VehicleType>(typeDist(gen));
			vehicle.currentSpeed = speedDist(gen);
			vehicle.maxSpeed = speedDist(gen);
			vehicle.coordinates = coordinates;
			vehicle.currentSegmentIndex = 0;
			vehicle.currentWay = find_way(*nodeIt);

			vehicles.push_back(vehicle);
		}

		return true;
	}

	namespace details {
		class OSMParser {
		public:
			static std::unique_ptr<WorldSegment> parse(std::string_view filename) {
				auto world = WorldSegment::create();
				pugi::xml_document doc;

				auto result = doc.load_file(filename.data());
				if (!result) {
					std::cerr << "Failed to load OSM file: " << filename << ": " << result.description() << std::endl;
					return nullptr;
				}

				// First pass: parse all nodes
				for (pugi::xml_node xml_node : doc.child("osm").children("node")) {
					parseNode(xml_node, *world);
				}

				// Second pass: parse all ways
				for (pugi::xml_node xml_way : doc.child("osm").children("way")) {
					parseWay(xml_way, *world);
				}

				return world;
			}

		private:
			static void parseNode(const pugi::xml_node& xml_node, WorldSegment& world) {
				uint64_t id = xml_node.attribute("id").as_ullong();
				double lat = xml_node.attribute("lat").as_double();
				double lon = xml_node.attribute("lon").as_double();

				NodeTags tags = NodeTags::None;

				// Parse node tags
				for (pugi::xml_node tag : xml_node.children("tag")) {
					std::string key = tag.attribute("k").as_string();
					if (key == "highway" && std::string(tag.attribute("v").as_string()) == "traffic_signals") {
						tags = tags | NodeTags::TrafficLight;
					}
					// Add other node tag checks as needed
				}

				world.nodes[id] = Node::create(id, Coordinates { lat, lon }, tags);
			}

			static void parseWay(const pugi::xml_node& xml_way, WorldSegment& world) {
				uint64_t id = xml_way.attribute("id").as_ullong();

				// Collect node references
				std::vector<uint64_t> nodeRefs;
				nodeRefs.resize(10);
				for (pugi::xml_node nd : xml_way.children("nd")) {
					nodeRefs.push_back(nd.attribute("ref").as_ullong());
				}

				// Skip ways with less than 2 nodes
				if (nodeRefs.size() < 2) {
					return;
				}

				// Parse way properties
				int lanes = 1;
				int maxSpeed = 50; // Default speed in km/h
				WayTags tags = WayTags::None;

				for (pugi::xml_node tag : xml_way.children("tag")) {
					std::string key = tag.attribute("k").as_string();
					std::string value = tag.attribute("v").as_string();

					if (key == "highway") {
						if (value == "motorway") {
							tags = tags | WayTags::Motorway;
						} else if (value == "trunk") {
							tags = tags | WayTags::Trunk;
						} else if (value == "primary") {
							tags = tags | WayTags::Primary;
						} else if (value == "secondary") {
							tags = tags | WayTags::Secondary;
						} else if (value == "tertiary") {
							tags = tags | WayTags::Tertiary;
						} else if (value == "residential") {
							tags = tags | WayTags::Residential;
						} else if (value == "service") {
							tags = tags | WayTags::Service;
						}
					} else if (key == "lanes") {
						lanes = tag.attribute("v").as_int();
					} else if (key == "maxspeed") {
						maxSpeed = parseSpeedValue(value);
					}
				}

				// Only add ways that have been classified as roads
				if (tags == WayTags::None) {
					return;
				}

				auto way = WayInfo::create(id, lanes, maxSpeed, tags);

				std::vector<Node*> nodes;
				nodes.reserve(nodeRefs.size());
				for (uint64_t nodeRef : nodeRefs) {
					auto node = world.nodes.find(nodeRef);
					if (node == world.nodes.end()) {
						continue;
					}
					node->second->tags = node->second->tags | NodeTags::Way;
					node->second->ways.emplace_back(way.get());
					nodes.push_back(node->second.get());
				}
				way->nodeRefs = std::move(nodeRefs);
				way->nodes = std::move(nodes);
				world.ways[id] = std::move(way);
			}

			static int parseSpeedValue(const std::string& speedStr) {
				try {
					// Handle numeric values (e.g., "50", "30 mph")
					size_t pos;
					int speed = std::stoi(speedStr, &pos);

					// Check for mph suffix
					if (pos < speedStr.length() && speedStr.find("mph") != std::string::npos) {
						speed = static_cast<int>(speed * 1.60934); // Convert mph to km/h
					}

					return speed;
				} catch (...) {
					return 50; // Default speed if parsing fails
				}
			}
		};
	} // namespace details

	bool WorldCreator::loadOSMXmlData(WorldData& data, std::string_view osmFilename) {
		auto segment = details::OSMParser::parse(osmFilename);
		if (!segment) {
			return false;
		}
		data.segments().clear();
		data.segments().push_back(std::move(segment));
		return true;
	}

} // namespace tjs::core
