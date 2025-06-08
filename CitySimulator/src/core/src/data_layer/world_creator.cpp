#include <core/stdafx.h>

#include <core/data_layer/world_creator.h>
#include <core/data_layer/world_data.h>
#include <core/map_math/contraction_builder.h>

#include <core/random_generator.h>

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
				if (!way->is_car_accessible()) {
					continue;
				}
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

		// TODO: RandomGenerator<Context>
		if (!settings.randomSeed) {
			RandomGenerator::set_seed(settings.seedValue);
		}

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
			auto nodeIt = std::next(allNodes.begin(), RandomGenerator::get().next_int(0, allNodes.size() - 1));
			const Coordinates& coordinates = (*nodeIt)->coordinates;

			// Create a vehicle with random attributes and the selected node's coordinates
			Vehicle vehicle;
			vehicle.uid = RandomGenerator::get().next_int(1, 10000000);
			vehicle.type = RandomGenerator::get().next_enum<VehicleType>();
			vehicle.currentSpeed = RandomGenerator::get().next_float(0.0f, 100.0f);
			vehicle.maxSpeed = RandomGenerator::get().next_float(0.0f, 100.0f);
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

				if (!(std::abs(lat) > 90.0 || std::abs(lon) > 180.0)) {
					world.nodes[id] = Node::create(id, Coordinates { lat, lon }, tags);
				}
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
				int lanes = 1;         // Default value
				int lanesForward = 0;  // Will be calculated after parsing all tags
				int lanesBackward = 0; // Will be calculated after parsing all tags
				bool isOneway = false;
				int maxSpeed = 50; // Default speed in km/h
				WayType type = WayType::None;

				// Default speeds by road type (in km/h)
				static const std::unordered_map<std::string, std::pair<WayType, int>> roadDefaults = {
					// Main road hierarchy
					{ "motorway", { WayType::Motorway, 120 } },
					{ "trunk", { WayType::Trunk, 90 } },
					{ "primary", { WayType::Primary, 80 } },
					{ "secondary", { WayType::Secondary, 60 } },
					{ "tertiary", { WayType::Tertiary, 50 } },
					{ "residential", { WayType::Residential, 30 } },
					{ "service", { WayType::Service, 20 } },
					// Link roads
					{ "motorway_link", { WayType::MotorwayLink, 80 } },
					{ "trunk_link", { WayType::TrunkLink, 60 } },
					{ "primary_link", { WayType::PrimaryLink, 60 } },
					{ "secondary_link", { WayType::SecondaryLink, 50 } },
					{ "tertiary_link", { WayType::TertiaryLink, 40 } },
					// Special roads
					{ "unclassified", { WayType::Unclassified, 40 } },
					{ "living_street", { WayType::Living_Street, 15 } },
					{ "pedestrian", { WayType::Pedestrian, 5 } },
					{ "track", { WayType::Track, 20 } },
					{ "path", { WayType::Path, 5 } },
					// Access roads
					{ "footway", { WayType::Footway, 5 } },
					{ "cycleway", { WayType::Cycleway, 15 } },
					{ "bridleway", { WayType::Bridleway, 10 } },
					// Amenities
					{ "parking", { WayType::Parking, 10 } },
					// Additional road types
					{ "steps", { WayType::Steps, 3 } },
					{ "corridor", { WayType::Corridor, 5 } },
					{ "platform", { WayType::Platform, 5 } },
					{ "construction", { WayType::Construction, 20 } },
					{ "proposed", { WayType::Proposed, 0 } },
					{ "bus_guideway", { WayType::Bus_Guideway, 50 } },
					{ "raceway", { WayType::Raceway, 200 } },
					{ "escape", { WayType::Escape, 30 } },
					{ "emergency_bay", { WayType::Emergency_Bay, 30 } },
					{ "rest_area", { WayType::Rest_Area, 20 } },
					{ "services", { WayType::Services, 20 } },
					// Special access roads
					{ "bus_stop", { WayType::Bus_Stop, 20 } },
					{ "emergency_access", { WayType::Emergency_Access, 50 } },
					{ "delivery_access", { WayType::Delivery_Access, 20 } }
				};

				bool lanes_found = false;

				for (pugi::xml_node tag : xml_way.children("tag")) {
					std::string key = tag.attribute("k").as_string();
					std::string value = tag.attribute("v").as_string();

					if (key == "highway") {
						auto it = roadDefaults.find(value);
						if (it != roadDefaults.end()) {
							type = it->second.first;
							maxSpeed = it->second.second; // Set default speed for this road type

							// Set default characteristics based on road type
							if (value == "motorway" || value == "motorway_link") {
								isOneway = true;                       // Motorways are always one-way
								lanes = (value == "motorway") ? 3 : 1; // Default 3 lanes for motorway, 1 for link
							} else if (value == "trunk" || value == "primary") {
								lanes = 2; // Default 2 lanes for major roads
							} else if (value == "footway" || value == "cycleway" || value == "path" || value == "bridleway" || value == "pedestrian") {
								lanes = 1;
								isOneway = false; // Always bidirectional
								maxSpeed = 5;     // Very low speed for pedestrian paths
							}
							else if (value == "service") {
								if (!lanes_found) {
									lanes = 2;
								}
								isOneway = false;
							}
						}
					} else if (key == "amenity") {
						if (value == "parking") {
							type = WayType::Parking;
							maxSpeed = 10; // Very low speed for parking areas
							lanes = 2;     // Default 2 lanes for parking areas (one each way)
							isOneway = false;
						}
					} else if (key == "lanes") {
						lanes = tag.attribute("v").as_int();
						lanes_found = true;
					} else if (key == "lanes:forward") {
						lanesForward = tag.attribute("v").as_int();
						lanes_found = true;
					} else if (key == "lanes:backward") {
						lanesBackward = tag.attribute("v").as_int();
						lanes_found = true;
					} else if (key == "oneway") {
						isOneway = (value == "yes" || value == "1" || value == "true");
					} else if (key == "maxspeed") {
						maxSpeed = parseSpeedValue(value);
					} else if (key == "junction" && value == "roundabout") {
						isOneway = true; // Roundabouts are always one-way
					} else if (key == "access") {
						// Handle access restrictions
						if (value == "private" || value == "no") {
							return; // Skip private or no-access ways
						}
					}
				}

				// Only add ways that have been classified
				if (type == WayType::None) {
					return;
				}

				// Calculate lanes in each direction if not explicitly specified
				if (lanesForward == 0 && lanesBackward == 0) {
					if (isOneway) {
						lanesForward = lanes;
						lanesBackward = 0;
					} else {
						// For bidirectional roads, split lanes evenly if not specified
						lanesForward = lanes / 2 + (lanes % 2); // Give extra lane to forward direction if odd
						lanesBackward = lanes / 2;
					}
				} else if (lanesForward == 0) {
					lanesForward = lanes - lanesBackward;
				} else if (lanesBackward == 0) {
					lanesBackward = lanes - lanesForward;
				}

				auto way = WayInfo::create(id, lanes, maxSpeed, type);
				way->isOneway = isOneway;
				way->lanesForward = lanesForward;
				way->lanesBackward = lanesBackward;

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
