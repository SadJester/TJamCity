#pragma once


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

}
