#include <list>
#include <string>
#include <map>
#include <Windows.h>
#include <vector>
#include <iostream>
#include <array>
#include <format>

using namespace std;

using Position = array<int, 2>;
using Cell = vector<string>;
using DataLayer = vector<Cell>;
using Grid = vector<DataLayer>;
using Size = array<unsigned int, 2>;

Grid globalGridMap;
Grid gridMap;

Size screenSize = {3, 3}; //y, x
Position position = {3, 3}; //y, x
Size levelDimensions = {5, 6}; //y, x
Size TileSize = {3, 3};

map<string, Cell> systemTiles = {
    {"solid", {"######", "######", "######"}},
    {"empty", {"      ", "      ", "      "}},
    {"error", {"eeeeee", "eeeeee", "eeeeee"}}
};
map<string, Cell> userTiles = {
    {"start", {"--  --", "  --  ", "--  --"}},
    {"player", {"  []  ", " +--+ ", " |  | "}}
};
DataLayer tiles = { // {type, fill, span}
    {"solid", "system", "7"},
    {"empty", "system", "4"},
    {"solid", "system", "2"},
    {"empty", "system", "1"},
    {"solid", "system", "2"},
    {"empty", "system", "1"},
    {"solid", "system", "2"},
    {"empty", "system", "1"},
    {"solid", "system", "1"},
    {"empty", "start", "1"},
    {"empty", "system", "1"},
    {"solid", "system", "2"},
    {"empty", "system", "1"},
    {"solid", "system", "4"}
};

string chainText(const string& str, const int& times) {
    string result;
    for (int i = 0; i < times; ++i) {
        result += str;
    }
    return result;
}

Position GetConsoleSize() { //y, x
    CONSOLE_SCREEN_BUFFER_INFO sbInfo;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbInfo);
    return {sbInfo.srWindow.Bottom - sbInfo.srWindow.Top, sbInfo.srWindow.Right - sbInfo.srWindow.Left};
}

void BuildGrid() {
    DataLayer row;
    for (Cell& cell : tiles) {
        for (int i = 0; i < stoi(cell[2]); ++i) {
            row.push_back({cell[0], cell[1]});
            if (row.size() == levelDimensions[1]) {
                globalGridMap.push_back(row);
                row.clear();
            }
        }
    }
    if (!row.empty()) {
        for (unsigned int i = 0; i < levelDimensions[1] - row.size(); ++i) {
            row.push_back({"error", "system"});
        }
        globalGridMap.push_back(row);
        row.clear();
    }
    if (globalGridMap.size() < levelDimensions[0]) {
        for (unsigned int i = 0; i < levelDimensions[0] - globalGridMap.size(); ++i) {
            for (unsigned int j = 0; j < levelDimensions[1]; ++j) {
                row.push_back({"error", "system"});
            }
            globalGridMap.push_back(row);
            row.clear();
        }
    }
    else if (globalGridMap.size() > levelDimensions[0]) {
        globalGridMap.resize(levelDimensions[0]);
    }
    gridMap = globalGridMap;
}

void render(const Position& cameraPosition = position, const Size& camera = screenSize) {
    int yTrasform = (camera[0] -1) / 2, xTrasform = (camera[0] -1) / 2;
    string buffer;

    auto consoleSize = GetConsoleSize();

    buffer += format("\033[{}A", consoleSize[0]);

    static int OldConsoleWdith = 0;
    string toCenter;

    //generating white space to center the image
    if (OldConsoleWdith != consoleSize[1]) {
        OldConsoleWdith = consoleSize[1];
        int tilesDrawn = (min(static_cast<int>(gridMap[0].size()) - 1, position[1] + xTrasform) - max(0, position[1] - xTrasform)) + 1;
        toCenter = chainText(" ", (consoleSize[1] - tilesDrawn * TileSize[1] * 2) / 2);
    }

    //rendering image
    for (int modY = -yTrasform; modY <= yTrasform; ++modY) {
        for (unsigned int layer = 0; layer < TileSize[0]; ++layer) {
            int Y = cameraPosition[0] + modY;
            if (Y >= (int)levelDimensions[0] || Y < 0) {
                continue;
            }
            buffer += toCenter;
            for (int modX = -xTrasform; modX <= xTrasform; ++modX) {
                Position drawPosition = {Y, cameraPosition[1] + modX};
                if (drawPosition[1] >= (int)gridMap[0].size() || drawPosition[1] < 0) {
                    continue;
                }
                else {
                    if (drawPosition == position) {
                        buffer += userTiles["player"][layer];
                        continue;
                    }
                    Cell cell = gridMap[drawPosition[0]][drawPosition[1]];
                    if (cell[1] == "system") {
                        buffer += systemTiles[cell[0]][layer];
                    }
                    else {
                        buffer += userTiles[cell[1]][layer];
                    }
               }
            }
            buffer += "\n";
        }
    }
    cout << buffer;
    cout.flush();
    gridMap = globalGridMap;
}

int main() {
    system("");
    BuildGrid();
    render();
    return 0;
}