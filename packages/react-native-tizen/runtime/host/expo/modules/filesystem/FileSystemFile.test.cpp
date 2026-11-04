#include "FileSystemTest.test.h"

#include <doctest/doctest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace rntizen::expo {

namespace {

namespace fs = std::filesystem;

std::string contents(const fs::path &file) {
  std::ifstream in(file, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

} // namespace

TEST_CASE("a File is created empty, then written, appended and read as text, base64 and bytes") {
  testing::ExpoTestRuntime expo;
  testing::FileSystemScope scope(expo, "file-io");
  expo.eval("globalThis.f = new FS.FileSystemFile(root + 'note.txt')");

  CHECK(expo.json("[f.exists, f.size, f.md5]") == "[false,0,null]");
  expo.eval("f.create()");
  CHECK(expo.json("[f.exists, f.size, f.md5]") == R"([true,0,"d41d8cd98f00b204e9800998ecf8427e"])");

  expo.eval("f.writeSync('héllo')");
  expo.eval("f.writeSync(' wörld', { append: true })");
  CHECK(contents(scope.directory() / "note.txt") == "h\xc3\xa9llo w\xc3\xb6rld");
  CHECK(expo.json("[f.textSync(), f.size, f.md5, f.base64Sync()]") ==
        R"(["héllo wörld",13,"ed0c22cc110ede12327851863c078138","aMOpbGxvIHfDtnJsZA=="])");

  expo.eval("f.writeSync('AQL/', { encoding: 'base64' })");
  CHECK(expo.json("Array.from(f.bytesSync())") == "[1,2,255]");
  expo.eval("f.writeSync(new Uint8Array([104, 105]))");
  CHECK(expo.json("f.textSync()") == R"("hi")");
  expo.eval("f.writeSync(new Uint8Array([33]).buffer, { append: true })");
  CHECK(contents(scope.directory() / "note.txt") == "hi!");

  CHECK(expo.settle("f.write('async')") == "undefined");
  CHECK(expo.settle("f.text()") == R"("async")");
  CHECK(expo.settle("f.base64()") == R"("YXN5bmM=")");
  CHECK(expo.settle("f.bytes().then(bytes => Array.from(bytes))") == "[97,115,121,110,99]");
  CHECK(expo.json("f.uri === root + 'note.txt'") == "true");
}

TEST_CASE("a File's info and times describe it on disk, with an MD5 only when asked") {
  testing::ExpoTestRuntime expo;
  testing::FileSystemScope scope(expo, "file-info");
  std::ofstream(scope.directory() / "data.json") << "{}";
  expo.eval("globalThis.f = new FS.FileSystemFile(root + 'data.json')");

  CHECK(expo.json("Object.keys(f.info()).sort()") == R"(["exists","modificationTime","size","uri"])");
  CHECK(expo.json("[f.info().exists, f.info().size, f.info().uri === f.uri]") == "[true,2,true]");
  CHECK(expo.json("f.info({ md5: true }).md5") == R"("99914b932bd37a50b983c5e7c90ae93b")");
  CHECK(expo.json("'md5' in f.info({ md5: false })") == "false");
  const auto written = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::clock_cast<std::chrono::system_clock>(fs::last_write_time(scope.directory() / "data.json"))
                               .time_since_epoch())
                           .count();
  CHECK(expo.eval("f.modificationTime").asNumber() == static_cast<double>(written));
  CHECK(expo.json("[f.lastModified === f.modificationTime, f.creationTime, f.type]") == R"([true,null,"application/json"])");

  expo.eval("globalThis.missing = new FS.FileSystemFile(root + 'missing.txt')");
  CHECK(expo.json("[missing.info().exists, missing.info().uri === root + 'missing.txt', Object.keys(missing.info()).length]") ==
        "[false,true,2]");
  CHECK(expo.json("[missing.modificationTime, missing.size, missing.md5]") == "[null,0,null]");
}

TEST_CASE("type follows the extension in any case, and is null for one it does not know") {
  testing::ExpoTestRuntime expo;
  testing::FileSystemScope scope(expo, "file-type");
  CHECK(expo.json("['a.PNG', 'b.jpeg', 'c.mp4', 'd.woff2', 'e.bin', 'noextension'].map(name => new FS.FileSystemFile(root + name).type)") ==
        R"(["image/png","image/jpeg","video/mp4","font/woff2",null,null])");
}

TEST_CASE("a missing File fails to read and to delete with Expo's codes") {
  testing::ExpoTestRuntime expo;
  testing::FileSystemScope scope(expo, "file-missing");
  expo.eval("globalThis.f = new FS.FileSystemFile(root + 'gone.txt')");
  const auto uri = "file://" + scope.directory().string() + "/gone.txt";
  CHECK(expo.json("fails(() => f.textSync())") == "\"ERR_UNABLE_TO_READ: Unable to read " + uri + "\"");
  CHECK(expo.json("fails(() => f.delete())") ==
        "\"ERR_UNABLE_TO_DELETE: Unable to delete '" + uri + "': path does not exist\"");
  CHECK_THROWS_WITH(expo.settle("f.text()"), ("ERR_UNABLE_TO_READ: Unable to read " + uri).c_str());
  CHECK(expo.json("fails(() => f.writeSync('x', { append: true }))") == "\"no error\"");
  CHECK(expo.json("f.textSync()") == R"("x")");
  expo.eval("f.delete()");
  CHECK(expo.json("f.exists") == "false");
}

TEST_CASE("create refuses an existing File unless told to overwrite, and makes its parents when asked") {
  testing::ExpoTestRuntime expo;
  testing::FileSystemScope scope(expo, "file-create");
  expo.eval("globalThis.f = new FS.FileSystemFile(root + 'once.txt'); f.create(); f.writeSync('kept')");
  const auto uri = "file://" + scope.directory().string() + "/once.txt";
  CHECK(expo.json("fails(() => f.create())") == "\"ERR_FILE_EXISTS: A file already exists at " + uri + "\"");
  CHECK(expo.json("f.textSync()") == R"("kept")");
  expo.eval("f.create({ overwrite: true })");
  CHECK(expo.json("f.size") == "0");

  expo.eval("globalThis.deep = new FS.FileSystemFile(root + 'a/b/c.txt')");
  CHECK(expo.json("fails(() => deep.create())") ==
        "\"ERR_UNABLE_TO_CREATE: Unable to create file://" + scope.directory().string() + "/a/b/c.txt\"");
  expo.eval("deep.create({ intermediates: true })");
  CHECK(fs::is_regular_file(scope.directory() / "a" / "b" / "c.txt"));
  CHECK(expo.json("fails(() => new FS.FileSystemFile(root + 'none/x.txt').writeSync('x', { append: true }))") ==
        "\"ERR_UNABLE_TO_WRITE: Unable to write file://" + scope.directory().string() + "/none/x.txt\"");
}

TEST_CASE("validatePath refuses a directory path and a directory already on disk") {
  testing::ExpoTestRuntime expo;
  testing::FileSystemScope scope(expo, "file-validate");
  fs::create_directories(scope.directory() / "sub");
  CHECK(expo.json("fails(() => new FS.FileSystemFile(root + 'dir/').validatePath())") ==
        R"("ERR_WRONG_TYPE: Tried to create a file with a directory path")");
  CHECK(expo.json("fails(() => new FS.FileSystemFile(root + 'sub').validatePath())") ==
        "\"ERR_WRONG_TYPE: A directory exists at file://" + scope.directory().string() + "/sub\"");
  CHECK(expo.json("fails(() => new FS.FileSystemFile(root + 'plain.txt').validatePath())") == R"("no error")");
}

TEST_CASE("a File copies, moves and renames to a File or into a Directory") {
  testing::ExpoTestRuntime expo;
  testing::FileSystemScope scope(expo, "file-copy");
  const auto &root = scope.directory();
  fs::create_directories(root / "into");
  expo.eval("globalThis.f = new FS.FileSystemFile(root + 'a.txt'); f.writeSync('a');"
            "globalThis.into = new FS.FileSystemDirectory(root + 'into')");

  expo.eval("f.copySync(new FS.FileSystemFile(root + 'b.txt'))");
  CHECK(contents(root / "b.txt") == "a");
  CHECK(expo.json("fails(() => f.copySync(new FS.FileSystemFile(root + 'b.txt'))) === 'no error'") == "false");
  expo.eval("f.writeSync('newer'); f.copySync(new FS.FileSystemFile(root + 'b.txt'), { overwrite: true })");
  CHECK(contents(root / "b.txt") == "newer");
  expo.eval("f.copySync(into)");
  CHECK(contents(root / "into" / "a.txt") == "newer");
  CHECK(expo.settle("f.copy(new FS.FileSystemFile(root + 'c.txt'))") == "undefined");
  CHECK(contents(root / "c.txt") == "newer");

  expo.eval("f.moveSync(new FS.FileSystemFile(root + 'moved.txt'))");
  CHECK(expo.json("f.uri === root + 'moved.txt'") == "true");
  CHECK_FALSE(fs::exists(root / "a.txt"));
  expo.eval("f.moveSync(new FS.FileSystemFile(root + 'b.txt'), { overwrite: true })");
  CHECK(expo.json("[f.uri === root + 'b.txt', f.textSync()]") == R"([true,"newer"])");
  CHECK(expo.settle("f.move(new FS.FileSystemDirectory(root + 'into'), { overwrite: true })") == "undefined");
  CHECK(expo.json("f.uri === root + 'into/b.txt'") == "true");
  expo.eval("f.rename('renamed.txt')");
  CHECK(expo.json("f.uri === root + 'into/renamed.txt'") == "true");
  CHECK(contents(root / "into" / "renamed.txt") == "newer");
}

TEST_CASE("a File's URI decodes percent escapes for the path and encodes them back") {
  testing::ExpoTestRuntime expo;
  testing::FileSystemScope scope(expo, "file-names");
  expo.eval("globalThis.f = new FS.FileSystemFile(root + 'two%20words%23%3F%25.txt'); f.writeSync('x')");
  CHECK(fs::is_regular_file(scope.directory() / "two words#?%.txt"));
  CHECK(expo.json("f.uri === root + 'two%20words%23%3F%25.txt'") == "true");
  expo.eval("globalThis.g = new FS.FileSystemFile(root + 'caf\\u00e9.txt?x=1#y')");
  CHECK(expo.json("g.uri === root + 'caf%C3%A9.txt'") == "true");
  CHECK(expo.json("new FS.FileSystemFile('" + scope.directory().string() + "/bare.txt').uri === root + 'bare.txt'") == "true");
}

TEST_CASE("a File refuses a URI that is not a local file") {
  testing::ExpoTestRuntime expo;
  testing::FileSystemScope scope(expo, "file-uri");
  CHECK(expo.json("fails(() => new FS.FileSystemFile('https://example.com/a.txt'))").find("is not a local file URI") !=
        std::string::npos);
  CHECK(expo.json("fails(() => new FS.FileSystemFile(42))") != R"("no error")");
}

} // namespace rntizen::expo
