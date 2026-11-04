#include "FileSystemTest.test.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>

namespace rntizen::expo {

namespace {

namespace fs = std::filesystem;

void write(const fs::path &file, const std::string &text) {
  fs::create_directories(file.parent_path());
  std::ofstream(file, std::ios::binary) << text;
}

} // namespace

TEST_CASE("a Directory is created once, idempotently, with its parents, or over what is there") {
  testing::ExpoTestRuntime expo;
  testing::FileSystemScope scope(expo, "directory-create");
  const auto &root = scope.directory();
  expo.eval("globalThis.d = new FS.FileSystemDirectory(root + 'made')");

  CHECK(expo.json("[d.exists, d.uri === root + 'made/']") == "[false,true]");
  expo.eval("d.create()");
  CHECK(fs::is_directory(root / "made"));
  CHECK(expo.json("fails(() => d.create())") ==
        "\"ERR_DIRECTORY_EXISTS: A file or directory already exists at file://" + root.string() + "/made/\"");
  CHECK(expo.json("fails(() => d.create({ idempotent: true }))") == R"("no error")");
  write(root / "made" / "inside.txt", "x");
  expo.eval("d.create({ overwrite: true })");
  CHECK(fs::is_empty(root / "made"));

  write(root / "plain", "a file");
  CHECK(expo.json("fails(() => new FS.FileSystemDirectory(root + 'plain').create({ idempotent: true }))")
            .starts_with("\"ERR_DIRECTORY_EXISTS"));

  expo.eval("globalThis.deep = new FS.FileSystemDirectory(root + 'a/b/c/')");
  CHECK(expo.json("fails(() => deep.create())").starts_with("\"ERR_UNABLE_TO_CREATE: Unable to create file://" + root.string() + "/a/b/c/: "));
  expo.eval("deep.create({ intermediates: true })");
  CHECK(fs::is_directory(root / "a" / "b" / "c"));
  CHECK(expo.json("fails(() => deep.validatePath())") == R"("no error")");
}

TEST_CASE("a Directory lists its entries, adds up its size and reports its info") {
  testing::ExpoTestRuntime expo;
  testing::FileSystemScope scope(expo, "directory-list");
  const auto &root = scope.directory();
  write(root / "tree" / "one.txt", "123");
  write(root / "tree" / "nested" / "two.txt", "45678");
  expo.eval("globalThis.d = new FS.FileSystemDirectory(root + 'tree/')");

  CHECK(expo.json("d.size") == "8");
  CHECK(expo.json("d.listAsRecords().map(r => [r.uri.slice(root.length), r.isDirectory]).sort()") ==
        R"([["tree/nested/",true],["tree/one.txt",false]])");
  CHECK(expo.json("[d.info().exists, d.info().size, d.info().files.sort(), typeof d.info().modificationTime]") ==
        R"([true,8,["nested","one.txt"],"number"])");

  expo.eval("globalThis.gone = new FS.FileSystemDirectory(root + 'gone')");
  CHECK(expo.json("[gone.exists, gone.size, gone.info().exists, gone.info().uri === root + 'gone/']") ==
        "[false,null,false,true]");
  CHECK(expo.json("fails(() => gone.listAsRecords())").starts_with("\"ERR_UNABLE_TO_LIST: Unable to list file://" + root.string() + "/gone/: "));
  CHECK(expo.json("fails(() => gone.delete())") ==
        "\"ERR_UNABLE_TO_DELETE: Unable to delete 'file://" + root.string() + "/gone/': path does not exist\"");
}

TEST_CASE("a Directory creates files and directories in itself") {
  testing::ExpoTestRuntime expo;
  testing::FileSystemScope scope(expo, "directory-children");
  const auto &root = scope.directory();
  fs::create_directories(root / "box");
  expo.eval("globalThis.d = new FS.FileSystemDirectory(root + 'box')");

  CHECK(expo.json("(() => { const f = d.createFile('new file.txt', 'text/plain'); f.writeSync('hi');"
                  " return [f instanceof FS.FileSystemFile, f.uri === root + 'box/new%20file.txt', f.textSync()]; })()") ==
        R"([true,true,"hi"])");
  CHECK(fs::is_regular_file(root / "box" / "new file.txt"));
  CHECK(expo.json("fails(() => d.createFile('new file.txt', null))") ==
        "\"ERR_FILE_EXISTS: A file already exists at file://" + root.string() + "/box/new%20file.txt\"");
  CHECK(expo.json("(() => { const child = d.createDirectory('child');"
                  " return [child instanceof FS.FileSystemDirectory, child.uri === root + 'box/child/', child.exists]; })()") ==
        "[true,true,true]");
  CHECK(expo.json("fails(() => d.createDirectory('child')).startsWith('ERR_DIRECTORY_EXISTS')") == "true");
}

TEST_CASE("a Directory copies, moves and renames into another Directory, never onto a File") {
  testing::ExpoTestRuntime expo;
  testing::FileSystemScope scope(expo, "directory-copy");
  const auto &root = scope.directory();
  write(root / "source" / "a.txt", "a");
  write(root / "source" / "deep" / "b.txt", "b");
  fs::create_directories(root / "target");
  write(root / "file.txt", "f");
  expo.eval("globalThis.d = new FS.FileSystemDirectory(root + 'source');"
            "globalThis.target = new FS.FileSystemDirectory(root + 'target')");

  expo.eval("d.copySync(target)");
  CHECK(fs::is_regular_file(root / "target" / "source" / "deep" / "b.txt"));
  CHECK(expo.json("fails(() => d.copySync(new FS.FileSystemFile(root + 'file.txt')))") ==
        R"("ERR_COPY_OR_MOVE: Cannot copy or move a directory to a file")");
  write(root / "source" / "a.txt", "newer");
  expo.eval("d.copySync(target, { overwrite: true })");
  CHECK(fs::file_size(root / "target" / "source" / "a.txt") == 5);
  fs::create_directories(root / "copies");
  CHECK(expo.settle("d.copy(new FS.FileSystemDirectory(root + 'copies'))") == "undefined");
  CHECK(fs::is_regular_file(root / "copies" / "source" / "deep" / "b.txt"));

  fs::create_directories(root / "elsewhere");
  expo.eval("d.moveSync(new FS.FileSystemDirectory(root + 'elsewhere'))");
  CHECK(expo.json("d.uri === root + 'elsewhere/source/'") == "true");
  CHECK_FALSE(fs::exists(root / "source"));
  CHECK(expo.settle("d.move(target, { overwrite: true })") == "undefined");
  CHECK(expo.json("[d.uri === root + 'target/source/', d.exists]") == "[true,true]");
  expo.eval("d.rename('renamed')");
  CHECK(expo.json("d.uri === root + 'target/renamed/'") == "true");
  CHECK(fs::is_regular_file(root / "target" / "renamed" / "deep" / "b.txt"));
  expo.eval("d.delete()");
  CHECK_FALSE(fs::exists(root / "target" / "renamed"));
}

TEST_CASE("a Directory keeps a child's name whole, with the characters a URI escapes") {
  testing::ExpoTestRuntime expo;
  testing::FileSystemScope scope(expo, "directory-names");
  const auto &root = scope.directory();
  fs::create_directories(root / "names");
  expo.eval("globalThis.d = new FS.FileSystemDirectory(root + 'names')");

  CHECK(expo.json("(() => { const f = d.createFile('a#b?c%41 d.txt', null); f.writeSync('x');"
                  " return f.uri.slice(root.length); })()") == R"("names/a%23b%3Fc%2541%20d.txt")");
  CHECK(fs::is_regular_file(root / "names" / "a#b?c%41 d.txt"));
  expo.eval("d.createDirectory('sub #1')");
  CHECK(fs::is_directory(root / "names" / "sub #1"));
  CHECK(expo.json("d.listAsRecords().map(r => r.uri.slice(root.length)).sort()") ==
        R"(["names/a%23b%3Fc%2541%20d.txt","names/sub%20%231/"])");
  CHECK(expo.json("d.listAsRecords().every(r => (r.isDirectory ? new FS.FileSystemDirectory(r.uri) : new FS.FileSystemFile(r.uri)).exists)") ==
        "true");
}

} // namespace rntizen::expo
