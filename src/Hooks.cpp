#include "Hooks.h"
#include "Manager.h"

namespace Hook
{
	struct MainUpdate
	{

		static void thunk()
		{
			func(); // Run original function

			static auto lastCallTime = std::chrono::steady_clock::now();
			auto now = std::chrono::steady_clock::now();

			if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCallTime).count() >= 500)
			{
				lastCallTime = now;

				const auto singleton = Manager::GetSingleton();
				singleton->toggleEffectWeather();
				singleton->toggleEffectTime();
			}

		};
		static inline REL::Relocation<decltype(thunk)> func;


		static void Install()
		{
			const bool marketplace = REL::Module::get().version() >= SKSE::RUNTIME_SSE_1_6_1130;
			REL::Relocation<std::uintptr_t> target1{ RELOCATION_ID(35565, 36564), REL::Relocate(0x748, (marketplace ? 0xC2b : 0xC26), 0x7EE) };
			stl::write_thunk_call<MainUpdate>(target1.address());
		}
	};

	struct InteriorChange
	{
		static void thunk(bool a_playerIsInInterior) // Runs on cell load
		{
			func(a_playerIsInInterior);

			Manager::GetSingleton()->toggleEffectInterior(a_playerIsInInterior);

		};
		static inline REL::Relocation<decltype(thunk)> func;


		static void Install()
		{
			std::array targets{
				std::make_pair(RELOCATION_ID(13171, 13316), REL::Relocate(0x2E6, 0x46D)),
				std::make_pair(RELOCATION_ID(13172, 13317), REL::Relocate(0x2A, 0x1E)),
			};

			for (auto& [id, offset] : targets)
			{
				REL::Relocation<std::uintptr_t> target(id, offset);
				stl::write_thunk_call<InteriorChange>(target.address());
			}

		}
	};



	void Install()
	{
		MainUpdate::Install();
		InteriorChange::Install();

	}
}