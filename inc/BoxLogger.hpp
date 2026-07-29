
#ifndef SPT_BOX_LOGGER_H
#define SPT_BOX_LOGGER_H

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace spt {
class BoxLogger : public std::ostringstream {
   public:
    explicit BoxLogger(const char* title, int width = 88) : m_title(title), m_width(width), m_target(&std::cout) {}
    ~BoxLogger() { flush(); }

    BoxLogger(const BoxLogger&)            = delete;
    BoxLogger& operator=(const BoxLogger&) = delete;
    BoxLogger(BoxLogger&&)                 = default;
    BoxLogger& operator=(BoxLogger&&)      = default;

    const std::string& getTitle() const { return m_title; }
    int getWidth() const { return m_width; }
    void setWidth(int width) { m_width = width; }
    BoxLogger& setTarget(std::ostream& os) {
        m_target = &os;
        return *this;
    }

    BoxLogger& info(const char* tag, const char* file, int line) {
        std::string file_name(file);
        size_t last_slash = file_name.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            file_name = file_name.substr(last_slash + 1);
        }

        *this << "[" << tag << " " << file_name << ":" << line << "] ";
        return *this;
    }
    void flush() {
        if (m_target == nullptr || m_width <= 0 || str().empty()) { return; }

        // 1. Get the content string and output stream
        std::string buf  = str();     // content
        std::ostream& os = *m_target; // output stream

        // 2. Generate the top border
        os << "\n┌── " << m_title << " ";
        for (int i = 0; i < m_width - m_title.size() - 4; i++) { os << "─"; }
        os << "┐\n";

        // 3. Split content buffer by line and wrap each line with '│'
        std::string_view bufv(buf);
        size_t pos = 0;
        while (pos < bufv.length()) {
            size_t npos = bufv.find('\n', pos);
            auto line   = (npos == std::string_view::npos) ? bufv.substr(pos) : bufv.substr(pos, npos - pos);

            // 3.1 Jump out if it is the last line and it is empty
            if (npos == std::string_view::npos && line.empty() && pos > 0) {
                break;
            }

            // 3.2 Wrap each line with '│'
            os << "│ " << std::left << std::setw(m_width - 1) << std::string(line) << "│\n";

            // 3.3 Move to next line
            pos = npos == std::string_view::npos ? bufv.length() : npos + 1; // std::string_view::npos is -1
        }

        // 4. Generate bottom border
        os << "└";
        for (int i = 0; i < m_width; ++i)
            os << "─";
        os << "┘\n";

        os.flush();
    }

   private:
    std::string m_title;              // box title for top border
    int m_width;                      // content width(exclude border | and \n)
    std::ostream* m_target = nullptr; // output stream(e.g. std::cout or file handler created by caller)
};

#define BOX_LOG(title, width) spt::BoxLogger(title, width)

#define BOX_LOG_TRACE(title, width) BOX_LOG(title, width).info("TRACE", __FILE__, __LINE__)
#define BOX_LOG_INFO(title, width)  BOX_LOG(title, width).info("INFO", __FILE__, __LINE__)
#define BOX_LOG_WARN(title, width)  BOX_LOG(title, width).info("WARN", __FILE__, __LINE__)
#define BOX_LOG_ERROR(title, width) BOX_LOG(title, width).info("ERROR", __FILE__, __LINE__)

} // namespace spt

#endif // SPT_BOX_LOGGER_H