//
// Created by Sidorenko Nikita on 2018-12-13.
//

#include "ModelLoaderUtils.h"

void loader::flipMatrix(mat4 &matrix) {
	// Invert 3rd column and 3rd row to convert matrix from RHS to LHS
	matrix[2] = -matrix[2];
	matrix[0].z = -matrix[0].z;
	matrix[1].z = -matrix[1].z;
	matrix[2].z = -matrix[2].z;
	matrix[3].z = -matrix[3].z;
}

mat4 loader::getMatrixFromJSON(json matrix) {
  mat4 result;

  if (matrix.is_array()) {
    int counter = 0;
    for (auto item : matrix) {
      auto number = item.get<float>();
      int column = counter / 4;
      int row = counter % 4;
      result[column][row] = number;
      counter++;
    }
  }

//    ENGLog("JSON Matrix:\n%s\nParsed matrix:\n%s", matrix.dump().c_str(), to_string(result).c_str());

  //flipMatrix(result);
  return result;
}

