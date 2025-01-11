#pragma once

namespace Utils
{
	RE::FormID getTrimmedFormID(const RE::TESForm* form);
	std::string getModName(const RE::TESForm* form);
	void loadINIStringSetting(const CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, std::string& a_setting);
	std::string tolower(std::string_view a_str);
	std::string getEditorID(RE::FormID a_formID);
	std::string getFormEditorID(const RE::TESForm* a_form);
}

/**
 * @brief Performs a case-insensitive substring search.
 *
 * This function searches for the first occurrence of the substring needle in the string haystack,
 * ignoring the case of both strings. If the substring is found, a pointer to the beginning of the
 * substring in haystack is returned. If the substring is not found, the function returns nullptr.
 *
 * @param haystack The string to be searched.
 * @param needle The substring to search for.
 * @return A pointer to the beginning of the located substring, or nullptr if the substring is not found.
 *
 * @note This function ensures cross-platform compatibility as strcasestr is not available on all platforms. Shamelessly stolen from Raven.
 */
inline const char* strcasestr(const char* haystack, const char* needle) noexcept
{
	if (!*needle)
	{
		return haystack;
	}

	// Get length of needle and convert it to lowercase
	size_t needle_len = strlen(needle);
	char* lower_needle = new char[needle_len + 1];
	for (size_t i = 0; i < needle_len; ++i)
	{
		lower_needle[i] = (char)tolower(needle[i]);
	}
	lower_needle[needle_len] = '\0';

	const char first_lower_needle = lower_needle[0];

	for (; *haystack; ++haystack)
	{
		if (tolower(*haystack) == first_lower_needle)
		{
			const char* h = haystack;
			const char* n = lower_needle;
			while (*h && *n && tolower(*h) == *n)
			{
				++h;
				++n;
			}

			if (!*n)
			{
				delete[] lower_needle;
				return haystack;
			}
		}
	}

	delete[] lower_needle;
	return nullptr;
}