#include <Warlin.h>

inline std::deque<std::string> split(const std::string& s);

Warlin_::Warlin_()
{

}

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
        SendDebugMessage("Empty string recieved");
        return;
    }

    if (parts[0] != PROTOCOL_MAGIC_BEGIN)
    {
        SendDebugMessage("Non-Warlin string recieved", line.c_str());
        return;
    }

    auto magic_begin = parts.front();
    parts.pop_front();

    if (parts.empty())
    {
        SendDebugMessage("No Request type was provided");
        return;
    }

    const auto strType = parts.front();
    parts.pop_front();

    const auto pack = StringToProtocolRequestType(strType);
    const auto type = pack.second;

    if (!pack.first)
    {
        SendDebugMessage("Unable to parse PROTOCOL_REQUEST_TYPE:", strType.c_str());
        return;
    }

    if (listeners.count(type) == 0)
    {
        SendDebugMessage("Unable to find corresponding listener to type", strType.c_str());
        return;
    }
    const auto listener = listeners.at(type);

    if (listener == nullptr)
    {
        SendDebugMessage("Listener was NULL for type", strType.c_str());
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
        SendDebugMessage("Attempt to bind nullptr function");
    }
}

void Warlin_::writeString(const std::string & str)
{
    Serial.write(PROTOCOL_MAGIC_BEGIN);
    Serial.write(DEFAULT_DELIMITER);
    Serial.write(str.c_str());
    Serial.write('\n');
}

inline void Warlin_::writeStrings(const std::initializer_list<std::string>& args)
{
    Serial.write(PROTOCOL_MAGIC_BEGIN);
    Serial.write(DEFAULT_DELIMITER);
    auto current = args.begin();
    Serial.write(current->c_str());
    for(current++; current != args.end(); current++)
    {
        Serial.write(DEFAULT_DELIMITER);
        Serial.write(current->c_str());
    }
    delete current;
}


inline std::string ProtocolRequestTypeToString(const PROTOCOL_REQUEST_TYPE & type)
{
    switch (type)
    {
    case PROTOCOL_REQUEST_TYPE::GET_TOTP:
        return "GET_TOTP";
    case PROTOCOL_REQUEST_TYPE::DISCOVER:
        return "DISCOVER";
    case PROTOCOL_REQUEST_TYPE::DH_KEY_PROPOSAL:
        return "DH_KEY_PROPOSAL";
    case PROTOCOL_REQUEST_TYPE::CAPABILITY_QUERY:
        return "CAPABILITY_QUERY";
    case PROTOCOL_REQUEST_TYPE::STORE_SECRET:
        return "STORE_SECRET";
    default:
        SendDebugMessage("Unable to parse PROTOCOL_REQUEST_TYPE");
        return "";
    }
}

inline std::pair<bool, PROTOCOL_REQUEST_TYPE> StringToProtocolRequestType(const std::string & str)
{
    if(str == "DISCOVER")
        return std::make_pair(true, PROTOCOL_REQUEST_TYPE::DISCOVER);
    if(str == "DH_KEY_PROPOSAL")
        return std::make_pair(true, PROTOCOL_REQUEST_TYPE::DH_KEY_PROPOSAL);
    if(str == "CAPABILITY_QUERY")
        return std::make_pair(true, PROTOCOL_REQUEST_TYPE::CAPABILITY_QUERY);
    if(str == "STORE_SECRET")
        return std::make_pair(true, PROTOCOL_REQUEST_TYPE::STORE_SECRET);
    if(str == "GET_TOTP")
        return std::make_pair(true, PROTOCOL_REQUEST_TYPE::GET_TOTP);

    return std::make_pair(false, PROTOCOL_REQUEST_TYPE::DISCOVER);
}

inline void SendDebugMessage(const char * const message)
{
#ifdef KEECHAIN_DEBUG_ALLOWED
    Serial.write(PROTOCOL_DEBUG_BEGIN);
    Serial.write(": ");
    Serial.write(message);
    Serial.write('\n');
#endif
}

inline void SendDebugMessage(const char* part1, const char* part2)
{
#ifdef KEECHAIN_DEBUG_ALLOWED
    Serial.write(PROTOCOL_DEBUG_BEGIN);
    Serial.write(": ");
    Serial.write(part1);
    Serial.write(" ");
    Serial.write(part2);
    Serial.write('\n');
#endif
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