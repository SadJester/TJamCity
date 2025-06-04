#pragma once

#include "render/render_primitives.h"

namespace tjs {
	class IRenderer {
	public:
		virtual ~IRenderer() {}

		virtual void initialize() = 0;
		virtual void release() = 0;

		virtual void update() = 0;
		virtual void beginFrame() = 0;
		virtual void endFrame() = 0;

		virtual void setDrawColor(FColor color) = 0;
		virtual void drawLine(int x1, int y1, int x2, int y2) = 0;
		virtual void drawGeometry(const Geometry& polygon) = 0;
		virtual void drawCircle(int centerX, int centerY, int radius) = 0;
		virtual void drawRect(const Rectangle& rect, bool fill = false) = 0;

		int screenWidth() const { return _screenWidth; }
		int screenHeight() const { return _screenHeight; }

		bool is_point_visible(int x, int y) const {
			return x >= 0 && x < _screenWidth && y >= 0 && y < _screenHeight;
		}

		virtual void setClearColor(FColor color) {
			_clearColor = color;
		}

	protected:
		void setScreenDimensions(int width, int height) {
			_screenWidth = width;
			_screenHeight = height;
		}

	protected:
		int _screenWidth = 0;
		int _screenHeight = 0;

		FColor _clearColor;
	};
} // namespace tjs
