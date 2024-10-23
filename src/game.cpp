#include <list>
#include <string>
#include <map>
#include <Windows.h>
#include <vector>
#include <iostream>
#include <array>
#include <format>
#include <variant>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

#include "../include/json.hpp"

using namespace std;

using Position = array<int, 2>;
using Vector = Position;
using Cell = vector<string>;
using DataLayer = vector<Cell>;
using Grid = vector<DataLayer>;
using Size = array<unsigned int, 2>;
using Event = map<string, variant<char, int>>;
using CellMap = map<string, Cell>;

using json = nlohmann::json;

Grid globalGridMap;
Grid gridMap;

Size screenSize = {3, 5}; //y, x
Position position = {3, 3}; //y, x
Size levelDimensions = {5, 6}; //y, x
Size TileSize = {3, 3};

//movement translation
set<string> possibleMovementTranslation = {"up", "down", "left", "right"};
map<string, char> directionText = {
    {"up", 'N'},
    {"down", 'S'},
    {"left", 'W'},
    {"right", 'E'}
};
map<char, Position> CompassDirection = {
    {'N', {-1, 0}},
    {'S', {1, 0}},
    {'W', {0, -1}},
    {'E', {0, 1}}
};

CellMap systemTiles = {
    {"solid", {"######", "######", "######"}},
    {"empty", {"      ", "      ", "      "}},
    {"error", {"eeeeee", "eeeeee", "eeeeee"}}
};
CellMap userTiles;
DataLayer tiles;

// *************** Helper functions ***************

Position GetConsoleSize() { //y, x
    CONSOLE_SCREEN_BUFFER_INFO sbInfo;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbInfo);
    return {sbInfo.srWindow.Bottom - sbInfo.srWindow.Top, sbInfo.srWindow.Right - sbInfo.srWindow.Left};
}

bool isInBounds (const Position positionToCheck) {
    return positionToCheck[0] >= 0 && positionToCheck[0] < (int)gridMap.size() && positionToCheck[1] >= 0 && positionToCheck[1] < (int)gridMap[0].size() ? true : false;
}
bool isTraversable (const Position positionToCheck) {
    return gridMap[positionToCheck[0]][positionToCheck[1]][0] == "empty" ? true : false;
}

string chainText(const string& str, const int& times) {
    string result;
    for (int i = 0; i < times; ++i) {
        result += str;
    }
    return result;
}

void clearLines (const unsigned int& amount, const bool& beforeAfter = true) {
    string lineString = chainText(" ", GetConsoleSize()[1]);

    if (beforeAfter) {
        cout << format("\u001b[{}A", amount);
    }

    for (unsigned int i = 0; i < amount; ++i) {
        cout << lineString << endl;
    }

    if (!beforeAfter) {
        cout << format("\u001b[{}B", amount);
    }
    
    cout.flush();
}

vector<string> splitText(const string& text, const char& separator) {
    if (text.empty()) {
        return {""};
    }
    vector<string> result;
    string buffer;
    for (const char& character : text) {
        if ( character == separator) {
            if (!buffer.empty()) {
                result.push_back(buffer);
                buffer.clear();
            }
        }
        else {
            buffer += character;
        }
    }
    if (!buffer.empty()) {
        result.push_back(buffer);
    }
    return result;
}

string toLowerString(const string& str) {
    string result;
    for (const char& character : str) {
        result += tolower(character);
    }
    return result;
}

void loadToCellmap(const json& jsonData, const string& jsonIdex, CellMap& endVariable) {
    for (auto customTile : jsonData[jsonIdex].items()) {
        endVariable[customTile.key()] = customTile.value().get<Cell>();
    }
}

void loadToDataLayer(const json& jsonData, const string& jsonIdex, DataLayer& endVariable) {
    for (auto tile : jsonData[jsonIdex]) {
        endVariable.push_back(tile.get<Cell>());
    }
}

// *************** end of helper functions ***************

string selectLevel(const string& levelDirectory = "../levels") {
    vector<string> levelNames;
    for (const auto& entry : filesystem::recursive_directory_iterator(levelDirectory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            levelNames.push_back(entry.path().filename().stem().string());
        }
    }
    while (true) {
        string selectedName;
        
        system("cls");

        cout << "Select a level:" << endl;

        for (auto name : levelNames) {
            cout << "\t>> " << name << endl;
        }

        cout << "\nEnter the name of the level to open: ";

        getline(cin, selectedName);

        if (find(levelNames.begin(), levelNames.end(), selectedName) != levelNames.end()) {
            system("cls");
            return selectedName;
        }
    }

}

bool loadLevel(const string& levelName, const string& levelDirectory = "../levels/", const string& fileExtension = ".json") {
    string filePath = levelDirectory + levelName + fileExtension;

    ifstream file(filePath);

    json levelData; // << setw(4) << adds spaces to make it more readable for output

    try {
        levelData = json::parse(file);
    }
    catch (...) {
        file.close();
        return false;
    }
    
    file.close();

    loadToDataLayer(levelData, "level", tiles);
    loadToCellmap(levelData, "user_tiles", userTiles);

    return true;
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
    int yTrasform = (camera[0] -1) / 2, xTrasform = (camera[1] -1) / 2;
    string buffer;

    auto consoleSize = GetConsoleSize();

    buffer += format("\033[{}A", consoleSize[0]);

    static int OldConsoleWdith = 0;
    static string toCenter;

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

void move(const Vector& direction, const int& tiles) {
    for (int i = 0; i < tiles; ++i) {
        Position newPosition = {position[0] + direction[0], position[1] + direction[1]};
        if (isInBounds(newPosition)) {
            if (isTraversable(newPosition)) {
                position = newPosition;
            }
            else {
                break;
            }
        }
        else {
            break;
        }

        render();

        if (i < tiles - 1) {
            Sleep(500);
        }
    }
}

Event commandHandler() {
    Event event;
    event["id"] = 0;

    // cleanup
    clearLines(3, false);

    // input handling
    cout << "\t $ ";
    string input;
    getline(cin, input);
    vector<string> command = splitText(toLowerString(input), ' ');

    // command and event handling
    if (command[0] == "move") {
        if (command.size() >= 2) {
            if (possibleMovementTranslation.find(command[1]) != possibleMovementTranslation.end()) {
                event["move"] = directionText[command[1]];
                event["id"] = 1;
            }
        }
        if (command.size() >= 3) {
            try {
                event["tiles"] = stoi(command[2]);
            }
            catch (...) {
                event["tiles"] = 1;
            }
        }
        else {
            event["tiles"] = 1;
        }
    }
    else if (command[0] == "clear") {
        system("CLS");
    }
    else if (command[0] = "exit") {
        exit(0);
    }

    clearLines(3, true);

    return event;
}

int main() {
    //initializing settings
    system("");
    cout << selectLevel();
    loadLevel(selectLevel());
    BuildGrid();

    //event loop
    while (true) {
        render();

        Event event = commandHandler();
        int eventId = get<int>(event["id"]);

        // event hande
        if (eventId == 0) { // get<Type> -> specificaly for variant, used for getting a specific value out of it.
            continue; // just for clarity...
        }
        else if (eventId == 1) {
            move(CompassDirection[get<char>(event["move"])], get<int>(event["tiles"]));
        }
    }
    return 0;
}