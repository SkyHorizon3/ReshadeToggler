#include "Events.h"
#include "Manager.h"

RE::BSEventNotifyControl Event::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_source)
{
	if (!a_event || !a_source)
		return RE::BSEventNotifyControl::kContinue;

	Manager::GetSingleton()->toggleEffectMenu(a_event->menuName.c_str(), a_event->opening);

	return RE::BSEventNotifyControl::kContinue;
}