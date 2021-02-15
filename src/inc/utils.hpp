#include "xxhash.hpp"
#include "Chrono.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace utl
{

    constexpr std::size_t BUFF_SIZE = 65536;
    const std::string VERSION = "0.0.1";

    struct Options {
        bool del;
        bool hashing;
        bool recursive;
        bool verbose;

        Options()
            : del(true)
            , hashing(true)
            , recursive(true)
            , verbose(true)
        {}
    };

    struct FileInfo {
        std::string name;
        std::string partialName;
        bool isDir;

        FileInfo(const std::string& n, const std::string& pn)
            : name(n)
            , partialName(pn)
            , isDir(false)
        {}

        FileInfo(const std::string& n, const std::string& pn, const bool isDir)
            : name(n)
            , partialName(pn)
            , isDir(isDir)
        {}

        friend bool operator < (const FileInfo& l, const FileInfo& r)
        {
            return l.partialName < r.partialName;
        }
    };

    xxh::hash_t<64> hash(const std::string& file)
    {
        std::vector<char> buffer(BUFF_SIZE);
        xxh::hash_state_t<64> hash_stream;
        std::ifstream stream(file, std::ios::in|std::ios::binary);

        do {
            stream.read(&buffer[0], buffer.size());
            hash_stream.update(buffer);
        } while(stream);

        return hash_stream.digest();
    }

    void getFiles(const fs::path& pathToShow, std::vector<FileInfo>& sources)
    {
        if(fs::exists(pathToShow)) {
            for(const auto& entry : fs::directory_iterator(pathToShow)) {
                fs::path filename = entry.path();
                std::string fn(filename.generic_string());
                std::string fn2(filename.generic_string().substr(pathToShow.generic_string().size()));
                if(fs::is_directory(entry.status())) {
                    sources.emplace_back(FileInfo(fn, fn2, true));
                    getFiles(entry, sources);
                } else if(fs::is_regular_file(entry.status())) {
                    sources.emplace_back(FileInfo(fn, fn2));
                }
            }
        }
    }

    fs::path dirPathFromString(const std::string& str)
    {
        return fs::path(str.back() != '/' ? str + "/" : str);
    }

    bool sameHash(const fs::path& source, const fs::path& target)
    {
        xxh::hash_t<64> crcSrc = hash(source.generic_string());
        xxh::hash_t<64> crcTgt = hash(target.generic_string());
        return crcSrc == crcTgt;
    }

    bool sameHash(const xxh::hash_t<64> hash64, const fs::path& target)
    {
        return hash64 == hash(target.generic_string());
    }

    void clean(std::vector<FileInfo>& sources, const std::string& target, const Options& opt)
    {
        std::vector<FileInfo> targets;
        targets.reserve(sources.size());
        getFiles(dirPathFromString(target), targets);
        std::sort(sources.begin(), sources.end());

        for(FileInfo s : targets) {
            if(!std::binary_search(sources.begin(), sources.end(), s)) {
                fs::path p(s.name);
                if(opt.verbose) {
                    std::cout << "Deleting " << p.generic_string() << "\n";
                }
                fs::remove(p);
            }
        }
    }

    std::string formatBytes(const std::uintmax_t bytes)
    {
        std::uintmax_t tb = 1099511627776;
        std::uintmax_t gb = 1073741824;
        std::uintmax_t mb = 1048576;
        std::uintmax_t kb = 1024;

        if (bytes >= tb) {
            return std::to_string(bytes / tb) + " TB";
        }
        else if (bytes >= gb && bytes < tb) {
            return std::to_string(bytes / gb) + " GB";
        } else if (bytes >= mb && bytes < gb) {
            return std::to_string(bytes / mb) + " MB";
        } else if (bytes >= kb && bytes < mb) {
            return std::to_string(bytes / kb) + "KB";
        } else if (bytes < kb) {
            return std::to_string(bytes) + " B";
        } else {
            return std::to_string(bytes) + " B";
        }
    }

    void sync(const std::string& s, const std::string& t, const Options& opt)
    {
        fs::path source = dirPathFromString(s);
        fs::path target = dirPathFromString(t);
        std::vector<FileInfo> sources;
        std::uintmax_t totalSize = 0;
        std::size_t totalFiles = 0;
        std::size_t totalErrors = 0;

        if(!fs::exists(target)) {
            fs::create_directory(target);
        }

        Chrono<double, std::ratio<1>> chrono;
        chrono.start();

        getFiles(dirPathFromString(source), sources);

        for(FileInfo s : sources) {
            fs::path src = fs::path(s.name);
            fs::path tgt = target.generic_string().append(s.name.substr(source.generic_string().size()));

            std::string curFile(s.name.substr(source.generic_string().size()));

            if(s.isDir) {
                if(!fs::exists(tgt)) {
                    if(opt.verbose) {
                        std::cout << "Creating dir " << tgt.generic_string() << "...\n";
                    }
                    fs::create_directory(tgt);
                }
            } else {
                if(!fs::exists(tgt)) {
                    if(opt.verbose) {
                        std::cout << "Copying " << curFile << " (" << src.extension() << ") ...\n";
                    }
                    totalSize += std::filesystem::file_size(src);
                    totalFiles++;
                    fs::copy(src, tgt);
                    if(!sameHash(src, tgt)) {
                        totalErrors++;
                        std::cout << "Error " << src << std::endl;
                    }
                } else {
                    bool resync = std::filesystem::file_size(src) != std::filesystem::file_size(tgt);
                    xxh::hash_t<64> srcHash;
                    if(!resync) {
                        srcHash = hash(src.generic_string());
                        resync = !sameHash(srcHash, tgt);
                    }
                    if(resync) {
                        if(opt.verbose) {
                            std::cout << "Replacing " << curFile << "...\n";
                        }
                        totalSize += std::filesystem::file_size(src);
                        totalFiles++;
                        fs::remove(tgt);
                        fs::copy(src, tgt);
                        if(!sameHash(srcHash, tgt)) {
                            totalErrors++;
                            std::cout << "Error " << src << std::endl;
                        }
                    }
                }
            }
        }

        if(opt.del) {
            clean(sources, target, opt);
        }

        chrono.stop();
        std::cout << "Total files synced: " << totalFiles << " (" << formatBytes(totalSize) << ")\n";
        std::cout << "Elapsed time: " << chrono.elapsedTime() << " seconds" << std::endl;
        std::cout << "Error: " << totalErrors << std::endl;
    }

}
