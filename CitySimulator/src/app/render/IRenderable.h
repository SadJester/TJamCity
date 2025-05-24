#pragma once

namespace tjs {
	class IRenderer;
} // namespace tjs

namespace tjs::render {
	class IRenderable {
	public:
		IRenderable(std::string_view name)
			: _name(name) {}
		virtual ~IRenderable() = default;

		virtual void render(IRenderer& renderer) = 0;
		virtual void update() = 0;

		std::string_view name() const {
			return _name;
		}

	protected:
		std::string _name;
	};
} // namespace tjs::render
