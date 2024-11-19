#pragma once

namespace Papyrus
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	std::vector<std::int32_t> GetVersion(RE::StaticFunctionTag*);
	bool IsReShadeInstalled(VM*, StackID, RE::StaticFunctionTag*);
	void ToggleEffect(VM* vm, StackID stackID, RE::StaticFunctionTag*, RE::BSFixedString effectName, bool state);
	void ToggleReShade(VM* vm, StackID stackID, RE::StaticFunctionTag*, bool state);

	bool Bind(VM* vm);
}