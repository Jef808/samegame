#include <array>
#include <algorithm>

namespace sg {

inline constexpr auto WIDTH = 15;
inline constexpr auto HEIGHT = 15;
inline constexpr auto MAX_COLORS = 5;
inline constexpr auto MAX_CELLS = HEIGHT * WIDTH;
inline constexpr auto CELL_UPPER_LEFT = 0;
inline constexpr auto CELL_UPPER_RIGHT = WIDTH - 1;
inline constexpr auto CELL_BOTTOM_LEFT = (HEIGHT - 1) * WIDTH;
inline constexpr auto CELL_BOTTOM_RIGHT = MAX_CELLS - 1;
inline constexpr auto CELL_NONE = MAX_CELLS;

typedef int Cell;
enum class Color { Empty = 0, Nb = MAX_COLORS+1 };

/**
 * Simple wrapper around std::array representing the grid of a Samegame state.
 */
struct Grid {
    using Array = std::array<Color, MAX_CELLS>;
    using value_type = Color;
    using reference = Color&;
    using const_reference = const Color&;
    using iterator = Array::iterator;
    using const_iterator = Array::const_iterator;
    using difference_type = Array::difference_type;
    using size_type = Array::size_type;

    constexpr Grid() = default;
    Grid(const Grid& other) = default;// : m_data{other.m_data};
    Grid(Grid&& other) = default;// : m_data{std::move(other)} {}
    Grid& operator=(const Grid& other) = default; //{ m_data = other.m_data; }
    Grid& operator=(Grid&& other) = default; // { m_data = std::move(other.m_data); }
    ~Grid() = default;

    bool operator==(const Grid& other) const { return m_data == other.m_data; }
    bool operator!=(const Grid& other) const { return m_data != other.m_data; }
    void swap(Grid& other) { std::swap(m_data, other.m_data); }
    size_type size() { return MAX_CELLS; }
    size_type max_size() { return MAX_CELLS; }
    bool empty() { return std::all_of(m_data.begin(), m_data.end(), [](const Color c) { return c == Color::Empty; }); }

    Color& operator[](Cell c) { return m_data[c]; }
    Color operator[](Cell c) const { return m_data[c]; }
    iterator begin() { return m_data.begin(); }
    iterator end() { return m_data.end(); }
    const_iterator begin() const { return m_data.cbegin(); }
    const_iterator end() const { return m_data.cend(); }
    const_iterator cbegin() { return m_data.cbegin(); }
    const_iterator cend() { return m_data.end(); }

    mutable size_type n_empty_rows { 0 };

private:
    Array m_data { Color::Empty };
};

inline constexpr Grid EMPTY_GRID {};

} // namespace sg
