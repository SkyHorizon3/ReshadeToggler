#include "Manager.h"
#include "Utils.h"
#include "glaze/glaze.hpp"

void Manager::parseJSONPreset(const std::string& presetName)
{
	const std::string fullPath = getPresetPath(presetName);

	std::ifstream openFile(fullPath);
	if (!openFile.is_open())
	{
		SKSE::log::error("Couldn't load preset {}!", fullPath);
		return;
	}

	std::stringstream buffer;
	buffer << openFile.rdbuf();

	const auto menuPair = std::make_pair("Menu", std::ref(m_menuToggleInfo));

	deserializeArbitraryVector(buffer.str(),
		menuPair
	);

	SKSE::log::info("Menu:");
	for (const auto& member : m_menuToggleInfo)
	{
		SKSE::log::info("\t{}, {}, {}", member.effectName, member.menuName, member.state);
	}
}

void Manager::serializeJSONPreset(const std::string& presetName)
{
	const std::string fullPath = getPresetPath(presetName);

	std::ofstream outFile(fullPath);
	if (!outFile.is_open())
	{
		SKSE::log::error("Couldn't save preset {}!", presetName);
		return;
	}

	std::string buffer = serializeArbitraryVector(
		std::make_pair(std::string("Menu"), m_menuToggleInfo)
	);

	// TODO: investigate why no good looking json. This is kinda irrelevant tho
	//buffer = glz::prettify_json(buffer);
	outFile << buffer;
	outFile.close();
}

void Manager::toggleEffectMenu(const std::set<std::string>& openMenus)
{
	for (const auto& menuInfo : m_menuToggleInfo)
	{
		if (openMenus.find(menuInfo.menuName) != openMenus.end())
		{
			toggleEffect(menuInfo.effectName.c_str(), menuInfo.state);
		}
		else
		{
			toggleEffect(menuInfo.effectName.c_str(), !menuInfo.state);
		}
	}
}

std::vector<std::string> Manager::enumeratePresets()
{
	std::vector<std::string> presets;
	constexpr auto presetDirectory = L"Data\\SKSE\\Plugins\\ReShadeEffectTogglerPresets";
	if (!std::filesystem::exists(presetDirectory))
		std::filesystem::create_directories(presetDirectory);

	for (const auto& preset : std::filesystem::recursive_directory_iterator(presetDirectory))
	{
		if (preset.is_regular_file() && preset.path().filename().extension() == ".json")
		{
			presets.emplace_back(preset.path().filename().string());
		}
	}

	std::sort(presets.begin(), presets.end());
	return presets;
}

std::vector<std::string> Manager::enumerateEffects()
{
	constexpr const char* shadersDirectory = "reshade-shaders\\Shaders";
	std::vector<std::string> effects;

	for (const auto& entry : std::filesystem::recursive_directory_iterator(shadersDirectory))
	{
		if (entry.is_regular_file() && entry.path().filename().extension() == ".fx")
		{
			effects.emplace_back(entry.path().filename().string());
		}
	}
	//sort files
	std::sort(effects.begin(), effects.end());
	return effects;
}

std::vector<std::string> Manager::enumerateMenus()
{
	const auto ui = RE::UI::GetSingleton();
	const auto& menuMap = ui->menuMap;
	std::vector<std::string> menuNames;

	for (const auto& menu : menuMap)
	{
		const auto& menuName = menu.first;
		menuNames.emplace_back(std::string(menuName));
	}
	std::sort(menuNames.begin(), menuNames.end());
	return menuNames;
}

std::string Manager::getPresetPath(const std::string& presetName)
{
	constexpr const char* configDirectory = "Data\\SKSE\\Plugins\\ReShadeEffectTogglerPresets";

	if (!std::filesystem::exists(configDirectory))
		std::filesystem::create_directories(configDirectory);

	return std::string(configDirectory) + "\\" + presetName;
}

void Manager::toggleEffectWeather()
{
	const auto sky = RE::Sky::GetSingleton();
	const auto player = RE::PlayerCharacter::GetSingleton();
	const auto ui = RE::UI::GetSingleton();

	if (m_weatherToggleInfo.empty() || !player || !sky || !sky->currentWeather || !ui || ui->GameIsPaused())
		return;

	const auto flags = sky->currentWeather->data.flags;
	std::string weatherFlag{};

	switch (flags.get())
	{
	case RE::TESWeather::WeatherDataFlag::kNone:
		weatherFlag = "kNone";
		break;
	case RE::TESWeather::WeatherDataFlag::kRainy:
		weatherFlag = "kRainy";
		break;
	case RE::TESWeather::WeatherDataFlag::kPleasant:
		weatherFlag = "kPleasant";
		break;
	case RE::TESWeather::WeatherDataFlag::kCloudy:
		weatherFlag = "kCloudy";
		break;
	case RE::TESWeather::WeatherDataFlag::kSnow:
		weatherFlag = "kSnow";
		break;
	case RE::TESWeather::WeatherDataFlag::kPermAurora:
		weatherFlag = "kPermAurora";
		break;
	case RE::TESWeather::WeatherDataFlag::kAuroraFollowsSun:
		weatherFlag = "kAuroraFollowsSun";
		break;
	}

	const auto ws = player->GetWorldspace();
	if (!ws || m_lastWs.first && m_lastWs.first->formID != ws->formID) // player is in interior or changed worldspace
	{
		if (m_lastWs.first && m_lastWs.second.weatherFlag != weatherFlag) // change effect state back to original if it was toggled before
		{
			toggleEffect(m_lastWs.second.effectName.c_str(), !m_lastWs.second.state);
			m_lastWs.first = nullptr;
		}
		return;
	}

	const auto it = m_weatherToggleInfo.find(std::to_string(Utils::getTrimmedFormID(ws)) + Utils::getModName(ws));
	if (it == m_weatherToggleInfo.end()) // no info for ws in unordered map
		return;

	const auto& weatherInfo = it->second;
	if (weatherInfo.weatherFlag == weatherFlag)
	{
		toggleEffect(weatherInfo.effectName.c_str(), weatherInfo.state);
		m_lastWs.first = ws;
		m_lastWs.second = weatherInfo;
	}
	else
	{
		toggleEffect(weatherInfo.effectName.c_str(), !weatherInfo.state);
	}
}

float Manager::getCurrentGameTime()
{
	using func_t = decltype(&Manager::getCurrentGameTime);
	static REL::Relocation<func_t> func{ REL::VariantID(56475, 56832, 0x9F3290) };
	return func();
}

void Manager::toggleEffect(const char* effect, const bool state) const
{
	s_pRuntime->enumerate_techniques(effect, [&state](reshade::api::effect_runtime* runtime, reshade::api::effect_technique technique)
		{
			runtime->set_technique_state(technique, state); // True = enabled; False = disabled
		});
}

template <typename T>
std::string Manager::serializeVector(const std::string& key, const std::vector<T>& vec)
{
	std::string vecJson;
	const auto result = glz::write_json(vec, vecJson);  // Serialize the vector
	if (result) {
		const std::string descriptive_error = glz::format_error(result, vecJson);
		SKSE::log::error("Error serializing vector: {}", descriptive_error);
	}

	const std::string jsonStr = "\"" + key + "\": " + vecJson;
	return jsonStr;
}

template <typename... Args>
std::string Manager::serializeArbitraryVector(const Args&... args)
{
	std::stringstream jsonStream;
	jsonStream << "{ ";

	bool first = true;
	((jsonStream << (first ? (first = false, "") : ", ") << serializeVector(args.first, args.second)), ...);


	jsonStream << " }";

	return jsonStream.str();
}

template<typename ...Args>
void Manager::deserializeArbitraryVector(const std::string& buf, Args& ...args)
{
	glz::json_t json{};
	std::ignore = glz::read_json(json, buf);

	auto process_pair = [&](const auto& pair) {
		const auto& key = pair.first;
		auto& vec = pair.second;

		if (json.contains(key))
		{
			//SKSE::log::info("Found");

			auto& jsonArray = json[key].get_array();
			vec.clear();
			vec.reserve(jsonArray.size());

			// Serialize the array to a string and then deserialize into the vector
			std::string newBuffer;
			std::ignore = glz::write_json(jsonArray, newBuffer);
			std::ignore = glz::read_json(vec, newBuffer);
		}
		else
		{
			SKSE::log::error("Key '{}' not found in JSON.", key);
		}
		};

	// Apply the lambda function to each argument pair
	(process_pair(args), ...);
}