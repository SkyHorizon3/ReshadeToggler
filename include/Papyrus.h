#pragma once
#include "RE/B/BSFixedString.h"

namespace Papyrus
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	void ToggleEffect(VM*, StackID, RE::StaticFunctionTag*, RE::BSFixedString effectName, bool state);

	bool Bind(VM* vm);
}