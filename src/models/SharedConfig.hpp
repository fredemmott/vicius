#pragma once

#include "ProductDetection.hpp"

using json = nlohmann::json;

namespace models
{
    /**
     * \brief Parameters that might be provided by both the server and the local configuration.
     */
    class SharedConfig
    {
    public:
        /** The classic window title, only shown in taskbar in our case */
        std::optional<std::string> windowTitle;
        /** Name of the product shown in UI */
        std::optional<std::string> productName;
        /** The detection method */
        std::optional<ProductVersionDetectionMethod> detectionMethod;
        /** The detection method for the installed software version */
        std::optional<json> detection;
        /** URL of the error article */
        std::optional<std::string> installationErrorUrl;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
        SharedConfig,
        windowTitle,
        productName,
        detectionMethod,
        detection,
        installationErrorUrl
    )
}
