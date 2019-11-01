#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/program_options.hpp>

#include <omp.h>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace po = boost::program_options;

std::string LockName(const int ware_house, const std::string& object_name)
{
    return std::to_string(ware_house) + "###" + object_name;
}

int main(int argc, char** argv)
{
    // Parse command line parameters
    std::string lockids_file_name;
    std::vector<std::string> input_file_names;
    std::vector<std::string> output_file_names;

    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("lockids_file,l",
         po::value<std::string>(&lockids_file_name)->required())
        ("input_files,i",
         po::value<decltype(input_file_names)>(&input_file_names)->multitoken())
        ("output_files,o",
         po::value<decltype(output_file_names)>(&output_file_names)->multitoken())
        ("help,h", "Produce this help message.")
        ;
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help"))
    {
        std::printf(
                "Convert JSON lock trace into CSV locktrace while translating "
                "lock names into IDs.\n");
        std::cout << desc << "\n";
        return 0;
    }

    po::notify(vm);

    if (input_file_names.size() != output_file_names.size())
    {
        throw std::invalid_argument(
                "The number of input and output files must match");
    }

    // Read lock IDs
    std::unordered_map<std::string, int> locks;
    {
        std::ifstream lockids_file(lockids_file_name);
        for (std::string line; std::getline(lockids_file, line);)
        {
            rapidjson::Document d;
            d.Parse(line.c_str());
            auto const ware_house = d["ware_house"].GetInt();
            auto const object_name = d["object_name"].GetString();
            auto const lock_id = d["lock_id"].GetInt();

            auto const ret =
                    locks.emplace(LockName(ware_house, object_name), lock_id);

            if (!ret.second)
            {
                throw std::runtime_error(std::string("duplicate lock: ") +
                                         std::to_string(lock_id) + " vs " +
                                         std::to_string(ret.first->second));
            }
        }
    }

    std::printf("done loading lock IDs (%zi locks)\n", locks.size());

#pragma omp parallel for
    for (size_t i = 0; i < input_file_names.size(); i++)
    {
        std::ifstream input_file(input_file_names[i]);
        std::ofstream output_file(output_file_names[i]);

        int current_trx_num = std::numeric_limits<int>::max();
        std::unordered_set<int> current_locks;

        for (std::string line; std::getline(input_file, line);)
        {
            rapidjson::Document d;
            d.Parse(line.c_str());
            auto const ware_house = d["ware_house"].GetInt();
            auto const object_name = d["object_name"].GetString();

            auto const lock_mode = d["lock_mode"].GetString();
            auto const trx_name = d["trx_name"].GetString();
            auto const trx_num = d["trx_num"].GetInt();

            auto const ret = locks.find(LockName(ware_house, object_name));
            if (ret == locks.end())
            {
                throw std::runtime_error(
                        std::string("lock not found: ") +
                        std::string(LockName(ware_house, object_name)));
            }
            auto const lock_id = ret->second;

            // Reset current locks on new transaction
            if (current_trx_num != trx_num)
            {
                current_trx_num = trx_num;
                current_locks.clear();
            }
            // Remember locks of current TRX, skip output if lock already held
            if (!current_locks.insert(lock_id).second)
                continue;

            output_file << trx_num << "," << lock_id << "," << lock_mode << ","
                        << trx_name << std::endl;
        }

#pragma omp critical
        {
            std::printf("Done converting %s to %s\n",
                        input_file_names[i].c_str(),
                        output_file_names[i].c_str());
        }
    }

    // Leak memory of locks to save time
    {
        auto p = new decltype(locks)();
        p->swap(locks);
    }
}
