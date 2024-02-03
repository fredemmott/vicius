#include "pch.h"
#include "Common.h"
#include "imgui_md.h"
#include <SFML/Graphics/Texture.hpp>
#include <SFML/OpenGL.hpp>
#include <restclient-cpp/restclient.h>
#include <restclient-cpp/connection.h>


// Fonts shared throughout app
ImFont* G_Font_H1 = nullptr;
ImFont* G_Font_H2 = nullptr;
ImFont* G_Font_H3 = nullptr;
ImFont* G_Font_Default = nullptr;

// Mapping of image URLs -> OpenGL textures
static std::map<std::string, std::shared_ptr<sf::Texture>> G_ImageTextures;


#if 0
// ReSharper disable once CppPassValueParameterByConstReference
static void LinkClickedCallback(ImGui::MarkdownLinkCallbackData data)
{
    const std::string url(data.link, data.linkLength);
	if (!data.isImage)
	{
		ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	}
}

// ReSharper disable once CppParameterMayBeConst
static void FormatChangelogCallback(const ImGui::MarkdownFormatInfo& markdownFormatInfo, bool start)
{
	defaultMarkdownFormatCallback(markdownFormatInfo, start);

	switch (markdownFormatInfo.type)
	{
	// example: change the colour of heading level 2
	case ImGui::MarkdownFormatType::HEADING:
		{
			if (markdownFormatInfo.level == 2)
			{
				if (start)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
				}
				else
				{
					ImGui::PopStyleColor();
				}
			}
			break;
		}
	default:
		{
			break;
		}
	}
}

inline ImGui::MarkdownImageData ImageCallback(ImGui::MarkdownLinkCallbackData data)
{
	// In your application you would load an image based on data_ input. Here we just use the imgui font texture.
    const ImTextureID image = ImGui::GetIO().Fonts->TexID;
	// > C++14 can use ImGui::MarkdownImageData imageData{ true, false, image, ImVec2( 40.0f, 20.0f ) };
	ImGui::MarkdownImageData imageData;
	imageData.isValid = true;
	imageData.useLinkCallback = false;
	imageData.user_texture_id = image;
	imageData.size = ImVec2(40.0f, 20.0f);

	// For image resize when available size.x > image width, add
	const ImVec2 contentSize = ImGui::GetContentRegionAvail();
	if (imageData.size.x > contentSize.x)
	{
		const float ratio = imageData.size.y / imageData.size.x;
		imageData.size.x = contentSize.x;
		imageData.size.y = contentSize.x * ratio;
	}

	return imageData;
}
#endif

/**
 * \brief Markdown parser and render widget.
 */
struct changelog : public imgui_md
{
    std::map<std::string, std::shared_ptr<sf::Texture>> _images;

    explicit changelog(const std::map<std::string, std::shared_ptr<sf::Texture>>& images) : _images(images)
    {
    }

    ImFont* get_font() const override
    {
        switch (m_hlevel)
        {
        case 1:
            return G_Font_H1;
        case 2:
            return G_Font_H2;
        case 3:
            return G_Font_H3;
        default:
        case 0:
            return G_Font_Default;
        }
    }

    void open_url() const override
    {
        ShellExecuteA(nullptr, "open", m_href.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    /**
     * \brief sf::Texture handle to ImTextureID.
     * \param glTextureHandle The SFML Texture object native handle.
     * \return The ImGui Texture ID.
     */
    static ImTextureID ConvertGlTextureHandleToImTextureId(GLuint glTextureHandle)
    {
        ImTextureID textureID = nullptr;
        std::memcpy(&textureID, &glTextureHandle, sizeof(GLuint));
        return textureID;
    }

    /**
     * \brief Downloads a remote image resource and caches it in memory.
     * \param url The href of the image.
     * \return Smart pointer of SFML texture or null.
     */
    std::shared_ptr<sf::Texture> GetImageTexture(const std::string& url)
    {
        if (!_images.contains(url))
        {
            const auto conn = new RestClient::Connection("");
            conn->FollowRedirects(true, 5);

            const RestClient::Response response = conn->get(url);

            if (response.code != 200)
                return nullptr;

            auto res = std::make_shared<sf::Texture>();
            res->loadFromMemory(response.body.data(), response.body.length());

            _images.emplace(url, res);
            return res;
        }

        return _images[url];
    }

    /**
     * \brief Pulls the texture for the requested image to render.
     * \param nfo Image info to fill out.
     * \return True on success, false otherwise.
     */
    bool get_image(image_info& nfo) const override
    {
        if (const auto texture = const_cast<changelog*>(this)->GetImageTexture(m_href); texture != nullptr)
        {
            const ImTextureID textureID = ConvertGlTextureHandleToImTextureId(texture->getNativeHandle());
            const auto size = texture->getSize();

            nfo.texture_id = textureID;
            nfo.size = {size.x * 1.0f, size.y * 1.0f};
            nfo.uv0 = {0, 0};
            nfo.uv1 = {1, 1};
            nfo.col_tint = {1, 1, 1, 1};
            nfo.col_border = {0, 0, 0, 0};
            return true;
        }

        return false;
    }

    void html_div(const std::string& dclass, bool e) override
    {
        if (dclass == "red")
        {
            if (e)
            {
                m_table_border = false;
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
            }
            else
            {
                ImGui::PopStyleColor();
                m_table_border = true;
            }
        }
    }
};

/**
 * \brief Parses a provided Markdown body string and renders it onto an ImGui widget.
 * \param markdown The Markdown document body.
 */
void markdown::RenderChangelog(const std::string& markdown)
{
    static changelog cl_render(G_ImageTextures);

    cl_render.print(markdown.c_str(), markdown.c_str() + markdown.length());
}
