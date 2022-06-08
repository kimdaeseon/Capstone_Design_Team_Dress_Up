#pragma once
#pragma warning(disable:4996)

#include <vector>
#include <string>

using namespace std;

#include "Vertex.h"



const int scale = 1000;

class ObjParser {
public:
	void parse(const char* line) {
		prasingWithTexture(line);
	}

	void calculateFace() {
		try {
			for (unsigned int i = 0; i < vertexIndices.size(); i++) {
				unsigned int vertexIndex = vertexIndices[i];
				Vertex tempVertex = vertex[vertexIndex - 1];
				realVertex.push_back(tempVertex);
			}
			for (unsigned int i = 0; i < uvIndices.size(); i++) {
				unsigned int uvIndex = uvIndices[i];
				Vertex tempUv = texture[uvIndex - 1];
				realTexture.push_back(tempUv);
			}
			for (unsigned int i = 0; i < colorIndices.size(); i++) {
				unsigned int colorIndex = colorIndices[i];
				Vertex tempColor = color[colorIndex - 1];
				realColor.push_back(tempColor);
			}
		}
		catch(string err) {
			return;
		}
	}

	vector<Vertex> vertex;
	vector<Vertex> color;
	vector<Vertex> texture;

	vector<Vertex> skeleton;

	vector<int> vertexIndices;
	vector<int> uvIndices;
	vector<int> colorIndices;

	vector<Vertex> realVertex;
	vector<Vertex> realColor;
	vector<Vertex> realTexture;
private:

	void prasingWithTexture(const char* line) {
		float x, y, z;
		char lineHeader[128];
		sscanf(line, "%s %f %f %f", lineHeader, &x, &y, &z);
		if (strcmp(lineHeader, "v") == 0) {
			Vertex tempVertex;
			tempVertex.X = x / scale;
			tempVertex.Y = -y / scale;
			tempVertex.Z = z / scale;
			vertex.push_back(tempVertex);
		}
		else if (strcmp(lineHeader, "b") == 0) {
			Vertex tempVertex;
			tempVertex.X = x / scale;
			tempVertex.Y = -y / scale;
			tempVertex.Z = z / scale;
			skeleton.push_back(tempVertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			Vertex tempVertex;
			tempVertex.X = x;
			tempVertex.Y = y;
			texture.push_back(tempVertex);
		}
		else if (strcmp(lineHeader, "c") == 0) {
			Vertex tempVertex;
			tempVertex.X = x / 255;
			tempVertex.Y = y / 255;
			tempVertex.Z = z / 255;
			color.push_back(tempVertex);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			unsigned int vertexIndex[4], uvIndex[4], colorIndex[4];
			int matches = sscanf(line, "%s %d/%d/%d %d/%d/%d %d/%d/%d\n", lineHeader, &vertexIndex[0], &uvIndex[0], &colorIndex[0], &vertexIndex[1], &uvIndex[1], &colorIndex[1], &vertexIndex[2], &uvIndex[2], &colorIndex[2]);
			if (matches != 10) {
				printf("File can't be read by our simple parser : ( Try exporting with other options\n");
				return;
			}
	
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);

			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);

			colorIndices.push_back(colorIndex[0]);
			colorIndices.push_back(colorIndex[1]);
			colorIndices.push_back(colorIndex[2]);
		}
	}
};