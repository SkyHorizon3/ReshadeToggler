#pragma once

#include "Manager.h"
#include "MenuManager.h"

class Menu : public ISingleton<Menu>, public MenuManager
{
public:
	void SettingsMenu();

private:
	void SpawnMainPage(ImGuiID dockspaceId);
	void SpawnMenuSettings(ImGuiID dockspaceId);
	void SpawnTimeSettings(ImGuiID dockspaceId);
	void SpawnInteriorSettings(ImGuiID dockspaceId);
	void SpawnWeatherSettings(ImGuiID dockspaceId);
private:
	void SaveFile();

	void AddNewMenu(std::map<std::string, std::vector<MenuToggleInformation>>& updatedInfoList);
	void AddNewWeather(std::map<std::string, std::vector<WeatherToggleInformation>>& updatedInfoList);
	void AddNewInterior(std::map<std::string, std::vector<InteriorToggleInformation>>& updatedInfoList);
	void AddNewTime(std::map<std::string, std::vector<TimeToggleInformation>>& updatedInfoList);
	void ClampInputValue(char* inputStr, int maxVal);
	void EditValues(const std::string& effectName, std::vector<UniformInfo>& toReturn);
	void HandleEffectEditing(std::vector<UniformInfo>& targetUniforms, std::string& currentEditingEffect, int& editingEffectIndex);
	void DisableReShade(const std::string& headerUniqueId, const std::string& key);
private:

	char m_inputBuffer[256] = { 0 };
	std::string m_selectedPreset = Manager::GetSingleton()->getLastPreset();
	std::vector<std::string> m_presets = Manager::GetSingleton()->enumeratePresets();

	std::vector<std::string> m_effects = Manager::GetSingleton()->enumerateEffects();
	std::vector<std::string> m_menuNames = Manager::GetSingleton()->enumerateMenus();
	std::vector<std::string> m_worldSpaces = Manager::GetSingleton()->enumerateWorldSpaces();
	std::vector<std::string> m_interiorCells = Manager::GetSingleton()->enumerateInteriorCells();
	std::vector<std::string> m_weatherFlags = {
		"kNone", "kRainy", "kPleasant", "kCloudy", "kSnow", "kPermAurora", "kAuroraFollowsSun"
	};

	std::string m_currentEditingEffect{};
	int m_editingEffectIndex = -1;

	ImVec4 m_lastMessageColor;
	std::string m_lastMessage;

	bool m_saveConfigPopupOpen = false;
	bool m_openSettingsMenu = false;
	bool m_showMenuSettings = false;
	bool m_showTimeSettings = false;
	bool m_showInteriorSettings = false;
	bool m_showWeatherSettings = false;
};