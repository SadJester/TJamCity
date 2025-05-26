#pragma once


namespace tjs::core
{
    struct Node;
    struct SpatialGrid;
    struct Coordinates;
} // namespace tjs::core



namespace tjs::simulation
{
    core::Node* findRandomGoal(
        const core::SpatialGrid& grid,
        const core::Coordinates& coord,
        double minRadius,
        double maxRadius
    );
} // namespace tjs::simulation

