#pragma once

#include "CommonTypes.hpp"

namespace models
{
    /**
     * \brief Setup exit code parameters.
     */
    class ExitCodeCheck
    {
    public:
        /** True to skip exit code check */
        bool skipCheck{false};
        /** The setup exit codes treated as success */
        std::vector<int> successCodes{0};
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ExitCodeCheck, skipCheck, successCodes)

    /**
     * \brief Details about checksum/hash calculation.
     */
    class ChecksumParameters
    {
    public:
        /** The checksum to compare against */
        std::string checksum;
        /** The checksum algorithm */
        ChecksumAlgorithm checksumAlg;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ChecksumParameters, checksum, checksumAlg)

    /**
     * \brief Represents an update release.
     */
    class UpdateRelease
    {
    public:
        /** The release display name */
        std::string name;
        /** The version as a 3-digit SemVer (1.0.0) or 4-digit (1.0.0.0) string */
        std::string version;
        /** The update summary/changelog, supports Markdown */
        std::string summary;
        /** The publishing timestamp as UTC ISO 8601 string */
        std::string publishedAt;
        /** URL of the new setup/release download */
        std::string downloadUrl;
        /** Size of the remote file in bytes */
        std::optional<size_t> downloadSize;
        /** The launch arguments (CLI arguments) if any */
        std::optional<std::string> launchArguments;
        /** The exit code parameters */
        std::optional<ExitCodeCheck> exitCode;
        /** The (optional) checksum of the remote setup */
        std::optional<ChecksumParameters> checksum;
        /** If set, this release is ignored and not presented to the user */
        std::optional<bool> disabled;
        /** The file hash to use in product detection */
        std::optional<ChecksumParameters> detectionChecksum;
        /** Size of the remote file in bytes */
        std::optional<size_t> detectionSize;
        /** The version to use in product detection */
        std::optional<std::string> detectionVersion;

        /** Full pathname of the local temporary file */
        std::filesystem::path localTempFilePath{};

        /**
         * \brief Converts the version string to a SemVer type.
         * \return The parsed version.
         */
        semver::version GetSemVersion() const
        {
            try
            {
                std::string value = util::trim(version, "v \t");
                util::toSemVerCompatible(value);
                // trim whitespaces and potential "v" prefix
                return semver::version{value};
            }
            catch (...)
            {
                return semver::version{0, 0, 0};
            }
        }

        /**
         * \brief Gets the detection version, if provided.
         * \return The parsed version.
         */
        std::optional<semver::version> GetDetectionSemVersion() const
        {
            if (!detectionVersion.has_value())
            {
                // fallback value is mandatory
                return GetSemVersion();
            }

            try
            {
                std::string value = util::trim(detectionVersion.value(), "v \t");
                util::toSemVerCompatible(value);
                // trim whitespaces and potential "v" prefix
                return semver::version{value};
            }
            catch (...)
            {
                return std::nullopt;
            }
        }
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
        UpdateRelease,
        name,
        version,
        summary,
        publishedAt,
        downloadUrl,
        downloadSize,
        launchArguments,
        exitCode,
        checksum,
        disabled,
        detectionChecksum,
        detectionSize,
        detectionVersion
    )
}
