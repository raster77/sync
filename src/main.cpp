#include "inc/argparse.hpp"
#include "inc/utils.hpp"
#include <iostream>

void showUsage();

int main(int argc, char **argv)
{
    //std::ios_base::sync_with_stdio(false);

    argparse::ArgumentParser program("sync", utl::VERSION);

    program.add_argument("-s", "--source")
        .required()
        .help("Source to sync.");

    program.add_argument("-t", "--target")
        .required()
        .help("Target.");

    program.add_argument("-v", "--verbose")
      .help("Increase output verbosity")
      .default_value(true)
      .implicit_value(true);

    try {
      program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
      showUsage();
      return 0;
    }

    std::string src = program.get<std::string>("-s");
    std::string tgt = program.get<std::string>("-t");
    
    if(!fs::exists(src)) {
        std::cout << "Error\n" << "Source " << src << " does not exists.";
        return 0;
    }
    
    utl::Options opts;
    utl::sync(src, tgt, opts);

    return 0;
}

void showUsage() {
    std::cout << "Usage : sync -s SOURCE -t TARGET\n";
    std::cout << "or    : sync --source SOURCE --target TARGET\n";
    std::cout << "\nSync SOURCE to TARGET.";
    std::cout << "\nOptions :\n";
    std::cout << "\t-v, --verbose increase verbosity" << std::endl << std::endl;
}