#pragma once

#include <span>

namespace tjs {

    struct Position {
        int x = 0;
        int y = 0;
    };

    struct FPoint {
        float x;
        float y;
    };
    static_assert(sizeof(FPoint) == 8, "FPoint should be 8 bytes");
    static_assert(std::is_pod<FPoint>::value, "FPoint should be POD");

    struct FColor {
        float r;
        float g;
        float b;
        float a; // Default alpha value is 1.0 (fully opaque)
    };
    static_assert(sizeof(FColor) == 16, "FColor should be 16 bytes");
    static_assert(std::is_pod<FColor>::value, "FColor should be POD");


    struct Vertex
    {
        FPoint position;        /**< Vertex position, in SDL_Renderer coordinates  */
        FColor color;           /**< Vertex color */
        FPoint tex_coord;       /**< Normalized texture coordinates, if needed */
    };
    static_assert(sizeof(Vertex) == 32, "Vertex should be 32 bytes");
    static_assert(std::is_pod<Vertex>::value, "Vertex should be POD");


    struct Geometry {
        std::span<Vertex> vertices; /**< Vertex data */
        std::span<int> indices;      /**< Index data */
    };


    class IRenderable {
    public:
        explicit IRenderable(std::string_view name)
            : _name(name)
        {}
        IRenderable(const IRenderable&) = delete;
        virtual ~IRenderable() = default;

        std::string_view name() const { 
            return _name;
        }

        virtual void render() = 0;
        virtual void update() = 0;
    private:
        std::string _name;
    };

    class IRenderer {
        public:
            virtual ~IRenderer(){}

            virtual void initialize() = 0;
            virtual void release() = 0;

            virtual void update() = 0;
            virtual void draw() = 0;

            virtual void setDrawColor(FColor color) = 0;
            virtual void drawLine(int x1, int y1, int x2, int y2) = 0;
            virtual void drawGeometry(const Geometry& polygon) = 0;
            virtual void drawCircle(int centerX, int centerY, int radius) = 0;

            int screenWidth() const { return _screenWidth; }
            int screenHeight() const { return _screenHeight; }

        protected:
            void setScreenDimensions(int width, int height) {
                _screenWidth = width;
                _screenHeight = height;
            }

        protected:
            int _screenWidth = 0;
            int _screenHeight = 0;
    };
}