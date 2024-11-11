#include "Manager.h"
#include "Utils.h"
#include "glaze/glaze.hpp"

bool Manager::parseJSONPreset(const std::string& presetName)
{
	const std::string fullPath = getPresetPath(presetName);

	std::ifstream openFile(fullPath);
	if (!openFile.is_open())
	{
		//SKSE::log::error("Couldn't load preset {}!", fullPath);
		return false;
	}

	std::stringstream buffer;
	buffer << openFile.rdbuf();

	const auto menuPair = std::make_pair("Menu", std::ref(m_menuToggleInfo));
	const auto timePair = std::make_pair("Time", std::ref(m_timeToggleInfo));
	const auto weatherPair = std::make_pair("Weather", std::ref(m_weatherToggleInfo));
	const auto interiorPair = std::make_pair("Interior", std::ref(m_interiorToggleInfo));

	return deserializeArbitraryData(buffer.str(), menuPair, timePair, weatherPair, interiorPair);
}

bool Manager::serializeJSONPreset(const std::string& presetName)
{
	const std::string fullPath = getPresetPath(presetName);

	std::ofstream outFile(fullPath);
	if (!outFile.is_open())
	{
		SKSE::log::error("Couldn't save preset {}!", presetName);
		return false;
	}

	std::string buffer;
	if (!serializeArbitraryData(buffer,
		std::make_pair("Menu", m_menuToggleInfo),
		std::make_pair("Time", m_timeToggleInfo),
		std::make_pair("Weather", m_weatherToggleInfo),
		std::make_pair("Interior", m_interiorToggleInfo)))
	{
		SKSE::log::error("Failed to serialize preset {}!", presetName);
		return false;
	}

	// TODO: prettify
	outFile << buffer;
	outFile.close();

	return true;
}

void Manager::parseINI()
{
	const auto path = std::format("Data/SKSE/Plugins/{}.ini", Plugin::NAME);

	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(path.c_str());

	Utils::loadINIStringSetting(ini, "Preset", "LastPreset", m_lastPresetName);

}

void Manager::serializeINI()
{
	const auto path = std::format("Data/SKSE/Plugins/{}.ini", Plugin::NAME);

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.SetValue("Preset", "LastPreset", m_lastPresetName.c_str());
	ini.SaveFile(path.c_str());

}

std::vector<std::string> Manager::enumeratePresets() const
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

std::vector<std::string> Manager::enumerateEffects() const
{
	std::vector<std::string> effects;

	s_pRuntime->enumerate_techniques(nullptr, [&](reshade::api::effect_runtime* runtime, reshade::api::effect_technique technique)
		{
			char nameBuffer[64];

			runtime->get_technique_effect_name(technique, nameBuffer);

			if (std::find(effects.begin(), effects.end(), nameBuffer) == effects.end())
			{
				effects.emplace_back(nameBuffer);
			}

		});

	std::sort(effects.begin(), effects.end());
	return effects;
}

std::vector<std::string> Manager::enumerateActiveEffects() const
{
	std::vector<std::string> effects;

	s_pRuntime->enumerate_techniques(nullptr, [&](reshade::api::effect_runtime* runtime, reshade::api::effect_technique technique)
		{
			if (runtime->get_technique_state(technique))
			{
				char nameBuffer[64];

				runtime->get_technique_effect_name(technique, nameBuffer);

				if (std::find(effects.begin(), effects.end(), nameBuffer) == effects.end())
				{
					effects.emplace_back(nameBuffer);
				}

			}
		});

	return effects;
}

std::vector<std::string> Manager::enumerateMenus() const
{
	const auto ui = RE::UI::GetSingleton();
	std::vector<std::string> menuNames;

	for (const auto& menu : ui->menuMap)
	{
		menuNames.emplace_back(menu.first.c_str());
	}
	std::sort(menuNames.begin(), menuNames.end());
	return menuNames;
}

std::vector<std::string> Manager::enumerateWorldSpaces() const
{
	const auto& ws = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESWorldSpace>();
	std::vector<std::string> worldSpaces;
	worldSpaces.reserve(ws.size());

	for (const auto& space : ws)
	{
		if (space)
			worldSpaces.emplace_back(constructKey(space));
	}

	return worldSpaces;
}

std::vector<std::string> Manager::enumerateInteriorCells() const
{
	const auto& cells = RE::TESDataHandler::GetSingleton()->interiorCells;

	std::vector<std::string> interiorCells;
	interiorCells.reserve(cells.size());

	for (const auto& cell : cells)
	{
		if (cell)
		{
			interiorCells.emplace_back(constructKey(cell));
		}
	}

	return interiorCells;
}


std::string Manager::getPresetPath(const std::string& presetName) const
{
	constexpr const char* configDirectory = "Data\\SKSE\\Plugins\\ReShadeEffectTogglerPresets";

	if (!std::filesystem::exists(configDirectory))
		std::filesystem::create_directories(configDirectory);

	return std::string(configDirectory) + "\\" + presetName;
}

std::string Manager::constructKey(const RE::TESForm* form) const
{
	if (!form)
		return "";

	return std::format("{:08X}|{}|{}", Utils::getTrimmedFormID(form), form->GetFormEditorID(), Utils::getModName(form));
}

void Manager::toggleEffectMenu(const std::unordered_set<std::string>& openMenus)
{
	for (auto& [menuName, menuInfoList] : m_menuToggleInfo)
	{
		const bool isOpen = openMenus.find(menuName) != openMenus.end();

		for (auto& info : menuInfoList)
		{
			if (isOpen && !info.isToggled)
			{
				toggleEffect(info.effectName.c_str(), info.state);
				info.isToggled = true;
			}
			else if (!isOpen && info.isToggled)
			{
				toggleEffect(info.effectName.c_str(), !info.state);
				info.isToggled = false;
			}

			for (auto& uniform : info.uniforms)
			{
				setUniformValues(uniform);
			}
		}
	}

}

void Manager::setUniformValues(UniformInfo& uniform)
{
	const auto manager = Manager::GetSingleton();

	if (!uniform.floatValues.empty())
	{
		manager->setUniformValue<float>(uniform.uniformVariable, uniform.floatValues.data(), uniform.floatValues.size());
	}
	else if (!uniform.intValues.empty())
	{
		manager->setUniformValue<int>(uniform.uniformVariable, uniform.intValues.data(), uniform.intValues.size());
	}
	else if (!uniform.uintValues.empty())
	{
		manager->setUniformValue<unsigned int>(uniform.uniformVariable, uniform.uintValues.data(), uniform.uintValues.size());
	}
	else if (!uniform.boolValues.empty())
	{
		manager->setUniformValue<bool>(uniform.uniformVariable, reinterpret_cast<bool*>(uniform.boolValues.data()), 1);
	}
}

bool Manager::allowtoggleEffectWeather(const WeatherToggleInformation& cachedweather, const std::map<std::string, std::vector<WeatherToggleInformation>>::iterator& it) const
{
	if (it == m_weatherToggleInfo.end())
		return true;

	for (const auto& newInfo : it->second)
	{
		if (cachedweather.effectName == newInfo.effectName &&
			cachedweather.state == newInfo.state &&
			cachedweather.weatherFlag == newInfo.weatherFlag)
		{
			return false;
		}
	}

	return true;
}

bool Manager::allowtoggleEffectTime(const TimeToggleInformation& cachedweather, const std::map<std::string, std::vector<TimeToggleInformation>>::iterator& it) const
{
	if (it == m_timeToggleInfo.end())
		return true;

	for (const auto& newInfo : it->second)
	{
		if (cachedweather.effectName == newInfo.effectName &&
			cachedweather.state == newInfo.state &&
			timeWithinRange(newInfo.startTime, newInfo.stopTime))
		{
			return false;
		}
	}

	return true;
}

bool Manager::allowtoggleEffectInterior(const InteriorToggleInformation& cachedInterior, const std::map<std::string, std::vector<InteriorToggleInformation>>::iterator& it) const
{
	if (it == m_interiorToggleInfo.end())
		return true;

	for (const auto& newInfo : it->second)
	{
		if (cachedInterior.effectName == newInfo.effectName &&
			cachedInterior.state == newInfo.state)
		{
			return false;
		}
	}

	return true;
}

void Manager::toggleEffectWeather()
{
	const auto sky = RE::Sky::GetSingleton();
	const auto player = RE::PlayerCharacter::GetSingleton();
	const auto ui = RE::UI::GetSingleton();

	if (m_weatherToggleInfo.empty() || !player || !sky || !sky->currentWeather || !ui || ui->GameIsPaused())
		return;

	static std::pair<RE::TESWorldSpace*, std::vector<WeatherToggleInformation>> lastWs;

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
	const auto it = m_weatherToggleInfo.find(constructKey(ws));
	const auto cachedWorldspace = lastWs.first;

	if (!ws || cachedWorldspace && cachedWorldspace->formID != ws->formID) // player is in interior or changed worldspace
	{
		if (cachedWorldspace)
		{
			for (const auto& info : lastWs.second)
			{
				if (!ws || allowtoggleEffectWeather(info, it)) // change effect state back to original if it was toggled before
				{
					toggleEffect(info.effectName.c_str(), !info.state);
				}
			}
			lastWs.first = nullptr;
			lastWs.second.clear();
		}
		return;
	}

	if (it == m_weatherToggleInfo.end()) // no info for ws in unordered map
		return;

	for (auto& info : it->second)
	{
		if (info.weatherFlag == weatherFlag)
		{
			toggleEffect(info.effectName.c_str(), info.state);
			info.isToggled = true;
			lastWs.first = ws;
			lastWs.second.emplace_back(info);
		}
		else if (info.isToggled)
		{
			toggleEffect(info.effectName.c_str(), !info.state);
			info.isToggled = false;
		}
	}
}

void Manager::toggleEffectTime()
{
	const auto ui = RE::UI::GetSingleton();
	const auto player = RE::PlayerCharacter::GetSingleton();
	if (m_timeToggleInfo.empty() || !player || !RE::Calendar::GetSingleton() || !ui || ui->GameIsPaused())
		return;

	static std::pair<RE::TESWorldSpace*, std::vector<TimeToggleInformation>> lastWs;

	const auto ws = player->GetWorldspace();
	const auto it = m_timeToggleInfo.find(constructKey(ws));
	const auto cachedWorldspace = lastWs.first;

	if (!ws || cachedWorldspace && cachedWorldspace->formID != ws->formID)
	{
		if (cachedWorldspace)
		{
			for (const auto& info : lastWs.second)
			{
				if (!ws || allowtoggleEffectTime(info, it))
				{
					toggleEffect(info.effectName.c_str(), !info.state);
				}
			}
			lastWs.first = nullptr;
			lastWs.second.clear();
		}
		return;
	}

	if (it == m_timeToggleInfo.end())
		return;

	for (auto& timeInfo : it->second)
	{
		const bool inRange = timeWithinRange(timeInfo.startTime, timeInfo.stopTime);

		if (inRange)
		{
			if (!timeInfo.isToggled)
			{
				toggleEffect(timeInfo.effectName.c_str(), timeInfo.state);
				timeInfo.isToggled = true;
				lastWs.first = ws;
				lastWs.second.emplace_back(timeInfo);
			}
		}
		else if (!inRange && timeInfo.isToggled)
		{
			toggleEffect(timeInfo.effectName.c_str(), !timeInfo.state);
			timeInfo.isToggled = false;
		}
	}

}

void Manager::toggleEffectInterior(const bool isInterior)
{
	const auto player = RE::PlayerCharacter::GetSingleton();
	if (m_interiorToggleInfo.empty() || !player)
		return;

	static std::pair<RE::TESObjectCELL*, std::vector<InteriorToggleInformation>> lastCell;
	const auto cell = player->GetParentCell();
	const auto it = m_interiorToggleInfo.find(constructKey(cell));

	const auto cachedCell = lastCell.first;

	if (!isInterior || cachedCell && isInterior && cachedCell->formID != cell->formID)
	{
		if (cachedCell)
		{
			for (auto& info : lastCell.second)
			{
				if (!isInterior || allowtoggleEffectInterior(info, it))
				{
					toggleEffect(info.effectName.c_str(), !info.state);
				}
			}
			lastCell.first = nullptr;
			lastCell.second.clear();
		}

		return;
	}

	if (it == m_interiorToggleInfo.end())
		return;


	for (auto& info : it->second)
	{
		toggleEffect(info.effectName.c_str(), info.state);

		lastCell.second.emplace_back(info);
	}
	lastCell.first = cell;

}

bool Manager::timeWithinRange(const float& startTime, const float& stopTime) const
{
	const auto calendar = RE::Calendar::GetSingleton();
	const std::uint32_t currentHour = static_cast<std::uint32_t>(calendar->GetHour());
	const float currentTime = currentHour + (calendar->GetMinutes() / 100.f);

	return currentTime >= startTime && currentTime <= stopTime;
}

void Manager::toggleEffect(const char* effect, const bool state) const
{
	s_pRuntime->enumerate_techniques(effect, [&state](reshade::api::effect_runtime* runtime, reshade::api::effect_technique technique)
		{
			runtime->set_technique_state(technique, state); // True = enabled; False = disabled
		});
}

#pragma region TemplateTomfoolery
template<>
struct glz::meta<UniformInfo>
{
	using T = UniformInfo;
	static constexpr auto value = object(
		"UniformName", &T::uniformName,
		"BoolValue", &T::boolValues,
		"IntValues", &T::intValues,
		"FloatValues", &T::floatValues,
		"UIntValues", &T::uintValues
	);
};


template <typename T>
bool Manager::serializeVector(const std::string& key, const std::vector<T>& vec, std::string& output)
{
	std::string vecJson;
	const auto result = glz::write_json(vec, vecJson);  // Serialize the vector
	if (result) {
		const std::string descriptive_error = glz::format_error(result, vecJson);
		SKSE::log::error("Error serializing vector: {}", descriptive_error);
		return false;
	}

	output = "\"" + key + "\": " + vecJson;
	return true;
}

template<typename T>
bool Manager::serializeMap(const std::string& key, const std::map<std::string, std::vector<T>>& map, std::string& output)
{
	std::stringstream mapJson;
	mapJson << "{ ";

	bool first = true;
	for (const auto& pair : map) {
		std::string vecJson;
		const auto result = glz::write_json(pair.second, vecJson);
		if (result) {
			const std::string descriptive_error = glz::format_error(result, vecJson);
			SKSE::log::error("Error serializing vector for key {}: {}", pair.first, descriptive_error);
			return false;
		}

		mapJson << (first ? (first = false, "") : ", ") << "\"" << pair.first << "\": " << vecJson;
	}

	mapJson << " }";
	output = "\"" + key + "\": " + mapJson.str();
	return true;
}

template <typename... Args>
bool Manager::serializeArbitraryData(std::string& output, const Args&... args)
{
	std::stringstream jsonStream;
	jsonStream << "{ ";

	bool first = true;
	bool success = true;

	auto processArg = [&](const auto& pair) {
		std::string serialized;
		if constexpr (std::is_same_v<std::decay_t<decltype(pair.second)>, std::vector<typename std::decay_t<decltype(pair.second)>::value_type>>) {
			// Handle vector case
			if (serializeVector(pair.first, pair.second, serialized)) {
				jsonStream << (first ? (first = false, "") : ", ") << serialized;
			}
			else {
				success = false;
			}
		}
		else if constexpr (std::is_same_v<std::decay_t<decltype(pair.second)>, std::map<std::string, std::vector<typename std::decay_t<decltype(pair.second)>::mapped_type::value_type>>>) {
			// Handle unordered_map case
			if (serializeMap(pair.first, pair.second, serialized)) {
				jsonStream << (first ? (first = false, "") : ", ") << serialized;
			}
			else {
				success = false;
			}
		}
		else {
			SKSE::log::error("Unsupported type for serialization");
			success = false;
		}
		};

	// Process each argument pair
	(processArg(args), ...);

	jsonStream << " }";
	output = jsonStream.str();

	return success;
}

template <typename T>
bool Manager::deserializeVector(const std::string& key, const glz::json_t& json, std::vector<T>& vec)
{
	if (json.contains(key)) {
		std::string buffer;
		const auto write_result = glz::write_json(json[key], buffer);
		if (write_result) {
			SKSE::log::error("Error serializing JSON for key '{}'.", key);
			return false;
		}

		const auto read_result = glz::read_json(vec, buffer);
		if (read_result) {
			SKSE::log::error("Error deserializing vector for key '{}'.", key);
			return false;
		}
	}
	else {
		SKSE::log::error("Key '{}' not found in JSON.", key);
		return false;
	}
	return true;
}

template <typename T>
bool Manager::deserializeMapOfVectors(const std::string& key, const glz::json_t& json, std::map<std::string, std::vector<T>>& map)
{
	if (json.contains(key)) {
		const auto& mapData = json[key].get_object();
		map.clear();
		for (const auto& [subKey, subValue] : mapData) {
			std::vector<T> vec;

			// Serialize the subValue to a string and then deserialize into the vector
			std::string buffer;
			const auto write_result = glz::write_json(subValue, buffer);
			if (write_result) {
				SKSE::log::error("Error serializing JSON for subKey '{}'.", subKey);
				return false;
			}

			const auto read_result = glz::read_json(vec, buffer);
			if (read_result) {
				SKSE::log::error("Error deserializing vector for subKey '{}'.", subKey);
				return false;
			}
			map[subKey] = std::move(vec);
		}
	}
	else {
		SKSE::log::error("Key '{}' not found in JSON.", key);
		return false;
	}
	return true;
}

template <typename... Args>
bool Manager::deserializeArbitraryData(const std::string& buf, Args&... args)
{
	glz::json_t json{};
	const auto result = glz::read_json(json, buf);
	if (result) {
		const std::string descriptive_error = glz::format_error(result, buf);
		SKSE::log::error("Error parsing JSON: {}", descriptive_error);
		return false;
	}
	bool success = true;

	auto process_pair = [&](auto& pair) {
		if constexpr (std::is_same_v<std::decay_t<decltype(pair.second)>, std::vector<typename std::decay_t<decltype(pair.second)>::value_type>>) {
			return deserializeVector(pair.first, json, pair.second);
		}
		else if constexpr (std::is_same_v<std::decay_t<decltype(pair.second)>, std::map<std::string, std::vector<typename std::decay_t<decltype(pair.second)>::mapped_type::value_type>>>) {
			return deserializeMapOfVectors(pair.first, json, pair.second);
		}
		else
		{
			return false;
		}
		};

	((success &= process_pair(args)), ...);
	return success;
}


template <typename T>
void Manager::setUniformValue(const reshade::api::effect_uniform_variable& uniformVariable, T* value, size_t count)
{
	assert(count > 0 && count <= 4);

	if constexpr (std::is_same<T, float>::value)
	{
		s_pRuntime->set_uniform_value_float(uniformVariable, value, count);
	}
	else if constexpr (std::is_same<T, int>::value)
	{
		s_pRuntime->set_uniform_value_int(uniformVariable, value, count);
	}
	else if constexpr (std::is_same<T, unsigned int>::value)
	{
		s_pRuntime->set_uniform_value_uint(uniformVariable, value, count);
	}
	else if constexpr (std::is_same<T, bool>::value)
	{
		s_pRuntime->set_uniform_value_bool(uniformVariable, value, count);
	}
	else
	{
		SKSE::log::error("Unsupported uniform type in setUniformValue!");
	}
}

template <typename T>
void Manager::getUniformValue(const reshade::api::effect_uniform_variable& uniformVariable, T* value, size_t count)
{
	assert(count > 0 && count <= 4);

	if constexpr (std::is_same<T, float>::value)
	{
		s_pRuntime->get_uniform_value_float(uniformVariable, value, count);
	}
	else if constexpr (std::is_same<T, int>::value)
	{
		s_pRuntime->get_uniform_value_int(uniformVariable, value, count);
	}
	else if constexpr (std::is_same<T, unsigned int>::value)
	{
		s_pRuntime->get_uniform_value_uint(uniformVariable, value, count);
	}
	else if constexpr (std::is_same<T, bool>::value)
	{
		s_pRuntime->get_uniform_value_bool(uniformVariable, value, count);
	}
	else
	{
		SKSE::log::error("Unsupported uniform type in getUniformValue!");
	}
}

template void Manager::getUniformValue<bool>(const reshade::api::effect_uniform_variable& uniformVariable, bool* values, size_t count);
template void Manager::setUniformValue<bool>(const reshade::api::effect_uniform_variable& uniformVariable, bool* values, size_t count);

template void Manager::getUniformValue<float>(const reshade::api::effect_uniform_variable& uniformVariable, float* values, size_t count);
template void Manager::setUniformValue<float>(const reshade::api::effect_uniform_variable& uniformVariable, float* values, size_t count);

template void Manager::getUniformValue<int>(const reshade::api::effect_uniform_variable& uniformVariable, int* values, size_t count);
template void Manager::setUniformValue<int>(const reshade::api::effect_uniform_variable& uniformVariable, int* values, size_t count);

template void Manager::getUniformValue<unsigned int>(const reshade::api::effect_uniform_variable& uniformVariable, unsigned int* values, size_t count);
template void Manager::setUniformValue<unsigned int>(const reshade::api::effect_uniform_variable& uniformVariable, unsigned int* values, size_t count);
#pragma endregion


std::vector<UniformInfo> Manager::enumerateUniformNames(const std::string& effectName)
{
	std::vector<UniformInfo> uniforms;

	s_pRuntime->enumerate_uniform_variables(effectName.c_str(), [&](reshade::api::effect_runtime* runtime, reshade::api::effect_uniform_variable uniform)
		{
			char name[256];
			size_t nameSize = sizeof(name);
			runtime->get_uniform_variable_name(uniform, name, &nameSize);

			// Create a UniformInfo object
			UniformInfo uniformInfo(name, uniform);

			// Fetch the type of the uniform
			std::string type = getUniformType(uniform);

			// Retrieve the current value based on the type
			if (type.find("bool") != std::string::npos)
			{
				bool value = false;
				getUniformValue(uniform, &value, 1);
				uint8_t boolAsUint8 = static_cast<uint8_t>(value);
				uniformInfo.setBoolValues(&boolAsUint8, 1);
			}
			else if (type.find("float") != std::string::npos)
			{
				float values[4] = { 0.0f };
				int numElements = std::min(4, getUniformDimension(uniform));
				getUniformValue(uniform, values, numElements);
				uniformInfo.setFloatValues(values, numElements);
			}
			else if (type.find("int") != std::string::npos)
			{
				int values[4] = { 0 };
				int numElements = std::min(4, getUniformDimension(uniform));
				getUniformValue(uniform, values, numElements);
				uniformInfo.setIntValues(values, numElements);
			}
			else if (type.find("unsigned int") != std::string::npos)
			{
				unsigned int values[4] = { 0 };
				int numElements = std::min(4, getUniformDimension(uniform));
				getUniformValue(uniform, values, numElements);
				uniformInfo.setUIntValues(values, numElements);
			}

			uniforms.emplace_back(std::move(uniformInfo));
		});

	return uniforms;
}

std::string Manager::getUniformType(const reshade::api::effect_uniform_variable& uniformVariable)
{
	using format = reshade::api::format;
	reshade::api::format baseType;
	uint32_t rows = 0, columns = 0, arrayLength = 0;

	// Call the API function to get the type information
	s_pRuntime->get_uniform_variable_type(uniformVariable, &baseType, &rows, &columns, &arrayLength);

	char name[256];
	size_t nameSize = sizeof(name);
	s_pRuntime->get_uniform_variable_name(uniformVariable, name, &nameSize);
	std::string typeName;

	std::string varName = name;

	switch (baseType)
	{
	case format::r32_float:
		typeName = "float";
		break;
	case format::r32_sint:
		typeName = "int";
		break;
	case format::r32_uint:
		typeName = "unsigned int";
		break;
	case format::r32_typeless:
		typeName = "typeless";
		break;
	default:
		typeName = "unknown";
		break;
	}

	// Heuristic: Check if the name follows a boolean convention
	if (varName.find("is") == 0 || varName.find("use") == 0 || varName.find("enable") == 0)
	{
		typeName = "bool";
	}

	// Append dimensions if applicable
	if (rows > 0 && columns > 0)
	{
		typeName += " (" + std::to_string(rows) + "x" + std::to_string(columns) + ")";
	}
	else if (arrayLength > 0)
	{
		typeName += " [Array of " + std::to_string(arrayLength) + "]";
	}

	return typeName;
}

int Manager::getUniformDimension(const reshade::api::effect_uniform_variable& uniformVariable) const
{

	using format = reshade::api::format;
	reshade::api::format baseType;
	uint32_t rows = 0, columns = 0, arrayLength = 0;

	s_pRuntime->get_uniform_variable_type(uniformVariable, &baseType, &rows, &columns, &arrayLength);

	// Determine the dimension based on the base type and dimensions
	if (arrayLength > 0)
	{
		// If it's an array, return the array length
		return arrayLength;
	}

	// For non-array types, determine based on rows and columns
	if (rows > 0 && columns > 0)
	{
		// Return total components for matrices (rows * columns)
		return rows * columns;
	}
	else if (rows > 0)
	{
		// Return number of rows for vector types
		return rows;
	}
	else if (columns > 0)
	{
		// Return number of columns for matrix types (though typically rows should be > 0)
		return columns;
	}

	// Default to 1 if none of the conditions above are met (scalar case)
	return 1;
}