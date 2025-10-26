#include <catch2/catch_amalgamated.hpp>
#include <fstream>
#include <string>
#include <sstream>

static std::string slurp(const std::string& path) {
  std::ifstream f(path);
  std::ostringstream ss; ss << f.rdbuf();
  return ss.str();
}

TEST_CASE("Determinism check produces identical checksums", "[det]") {
  // Run the nanohft binary with determinism-check for 3 seconds to out/det
  // Assume working dir is the build directory where the binary resides
  std::string cmd = std::string("./nanohft --determinism-check --duration-s 3 --report out/det > /dev/null 2>&1");
  int rc = std::system(cmd.c_str());
  REQUIRE(rc == 0);
  std::string content = slurp("out/det/determinism_result.json");
  REQUIRE(content.find("\"pass\": true") != std::string::npos);
}
