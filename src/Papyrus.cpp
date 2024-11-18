#include "Papyrus.h"
#include "Manager.h"

namespace Papyrus
{
	void ToggleEffect(VM*, StackID, RE::StaticFunctionTag*, RE::BSFixedString effectName, bool state)
	{
		Manager::GetSingleton()->toggleEffect(effectName.c_str(), state);
	}

	bool Bind(VM* vm)
	{
		if (!vm)
		{
			SKSE::log::critical("Couldn't get VM State"sv);
			return false;
		}

		vm->RegisterFunction("ToggleEffect"sv, "ReShadeEffectToggler"sv, ToggleEffect, true);
		return true;
	}
}

