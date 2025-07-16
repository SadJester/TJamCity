# 🏛️ Application Architecture Overview

This document describes the major components of the traffic simulation project, how they interact, and where each responsibility is located. The architecture is designed to support clean separation of concerns, extensibility, and eventual migration of simulation logic to a dedicated server.

---

## 🧩 Top-Level Structure

The application is initialized via:


The `Application` object creates and wires together all major subsystems:

- **UI System** — user interface built with Qt
- **Rendering System** — visualization backend using SDL3.0 and scene graph
- **Logic System** — UI tools for interaction
- **Scene Graph** — visual elements arranged hierarchically
- **Data Models** — rendering/debugging model state
- **World Data** — authoritative simulation and map state
- **Traffic Simulation System** — tick-driven logic (to be moved to server)

---

## 1. 🖥️ UI System (Client-side)

Handles all UI and window-related elements using Qt. There are different widgets that depends on field (render, vehicle analyzing, map info)

## 2. 🎨 Rendering System

Layered rendering using SDL3.0 backend and scene graph pattern. 
> Each element in the world is visualized as a `SceneNode` subclass.
Most valuable info in MapElement.h/cpp - rendering of map.
Other nodes is vehicles and debug path rendering.

## 3. ⚙️ Logic System

UI-driven logic for positioning, selection, and interaction. All classes should be added via `application.logic_modules()`.
They don't coupled with other systems. All interaction through `MessageSystem` and `IDataModel` (get from `application.stores()`)

> These tools allow the user to interact with the map and vehicles.

---

## 4. 📦 Data Layer

### 🔶 Data Model Store

Abstracts view-related data for rendering and debugging.

### 🔷 World Data

Holds the simulation state and spatial structures.
> This is the true source of spatial and connectivity data. `WorldData` is authoritative for simulation.

---

## 5. 🚦 Traffic Simulation System (client-side now, will be Server-bound)

Encapsulates the tick-based simulation logic, vehicle control, and planning.

## 🧼 Design Principles

- **Modularity**: All simulation logic is organized in pluggable modules.
- **Testability**: Each module is deterministic and independently testable.
- **Decoupling**: Clear boundaries between view, logic, and data.
- **Extensibility**: Modules, scene elements, and rendering logic are pluggable.
- **Migration-ready**: Simulation can be decoupled from client and hosted headlessly.
