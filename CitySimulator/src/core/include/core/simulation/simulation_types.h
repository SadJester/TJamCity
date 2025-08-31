#pragma once

namespace tjs::core {

	ENUM_FLAG(TacticalBehaviour, char,
		Normal,
		Aggressive,
		Defensive,
		Emergency);

	ENUM(AgentGoalSelectionType, char,
		RandomSelection,
		GoalNodeId,
		Profile);

	ENUM(MovementAlgoType, char,
		Agent,
		IDM);

} // namespace tjs::core
