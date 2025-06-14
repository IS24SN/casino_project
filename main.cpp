#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>

// === COMPONENT INTERFACE ===
class Component {
public:
    virtual ~Component() = default;
    virtual void display(int indent = 0) = 0;
    virtual double getRevenue() = 0;
    virtual void save(std::ofstream& out, int depth = 0) = 0;
    virtual bool isGroup() = 0;
};

// === LEAF: GAME ===
class Game : public Component {
private:
    std::string name;
    double revenue;
public:
    Game(std::string n, double r) : name(n), revenue(r) {}

    void display(int indent = 0) override {
        for (int i = 0; i < indent; ++i) std::cout << "  ";
        std::cout << name << " | Revenue: " << revenue << std::endl;
    }

    double getRevenue() override { return revenue; }

    void save(std::ofstream& out, int depth = 0) override {
        for (int i = 0; i < depth; ++i) out << "  ";
        out << "GAME " << name << " " << revenue << std::endl;
    }

    bool isGroup() override { return false; }

    std::string getName() { return name; }
    void addRevenue(double amount) { revenue += amount; }
};

// === COMPOSITE: GROUP ===
class Group : public Component {
private:
    std::string name;
    std::vector<std::unique_ptr<Component>> children;
public:
    Group(std::string n) : name(n) {}

    void add(std::unique_ptr<Component> component) {
        children.push_back(std::move(component));
    }

    void display(int indent = 0) override {
        for (int i = 0; i < indent; ++i) std::cout << "  ";
        std::cout << "----- " << name << " -----" << std::endl;
        for (auto& child : children) {
            child->display(indent + 1);
        }
        for (int i = 0; i < indent; ++i) std::cout << "  ";
        std::cout << "Total: " << getRevenue() << std::endl;
    }

    double getRevenue() override {
        double total = 0.0;
        for (auto& child : children) {
            total += child->getRevenue();
        }
        return total;
    }

    void save(std::ofstream& out, int depth = 0) override {
        for (int i = 0; i < depth; ++i) out << "  ";
        out << "GROUP " << name << std::endl;
        for (auto& child : children) {
            child->save(out, depth + 1);
        }
    }

    bool isGroup() override { return true; }

    std::string getName() { return name; }
    std::vector<Component*> getChildren() {
        std::vector<Component*> result;
        for (auto& child : children) {
            result.push_back(child.get());
        }
        return result;
    }

    std::vector<Game*> getAllGames() {
        std::vector<Game*> games;
        for (auto& child : children) {
            if (child->isGroup()) {
                auto subGames = static_cast<Group*>(child.get())->getAllGames();
                games.insert(games.end(), subGames.begin(), subGames.end());
            } else {
                games.push_back(static_cast<Game*>(child.get()));
            }
        }
        return games;
    }
};

// === FILE OPERATIONS ===
int getDepth(const std::string& line) {
    int depth = 0;
    for (size_t i = 0; i + 1 < line.size() && line[i] == ' ' && line[i + 1] == ' '; i += 2) {
        depth++;
    }
    return depth;
}

std::string trimSpaces(const std::string& line) {
    size_t pos = 0;
    while (pos + 1 < line.size() && line[pos] == ' ' && line[pos + 1] == ' ') {
        pos += 2;
    }
    return line.substr(pos);
}

std::unique_ptr<Component> parse(const std::vector<std::string>& lines, size_t& index) {
    if (index >= lines.size()) return nullptr;

    std::string line = lines[index];
    if (line.empty()) {
        index++;
        return parse(lines, index);
    }

    int currentDepth = getDepth(line);
    std::string trimmed = trimSpaces(line);

    if (trimmed.substr(0, 6) == "GROUP ") {
        std::string groupName = trimmed.substr(6);
        auto group = std::make_unique<Group>(groupName);
        index++;

        while (index < lines.size()) {
            if (lines[index].empty()) {
                index++;
                continue;
            }

            int nextDepth = getDepth(lines[index]);
            if (nextDepth <= currentDepth) break;
            if (nextDepth == currentDepth + 1) {
                auto child = parse(lines, index);
                if (child) group->add(std::move(child));
            } else {
                index++;
            }
        }
        return group;
    }
    else if (trimmed.substr(0, 5) == "GAME ") {
        std::istringstream iss(trimmed.substr(5));
        std::vector<std::string> parts;
        std::string part;
        while (iss >> part) parts.push_back(part);

        if (parts.size() >= 2) {
            double revenue = std::stod(parts.back());
            std::string gameName = parts[0];
            for (size_t i = 1; i < parts.size() - 1; ++i) {
                gameName += " " + parts[i];
            }
            index++;
            return std::make_unique<Game>(gameName, revenue);
        }
    }

    index++;
    return nullptr;
}

std::unique_ptr<Component> loadFromFile(const std::string& filename) {
    std::ifstream in(filename);
    if (!in) return nullptr;

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }

    size_t index = 0;
    return parse(lines, index);
}

void saveToFile(Component* root, const std::string& filename) {
    std::ofstream out(filename);
    if (out) {
        root->save(out, 0);
        std::cout << "Saved to: " << filename << std::endl;
    }
}

// === MAIN PROGRAM ===
int main() {
    auto root = std::make_unique<Group>("Casino Games");

    // Initialize with sample data
    auto tableGames = std::make_unique<Group>("Table Games");
    tableGames->add(std::make_unique<Game>("Blackjack", 0));
    tableGames->add(std::make_unique<Game>("Roulette", 0));

    auto slotGames = std::make_unique<Group>("Slot Games");
    slotGames->add(std::make_unique<Game>("Mega Joker", 0));

    root->add(std::move(tableGames));
    root->add(std::move(slotGames));

    const std::string filename = "casino.txt";
    int choice;

    while (true) {
        std::cout << "\n1. Display games\n2. Add game\n3. Add revenue\n4. Save\n5. Load\n0. Exit\n";
        std::cin >> choice;
        std::cin.ignore();

        if (choice == 0) break;

        switch (choice) {
            case 1:
                root->display();
                break;

            case 2: {
                auto children = static_cast<Group*>(root.get())->getChildren();
                std::cout << "Groups:\n";
                for (size_t i = 0; i < children.size(); ++i) {
                    if (children[i]->isGroup()) {
                        std::cout << i + 1 << ". " << static_cast<Group*>(children[i])->getName() << std::endl;
                    }
                }

                std::cout << "Select group: ";
                size_t groupIndex;
                std::cin >> groupIndex;
                std::cin.ignore();

                if (groupIndex > 0 && groupIndex <= children.size() && children[groupIndex - 1]->isGroup()) {
                    std::cout << "Game name: ";
                    std::string gameName;
                    std::getline(std::cin, gameName);

                    std::cout << "Revenue: ";
                    double revenue;
                    std::cin >> revenue;
                    std::cin.ignore();

                    static_cast<Group*>(children[groupIndex - 1])->add(std::make_unique<Game>(gameName, revenue));
                    std::cout << "Game added!\n";
                }
                break;
            }

            case 3: {
                auto games = static_cast<Group*>(root.get())->getAllGames();
                std::cout << "Games:\n";
                for (size_t i = 0; i < games.size(); ++i) {
                    std::cout << i + 1 << ". " << games[i]->getName() << " (Revenue: " << games[i]->getRevenue() << ")\n";
                }

                std::cout << "Select game: ";
                size_t gameIndex;
                std::cin >> gameIndex;
                std::cin.ignore();

                if (gameIndex > 0 && gameIndex <= games.size()) {
                    std::cout << "Add revenue: ";
                    double addRevenue;
                    std::cin >> addRevenue;
                    std::cin.ignore();

                    games[gameIndex - 1]->addRevenue(addRevenue);
                    std::cout << "Revenue added!\n";
                }
                break;
            }

            case 4:
                saveToFile(root.get(), filename);
                break;

            case 5: {
                auto loaded = loadFromFile(filename);
                if (loaded && loaded->isGroup()) {
                    root.reset(static_cast<Group*>(loaded.release()));
                    std::cout << "Loaded from: " << filename << std::endl;
                    root->display();
                } else {
                    std::cout << "Failed to load file!\n";
                }
                break;
            }
        }
    }

    return 0;
}