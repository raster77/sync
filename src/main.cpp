#include "inc/argparse.hpp"
#include "inc/utils.hpp"
#include <iostream>

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

    program.add_argument("--verbose")
      .help("Increase output verbosity")
      .default_value(true)
      .implicit_value(true);

    std::cout << program << std::endl;

    utl::Options opts;
    utl::sync("/home/thierry/Téléchargements", "/home/thierry/toto", opts);

    return 0;
}
