#pragma once

namespace tjs::core {

	ENUM_FLAG(TacticalBehaviour, char,
		Normal,
		Aggressive,
		Defensive,
		Emergency);

	ENUM(AgentGoalSelectionType, char,
		RemoveAgent,
		RandomSelection,
		GoalNodeId,
		Profile);

} // namespace tjs::core
