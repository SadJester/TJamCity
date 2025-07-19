# 🧠 Codex Instructions

Welcome, AI assistant or human collaborator! This project is a modular traffic simulation framework written in C++. The simulation logic is on the server, the client handles only rendering.

Please follow the guidelines and constraints outlined below. Refer to individual files in the `instructions/` folder for detailed breakdowns.

---

## 📁 Instruction Files

| File                          | Purpose |
|-------------------------------|---------|
| `instructions/simulation.md`  | Simulation loop, ticks, movement, vehicle lifecycle |
| `instructions/architecture.md`| Module boundaries, ownership, system layout |
| `instructions/do_not_touch.md`| Legacy or restricted areas in the codebase |

All instructions follow the principles of **modularity**, **stateless logic**, and **explicit data flow**.

---

## 🧱 Architectural Rules

- All simulation logic belongs in `src/core/simulation`.
- Vehicle behavior is driven by `AgentData` and `VehicleState`, not global state.

---

## 🧼 Coding Guidelines

- Prefer `const`, `noexcept`, and `explicit` when possible.
- Use `std::vector` and `std::optional`, avoid raw pointers.
- Unit tests go into `{module_name]/tests/`, using `GTest`.
- Always use braces near if, for, while even if it is one line (if (...){one_line(); })
- Use coding style from .clang-format (or run ./ci/format_changed.sh)
- Classes name is UpperCamelCase
- files, variables, methods and functions is snake_case
- class members variables must begin with _ (ClassName::_my_variable)
- common standard libraries from std put in stdafx.h. In file place only specific headers
- all includes must be with <> and not ""
- Use forward declarations in header files where it is possible
- Split into header and cpp files declaration and implementation if it is not template
- Write #pragma once instead of #ifdef in .h files

---

## 🚫 Do Not Touch
- Avoid introducing `static` mutable state unless absolutely necessary.

See `instructions/do_not_touch.md` for more.

---

## 📚 Reference Projects

The following projects are used as **design and implementation references**:

- [SUMO](https://github.com/eclipse/sumo): For large-scale traffic flow modeling, route assignment.
-- [Microsim](https://github.com/eclipse-sumo/sumo/tree/main/src/microsim) for simulation
-- [netbuild](https://github.com/eclipse-sumo/sumo/tree/main/src/netbuild) for lane creation
- **Don`t use it for now!** [Apollo](https://github.com/ApolloAuto/apollo): Inspiration for modularity and clear system decomposition.

You can cross-reference specific ideas in:
- OSRM's `lane_processing.cpp` for lane merges
- SUMO’s `src/microsim/MSLane.cpp` for car-following
- Carla’s route planner for hierarchy

---

## ✅ Final Note

If unsure where to place code, assume separation of concerns:

- **Simulation logic** → `src/core/simulation`
- **Vehicle & agent data** → `core/agent/`, `core/vehicle/`
- **Data structures (R-trees, etc.)** → `core/spatial/`
- **UI or rendering** → outside core, in `app/`

More details in `instructions/architecture.md`

Always prefer **small, composable modules** over large procedural logic.

---
