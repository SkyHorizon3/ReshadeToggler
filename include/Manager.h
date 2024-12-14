#pragma once
#include <glaze/json/json_t.hpp>

struct UniformInfo
{
	std::string uniformName;
	reshade::api::effect_uniform_variable uniformVariable;
	std::uint8_t boolValue; // not multi-dimensional
	std::vector<int> intValues;
	std::vector<float> floatValues;
	std::vector<unsigned int> uintValues;

	bool prefetched = false;
	bool tempBoolValue = false;
	float tempFloatValues[4] = { 0.0f };
	int tempIntValues[4] = { 0 };
	unsigned int tempUIntValues[4] = { 0 };

	void setBoolValues(const std::uint8_t& value)
	{
		boolValue = value;
	}

	void setIntValues(int* values, const size_t& count)
	{
		if (count > intValues.size())
		{
			intValues.resize(count);
		}
		std::copy(values, values + count, intValues.begin());
	}

	void setFloatValues(float* values, const size_t& count)
	{
		if (count > floatValues.size())
		{
			floatValues.resize(count);
		}
		std::copy(values, values + count, floatValues.begin());
	}

	void setUIntValues(unsigned int* values, const size_t& count)
	{
		if (count > uintValues.size())
		{
			uintValues.resize(count);
		}
		std::copy(values, values + count, uintValues.begin());
	}

};

struct MenuToggleInformation
{
	std::string effectName{};
	std::string menuName{};
	bool state = true;
	bool isToggled = false;

	std::vector<UniformInfo> uniforms;
};

struct WeatherToggleInformation
{
	std::string effectName{};
	std::string weather{};
	bool state = true;
	bool isToggled = false;

	std::vector<UniformInfo> uniforms;
};

struct InteriorToggleInformation
{
	std::string effectName{};
	bool state = true;

	std::vector<UniformInfo> uniforms;
};

struct TimeToggleInformation
{
	std::string effectName{};
	float startTime = 0.f;
	float stopTime = 0.f;
	bool state = true;
	bool isToggled = false;

	std::vector<UniformInfo> uniforms;
};

class Manager : public ISingleton<Manager>
{
	// class for main functions used for all features

public:

	bool parseJSONPreset(const std::string& presetName);

	bool serializeJSONPreset(const std::string& presetName);

	void parseINI();

	void serializeINI();

	std::vector<std::string> enumeratePresets() const;
	std::vector<std::string> enumerateEffects() const;
	std::vector<std::string> enumerateMenus();
	std::vector<std::string> enumerateWorldSpaces();
	std::vector<std::string> enumerateInteriorCells();
	std::vector<std::string> enumerateWeathers();
	std::vector<UniformInfo> enumerateUniformNames(const std::string& effectName);

	bool toggleReShadeMenu(const std::unordered_set<std::string>& openMenus);
	void toggleEffectMenu(const std::unordered_set<std::string>& openMenus);

	void toggleEffectWeather();

	void toggleEffectTime();

	void toggleEffectInterior(const bool isInterior);

	void toggleEffect(const char* technique, bool state) const;

	void toggleReshade(bool state);

	std::map<std::string, std::vector<MenuToggleInformation>> getMenuToggleInfo() const { return m_menuToggleInfo; }
	void setMenuToggleInfo(const std::map<std::string, std::vector<MenuToggleInformation>>& info) { m_menuToggleInfo = info; }

	std::map<std::string, std::vector<TimeToggleInformation>> getTimeToggleInfo() const { return m_timeToggleInfo; }
	void setTimeToggleInfo(const std::map<std::string, std::vector<TimeToggleInformation>>& info) { m_timeToggleInfo = info; }

	std::map<std::string, std::vector<WeatherToggleInformation>> getWeatherToggleInfo() const { return m_weatherToggleInfo; }
	void setWeatherToggleInfo(const std::map<std::string, std::vector<WeatherToggleInformation>>& info) { m_weatherToggleInfo = info; }

	std::map<std::string, std::vector<InteriorToggleInformation>> getInteriorToggleInfo() const { return m_interiorToggleInfo; }
	void setInteriorToggleInfo(const std::map<std::string, std::vector<InteriorToggleInformation>>& info) { m_interiorToggleInfo = info; }

	std::string getLastPreset() const { return m_lastPresetName; }
	void setLastPreset(const std::string& updatedPreset) { m_lastPresetName = updatedPreset; }

	std::string getPresetPath(const std::string& presetName) const;

	bool isReShadeInstalled() const { return m_isReshadeInstalled; }
	void setReShadeInstalled(const bool state) { m_isReshadeInstalled = state; }

	template<typename T>
	void setUniformValue(const reshade::api::effect_uniform_variable& uniformVariable, T* value, size_t count);

	template<typename T>
	void getUniformValue(const reshade::api::effect_uniform_variable& uniformVariable, T* value, size_t count);

	int getUniformDimension(const reshade::api::effect_uniform_variable& uniformVariable) const;

	bool effectExists(const char* effect);

	std::unordered_map<std::string, bool>& getReShadeToggle() { return m_reshadeToggle; }

private:

	void setUniformValues(UniformInfo& uniform);

	bool timeWithinRange(const float& startTime, const float& stopTime) const;

	bool allowtoggleEffectWeather(const WeatherToggleInformation& cachedweather, const std::map<std::string, std::vector<WeatherToggleInformation>>::iterator& it) const;

	bool allowtoggleEffectTime(const TimeToggleInformation& cachedweather, const std::map<std::string, std::vector<TimeToggleInformation>>::iterator& it) const;

	bool allowtoggleEffectInterior(const InteriorToggleInformation& cachedinterior, const std::map<std::string, std::vector<InteriorToggleInformation>>::iterator& it) const;

	std::string constructKey(const RE::TESForm* form) const;

	std::map<std::string, std::vector<MenuToggleInformation>> m_menuToggleInfo;
	std::map<std::string, std::vector<WeatherToggleInformation>> m_weatherToggleInfo;
	std::map<std::string, std::vector<InteriorToggleInformation>> m_interiorToggleInfo;
	std::map<std::string, std::vector<TimeToggleInformation>> m_timeToggleInfo;

	std::unordered_map<std::string, bool> m_reshadeToggle;

	// INI settings
	std::string m_lastPresetName = "";


	//Papyrus stuff
	bool m_isReshadeInstalled = false;

private:
	template<typename T>
	bool serializeVector(const std::string& key, const std::vector<T>& vec, std::string& output);

	template<typename T>
	bool serializeMap(const std::string& key, const std::map<std::string, std::vector<T>>& map, std::string& output);

	template<typename... Args>
	bool serializeArbitraryData(std::string& output, const Args&... args);

	template<typename T>
	bool deserializeVector(const std::string& key, const glz::json_t& json, std::vector<T>& vec);

	template<typename T>
	bool deserializeMapOfVectors(const std::string& key, const glz::json_t& json, std::map<std::string, std::vector<T>>& map);

	template<typename... Args>
	bool deserializeArbitraryData(const std::string& buf, Args&... args);

	bool serializeReshadeToggle(const std::string& name, const std::unordered_map<std::string, bool>& reshadeToggle, std::string& output);
	bool deserializeReshadeToggle(const std::string& key, const glz::json_t& json, std::unordered_map<std::string, bool>& reshadeToggle);
};

extern reshade::api::effect_runtime* s_pRuntime;