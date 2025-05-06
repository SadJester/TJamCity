#include "core/stdafx.h"

#include "core/dataLayer/WorldCreator.h"
#include "core/dataLayer/WorldData.h"


namespace tjs::core {
    namespace math {
        const double R = 6378137.0; // Radius of the Earth in meters
        const double M_PI = 3.1418;

        // Function to convert longitude and latitude to Web Mercator coordinates
        void lonLatToWebMercator(double longitude, double latitude, double& x, double& y) {
            // Convert degrees to radians
            double lonRad = longitude * M_PI / 180.0;
            double latRad = latitude * M_PI / 180.0;
    
            // Apply the transformation to Web Mercator
            x = R * lonRad;
            y = R * std::log(std::tan(M_PI / 4 + latRad / 2));
        }
    }

    void WorldCreator::loadOSMData(WorldData& data, std::string_view osmFilename) {
        if (osmFilename.ends_with(".osmx")) {
            WorldCreator::loadOSMXmlData(data, osmFilename);
        }
    }


    namespace details {
        class OSMParser {
            public:
                static std::unique_ptr<WorldSegment> parse(std::string_view filename) {
                    auto world = WorldSegment::create();
                    pugi::xml_document doc;
                    
                    if (!doc.load_file(filename.data())) {
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
                    
                    world.nodes[id] = Node::create(id, Coordinates(lat, lon), tags);
                }
            
                static void parseWay(const pugi::xml_node& xml_way, WorldSegment& world) {
                    uint64_t id = xml_way.attribute("id").as_ullong();
                    
                    // Collect node references
                    std::vector<uint64_t> nodeRefs;
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
                            if (value == "motorway") tags = tags | WayTags::Motorway;
                            else if (value == "trunk") tags = tags | WayTags::Trunk;
                            else if (value == "primary") tags = tags | WayTags::Primary;
                            else if (value == "secondary") tags = tags | WayTags::Secondary;
                            else if (value == "tertiary") tags = tags | WayTags::Tertiary;
                            else if (value == "residential") tags = tags | WayTags::Residential;
                            else if (value == "service") tags = tags | WayTags::Service;
                        }
                        else if (key == "lanes") {
                            lanes = tag.attribute("v").as_int();
                        }
                        else if (key == "maxspeed") {
                            maxSpeed = parseSpeedValue(value);
                        }
                    }
                    
                    // Only add ways that have been classified as roads
                    if (tags != WayTags::None) {
                        auto way = WayInfo::create(id, lanes, maxSpeed, tags);
                        way->nodeRefs = std::move(nodeRefs);
                        world.ways[id] = std::move(way);
                    }
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
    }


    void WorldCreator::loadOSMXmlData(WorldData& data, std::string_view osmFilename) {
        auto segment = details::OSMParser::parse(osmFilename);
        data.segments().push_back(std::move(segment));
    }

}
