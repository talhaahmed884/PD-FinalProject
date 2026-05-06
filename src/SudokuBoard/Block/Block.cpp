#include "Block.h"

#include <iostream>
#include <iomanip>
using namespace std;

Block::Block() {
    this->blockValue = -1;
    this->isFilled = false;
}

void Block::setBlockValue(const int value) {
    this->isFilled = true;
    this->blockValue = value;
}

int Block::getBlockValue() const {
    return this->blockValue;
}

void Block::setIsFilled(const bool value) {
    this->isFilled = value;
}

bool Block::getIsFilled() const {
    return this->isFilled;
}

void Block::resetBlock() {
    this->isFilled = false;
    this->blockValue = -1;
}

void Block::print() const {
    cout << setw(3) << this->blockValue;
}
