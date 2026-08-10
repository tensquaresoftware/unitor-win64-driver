#include "PortNameNormalize.h"

#include <cctype>

std::string normalizePortLabel(const std::string& name)
{
    const std::size_t lastSpace = name.find_last_of(' ');
    if (lastSpace == std::string::npos || lastSpace + 1 >= name.size())
    {
        return name;
    }
    const std::string tail = name.substr(lastSpace + 1);
    if (tail.empty())
    {
        return name;
    }
    for (const char ch : tail)
    {
        if (!std::isdigit(static_cast<unsigned char>(ch)))
        {
            return name;
        }
    }
    return name.substr(0, lastSpace);
}

int portMatchRank(const std::string& name, const std::string& needle)
{
    if (name == needle)
    {
        return 0;
    }
    if (normalizePortLabel(name) == needle)
    {
        return 1;
    }
    if (name.size() <= needle.size())
    {
        return -1;
    }
    if (name.compare(0, needle.size(), needle) != 0)
    {
        return -1;
    }
    const std::string rest = name.substr(needle.size());
    if (rest.empty())
    {
        return 0;
    }
    if (rest[0] != ' ')
    {
        return -1;
    }
    for (std::size_t index = 1; index < rest.size(); ++index)
    {
        if (!std::isdigit(static_cast<unsigned char>(rest[index])) && rest[index] != ' ')
        {
            return -1;
        }
    }
    return 2;
}
