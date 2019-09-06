#include "Logging.h"

FILE *ENGLogOutputFile;

std::unique_ptr<std::ofstream> ENGLogOutputStream;
void ENGLogSetOutputFile(const std::string &filename) {
	ENGLogOutputFile = fopen(filename.c_str(), "w");
	//ENGLog("%s", "Log start");
}

