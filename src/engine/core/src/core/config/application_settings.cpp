#include "application_settings.h"
#include <array>
#include <foundation/text/config_line_reader.h>
#include <fstream>
#include <sstream>
#include <string_view>
#include <utility>

namespace enishi::core {
    namespace {
        enum Section : std::size_t { Camera, Cursor, Events, Count };
        using Sections = std::array<std::string, Count>;
        constexpr std::array<std::string_view, Count> HEADERS{
            "[camera]", "[cursor_tracking]", "[model_events]"};

        foundation::Result<Sections, SettingsError> split_sections(std::istream& stream) {
            Sections sections;
            std::array<bool, Count> seen{};
            std::size_t current = Count;
            std::size_t previous_line = 0;
            foundation::ConfigLineReader reader(stream);
            std::istringstream fields;
            while (reader.next(fields)) {
                // Gap = current line - previous line - 1; blanks preserve physical error lines.
                const auto skipped = reader.line_number() - previous_line - 1;
                for (auto& section : sections) {
                    section.append(skipped, '\n');
                }
                previous_line = reader.line_number();
                if (fields.peek() == '[') {
                    std::string header;
                    fields >> header;
                    current = Count;
                    for (std::size_t index = 0; index < HEADERS.size(); ++index) {
                        if (header == HEADERS[index]) {
                            current = index;
                            break;
                        }
                    }
                    if (current == Count || seen[current] ||
                        foundation::has_config_trailing_fields(fields)) {
                        return foundation::Error(SettingsError::InvalidSetting,
                            "Unknown, duplicate or malformed section at line " +
                                std::to_string(previous_line));
                    }
                    seen[current] = true;
                } else {
                    if (current == Count) {
                        return foundation::Error(SettingsError::InvalidSetting,
                            "Setting outside a section at line " + std::to_string(previous_line));
                    }
                    sections[current] += fields.str();
                }
                for (auto& section : sections) {
                    section += '\n';
                }
            }
            if (reader.read_failed()) {
                return foundation::Error(
                    SettingsError::ReadFailed, "Cannot read application settings");
            }
            return sections;
        }
    } // namespace
    foundation::Result<ApplicationSettings, SettingsError> parse_application_settings(
        std::istream& stream, const std::filesystem::path& directory) {
        auto split = split_sections(stream);
        if (split.is_err()) {
            return std::move(split).unwrap_err();
        }
        auto sections = std::move(split).unwrap();
        std::istringstream camera_stream(sections[Camera]);
        std::istringstream cursor_stream(sections[Cursor]);
        std::istringstream event_stream(sections[Events]);
        auto camera = parse_camera_zoom_config(camera_stream);
        if (camera.is_err()) {
            return foundation::Error(
                SettingsError::InvalidSetting, "[camera] " + camera.unwrap_err().get_message());
        }
        auto cursor = parse_cursor_pose_config(cursor_stream);
        if (cursor.is_err()) {
            return foundation::Error(SettingsError::InvalidSetting,
                "[cursor_tracking] " + cursor.unwrap_err().get_message());
        }
        ApplicationSettings settings;
        auto events = parse_model_event_motions(event_stream, directory, &settings.click);
        if (events.is_err()) {
            return foundation::Error(SettingsError::InvalidSetting,
                "[model_events] " + events.unwrap_err().get_message());
        }
        settings.camera = std::move(camera).unwrap();
        settings.cursor_tracking = std::move(cursor).unwrap();
        settings.model_events = std::move(events).unwrap();
        return settings;
    }
    foundation::Result<ApplicationSettings, SettingsError> load_application_settings(
        const std::filesystem::path& path) {
        std::ifstream stream(path);
        if (!stream.is_open()) {
            return foundation::Error(
                SettingsError::ReadFailed, "Cannot open application settings: " + path.string());
        }
        return parse_application_settings(stream, path.parent_path());
    }
} // namespace enishi::core
