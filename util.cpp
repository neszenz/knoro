#include "util.hpp"

namespace neszenz {

std::string here(std::source_location loc)
{
    return fmt::format("{}:{}", loc.file_name(), loc.line());
}

} // namespace neszenz
