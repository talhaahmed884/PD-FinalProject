#ifndef PDC_FINALPROJECT_BLOCK_H
#define PDC_FINALPROJECT_BLOCK_H
#pragma once

class Block {
public:
    Block();

    void setBlockValue(int value);

    [[nodiscard]] int getBlockValue() const;

    [[nodiscard]] bool getIsFilled() const;

    void setIsFilled(bool value);

    void resetBlock();

    void print() const;

private:
    int blockValue;
    bool isFilled;
};


#endif //PDC_FINALPROJECT_BLOCK_H
