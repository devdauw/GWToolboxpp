#include "stdafx.h"

#include "GuiUtils.h"

#include <imgui.h>

#include <GWCA/Constants/Constants.h>
#include <GWCA/Utilities/Scanner.h>

#include <Modules/Resources.h>
#include <Utf8.h>

namespace {
	ImFont* font16 = nullptr;
	ImFont* font18 = nullptr;
	ImFont* font20 = nullptr;
	ImFont* font24 = nullptr;
	ImFont* font42 = nullptr;
	ImFont* font48 = nullptr;

    bool fonts_loading = false;
    bool fonts_loaded = false;
}
// Has GuiUtils::LoadFonts() finished?
bool GuiUtils::FontsLoaded() {
    return fonts_loaded;
}
// Loads fonts asynchronously. CJK font files can by over 20mb in size!
void GuiUtils::LoadFonts() {
    if (fonts_loaded || fonts_loading)
        return;
    fonts_loading = true;

	std::thread t([] {
        printf("Loading fonts\n");

		ImGuiIO& io = ImGui::GetIO();
		std::vector<std::pair<wchar_t*, const ImWchar*>> extra_fonts;
		extra_fonts.push_back({ L"Font_Japanese.ttf", io.Fonts->GetGlyphRangesJapanese() });
		extra_fonts.push_back({ L"Font_Cyrillic.ttf", io.Fonts->GetGlyphRangesCyrillic() });
		extra_fonts.push_back({ L"Font_ChineseTraditional.ttf", io.Fonts->GetGlyphRangesChineseFull() });
		extra_fonts.push_back({ L"Font_Korean.ttf", io.Fonts->GetGlyphRangesKorean() });

        // Pre-load font configs
        ImFontConfig* fontCfg;
        
        // Preload font size 16, then re-use the binary data for other sizes to avoid re-reading the file.
        if (PathFileExistsW(Resources::GetPath(L"Font.ttf").c_str())) {
            utf8::string f = Resources::GetPathUtf8(L"Font.ttf");
            io.Fonts->AddFontFromFileTTF(f.bytes, 16.0f, 0, io.Fonts->GetGlyphRangesDefault());
            printf("Font.ttf found and pre-loaded\n");
        }
        else {
            printf("Failed to find Font.ttf file\n");
            fonts_loaded = true;
            fonts_loading = false;
            return;
        }
		// Collect the final font. This is the default (16px) font with all special chars merged. in.
		fontCfg = &io.Fonts->ConfigData.back();

		for (unsigned int i = 0; i < extra_fonts.size(); i++) {
			if (PathFileExistsW(Resources::GetPath(extra_fonts[i].first).c_str())) {
				ImFontConfig c;
				c.MergeMode = true;
				io.Fonts->AddFontFromFileTTF(Resources::GetPathUtf8(extra_fonts[i].first).bytes, 16.0f, &c, extra_fonts[i].second);
				printf("%ls found and pre-loaded\n", extra_fonts[i].first);
			}
			else {
				printf("%ls not found. Add this file to load special chars.\n", extra_fonts[i].first);
			}
		}
        
        font16 = fontCfg->DstFont;

		// Recycle the binary data from original ImFontConfig for the other font sizes.
        ImFontConfig copyCfg;
        copyCfg.FontData = fontCfg->FontData;
        copyCfg.FontDataSize = fontCfg->FontDataSize;
        copyCfg.FontDataOwnedByAtlas = false; // Don't let ImGui try to free this data - it'll do it via defaultFontCfg
        copyCfg.GlyphRanges = fontCfg->GlyphRanges;

        copyCfg.SizePixels = 18.0f;
        sprintf(copyCfg.Name,"Default_18");
        font18 = io.Fonts->AddFont(&copyCfg);

        copyCfg.SizePixels = 20.0f;
        sprintf(copyCfg.Name, "Default_20");
        font20 = io.Fonts->AddFont(&copyCfg);

        copyCfg.SizePixels = 24.0f;
        sprintf(copyCfg.Name, "Default_24");
        font24 = io.Fonts->AddFont(&copyCfg);

        copyCfg.SizePixels = 42.0f;
        sprintf(copyCfg.Name, "Default_42");
        font42 = io.Fonts->AddFont(&copyCfg);

        copyCfg.SizePixels = 48.0f;
        sprintf(copyCfg.Name, "Default_48");
        font48 = io.Fonts->AddFont(&copyCfg);

		printf("Building fonts...\n");
		io.Fonts->Build();
        printf("Fonts loaded\n");
        fonts_loaded = true;
        fonts_loading = false;
	});
	t.detach();
}
ImFont* GuiUtils::GetFont(GuiUtils::FontSize size) {
	ImFont* font = [](FontSize size) -> ImFont* {
		switch (size) {
		case GuiUtils::f16: return font16;
		case GuiUtils::f18: return font18;
		case GuiUtils::f20:	return font20;
		case GuiUtils::f24:	return font24;
		case GuiUtils::f42:	return font42;
		case GuiUtils::f48:	return font48;
		default: return nullptr;
		}
	}(size);
	if (font && font->IsLoaded()) {
		return font;
	} else {
		return ImGui::GetIO().Fonts->Fonts[0];
	}
}

int GuiUtils::GetPartyHealthbarHeight() {
	static DWORD* optionarray = nullptr;
	if (!optionarray) {
		optionarray = (DWORD*)GW::Scanner::Find("\x8B\x4D\x08\x85\xC9\x74\x0A", "xxxxxxx", -9);
		printf("[SCAN] optionarray = %p\n", optionarray);
		if (optionarray)
			optionarray = *(DWORD**)optionarray;
		else
			return GW::Constants::HealthbarHeight::Normal;
	}
	GW::Constants::InterfaceSize interfacesize =
		static_cast<GW::Constants::InterfaceSize>(optionarray[6]);

	switch (interfacesize) {
	case GW::Constants::InterfaceSize::SMALL: return GW::Constants::HealthbarHeight::Small;
	case GW::Constants::InterfaceSize::NORMAL: return GW::Constants::HealthbarHeight::Normal;
	case GW::Constants::InterfaceSize::LARGE: return GW::Constants::HealthbarHeight::Large;
	case GW::Constants::InterfaceSize::LARGER: return GW::Constants::HealthbarHeight::Larger;
	default:
		return GW::Constants::HealthbarHeight::Normal;
	}
}

std::string GuiUtils::ToLower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}
std::wstring GuiUtils::ToLower(std::wstring s) {
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}
// Convert a wide Unicode string to an UTF8 string
std::string GuiUtils::WStringToString(const std::wstring& wstr)
{
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

// Convert an UTF8 string to a wide Unicode String
std::wstring GuiUtils::StringToWString(const std::string& str)
{
	if (str.empty()) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

std::string GuiUtils::RemovePunctuation(std::string s) {
	for (int i = 0, len = s.size(); i < len; i++) {
		if (!ispunct(s[i])) continue;
		s.erase(i--, 1);
		len--;
	}
	return s;
}
std::wstring GuiUtils::RemovePunctuation(std::wstring s) {
	for (int i = 0, len = s.size(); i < len; i++)	{
		if (!ispunct(s[i])) continue;
		s.erase(i--, 1);
		len--;
	}
	return s;
}
bool GuiUtils::ParseInt(const char *str, int *val, int base) {
	char *end;
	*val = strtol(str, &end, base);
	if (*end != 0 || errno == ERANGE)
		return false;
	else
		return true;
}
bool GuiUtils::ParseInt(const wchar_t *str, int *val, int base) {
	wchar_t *end;
	*val = wcstol(str, &end, base);
	if (*end != 0 || errno == ERANGE)
		return false;
	else
		return true;
}
bool GuiUtils::ParseFloat(const char *str, float *val) {
	char *end;
	*val = strtof(str, &end);
	return str != end && errno != ERANGE;
}
bool GuiUtils::ParseFloat(const wchar_t *str, float *val) {
	wchar_t *end;
	*val = wcstof(str, &end);
	return str != end && errno != ERANGE;
}
bool GuiUtils::ParseUInt(const char *str, unsigned int *val, int base) {
	char *end;
	*val = strtoul(str, &end, base);
	if (str == end || errno == ERANGE)
		return false;
	else
		return true;
}
bool GuiUtils::ParseUInt(const wchar_t *str, unsigned int *val, int base) {
	wchar_t *end;
	*val = wcstoul(str, &end, base);
	if (str == end || errno == ERANGE)
		return false;
	else
		return true;
}
char *GuiUtils::StrCopy(char *dest, const char *src, size_t dest_size) {
	strncpy(dest, src, dest_size - 1);
	dest[dest_size - 1] = 0;
	return dest;
}
