#ifndef MazeGen
#define MazeGen

#include <vector>
#include <stack>
#include <set>
#include <map>
#include <unordered_map>
#include <array>
#include <random>
#include <iostream>
#include <algorithm>


namespace mazegen {

const int NOTHING_ID = -1;
const int MAX_ROOMS = 1000000;
const int HALL_ID_START = 0;
const int ROOM_ID_START = MAX_ROOMS;
const int DOOR_ID_START = MAX_ROOMS * 2;

namespace {
    typedef std::vector<std::vector<int>> Grid;

    // Used to iterate through neighbors to a point
    struct Direction {
        int dx = 0, dy = 0;
        bool operator==(const Direction& rhs) const {
            return dx == rhs.dx && dy == rhs.dy;
        }
        Direction operator-() const {
            return Direction{-1 * dx, -1 * dy};
        }
        Direction operator*(const int a) const {
            return Direction{a * dx, a * dy};
        }
    };
    typedef std::array<Direction, 4> Directions;
    const Directions CARDINALS  {{ {0, -1}, {1, 0}, {0, 1}, {-1, 0} }};

}

struct Config {
    // Probability to not remove deadends
    float DEADEND_CHANCE = 0.5;
    // True if use reconnect_deadends() step - connect deadends adjacent to rooms with a door
    float RECONNECT_DEADENDS_CHANCE = 0.5;
    // Probability for a hall to change direction during growth
    float WIGGLE_CHANCE = 0.5;
    // All unneccessary doors are removed with this probability
    float EXTRA_CONNECTION_CHANCE = 0.0;
    // How many times room placement is attempted
    int ROOM_BASE_NUMBER = 30;
    // Room minimum dimension
    int ROOM_SIZE_MIN = 7;
    // Room maximum dimension
    int ROOM_SIZE_MAX = 9;
    // True if hall constaraints are to be exclusively in halls, not in rooms
    bool CONSTRAIN_HALL_ONLY = false;
};


// Represents a point on a grid
struct Point {
    int x;
    int y;
    Point(int _x, int _y): x(_x), y(_y) {}
    Point(std::array<int, 2> a): x(a.at(0)), y(a.at(1)) {}
    bool operator<(const Point& another) const {
        return x < another.x || y < another.y;
    }
    bool operator==(const Point& another) const {
        return x == another.x && y == another.y;
    }
    Point neighbour_to(const Direction& d) const {
        return Point{x + d.dx, y + d.dy};
    }
};
typedef std::vector<Point> Points;
typedef std::set<Point> PointSet;


// Represents a room with dimensions and id
struct Room {
    Point min_point;
    Point max_point;
    int id;
    bool too_close(const Room& another, int distance) {
        return 
            min_point.x - distance < another.max_point.x 
            && max_point.x + distance > another.min_point.x 
            && min_point.y - distance < another.max_point.y 
            && max_point.y + distance > another.min_point.y;
    }
    bool has_point(const Point& point) {
        return point.x >= min_point.x && point.x <= max_point.x 
            && point.y >= min_point.y && point.y <= max_point.y;
    }
};


// Represents a hall region
struct Hall {
    Point start; // a point belonging to this hall
    int id;
};


// Represents a door connecting a room to a hall region
struct Door {
    Point position;
    int id;
    int room_id;
    int hall_id;
    bool is_hidden = false; // the doors removed in the "reduce_connectivity" step become hidden
};


inline bool is_hall(int id) {
    return id >= HALL_ID_START && id < ROOM_ID_START;
}


inline bool is_room(int id) {
    return id >= ROOM_ID_START && id < DOOR_ID_START;
}


inline bool is_door(int id) {
    return id >= DOOR_ID_START;
}


// Class to generate the maze
class Generator {

public:
// Generates a maze
// Constraints are Points between (1, 1) and (rows - 2, cols - 2),
// those points are fixed on the generation - they are never a wall 
void generate(int width, int height, const Config& user_config, const PointSet& hall_constraints = {}) noexcept {
    clear();
    init_generation(width, height, user_config, hall_constraints);
    place_rooms();
    build_maze();
    connect_regions();
    reduce_connectivity();
    reduce_maze();
    reconnect_dead_ends();
}


const std::string& get_warnings() const noexcept {
    return warnings;
}


// predefines generation seed
void set_seed(unsigned int seed) noexcept {
    is_seed_set = true;
    random_seed = seed;
}


const unsigned int get_seed() const noexcept {
    return random_seed;
}


// returns region id of a point or NOTHING_ID if point is out of bounds or not in any maze region, i.e. is wall
int region_at(int x, int y) const noexcept {
    if (!is_in_bounds(x, y)) return NOTHING_ID;
    return grid[y][x];
}


// returns region id of a point or NOTHING_ID if point is out of bounds or not in any maze region, i.e. is wall
int region_at(const Point& p) const noexcept {
    return region_at(p.x, p.y);
}


const std::vector<Room>& get_rooms() const noexcept {
    return rooms;
}


const std::vector<Hall>& get_halls() const noexcept {
    return halls;
}


const std::vector<Door>& get_doors() const noexcept {
    return doors;
}


const Config& get_config() const noexcept{
    return cfg;
}


int maze_height() const noexcept{
    return grid.size();
}


int maze_width() const noexcept{
    return grid.empty() ? 0 : grid.front().size();
}


private:

Config cfg;

std::vector<Room> rooms;
std::vector<Door> doors;
std::vector<Hall> halls;

std::string warnings;
Points dead_ends;
PointSet point_constraints;

Grid grid;

std::mt19937 rng;
int maze_region_id = HALL_ID_START;
int room_id = ROOM_ID_START;
int door_id = DOOR_ID_START;

bool is_seed_set = false;
unsigned int random_seed;


// Clears all the generated data for consequent generation 
void clear() {
    grid.clear();
    rooms.clear();
    halls.clear();
    doors.clear();
    warnings.clear();
    maze_region_id = HALL_ID_START;
    room_id = ROOM_ID_START;
    door_id = DOOR_ID_START;
}


// Initializes generation internal variables
void init_generation(int width, int height, const Config& user_config, const PointSet& hall_constraints = {}) {
    auto fixed_size = fix_boundaries(width, height);
    int grid_width = fixed_size.first;
    int grid_height = fixed_size.second;
    grid = Grid(grid_height, std::vector<int>(grid_width, NOTHING_ID));
    cfg = fix_config(user_config);
    point_constraints = fix_constraint_points(hall_constraints);
    if (is_seed_set) {
        rng.seed(random_seed);
    } else {
        std::random_device rd;
        random_seed = rd();
        rng.seed(random_seed);
    }
}


std::pair<int, int> fix_boundaries(int width, int height) {
    if (width % 2 == 0 || height % 2 == 0) {
        warnings.append("Warning! Maze height and width must be odd! Fixed by subtracting 1.\n"
        );
        if (width % 2 == 0) width -= 1;
        if (height % 2 == 0) height -= 1;
    }
    if (width < 3 || height < 3) {
        warnings.append("Warning! Maze height and width must be >= 3! Fixed by increasing to 3.\n");
        if (width < 3) width = 3;
        if (height < 3 ) height = 3;
    }
    return std::make_pair(width, height);    
}


// Adds only those constraints that have odd x and y and are not out of grid bounds
PointSet fix_constraint_points(const PointSet& hall_constraints) {
    PointSet constraints;
    for (auto& constraint: hall_constraints) {
        if (!is_in_bounds(constraint)) {
            warnings.append("Warning! Constraint (" 
                + std::to_string(constraint.x) + ", " + std::to_string(constraint.y) 
                + ") is out of grid bounds. Skipped.\n"
            );
        } else if (constraint.x % 2 == 0 || constraint.y % 2 == 0) {
            warnings.append("Warning! Constraint (" 
                + std::to_string(constraint.x) + ", " + std::to_string(constraint.y) 
                + ") must have odd x and y. Skipped.\n"
            );
        } else {
            constraints.insert(constraint);
        }
    }
    return constraints;
}


Config fix_config(const Config& user_config) {
    Config fixed{user_config};
    if (fixed.DEADEND_CHANCE < 0.0f || fixed.DEADEND_CHANCE > 1.0f ||
            fixed.RECONNECT_DEADENDS_CHANCE < 0.0f || fixed.RECONNECT_DEADENDS_CHANCE > 1.0f ||
            fixed.WIGGLE_CHANCE < 0.0f || fixed.WIGGLE_CHANCE > 1.0f ||
            fixed.EXTRA_CONNECTION_CHANCE < 0.0f || fixed.DEADEND_CHANCE > 1.0f) {
        warnings.append("Warning! All chances should be between 0.0f and 1.0f. Fixed by clamping.\n");
    }
    if (fixed.ROOM_BASE_NUMBER >= MAX_ROOMS || fixed.ROOM_BASE_NUMBER < 0) {
        if (fixed.ROOM_BASE_NUMBER >= MAX_ROOMS) fixed.ROOM_BASE_NUMBER = MAX_ROOMS - 1;
        if (fixed.ROOM_BASE_NUMBER < 0) fixed.ROOM_BASE_NUMBER = 0;
        warnings.append("Warning! ROOM_BASE_NUMBER must belong to[0, " + std::to_string(MAX_ROOMS - 1) + "]. Fixed by clamping.\n");
    }
    if (fixed.ROOM_SIZE_MIN % 2 == 0 || fixed.ROOM_SIZE_MAX % 2 == 0) {
        if (fixed.ROOM_SIZE_MIN % 2 == 0) fixed.ROOM_SIZE_MIN -= 1; 
        if (fixed.ROOM_SIZE_MAX % 2 == 0) fixed.ROOM_SIZE_MAX -= 1; 
        warnings.append("Warning! ROOM_SIZE_MIN and ROOM_SIZE_MAX must be odd. Fixed by subtracting 1.\n");
    }
    int min_dimension = std::min(maze_width(), maze_height());
    if (fixed.ROOM_SIZE_MIN > min_dimension || fixed.ROOM_SIZE_MAX > min_dimension) {
        if (fixed.ROOM_SIZE_MIN > min_dimension) fixed.ROOM_SIZE_MIN = min_dimension;
        if (fixed.ROOM_SIZE_MAX > min_dimension) fixed.ROOM_SIZE_MAX = min_dimension;
        warnings.append("Warning! ROOM_SIZE_MIN and ROOM_SIZE_MAX must less than both width and height of the maze. Fixed.\n");
    }

    if (fixed.ROOM_SIZE_MIN < 0 || fixed.ROOM_SIZE_MAX < 0 || fixed.ROOM_SIZE_MAX < fixed.ROOM_SIZE_MIN) {
        if (fixed.ROOM_SIZE_MIN < 0) fixed.ROOM_SIZE_MIN = 0;
        if (fixed.ROOM_SIZE_MAX < 0) fixed.ROOM_SIZE_MAX = 0;
        if (fixed.ROOM_SIZE_MAX < fixed.ROOM_SIZE_MIN) fixed.ROOM_SIZE_MAX = fixed.ROOM_SIZE_MIN;
        warnings.append("Warning! ROOM_SIZE_MIN and ROOM_SIZE_MAX must be > 0 and ROOM_SIZE_MAX must be >= ROOM_SIZE_MIN. Fixed.\n");
    }
    return fixed;
}


// Places the rooms randomly
void place_rooms() {
    std::uniform_int_distribution<> room_size_distribution(cfg.ROOM_SIZE_MIN, cfg.ROOM_SIZE_MAX);
    int room_avg = cfg.ROOM_SIZE_MIN + (cfg.ROOM_SIZE_MAX - cfg.ROOM_SIZE_MIN) / 2;
    std::uniform_int_distribution<> room_position_x_distribution(0, maze_width() - room_avg);
    std::uniform_int_distribution<> room_position_y_distribution(0, maze_height() - room_avg);

    for (int i = 0; i < cfg.ROOM_BASE_NUMBER; i++) {
        bool room_is_placed = false;
        int width = room_size_distribution(rng) / 2 * 2 + 1;
        int height = room_size_distribution(rng) / 2 * 2 + 1;
        int room_x = room_position_x_distribution(rng) / 2 * 2 + 1;
        int room_y = room_position_y_distribution(rng) / 2 * 2 + 1;

        int x_overshoot = maze_width() - room_x;
        int y_overshoot = maze_height() - room_y;

        if (width >= x_overshoot) width = x_overshoot / 2 * 2 - 1;
        if (height >= y_overshoot) height = y_overshoot / 2 * 2 - 1;

        Room room{{room_x, room_y}, {room_x + width - 1, room_y + height - 1}, room_id};
        bool too_close = false;
        for (const auto& another_room : rooms) {
            if (room.too_close(another_room, 1)) {
                too_close = true;
                break;
            }
        }
        if (too_close) continue;
        if (cfg.CONSTRAIN_HALL_ONLY) {
            bool breaks_hall_constraint = false;
            for (const Point& point: point_constraints) {
                if (room.has_point(point)) {
                    breaks_hall_constraint = true;
                    break;
                }  
            }
            if (breaks_hall_constraint) continue;
        }
        rooms.push_back(room);
        room_is_placed = true;
        for (int x = room.min_point.x; x <= room.max_point.x; x++) {
            for (int y = room.min_point.y; y <= room.max_point.y; y++) {
                grid[y][x]= room_id;
            }
        }
        room_id++;
    }
}


// Constraints are the points which are always in the maze, never in the wall
void build_maze() {
    Points unmet_constraints(point_constraints.begin(), point_constraints.end());
    // first grow from the constraints
    while (!unmet_constraints.empty()) {
        Point p = unmet_constraints.back();
        unmet_constraints.pop_back();
        if (grid[p.y][p.x] != NOTHING_ID) {
            continue;
        }
        grow_maze(p);
    }
    // then from all the empty points 
    for (int x = 0; x < maze_width() / 2; x++) {
        for (int y = 0; y < maze_height() / 2; y++) {
            // if (grid[y * 2 + 1][x * 2 + 1] == NOTHING_ID) grow_maze({x * 2 + 1, y * 2 + 1});
            if (region_at(x, y) == NOTHING_ID) grow_maze({x * 2 + 1, y * 2 + 1});
        }
    }
}



// Returns true if point is inside the maze boundaries
bool is_in_bounds(int x, int y) const {
    return x > 0 && y > 0 && x < maze_width() - 1 && y < maze_height() - 1;
}


// Returns true if point is inside the maze boundaries
bool is_in_bounds(const Point& p) const {
    return is_in_bounds(p.x, p.y);
}


// Returns true if point does not have halls or rooms or doors
bool is_cell_empty(int x, int y) const {
    return is_in_bounds(x, y) && grid[y][x] == NOTHING_ID;
}


// Returns true if point does not have halls or rooms or doors
bool is_cell_empty(const Point& p) const {
    return is_cell_empty(p.x, p.y);
}


// Grows a hall from the point
// maze_region_id is an id of an interconnected part of the maze
void grow_maze(Point start_p) {
    Point p {start_p};
    if (!is_cell_empty(p)) {
        return;
    }
    ++maze_region_id;
    halls.push_back({p, maze_region_id});
    grid[p.y][p.x] = maze_region_id;

    std::set<Point> dead_ends_set{p};
    std::stack<Point> test_points;
    test_points.push(Point(p));
    std::uniform_real_distribution<double> directions_distribution(0.0, 1.0);
    Directions random_dirs {CARDINALS};
    bool dead_end = false;
    Direction dir;

    while (!test_points.empty()) {
        if (directions_distribution(rng) < cfg.WIGGLE_CHANCE) {
            std::shuffle(random_dirs.begin(), random_dirs.end(), rng);
            for (auto& d: random_dirs) {
                if (d == dir) {
                    std::swap(d, random_dirs.back());
                    break;
                }
            }
        }
        dead_end = true;
        for (Direction& d : random_dirs) {
            Point test = p.neighbour_to(d * 2);
            // test for unoccupied space to grow
            if (is_cell_empty(test)) {
                dir = d;
                dead_end = false;
                break;
            }
        }
        // nowhere to grow
        if (dead_end) {
            if (is_dead_end(p)) {
                dead_ends_set.insert(p);
            }
            p = test_points.top();
            test_points.pop();
        } else {
            p = p.neighbour_to(dir);
            grid[p.y][p.x] = maze_region_id;
            p = p.neighbour_to(dir);
            grid[p.y][p.x] = maze_region_id;
            test_points.push(Point(p));
        }
    }
    dead_ends.insert(dead_ends.end(), dead_ends_set.begin(), dead_ends_set.end());
}


// Adding potential doors
void add_connector(const Point& test_point, const Point& connect_point, 
        std::unordered_map<int, Points>& connections) {
    int region_id = region_at(test_point);
    if (region_id != NOTHING_ID) {
        if (connections.find(region_id) == connections.end()) {
            connections[region_id] = Points{};
        }
        connections[region_id].push_back(connect_point);
    }
}


// Connects rooms to the adjacent halls at least once for each maze region
void connect_regions() {
    if (rooms.empty()) return;
    std::set<int> connected_rooms; // prevent room double connections to the same region
    for (auto& room: rooms) {
        // add all potential connectors around the room to other regions
        std::unordered_map<int, Points> connectors_map;
        for (int x = room.min_point.x; x <= room.max_point.x; x += 2) {
            add_connector(Point{x, room.min_point.y - 2}, Point{x, room.min_point.y - 1}, connectors_map);
            add_connector(Point{x, room.max_point.y + 2}, Point{x, room.max_point.y + 1}, connectors_map);
        }
        for (int y = room.min_point.y; y <= room.max_point.y; y += 2) {
            add_connector(Point{room.min_point.x - 2, y}, Point{room.min_point.x - 1, y}, connectors_map);
            add_connector(Point{room.max_point.x + 2, y}, Point{room.max_point.x + 1, y}, connectors_map);                
        }
        // select random connector from the connector map
        for (auto& [hall_id, region_connect_points]: connectors_map) {
            if (connected_rooms.find(hall_id) != connected_rooms.end()) continue;
            std::uniform_int_distribution<> connector_distribution(0, region_connect_points.size() - 1);
            Point p = region_connect_points[connector_distribution(rng)];
            grid[p.y][p.x] = ++door_id;
            doors.push_back({p, door_id, room.id, hall_id});
        }
        connected_rooms.insert(room.id);
    }
}


// returns true if a point is a dead end
bool is_dead_end(const Point& p) {
    int passways = 0;
    for (const auto& d : CARDINALS) {
        Point test_p = p.neighbour_to(d);
        if (region_at(test_p) != NOTHING_ID) {
            passways += 1;
        }
    }
    return passways == 1;
}


// Removes blind parts of the maze with (1.0 - DEADEND_CHANCE) probability
void reduce_maze() {
    bool done = false;
    std::uniform_real_distribution<double> reduce_distribution(0.0, 1.0);
    for (auto& end_p : dead_ends) {
        if (reduce_distribution(rng) < cfg.DEADEND_CHANCE) continue;
        Point p{end_p};
        while (is_dead_end(p)) {
            if (point_constraints.find(p) != point_constraints.end()) break; // do not remove constrained points
            for (const auto& d : CARDINALS) {
                Point test_point = p.neighbour_to(d);
                if (region_at(test_point) != NOTHING_ID) {
                    grid[p.y][p.x] = NOTHING_ID;
                    p = test_point;
                    break;
                }
            }
        }
        end_p.x = p.x;
        end_p.y = p.y;
    }
    dead_ends.erase(
        std::remove_if(
            dead_ends.begin(),
            dead_ends.end(),
            [this](Point p){return !is_dead_end(p);}),
        dead_ends.end()
    );
}


// Disjoint sets (union-find)
int find(const std::unordered_map<int, int>& regions, int region_id) {
    if (regions.at(region_id) == region_id) {
        return region_id;
    } 
    return find(regions, regions.at(region_id));
}


// Deletes duplicate doors created previously with (1.0 - EXTRA_CONNECTION_CHANCE) probability
void reduce_connectivity() {
    std::unordered_map<int, int> region_sets;
    for (const auto& room: rooms) {
        region_sets[room.id] = room.id;
    }
    for (const auto& hall: halls) {
        region_sets[hall.id] = hall.id;
    }
    std::uniform_real_distribution<double> connection_chance_distribution(0, 1);
    for (Door& door: doors) {
        int parent_room_id = find(region_sets, door.room_id); 
        int parent_hall_id = find(region_sets, door.hall_id); 
        if (parent_room_id == parent_hall_id) {
            if (connection_chance_distribution(rng) > cfg.EXTRA_CONNECTION_CHANCE) {
                door.is_hidden = true;
                grid[door.position.y][door.position.x] = NOTHING_ID;
            }
            continue;
        }
        int min_parent = std::min(parent_room_id, parent_hall_id);
        int max_parent = std::max(parent_room_id, parent_hall_id);
        region_sets[max_parent] = min_parent;
    }
}


// If a dead end is adjacent to the room, connects it by the door
void reconnect_dead_ends() {
    std::uniform_real_distribution<double> reconnect_distribution(0.0, 1.0);
    for (const Point& dead_end: dead_ends) {
        int hall_id = region_at(dead_end);
        if (hall_id == NOTHING_ID) continue;
        std::map<Point, int> candidates;
        int connection_number = 0;
        for (const Direction& dir : CARDINALS) {
            Point test_p = dead_end.neighbour_to(dir * 2);
            int neighbor_id = region_at(test_p);
            if (neighbor_id != NOTHING_ID) {
                Point door_p {dead_end.x + dir.dx, dead_end.y + dir.dy};
                if (region_at(door_p) != NOTHING_ID) {
                    ++connection_number;
                    continue;
                }
                if (hall_id == neighbor_id) continue;
                candidates[door_p] = neighbor_id;
            }
        }
        if (reconnect_distribution(rng) >= cfg.RECONNECT_DEADENDS_CHANCE) continue;
        if (connection_number > 1) continue; // not a true dead end
        if (candidates.size() == 0) continue;

        auto& [door_p, room_id] = *candidates.begin();

        grid[door_p.y][door_p.x] = ++door_id;
        doors.push_back({door_p, door_id, room_id, hall_id});
    }
}

};
}

#endif