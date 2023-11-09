#include "pch.h"
#include "InstanceConfig.hpp"
#include <inja/inja.hpp>
#include "inipp.h"


std::string models::InstanceConfig::RenderInjaTemplate(const std::string& tpl, const json& data) const
{
    inja::Environment env;

    // provides reading environment variable, example: envar("PROGRAMDATA") -> C:\ProgramData
    env.add_callback("envar", [](const inja::Arguments& args)
    {
        if (args.empty())
        {
            return std::string{};
        }

        const auto envarName = args.at(0)->get<std::string>();

        std::string envarValue(MAX_PATH, '\0');

        if (!GetEnvironmentVariableA(envarName.c_str(), envarValue.data(), static_cast<DWORD>(envarValue.size())))
        {
            // return fallback/default value on failure, if provided
            return args.size() > 1 ? args.at(1)->get<std::string>() : std::string{};
        }

        // strip redundant NULLs
        envarValue.erase(std::ranges::find(envarValue, '\0'), envarValue.end());

        return envarValue;
    });

    // reads a registry value, mandatory params:
    // - view
    // - hive
    // - key
    // - value
    //
    // optional param:
    // - default/fallback value if not found
    env.add_callback("regval", [](const inja::Arguments& args)
    {
        std::string defaultRet{};

        if (args.size() < 4)
        {
            return defaultRet;
        }

        if (args.size() > 4)
        {
            defaultRet = args.at(4)->get<std::string>();
        }

        const auto viewName = args.at(0)->get<std::string>();
        const auto view = magic_enum::enum_cast<RegistryView>(viewName);
        const auto hiveName = args.at(1)->get<std::string>();
        const auto subKeyValue = args.at(2)->get<std::string>();
        const auto subKey = ConvertAnsiToWide(subKeyValue);
        const auto valueNameValue = args.at(3)->get<std::string>();
        const auto valueName = ConvertAnsiToWide(valueNameValue);

        HKEY hive = nullptr;

        const auto regHive = magic_enum::enum_cast<RegistryHive>(hiveName);

        if (!regHive.has_value())
        {
            spdlog::error("Unsupported hive provided");
            return defaultRet;
        }

        switch (regHive.value())
        {
        case RegistryHive::HKCU:
            hive = HKEY_CURRENT_USER;
            break;
        case RegistryHive::HKLM:
            hive = HKEY_LOCAL_MACHINE;
            break;
        case RegistryHive::HKCR:
            hive = HKEY_CLASSES_ROOT;
            break;
        case RegistryHive::Invalid:
            spdlog::error("Unsupported hive provided");
            return defaultRet;
        }

        winreg::RegKey key;
        REGSAM flags = KEY_READ;

        if (view.has_value() && view > RegistryView::Default)
        {
            flags |= static_cast<REGSAM>(view.value());
        }

        if (const winreg::RegResult result = key.TryOpen(hive, subKey, flags); !result)
        {
            spdlog::error("Failed to open {}\\{} key", hiveName, subKeyValue);
            return defaultRet;
        }

        const auto& resource = key.TryGetStringValue(valueName);

        if (!resource.IsValid())
        {
            spdlog::error("Failed to access value {}", valueNameValue);
            return defaultRet;
        }

        std::string value = ConvertWideToANSI(resource.GetValue());

        spdlog::debug("value = {}", value);

        return value;
    });

    env.add_callback("inival", [](const inja::Arguments& args)
    {
        std::string defaultRet{};

        if (args.size() < 3)
        {
            return defaultRet;
        }

        if (args.size() > 3)
        {
            defaultRet = args.at(3)->get<std::string>();
        }

        const auto filePath = args.at(0)->get<std::string>();
        const auto section = args.at(1)->get<std::string>();
        const auto keyName = args.at(2)->get<std::string>();

        try
        {
            if (std::filesystem::file_size(filePath) <= 3)
            {
                spdlog::warn("File {} is too small for successful parsing", filePath);
                return defaultRet;
            }

            inipp::Ini<char> ini;
            std::ifstream is(filePath);

            char a, b, c;
            a = is.get();
            b = is.get();
            c = is.get();
            // checks for UTF-8 BOM presence and skips it (1st found section extraction will fail otherwise)
            if (a != static_cast<char>(0xEF) || b != static_cast<char>(0xBB) || c != static_cast<char>(0xBF))
            {
                is.seekg(0);
            }

            ini.parse(is);

            std::string value;
            if (!inipp::get_value(ini.sections[section], keyName, value))
            {
                spdlog::warn("Section {} not found", section);
                return defaultRet;
            }

            return value;
        }
        catch (const std::exception& e)
        {
            spdlog::error("Failed to read INI file {}, error {}", filePath, e.what());
            return defaultRet;
        }
    });

    try
    {
        auto rendered = env.render(tpl, data);

        spdlog::debug("rendered = {}", rendered);

        return rendered;
    }
    catch (const std::exception& ex)
    {
        spdlog::error("Failed to render inja template, error {}", ex.what());

        return std::string{};
    }
}
