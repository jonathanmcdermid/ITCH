#include <iostream>

#include "itch_parser.hpp"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return EXIT_FAILURE;
    }

    ItchParser parser;
    parser.Parse(argv[1]);
    parser.CalculateAndPrintVwap();

    return EXIT_SUCCESS;
}