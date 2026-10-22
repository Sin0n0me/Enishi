#include <chrono>
#include <core/system/animation/cursor_pose_config.h>
#include <core/system/animation/model_event_motion_config.h>
#include <core/system/camera/camera_zoom_config.h>
#include <foundation/text/config_line_reader.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

using namespace enishi;
namespace {
    void require(bool value, const char* message) {
        if (!value) {
            throw std::runtime_error(message);
        }
    }

    class TemporaryConfig {
      public:
        std::filesystem::path path;
        explicit TemporaryConfig(const std::string& text);
        ~TemporaryConfig();
    };

    TemporaryConfig::TemporaryConfig(const std::string& text)
        : path(std::filesystem::temp_directory_path() /
               ("enishi-config-" +
                   std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                   ".txt")) {
        std::ofstream file(this->path, std::ios::binary);
        file << text;
        require(file.good(), "Cannot write temporary configuration");
    }

    TemporaryConfig::~TemporaryConfig() {
        std::error_code error;
        std::filesystem::remove(this->path, error);
    }

    void reader_tests() {
        std::istringstream input("\xEF\xBB\xBF# comment\r\n \t\r\nfirst 1 # tail\n\"#quoted\" 2");
        foundation::ConfigLineReader reader(input);
        std::istringstream fields;
        require(reader.next(fields) && reader.line_number() == 3,
            "Physical line numbers must include skipped lines");
        std::string key;
        int value = 0;
        fields >> key >> value;
        require(key == "first" && value == 1 && !foundation::has_config_trailing_fields(fields),
            "Trailing comments must be accepted");
        require(reader.next(fields), "Reader must reset field stream state between lines");
        fields >> std::quoted(key) >> value;
        require(key == "#quoted" && value == 2, "Quoted hash must remain part of the value");
        require(!reader.next(fields) && !reader.read_failed(),
            "EOF without a newline is not an I/O failure");
        std::istringstream failed;
        failed.setstate(std::ios::failbit);
        foundation::ConfigLineReader failed_reader(failed);
        require(!failed_reader.next(fields) && failed_reader.read_failed(),
            "Non-EOF stream failure must be detected");
        std::istringstream bad;
        bad.setstate(std::ios::badbit);
        foundation::ConfigLineReader bad_reader(bad);
        require(
            !bad_reader.next(fields) && bad_reader.read_failed(), "Bad stream must be reported");
    }

    void setting_tests() {
        std::istringstream camera("\xEF\xBB\xBF# header\n\ninvert_wheel on # reversed");
        const auto parsed = core::parse_camera_zoom_config(camera);
        require(parsed.is_ok() && parsed.unwrap().invert_wheel,
            "Camera parser must preserve BOM/comment support");
        std::istringstream invalid_camera("# header\n\ninvert_wheel on extra");
        const auto rejected = core::parse_camera_zoom_config(invalid_camera);
        require(rejected.is_err() &&
                    rejected.unwrap_err().get_message().find("line 3") != std::string::npos,
            "Camera parser must retain error line numbers");
        std::istringstream duplicate("invert_wheel on\ninvert_wheel off");
        require(core::parse_camera_zoom_config(duplicate).is_err(),
            "Duplicate camera keys must still fail");
        std::istringstream cursor("# header\n\nneck_yaw 91");
        const auto limited = core::parse_cursor_pose_config(cursor);
        require(limited.is_err() &&
                    limited.unwrap_err().get_message().find("line 3") != std::string::npos,
            "Cursor validation and physical error lines must be preserved");
        std::istringstream failed;
        failed.setstate(std::ios::badbit);
        require(core::parse_camera_zoom_config(failed).is_err(),
            "Camera parser must propagate read errors");
    }

    void motion_tests() {
        TemporaryConfig valid("\xEF\xBB\xBF# header\n\npush_states Dynamic # "
                              "selected\n\"#generic\" \"click\" \"*\" none # disabled action");
        types::ModelClickSettings settings;
        const auto parsed = core::load_model_event_motions(valid.path, &settings);
        require(parsed.is_ok() && parsed.unwrap().size() == 1 &&
                    parsed.unwrap().front().model == "#generic",
            "Motion parser must preserve quoted values and final unterminated lines");
        require(settings.push_states ==
                    std::vector<types::RigidBodyKind>{types::RigidBodyKind::Dynamic},
            "Motion directives must use the common line reader");
        TemporaryConfig invalid("# header\n\n\"*\" \"click\" \"*\" none extra");
        const auto rejected = core::load_model_event_motions(invalid.path);
        require(rejected.is_err() &&
                    rejected.unwrap_err().get_message().find("line 3") != std::string::npos,
            "Motion parser must preserve trailing-field errors and line numbers");
    }
} // namespace

void config_reader_tests() {
    reader_tests();
    setting_tests();
    motion_tests();
}
