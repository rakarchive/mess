#include <cstdint>
#include <iostream>

#include "chess/board.hpp"
#include "chess/version.hpp"

// NOLINTNEXTLINE chrono header unnecessary in Darwin.
#include <chrono>

using namespace Chess;

int main(int argc, char const *argv[]) {
    constexpr auto defaultFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    constexpr auto defaultDep = "6";

    std::cout << "Mess v1.0.0 by Rak Laptudirm\n" << std::endl;
    std::cout << "pkg/uci   v1.0.0 by Rak Laptudirm (Apache License 2.0)" << std::endl;
    std::cout << "pkg/chess v" << Chess::Meta::Version << " by " << Chess::Meta::Author << " (" << Chess::Meta::License << ")\n" << std::endl;

    const auto fen = argc >= 2 && std::string(argv[1]) != "-" ? argv[1] : defaultFEN;
    const auto dep = argc >= 3 && std::string(argv[2]) != "-" ? argv[2] : defaultDep;

    Board board = Board(FEN(fen));

    const auto start = std::chrono::steady_clock::now();
    const auto nodes = board.Perft<true, true>(std::atoi(dep));
    const auto end = std::chrono::steady_clock::now();

    const std::chrono::duration<double> delta = end - start;
    std::cout << "nodes " << nodes << " nps " << (uint64_t)((nodes / delta.count()) / 1'000'000) << std::endl;
}
