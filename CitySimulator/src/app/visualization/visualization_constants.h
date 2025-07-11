#pragma once

#include "render/render_base.h"

namespace tjs::visualization {
	struct Constants {
		static constexpr float LANE_WIDTH = 2.5f;            // meters in world space
		static constexpr double DRAW_LANE_DETAILS_MPP = 3.0; // what meters per pixel should be to draw more detailed info
		static constexpr float DEFAULT_SELECTION_DISTANCE = 10.0f;
		static constexpr float DEBUG_INCOMING_LANE_THICKNESS = 0.3f;
		static constexpr float DEBUG_OUTGOING_LANE_THICKNESS = 0.3f;

		// Color definitions
		static constexpr FColor ROAD_COLOR = { 0.392f, 0.392f, 0.392f, 1.0f };
		static constexpr FColor LANE_MARKER_COLOR = { 1.0f, 1.0f, 0.0f, 1.0f };
		static constexpr FColor PATH_COLOR = { 0.0f, 1.0f, 0.0f, 1.0f };
		static constexpr FColor PATH_MARK_COLOR = { 1.0f, 0.0f, 0.0f, 1.0f };

		static constexpr FColor MOTORWAY_COLOR = { 0.314f, 0.314f, 0.471f, 1.0f };
		static constexpr FColor PRIMARY_COLOR = { 0.353f, 0.353f, 0.353f, 1.0f };
		static constexpr FColor RESIDENTIAL_COLOR = { 0.471f, 0.471f, 0.471f, 1.0f };

		// Additional road type colors
		static constexpr FColor STEPS_COLOR = { 0.6f, 0.6f, 0.6f, 1.0f };
		static constexpr FColor CONSTRUCTION_COLOR = { 1.0f, 0.5f, 0.0f, 1.0f };
		static constexpr FColor RACEWAY_COLOR = { 1.0f, 0.0f, 0.0f, 1.0f };
		static constexpr FColor EMERGENCY_COLOR = { 1.0f, 0.0f, 0.0f, 1.0f };
		static constexpr FColor SERVICE_AREA_COLOR = { 0.5f, 0.5f, 0.5f, 1.0f };
		static constexpr FColor BUS_STOP_COLOR = { 0.0f, 0.0f, 1.0f, 1.0f };

		static constexpr FColor ARROW_COLOR = { 1.0f, 1.0f, 1.0f, 1.0f };
		static constexpr FColor INCOMING_COLOR = { 1.0f, 0.0f, 0.0f, 1.0f };
		static constexpr FColor OUTGOING_COLOR = { 0.0f, 1.0f, 0.0f, 1.0f };
		static constexpr FColor selected_outgoing = { 0.8f, 0.8f, 0.f, 1.f };
		static constexpr FColor selected_incoming = { 0.8f, 0.3f, 0.f, 1.f };
	};
} // namespace tjs::visualization
