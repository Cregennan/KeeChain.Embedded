// (c) 2024 Takhir Latypov <cregennandev@gmail.com>
// MIT License

#include <Warlin.h>
inline std::deque<std::string> split(const std::string& s);

Warlin_::Warlin_() = default;

bool Warlin_::available()
{
    return Serial.available();
}

void Warlin_::process()
{
    std::string line(Serial.readStringUntil('\n').c_str());
    auto parts = split(line);
    if (parts.empty())
    {
        SendErrorMessage("Empty string recieved");
        return;
    }

    if (parts[0] != PROTOCOL_MAGIC_BEGIN)
    {
        SendErrorMessage("Non-Warlin string recieved", line.c_str());
        return;
    }

    auto magic_begin = parts.front();
    parts.pop_front();

    if (parts.empty())
    {
        SendErrorMessage("No Request type was provided");
        return;
    }

    const auto strType = parts.front();
    parts.pop_front();

    auto& reflector = EnumReflector::For<PROTOCOL_REQUEST_TYPE>();
    auto parseResult = reflector.Find(strType);
    if (!parseResult.IsValid())
    {
        SendErrorMessage("Unable to parse PROTOCOL_REQUEST_TYPE:", strType.c_str());
        return;
    }
    const auto requestType = static_cast<PROTOCOL_REQUEST_TYPE>(parseResult.Value());

    if (listeners.count(requestType) == 0)
    {
        SendErrorMessage("Unable to find corresponding listener to type", strType.c_str());
        return;
    }
    const auto listener = listeners.at(requestType);

    if (listener == nullptr)
    {
        SendErrorMessage("Listener was NULL for type", strType.c_str());
        return;
    }

    listener(parts);

    parts.clear(); // Чистим память после себя
    line.clear();
}

void Warlin_::bind(const PROTOCOL_REQUEST_TYPE type, void (*func)(std::deque<std::string>&))
{
    if (func != nullptr)
    {
        this->listeners[type] = func;
    }else
    {
        SendErrorMessage("Attempt to bind nullptr function");
    }
}

void Warlin_::writeLine(const std::string & str)
{
    Serial.write(PROTOCOL_MAGIC_BEGIN);
    Serial.write(DEFAULT_DELIMITER);
    Serial.write(str.c_str());
    Serial.write('\n');
}

void Warlin_::writeLine(const std::initializer_list<std::string>& args)
{
    Serial.write(PROTOCOL_MAGIC_BEGIN);
    Serial.write(DEFAULT_DELIMITER);
    auto iterator = args.begin();
    Serial.write((*iterator++).c_str());
    while(iterator != args.end()) {
        Serial.write((*iterator++).c_str());
        Serial.write(DEFAULT_DELIMITER);
    }
    Serial.write('\n');
}

std::string NameOf(const PROTOCOL_REQUEST_TYPE type)
{
    const auto index = static_cast<uint8_t>(type);
    const auto& reflector = EnumReflector::For<PROTOCOL_REQUEST_TYPE>();
    return reflector[index].Name();
}

std::string NameOf(const PROTOCOL_RESPONSE_TYPE type)
{
    const auto index = static_cast<uint8_t>(type);
    const auto& reflector = EnumReflector::For<PROTOCOL_RESPONSE_TYPE>();
    return reflector[index].Name();
}

void SendDebugMessage(const char * const message)
{
    #ifdef KEECHAIN_DEBUG_ENABLED
        Serial.write(PROTOCOL_DEBUG_BEGIN);
        Serial.write(": ");
        Serial.write(message);
        Serial.write('\n');
    #endif
}

void SendDebugMessage(const char* part1, const char* part2)
{
#ifdef KEECHAIN_DEBUG_ENABLED
    Serial.write(PROTOCOL_DEBUG_BEGIN);
    Serial.write(": ");
    Serial.write(part1);
    Serial.write(" ");
    Serial.write(part2);
    Serial.write('\n');
#endif
}

void SendErrorMessage(const char * const message)
{
    Serial.write(PROTOCOL_ERROR_BEGIN);
    Serial.write(": ");
    Serial.write(message);
    Serial.write('\n');
}

void SendErrorMessage(const char* part1, const char* part2)
{
    Serial.write(PROTOCOL_ERROR_BEGIN);
    Serial.write(": ");
    Serial.write(part1);
    Serial.write(" ");
    Serial.write(part2);
    Serial.write('\n');
}

inline std::deque<std::string> split(const std::string& s)
{
    std::deque<std::string> output;

    std::string::size_type prev_pos = 0, pos = 0;

    while((pos = s.find(DEFAULT_DELIMITER, pos)) != std::string::npos)
    {
        std::string substring( s.substr(prev_pos, pos-prev_pos) );

        output.push_back(substring);

        pos += strlen(DEFAULT_DELIMITER);

        prev_pos = pos;
    }

    output.push_back(s.substr(prev_pos, pos-prev_pos));

    return output;
}