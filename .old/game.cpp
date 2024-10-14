#include <iostream>
#include <list>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <format>
#include <string>
#include <map>
#include <Windows.h>
#include <algorithm>
#include <regex>
#include <array>
#include <format>
#include <set>

using namespace std;

namespace fs = std::filesystem;

using DataLayer = list<list<string>>;
using Grid = list<DataLayer>;

Grid globalGridMap;
Grid gridMap;
set<string> expecterFiles = {"tiles", "custom_tiles", "data_simple", "data_array"};

int screenSize[2] = {5, 5}; //y, x
int levelHeight = 3; 
int tileHeight = 3; // must be the same as the amount of lines in systemTiles and userTiles
int position[2] = {0, 0}; //y, x
map<string, list<string>> systemTiles = {
    {"solid", {"######", "######", "######"}},
    {"empty", {"      ", "      ", "      "}},
    {"error", {"eeeeee", "eeeeee", "eeeeee"}}
};
map<string, list<string>> userTiles = {
    {"customEmpty", {"--  --", "  --  ", "--  --"}},
    {"player", {"  []  ", " +--+ ", " |  | "}}
};
DataLayer tiles = { // {type, fill, span}
    {"solid", "system", "8"},
    {"empty", "customEmpty", "1"}
};

template <typename T>
T index(const list<T>& lst, const unsigned int& index) {
    auto it = lst.begin();
    if (index >= lst.size()) {
        throw out_of_range("Index out of range");
    }
    advance(it, index);
    return *it;
}

string chainString(const string& text, const unsigned int& times) {
    string output;
    for (int unsigned i = 0; i < times; ++i) {
        output += text;
    }
    return output;
}

Grid parseTiles(const DataLayer& tiles, const unsigned int& width) { // unsigned means positive integers (sign meaning "-")
    Grid gridMap;
    DataLayer row;
    for (const list<string>& tile : tiles) {
        for (int i = 0; i < stoi(index(tile, 2)); ++i) {
            row.push_back({index(tile, 0), index(tile, 1)});
            if (row.size() == width) {
                gridMap.push_back(row);
                row.clear();
            }
        }
    }
    if (!row.empty()) {
        for (int i = 0; i < width - row.size(); ++i) {
            row.push_back({"error", "system"});
        }
        gridMap.push_back(row);
        row.clear();
    }
    return gridMap;
}

array<int, 2> getConsoleInfo() {
    CONSOLE_SCREEN_BUFFER_INFO sbInfo;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbInfo);
    return {sbInfo.srWindow.Bottom - sbInfo.srWindow.Top, sbInfo.srWindow.Right - sbInfo.srWindow.Left};
};

void renderGrid(const int position[2], int displaySize[2] = screenSize, const int& blockHeight = tileHeight, const int& tileWidth = 6, const map<string, list<string>>& defaultTiles = systemTiles) {
    if (displaySize[0] % 2 == 0) {
        ++displaySize[0];
    }
    if (displaySize[1] % 2 == 0) {
        ++displaySize[1];
    }

    auto consoleInfo = getConsoleInfo();
    
    string pushCenter = chainString(" ", (consoleInfo[1] - (displaySize[1] * tileWidth)) / 2);

    string displayBuffer;

    displayBuffer += format("\033[{}A", consoleInfo[0]);

    for (int y = 0; y < displaySize[0]; ++y) {
        int modY = position[0] - (displaySize[0] - 1) / 2 + y;
        for (int line = 0; line < blockHeight; ++line) {
            if (modY == levelHeight) {
                return;
            }
            displayBuffer += pushCenter;
            for (int x = 0; x < displaySize[1]; ++x) {
                int modX = position[1] - (displaySize[1] - 1) / 2 + x;
                list<string> tile;
                try { 
                    tile = index(index(gridMap, modY), modX);
                }
                catch (out_of_range) {
                    continue;
                }
                if (index(tile, 1) == "system") {
                    displayBuffer += index(defaultTiles.at(index(tile, 0)), line);
                }
                else {
                    displayBuffer += index(userTiles.at(index(tile, 1)), line);
                }
            }
            displayBuffer += "\n";
        }
    }
    cout << displayBuffer;
    cout.flush();
    gridMap = globalGridMap;
}

template <typename T>
bool contains(const list<T>& lst, const string target) {
    for (const auto& item : lst) {
        if (item == target) {
            return true;
        }
    }
    return false;
}

bool endsWith(const string& fullString, const string& target) {
    if (fullString.length() > target.length()) {
        if (fullString.compare(fullString.length() - target.length() - 1, target.length(), target) == 0) {
            return true;
        }
    }
    return false;
}

void loadLevel(const string& levelsDirectory = "levels") {
    list<string> files;
    for (auto file : fs::directory_iterator(levelsDirectory)) {
        if (endsWith(file.path().generic_string(), "_level")) {
            files.push_back(file.path().generic_string());
        }
    }
}

int main() {
    system("");
    globalGridMap = parseTiles(tiles, levelHeight);
    gridMap = globalGridMap;
    while (true) {
        renderGrid(position);
        system("pause");
    }
    return 0;
}