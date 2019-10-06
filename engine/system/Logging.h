#pragma once

#include "CommonIncludes.h"
#include <iostream>
#include <string>

extern FILE *ENGLogOutputFile;

#define ENGLog(fmt, ...) fprintf(ENGLogOutputFile, fmt, __VA_ARGS__);fprintf(ENGLogOutputFile, "%s", "\n");fflush(ENGLogOutputFile);

void ENGLogSetOutputFile(const std::string &filename);
