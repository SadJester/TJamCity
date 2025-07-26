#pragma once

#include <nlohmann/json.hpp>

namespace tjs {
	struct Position {
		int x = 0;
		int y = 0;

		Position() = default;
		Position(int x_, int y_)
			: x(x_)
			, y(y_) {}
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Position,
			x,
			y)
	};

	struct FPoint {
		float x;
		float y;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(FPoint,
			x,
			y)
	};
	static_assert(sizeof(FPoint) == 8, "FPoint should be 8 bytes");
	static_assert(std::is_pod<FPoint>::value, "FPoint should be POD");

	struct FColor {
		float r;
		float g;
		float b;
		float a; // Default alpha value is 1.0 (fully opaque)
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(FColor,
			r,
			g,
			b,
			a);

		static const FColor Red;
		static const FColor Green;
		static const FColor Blue;
		static const FColor Yellow;
		static const FColor Cyan;
		static const FColor Magenta;
		static const FColor White;
		static const FColor Black;
		static const FColor Gray;
	};
	static_assert(sizeof(FColor) == 16, "FColor should be 16 bytes");
	static_assert(std::is_pod<FColor>::value, "FColor should be POD");

	struct Rectangle {
		int x;
		int y;
		int width;
		int height;

		Rectangle()
			: x(0)
			, y(0)
			, width(0)
			, height(0) {}
		Rectangle(int x_, int y_, int width_, int height_)
			: x(x_)
			, y(y_)
			, width(width_)
			, height(height_) {}

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Rectangle,
			x,
			y,
			width,
			height);
	};
	static_assert(sizeof(Rectangle) == 16, "Rectangle should be 16 bytes");

	struct Vertex {
		FPoint position;  /**< Vertex position, in SDL_Renderer coordinates  */
		FColor color;     /**< Vertex color */
		FPoint tex_coord; /**< Normalized texture coordinates, if needed */
	};
	static_assert(sizeof(Vertex) == 32, "Vertex should be 32 bytes");
	static_assert(std::is_pod<Vertex>::value, "Vertex should be POD");

	struct Geometry {
		std::span<Vertex> vertices; /**< Vertex data */
		std::span<int> indices;     /**< Index data */
	};

} // namespace tjs
