#include <filesystem>
#include <iostream>
#include <string>

#include "phonefinder/hotkey_rules.hpp"
#include "phonefinder/service.hpp"
#include "phonefinder/settings.hpp"

static void print_usage() {
  std::cout << "PhoneFinderCLI usage:\n"
            << "  PhoneFinderCLI search <csv_path> <query> [limit]\n"
            << "  PhoneFinderCLI lookup <csv_path> <query>\n"
            << "  PhoneFinderCLI hotkey <hotkey_text>\n"
            << "  PhoneFinderCLI repl [csv_path]\n";
}

int main(int argc, char** argv) {
  try {
    if (argc < 2) {
      print_usage();
      return 0;
    }

    std::string cmd = argv[1];
    if (cmd == "hotkey") {
      if (argc < 3) {
        std::cerr << "missing hotkey text\n";
        return 1;
      }
      std::string normalized;
      std::string err;
      if (!phonefinder::normalize_hotkey(argv[2], normalized, err)) {
        std::cerr << "invalid: " << err << "\n";
        return 2;
      }
      std::cout << normalized << "\n";
      return 0;
    }

    if (cmd == "search") {
      if (argc < 4) {
        std::cerr << "missing arguments\n";
        return 1;
      }
      const std::string path = argv[2];
      const std::string query = argv[3];
      const int limit = argc >= 5 ? std::stoi(argv[4]) : 9;

      phonefinder::Service svc(path);
      const auto out = svc.search(query, limit);
      for (std::size_t i = 0; i < out.size(); ++i) {
        std::cout << (i + 1) << ". " << out[i].entry.department << " -> "
                  << out[i].entry.phone << " [score=" << out[i].score << "]\n";
      }
      return 0;
    }

    if (cmd == "lookup") {
      if (argc < 4) {
        std::cerr << "missing arguments\n";
        return 1;
      }
      const std::string path = argv[2];
      const std::string query = argv[3];

      phonefinder::Service svc(path);
      const auto out = svc.search(query, 1);
      if (out.empty()) {
        return 0;
      }
      std::cout << out.front().entry.phone << "\n";
      return 0;
    }

    if (cmd == "repl") {
      std::string source = argc >= 3 ? argv[2] : "samples/医院科室电话数据模板_demo.csv";

      auto settings = phonefinder::load_settings(source);
      if (!settings.source_path.empty()) {
        source = settings.source_path;
      }

      phonefinder::Service svc(source);
      std::cout << "loaded " << svc.count() << " entries from: " << source << "\n";
      std::cout << "query (empty to exit):\n";

      std::string q;
      while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, q)) break;
        if (q.empty()) break;
        auto out = svc.search(q, 9);
        if (out.empty()) {
          std::cout << "(no match)\n";
          continue;
        }
        for (std::size_t i = 0; i < out.size(); ++i) {
          std::cout << (i + 1) << ". " << out[i].entry.department << " -> "
                    << out[i].entry.phone << "\n";
        }
      }
      return 0;
    }

    print_usage();
    return 1;
  } catch (const std::exception& ex) {
    std::cerr << "error: " << ex.what() << "\n";
    return 2;
  }
}
