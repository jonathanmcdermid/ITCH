#include <array>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef WIN32
#include <arpa/inet.h>
#endif
#include <zlib.h>

struct TradeData
{
    uint64_t volume{};
    double priceVolume{};
    [[nodiscard]] double GetVwap() const { return volume == 0 ? std::numeric_limits<double>::quiet_NaN() : priceVolume / volume; }
};

// Returns the message length, including the message type character.
size_t MessageLength(char c)
{
    switch (c)
    {
    case 'S': return 12; // System event
    case 'R': return 39; // Stock directory
    case 'H': return 25; // Stock trading action
    case 'Y': return 20; // Reg SHO restriction
    case 'L': return 26; // Market participant position
    case 'V': return 35; // MWCB Decline
    case 'W': return 12; // MWCB Status
    case 'K': return 28; // IPO Quoting Period Update
    case 'J': return 35; // LULD Auction Collar
    case 'h': return 1;  // Operational halt
    case 'A': return 36; // Add order
    case 'F': return 40; // Add order MPID Attribution
    case 'E': return 31; // Order executed
    case 'C': return 36; // Order executed with price
    case 'X': return 23; // Order cancel
    case 'D': return 19; // Order delete
    case 'U': return 35; // Order replace
    case 'P': return 44; // Trade
    case 'Q': return 40; // Cross trade
    case 'B': return 19; // Broken trade
    case 'I': return 50; // NOII
    case 'O': return 1;  // Direct listing
    default: return -1;  // Default case
    }
}

// Swap Network (big endian) byte order to Host byte order (little endian).
uint32_t NetworkToHost(uint32_t val)
{
#ifdef _WIN32
    return _byteswap_ulong(val);
#else
    return ntohl(val);
#endif
}

// Swap Network (big endian) byte order to Host byte order (little endian).
uint64_t NetworkToHost(uint64_t val)
{
#ifdef _WIN32
    return _byteswap_uint64(val);
#else
    return be64toh(val);
#endif
}

void CalculateAndPrintVwap(const std::unordered_map<std::string, std::array<TradeData, 24>>& stockData)
{
    for (const auto& [ticker, hourlyData] : stockData)
    {
        for (int hour = 0; hour < 24; ++hour)
        {
            std::cout << std::left << std::setw(10) << "Ticker:" << std::setw(10) << ticker << std::setw(10) << "Hour:" << std::setw(10) << hour
                      << std::setw(10) << "VWAP:" << std::fixed << std::setprecision(4) << hourlyData[hour].GetVwap() << '\n';
        }
    }
}

void ProcessFile(const char* filename)
{
    gzFile gfile = gzopen(filename, "rb");
    if (!gfile)
    {
        std::cerr << "Failed to open file: " << filename << '\n';
        return;
    }

    std::unordered_map<std::string, std::array<TradeData, 24>> stockData;

    while (true)
    {
        size_t messageLength = (gzgetc(gfile) << 8) | gzgetc(gfile);
        char messageType = static_cast<char>(gzgetc(gfile));

        // We use the expected length as a check to ensure the message is valid.
        if (MessageLength(messageType) != messageLength)
        {
            if (gzeof(gfile)) { break; }
            std::cerr << "Failed to parse data. Char: " << messageType << " Length: " << messageLength;
            return;
        }

        std::array<char, 50> message{};
        size_t bytesRead = gzread(gfile, message.data(), messageLength - 1);

        if (bytesRead != messageLength - 1)
        {
            if (gzeof(gfile)) { break; }
            std::cerr << "Failed to read data. Char: " << messageType << " Bytes read: " << bytesRead;
            return;
        }

        // We are ignoring all messages except for 'P' (Trade) messages.
        if (messageType == 'P')
        {
            const uint64_t timestamp = NetworkToHost(*reinterpret_cast<const uint64_t*>(message.data() + 4)) & ((1ULL << 48) - 1);
            const uint32_t shares = NetworkToHost(*reinterpret_cast<const uint32_t*>(message.data() + 19));
            const double price = NetworkToHost(*reinterpret_cast<const uint32_t*>(message.data() + 31)) / 10000.0;
            const std::string ticker(message.data() + 23, 8);

            constexpr uint64_t nsPerHour = 60ULL * 60ULL * 1E9;

            const int hour = (timestamp / nsPerHour) % 24;

            TradeData& tradeData = stockData[ticker][hour];
            tradeData.volume += shares;
            tradeData.priceVolume += shares * price;
        }
    }

    gzclose(gfile);

    CalculateAndPrintVwap(stockData);
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return EXIT_FAILURE;
    }

    ProcessFile(argv[1]);

    return EXIT_SUCCESS;
}
