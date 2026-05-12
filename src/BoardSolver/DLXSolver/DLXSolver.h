#pragma once
#include "../Solver.h"
#include <atomic>
#include <vector>

class DLXSolver : public Solver
{
public:
    explicit DLXSolver(int maxThreads = 1);
    void solve(Board &board) override;

private:
    int maxThreads;

    struct Node
    {
        Node *left, *right;
        Node *up, *down;
        Node *col;
        int rowId;
        int size;
    };

    static constexpr int NUM_CANDIDATES  = 729;
    static constexpr int NUM_CONSTRAINTS = 324;
    static constexpr int POOL_SIZE = NUM_CANDIDATES * 4 + NUM_CONSTRAINTS + 1;

    Node pool[POOL_SIZE];
    int  poolIdx;
    Node *root;

    int  solution[NUM_CANDIDATES];
    int  solutionDepth;
    bool foundSolution;

    void  init();
    void  cover(Node *col);
    void  uncover(Node *col);
    Node *chooseColumn();
    void  search(int depth);
    void  buildMatrix(const Board &board);
    void  applyClue(int r, int c, int d);
    Node *addRow(int rowId, int col0, int col1, int col2, int col3);
    void  fillBoard(Board &board);

    static int cellConstraint(int r, int c)      { return r * 9 + c; }
    static int rowDigitConstraint(int r, int d)  { return 81  + r * 9 + d; }
    static int colDigitConstraint(int c, int d)  { return 162 + c * 9 + d; }
    static int boxDigitConstraint(int r, int c, int d) {
        return 243 + ((r / 3) * 3 + (c / 3)) * 9 + d;
    }
};
