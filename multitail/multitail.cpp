#include <string>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "concurrent_queue.h"
#include "FileReader.h"
#include "FileWriter.h"

using namespace std;
namespace po = boost::program_options;

int main(int argc, char** argv)
{
    string outputFile;
    vector<string> inputFiles;

    po::options_description options("Options");
    options.add_options()
        ("help,h", "Print help message")
        ("output-file,o", po::value<string>(&outputFile)->required(), "Output file")
        ("input-files", po::value<vector<string>>(&inputFiles)->required(), "Input files")
        ;

    po::positional_options_description positionalOptions;
    positionalOptions.add("input-files", -1);

    po::variables_map vm;

    try
    {
        po::store(po::command_line_parser(argc, argv).options(options).positional(positionalOptions).run(), vm);

        if (vm.count("help"))
        {
            cout << "Usage: " << boost::filesystem::basename(argv[0]) << " -o output input1 input2 ...\n" << options << endl;
            return 1;
        }

        po::notify(vm);
    }
    catch (const exception& e)
    {
        cout << "ERROR: " << e.what() << endl;
        return 1;
    }

    shared_ptr<concurrent_queue<string>> queue(new concurrent_queue<string>());
    vector<unique_ptr<FileReader>> readers;
    FileWriter writer(wstring(outputFile.begin(), outputFile.end()), queue);

    for (auto inputFile : inputFiles)
    {
        readers.push_back(unique_ptr<FileReader>(new FileReader(wstring(inputFile.begin(), inputFile.end()), queue)));
    }

    cout << "Press Enter to stop the programm..." << endl;
    cin.ignore();

    return 0;
}
