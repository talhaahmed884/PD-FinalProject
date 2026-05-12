#include "DLXSolver.h"
#ifdef _OPENMP
#include <omp.h>
#endif

DLXSolver::DLXSolver(int maxThreads) : maxThreads(maxThreads) {
#ifdef _OPENMP
    if (maxThreads > 0)
        omp_set_num_threads(maxThreads);
#endif
}

// ─────────────────────────────────────────────
// Step 1: init()   — build the empty header ring
// ─────────────────────────────────────────────
void DLXSolver::init()
{
    for (int i = 0; i <= NUM_CONSTRAINTS; i++)
    {
        Node *n = &pool[i];
        n->up = n; // column is empty, points to itself
        n->down = n;
        n->col = n;
        n->size = 0;
        n->rowId = -1;
        if (i == 0)
            n->left = &pool[NUM_CONSTRAINTS]; // root wraps back to the LAST header
        else
            n->left = &pool[i - 1];
        if (i == NUM_CONSTRAINTS)
            n->right = &pool[0]; // last header wraps back to root
        else
            n->right = &pool[i + 1]; // everyone else points to the NEXT node
    }
    poolIdx = NUM_CONSTRAINTS + 1;
    root = &pool[0];
    foundSolution = false;
    solutionDepth = 0;
}

// ─────────────────────────────────────────────
// Step 2: addRow() — insert one candidate into the matrix
// ─────────────────────────────────────────────
DLXSolver::Node *DLXSolver::addRow(int rowId, int col0, int col1, int col2, int col3)
{
    Node *nodes[4];
    int cols[4] = {col0, col1, col2, col3};
    for (int i = 0; i < 4; i++)
    {
        nodes[i] = &pool[poolIdx++];
        nodes[i]->rowId = rowId;
        nodes[i]->col = &pool[cols[i] + 1];
        nodes[i]->size = 0;
    }
    for (int i = 0; i < 4; i++)
    {
        nodes[i]->right = nodes[(i + 1) % 4];
        nodes[i]->left = nodes[(i + 3) % 4];
    }
    for (int i = 0; i < 4; i++)
    {
        Node *col = nodes[i]->col; // this node's column header
        nodes[i]->up = col->up;
        nodes[i]->down = col;
        col->up->down = nodes[i];
        col->up = nodes[i];
        col->size++;
    }
    return nodes[0];
}

// ─────────────────────────────────────────────
// Step 3: buildMatrix() — populate the full matrix from the board
// ─────────────────────────────────────────────
void DLXSolver::buildMatrix(const Board &board)
{
    init();

    for (int r = 0; r < 9; r++)
    {
        for (int c = 0; c < 9; c++)
        {
            for (int d = 0; d < 9; d++)
            {
                int rowId = r * 81 + c * 9 + d;
                addRow(rowId,
                       cellConstraint(r, c),
                       rowDigitConstraint(r, d),
                       colDigitConstraint(c, d),
                       boxDigitConstraint(r, c, d));
            }
        }
    }
    for (int r = 0; r < 9; r++)
    {
        for (int c = 0; c < 9; c++)
        {
            int val = board.getBoardValue(r, c);
            if (val != -1)
            {
                applyClue(r, c, val - 1); // val-1 because d is 0-indexed
            }
        }
    }
}

// ─────────────────────────────────────────────
// Step 4: applyClue() — pre-cover a given clue cell
// ─────────────────────────────────────────────
void DLXSolver::applyClue(int r, int c, int d)
{
    int rowId = r * 81 + c * 9 + d;
    Node *n = &pool[NUM_CONSTRAINTS + 1 + rowId * 4];
    Node *cur = n;
    do
    {
        cover(cur->col);
        cur = cur->right;
    } while (cur != n);
}

// ─────────────────────────────────────────────
// Step 5: cover() / uncover() — the "dancing" part
// ─────────────────────────────────────────────
void DLXSolver::cover(Node *col)
{
    // Step A: remove column header from the ring
    col->left->right = col->right;
    col->right->left = col->left;

    // Step B: for each row in this column, unlink its nodes vertically
    for (Node *row = col->down; row != col; row = row->down)
    {
        for (Node *node = row->right; node != row; node = node->right)
        {
            node->up->down = node->down;
            node->down->up = node->up;
            node->col->size--;
        }
    }
}

void DLXSolver::uncover(Node *col)
{
    // Step B reversed: walk UP, walk LEFT, relink vertically
    for (Node *row = col->up; row != col; row = row->up)
    {
        for (Node *node = row->left; node != row; node = node->left)
        {
            node->up->down = node;
            node->down->up = node;
            node->col->size++;
        }
    }

    // Step A reversed: relink column header into the ring
    col->left->right = col;
    col->right->left = col;
}

// ─────────────────────────────────────────────
// Step 6: chooseColumn() — MRV heuristic
// ─────────────────────────────────────────────
DLXSolver::Node *DLXSolver::chooseColumn()
{
    Node *best = root->right;
    for (Node *col = best->right; col != root; col = col->right)
    {
        if (col->size < best->size)
            best = col;
    }
    return best;
}

// ─────────────────────────────────────────────
// Step 7: search() — Algorithm X recursion
// ─────────────────────────────────────────────
void DLXSolver::search(int depth)
{
    // Base case: no columns left → every constraint satisfied → solved
    if (root->right == root)
    {
        solutionDepth = depth;
        foundSolution = true;
        return;
    }

    // Pick the column with fewest candidates (MRV)
    Node *col = chooseColumn();

    // Cover it
    cover(col);

    // Try each candidate that can satisfy this column
    for (Node *row = col->down; row != col; row = row->down)
    {
        // Tentatively select this candidate
        solution[depth] = row->rowId;

        // Cover all other columns this candidate satisfies
        for (Node *node = row->right; node != row; node = node->right)
            cover(node->col);

        // Recurse — try to solve the reduced matrix
        search(depth + 1);

        // If solved, stop immediately
        if (foundSolution)
            return;

        // Backtrack — uncover in reverse order
        for (Node *node = row->left; node != row; node = node->left)
            uncover(node->col);
    }

    // No candidate worked — uncover and signal failure to caller
    uncover(col);
}

// ─────────────────────────────────────────────
// Step 8: fillBoard() — decode solution[] into the Board
// ─────────────────────────────────────────────
void DLXSolver::fillBoard(Board &board)
{
    for (int i = 0; i < solutionDepth; i++)
    {
        int rowId = solution[i];
        int r = rowId / 81;
        int c = (rowId % 81) / 9;
        int digit = rowId % 9 + 1;
        board.setBoardValue(r, c, digit);
    }
}

// ─────────────────────────────────────────────
// Entry point
// ─────────────────────────────────────────────
void DLXSolver::solve(Board &board)
{
    buildMatrix(board);

#ifdef _OPENMP
    if (maxThreads != 1) {
        // Collect all candidates from the first chosen column
        Node *col = chooseColumn();
        std::vector<int> candidates;
        for (Node *row = col->down; row != col; row = row->down)
            candidates.push_back(row->rowId);

        std::atomic<bool> found(false);
        Board result;

        #pragma omp parallel
        #pragma omp single nowait
        {
            for (int rowId : candidates) {
                #pragma omp task firstprivate(rowId) shared(found, result, board)
                {
                    if (!found) {
                        // Pre-place this candidate on a board copy
                        int r     = rowId / 81;
                        int c     = (rowId % 81) / 9;
                        int digit = rowId % 9 + 1;
                        Board copy = board;
                        copy.setBoardValue(r, c, digit);

                        // Each task gets its own independent DLX instance
                        DLXSolver local(1);
                        local.buildMatrix(copy);
                        local.search(0);

                        if (local.foundSolution && !found.exchange(true)) {
                            local.fillBoard(copy);
                            result = copy;
                        }
                    }
                }
            }
        }

        if (found) board = result;
        return;
    }
#endif

    // Serial path
    search(0);
    if (foundSolution)
        fillBoard(board);
}
