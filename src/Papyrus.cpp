#include "Papyrus.h"
#include "Manager.h"

namespace Papyrus
{

	bool IsReShadeInstalled(VM*, StackID, RE::StaticFunctionTag*)
	{
		return Manager::GetSingleton()->isReShadeInstalled();
	}

	void ToggleEffect(VM* vm, StackID stackID, RE::StaticFunctionTag*, RE::BSFixedString effectName, bool state)
	{
		const auto manager = Manager::GetSingleton();
		if (!manager->isReShadeInstalled())
		{
			vm->TraceStack("ReShade with full add-on support not installed!", stackID);
			return;
		}

		if (!manager->effectExists(effectName.c_str()))
		{
			const auto message = std::format("ReShade effect {} not found!", effectName.c_str());
			vm->TraceStack(message.c_str(), stackID);
			return;
		}

		manager->toggleEffect(effectName.c_str(), state);
	}

	void ToggleReShade(VM* vm, StackID stackID, RE::StaticFunctionTag*, bool state)
	{
		if (!Manager::GetSingleton()->isReShadeInstalled())
		{
			vm->TraceStack("ReShade with full add-on support not installed!", stackID);
			return;
		}

		s_pRuntime->set_effects_state(state);
	}

	bool Bind(VM* vm)
	{
		if (!vm)
		{
			SKSE::log::critical("Couldn't get VM State"sv);
			return false;
		}

		constexpr std::string_view className = "ReShadeEffectToggler";

		vm->RegisterFunction("ToggleEffect"sv, className, ToggleEffect, true);
		vm->RegisterFunction("IsReShadeInstalled"sv, className, IsReShadeInstalled, true);
		vm->RegisterFunction("ToggleReShade"sv, className, ToggleReShade, true);
		return true;
	}
}

