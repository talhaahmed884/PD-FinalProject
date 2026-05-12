#pragma once
#include "../Solver.h"

class DLXSolver : public Solver
{
public:
    void solve(Board &board) override;

private:
    // --- Node ---
    // Every cell in the sparse matrix is a Node.
    // Column headers are also Nodes (they live in the same pool).
    struct Node
    {
        Node *left, *right; // horizontal links (same row, or header ring)
        Node *up, *down;    // vertical links   (same column)
        Node *col;          // pointer back to this column's header
        int rowId;          // which candidate placement this node belongs to
        int size;           // only used by header nodes: # of nodes in this column
    };

    // --- Constants ---
    static constexpr int NUM_CANDIDATES = 729;  // 9 rows x 9 cols x 9 digits
    static constexpr int NUM_CONSTRAINTS = 324; // 4 constraint types x 81 each
    static constexpr int POOL_SIZE = NUM_CANDIDATES * 4 + NUM_CONSTRAINTS + 1;

    // --- Storage ---
    Node pool[POOL_SIZE]; // all nodes pre-allocated (no dynamic allocation)
    int poolIdx;          // next free slot in pool[]
    Node *root;           // sentinel header (the "h" node in Knuth's paper)

    // --- Solution state ---
    int solution[NUM_CANDIDATES]; // selected rowIds during search
    int solutionDepth;
    bool foundSolution;

    // --- Internal methods (stubs — we fill these in together) ---
    void init();
    void cover(Node *col);
    void uncover(Node *col);
    Node *chooseColumn();
    void search(int depth);
    void buildMatrix(const Board &board);
    void applyClue(int r, int c, int d);
    Node *addRow(int rowId, int col0, int col1, int col2, int col3);
    void fillBoard(Board &board);

    // --- Constraint index helpers ---
    static int cellConstraint(int r, int c) { return r * 9 + c; }
    static int rowDigitConstraint(int r, int d) { return 81 + r * 9 + d; }
    static int colDigitConstraint(int c, int d) { return 162 + c * 9 + d; }
    static int boxDigitConstraint(int r, int c, int d)
    {
        return 243 + ((r / 3) * 3 + (c / 3)) * 9 + d;
    }
};
