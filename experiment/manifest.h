#ifndef MANIFEST_H
#define MANIFEST_H

#include <cstdint>
#include <string>
#include <vector>


namespace tools {
  struct CorpusFile {
    std::string type;
    std::string subset;
    std::string file_name;
    std::string path;
    uint64_t size_bytes;
    std::string sha256;
    std::string archive_tier;
    std::string download_date;
  };

  std::vector<CorpusFile> read_manifest(const char* manifestCsvFilepath);
}

#endif
