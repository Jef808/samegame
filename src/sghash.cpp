#include "sghash.h"
#include "zobrist.h"
#include "clusterhelper.h"
#include "types.h"

namespace sg::zobrist {


ZTable Table { };

Key get_key(const Cell _cell, const Color _color)
{
    return Table(_cell, _color);
}

/**
 * Xor with a unique random key for each (index, color) appearing in the grid.
 * Also compute is_terminal() (Key will have first bit on once is_terminal() is known,
 * and it will be terminal iff the second bit is on).
 * Also records the number of empty rows in passing.
 */
Key get_key(const Grid& _grid)
{
    Key key = 0;
    bool row_empty = false, terminal_status_known = false;

    // All rows except the first one
    for (auto row = HEIGHT - 1; row > 0; --row) {
        row_empty = true;

        for (auto cell = row * WIDTH; cell < (row + 1) * WIDTH; ++cell) {
            if (const Color color = _grid[cell]; color != Color::Empty) {
                row_empty = false;

                key ^= Table(cell, color);

                // If the terminal status of the _grid is known, continue
                if (terminal_status_known) {
                    continue;
                }
                // Otherwise keep checking for nontrivial clusters upwards and forward
                if (clusters::same_as_right_or_up_nbh(_grid, cell)) {
                    // Indicate that terminal status is known
                    terminal_status_known = true;
                }
            }
        }
        if (row_empty) {
            break;
        }
    }
    // First row
    for (auto cell = CELL_UPPER_LEFT; cell < CELL_UPPER_RIGHT; ++cell) {
        if (const Color color = _grid[cell]; color != Color::Empty) {
            row_empty = false;

            key ^= Table(cell, color);

            // If the terminal status of the _grid is known, continue
            if (terminal_status_known) {
                continue;
            }
            // Otherwise keep checking for nontrivial clusters upwards and forward
            if (clusters::same_as_right_nbh(_grid, cell)) {
                // Indicate that terminal status is known
                terminal_status_known = true;
            }
        }
    }

    // flip the first bit if we found out _grid was non-terminal earlier,
    // otherwise flip both the first and second bit.
    key += terminal_status_known ? 1 : 3;

    return key;
}


} // namespace sg::zobrist
