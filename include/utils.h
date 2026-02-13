#ifndef UTILS_H
#define UTILS_H
#include "include/main.h"
uint32_t simple_hash(const char *str);
Rectangle GetButtonRect(int index);
bool IsValidMediaFile(const char *path);
char *OpenFileDialog(void);
#endif