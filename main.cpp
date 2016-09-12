#include <iostream>
#include <math.h>
#include <array>
#include <vector>
#include <chrono>
#include <thread>
#include <random>

/*
 * Represents a cell on a game of life board.
 * Contains info on the cells status and (x,y) position
 */
class Cell {
  const int m_status;
  const std::array<int, 2> m_location;

  public:
    Cell()
      : m_status(-1), m_location{{-1, -1}} {};
    Cell(int val, int x, int y)
      : m_status(val), m_location{{x, y}} {};
    Cell(int val, std::array<int,2> coord)
      : m_status(val), m_location(coord) {};

    bool is_alive() const { return bool(m_status); };
    std::array<int, 2> coord() const { return m_location; };
};

/*
 * Represents the cells around a center cell and that center cell.
 * Can say where the center cell is, it's status, and provides
 * a count of the living neighbors.
 */
class Neighborhood {
  const Cell * m_cell;
  const std::vector<Cell> m_neighbors;

  public:
    Neighborhood()
      : m_cell(new Cell()) {};
    Neighborhood(const Cell * cell, const std::vector<Cell> neigh)
      : m_cell(cell), m_neighbors(neigh) {}

    bool cell_status() const { return m_cell->is_alive(); };

    std::array<int,2> cell_location() const { return m_cell->coord(); };

    int count_alive_neighbors() const {
      int count = 0;
      auto start = m_neighbors.begin();
      auto end = m_neighbors.end();
      for (auto i = start; i != end; i++)
        if (i->is_alive()) count++;
      return count;
    };
};

/*
 * A Game of life board.
 * Provides methods to access neighborhoods of cells
 * and to activate/deactive individual cells on the board
 */
class Grid {
  int * m_contents;
  int m_width, m_height, m_max;
  std::default_random_engine m_engine;
  std::uniform_int_distribution<int> m_distribution;
  const int m_neighbor_offsets[8][2] = {
    { -1, -1 }, { 0, -1 }, { 1, -1 },
    { -1, 0 }, { 1, 0 },
    { -1, 1 }, { 0, 1 }, { 1, 1 }
  };

  public:
    Grid() : Grid(10, 10) {};
    Grid(int width, int height)
      : m_width(width), m_height(height), m_max(width * height), m_distribution(0,1)
    {
      m_contents = new int[m_max];
      for (int i = 0; i < m_max; i++) 
        m_contents[i] = m_distribution(m_engine);
    }
    ~Grid() { delete[] m_contents; };

    /*
     * Facilitate stream printing
     */
    friend std::ostream& operator<<(std::ostream &, const Grid &);

    /* 
     * Return a vector of the neighborhoods in the grid.
     * A neighborhood is an object containing the cell
     * at the center of a 3x3 grid and a vector of the cells
     * surrounding it
     */
    const std::vector<Neighborhood> neighborhoods() const {
      std::vector<Neighborhood> hoods;
      int cell_status;
      std::array<int,2> cell_coord;
      std::vector<Cell> cell_neighbors;

      for (int i = 0; i < m_max; i++) {
        cell_status = m_contents[i];
        cell_coord = to_2d(i);
        cell_neighbors = neighbors(i);
        hoods.push_back(
            Neighborhood(
              new Cell(cell_status, cell_coord), 
              cell_neighbors));
      };
      return hoods;
    };

    /*
     * Make a cell on the board alive
     */
    void activate(std::array<int,2> coord) {
      m_contents[to_1d(coord)] = 1;
    }

    /*
     * Make a cell on the board dead
     */
    void deactivate(std::array<int,2> coord) {
      m_contents[to_1d(coord)] = 0;
    };

  private:
    /*
     * Convert a 1-dimensional index to a 2-dimensional coordinate
     */
    std::array<int,2> to_2d(int idx) const {
      const int x = floor(idx / m_height);
      const int y = idx % m_height;
      std::array<int,2> coord{{ x, y }};
      return coord;
    };

    /*
     * Convert a 2-dimensional coordinate to a 1-dimensional index.
     */
    int to_1d(std::array<int,2> coord) const {
      int result = coord[1] + (coord[0] * m_height);
      return result;
    };

    /*
     * Fetch the value in the grid array given a coordinate
     */
    int value_at(std::array<int,2> coord) const {
      int idx = to_1d(coord);
      return m_contents[idx];
    };

    /*
     * Collect the 8 neighbors of a 3x3 grid around a given index,
     * assumed to be the middle of the grid.
     */
    std::vector<Cell> neighbors(int idx) const {
      std::vector<Cell> neighbors;
      std::array<int, 2> this_coord = to_2d(idx);
      int neighbor_status;
      std::array<int, 2> neighbor_coord;

      for (int i = 0; i < 8; i++) {
        neighbor_coord[0] = this_coord[0] + m_neighbor_offsets[i][0];
        neighbor_coord[1] = this_coord[1] + m_neighbor_offsets[i][1];
        if (-1 < neighbor_coord[0] && neighbor_coord[0] < m_width
            && -1 < neighbor_coord[1] && neighbor_coord[1] < m_height) {
          neighbor_status = value_at(neighbor_coord);
          neighbors.push_back(Cell(neighbor_status, neighbor_coord));
        };
      };
      return neighbors;
    };
};

std::ostream & operator<<(std::ostream & out, const Grid & grid) {
  for (int i = 0; i < grid.m_max; i++) {
    if (i % grid.m_width == 0) {
      if (i > 0) out << " ";
      out << std::endl;
    }
    if (grid.m_contents[i] == 0) out << "  ";
    else out << " *";
  }
  out << " ";
  return out;
};

/*
 * Class embodying the classic Conway rules for a game of life:
 *    1. Any live cell with fewer than two live neighbours dies, 
 *        as if caused by under-population.
 *    2. Any live cell with two or three live neighbours lives on 
 *        to the next generation.
 *    3. Any live cell with more than three live neighbours dies, 
 *        as if by over-population.
 *    4. Any dead cell with exactly three live neighbours becomes 
 *        a live cell, as if by reproduction.
 */
class PlayStrategy {
  public:
    PlayStrategy() {};

    void applyTo(Grid & grid) const {
      const std::vector<Neighborhood> hoods = grid.neighborhoods();
      auto start = hoods.begin();
      auto end = hoods.end();
      for (auto hood = start; hood != end; hood++) {
        if (cell_lives(hood->cell_status(), hood->count_alive_neighbors()))
          grid.activate(hood->cell_location());
        else
          grid.deactivate(hood->cell_location());
      };
    };

  private:
    bool cell_lives(bool cell_lives, int living_neighbor_count) const {
      // rules 1 & 3 - everything dies in the range
      if (living_neighbor_count < 2 || 3 < living_neighbor_count)
        return false;
      // rule 2 - now 2 || 3 and only living things
      else if (cell_lives) return true;
      // rule 4 - 3 and only dead things
      else if (living_neighbor_count == 3) return true;
      else return false;
    };
};

/*
 * Players apply strategies to a grid
 */
class Player {
  const PlayStrategy * m_strategy;

  public:
    Player() : m_strategy(new PlayStrategy()) {}
    Player(const PlayStrategy * strat) : m_strategy(strat) {}
    ~Player() { delete m_strategy; }

    void set_strategy(const PlayStrategy * strat) {
      m_strategy = strat;
    };

    void play(Grid & grid) const {
      m_strategy->applyTo(grid);
    }
};

/*
 * Games accept a player and contain a board.
 * Games can run through a single round of a GOL,
 * or can be told to loop for a certain number of times.
 */
class Game {
  const Player & m_player;
  Grid * m_grid;

  public:
    Game(const Player & player)
      :  m_player(player) {
      m_grid = new Grid();
    }
    Game(const Player & player, int grid_width, int grid_height)
      : m_player(player) {
      m_grid = new Grid(grid_width, grid_height);
    }

    ~Game() { delete m_grid; };

    void show_board() {
      std::cout << "-------------------------------------";
      std::cout << *m_grid << std::endl;
    };

    void play_round() {
      m_player.play(*m_grid);
    };

    void loop(int time_step) {};
};

int main(int argc, char ** argv) {
  Player player = Player();
  Game game = Game(player, 50, 50);
  for (int i = 0; i < 1000; i++) {
    std::cout << "Frame " << i+1 << ":" << std::endl;
    game.show_board();
    game.play_round();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
  };
  return 0;
};
