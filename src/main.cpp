#include "Hooks.h"
#include "Events.h"
#include "Manager.h"
#include "Menu.h"
#include <Papyrus.h>

reshade::api::effect_runtime* s_pRuntime = nullptr;
HMODULE g_hModule = nullptr;

// Callback when Reshade begins effects
static void on_reshade_begin_effects(reshade::api::effect_runtime* runtime)
{
	s_pRuntime = runtime;
}

static void DrawMenu(reshade::api::effect_runtime*)
{
	Menu::GetSingleton()->SettingsMenu();
}

// Register and unregister addon events
void register_addon_events()
{
	reshade::register_event<reshade::addon_event::init_effect_runtime>(on_reshade_begin_effects);
	reshade::register_overlay(nullptr, &DrawMenu);
}

void unregister_addon_events()
{
	reshade::unregister_event<reshade::addon_event::init_effect_runtime>(on_reshade_begin_effects);
	reshade::unregister_overlay(nullptr, &DrawMenu);
}

bool Load()
{
	if (reshade::register_addon(g_hModule))
	{
		SKSE::log::info("Registered addon");
		register_addon_events();
		return true;
	}
	else
	{
		SKSE::log::info("ReShade not present.");
		return false;
	}
}

void MessageListener(SKSE::MessagingInterface::Message* message)
{
	switch (message->type)
	{
		// https://github.com/ianpatt/skse64/blob/09f520a2433747f33ae7d7c15b1164ca198932c3/skse64/PluginAPI.h#L193-L212
	case SKSE::MessagingInterface::kPostPostLoad:
	{
		Hook::Install();
		const auto manager = Manager::GetSingleton();
		manager->parseINI();
		if (!manager->parseJSONPreset(manager->getLastPreset()))
		{
			manager->setLastPreset("");
		}

		RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(Event::GetSingleton());
	}
	break;

	default:
		break;
	}
}

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []()
	{
		SKSE::PluginVersionData v;
		v.PluginName(Plugin::NAME);
		v.AuthorName("SkyHorizon and PhilikusHD");
		v.PluginVersion(Plugin::VERSION);
		v.UsesAddressLibrary();
		v.UsesNoStructs();
		return v;
	}
();

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}

extern "C" DLLEXPORT const char* NAME = "SSE ReShade Effect Toggler";
extern "C" DLLEXPORT const char* DESCRIPTION = "ReShade Effect Toggler by SkyHorizon and PhilikusHD.";


int __stdcall DllMain(HMODULE hModule, uint32_t fdwReason, void*)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		g_hModule = hModule;
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		unregister_addon_events();
		reshade::unregister_addon(hModule);
	}

	return 1;
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
	SKSE::Init(skse, true);

	spdlog::set_pattern("[%H:%M:%S:%e] [%l] %v"s);

#ifndef NDEBUG
	spdlog::set_level(spdlog::level::trace);
	spdlog::flush_on(spdlog::level::trace);
#else
	spdlog::set_level(spdlog::level::info);
	spdlog::flush_on(spdlog::level::info);
#endif

	SKSE::log::info("Game version: {}", skse->RuntimeVersion());

	SKSE::AllocTrampoline(28);

	if (!Load())
		SKSE::stl::report_and_fail("ReShade not present!\nIf you want to use ReShade Effect Toggler, please install ReShade with full add-on support."sv);

	auto papyrus = SKSE::GetPapyrusInterface();
	papyrus->Register(Papyrus::Bind);

	SKSE::GetMessagingInterface()->RegisterListener(MessageListener);

	return true;
}