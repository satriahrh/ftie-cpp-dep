#include "manifest.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace tools {
  std::vector<CorpusFile> read_manifest(const char* manifestCsvFilepath) {
    std::ifstream file(manifestCsvFilepath);
    std::vector<CorpusFile> entries;

    std::string line;
    std::getline(file, line); // header: type,subset,file_name,path,size_bytes,sha256,archive_tier,download_date
    while (std::getline(file, line)) {
      if (line.empty()) continue;
      std::stringstream ss(line);

      CorpusFile entry;
      std::string sizeBytesStr;
      std::getline(ss, entry.type, ',');
      std::getline(ss, entry.subset, ',');
      std::getline(ss, entry.file_name, ',');
      std::getline(ss, entry.path, ',');
      std::getline(ss, sizeBytesStr, ',');
      std::getline(ss, entry.sha256, ',');
      std::getline(ss, entry.archive_tier, ',');
      std::getline(ss, entry.download_date, ',');

      entry.size_bytes = std::stoull(sizeBytesStr);
      std::transform(entry.sha256.begin(), entry.sha256.end(), entry.sha256.begin(),
        [](unsigned char c) { return std::tolower(c); });

      entries.push_back(entry);
    }
    return entries;
  }
}
