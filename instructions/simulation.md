# 🔁 Simulation Loop & Vehicle Lifecycle

This document defines the core structure of the simulation tick and how entities behave over time.

---

## World structure

Lanes are stored in Edge::lanes and creates from right to left. So lanes[0] is the most right lane.


## ⏱️ Timestep Flow

In TrafficSimulationSystem makes SimulationSettings::steps_on_update steps on each update. Each step has fixed SimulationSettings::step_delta_sec.

Each simulation tick proceeds as follows:

1. **Strategic planning**  
   - In `StrategicPlanningModule` for each agent selects goal.

2. **Tactical Planning Phase**  
   - `TacticalPlanningModule` checks that vehicle is already already arrived. If not - do nothing. In the future will be route replanning and changing lanes.

3. **Vehicle movement**
- `VehicleMovementModule`: Updates `s_on_lane`, resolves transitions.
