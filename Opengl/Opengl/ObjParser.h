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
		}
		catch(string err) {
			return;
		}

		/*
		for (unsigned int i = 0; i < uvIndices.size(); i++) {
			unsigned int uvIndex = uvIndices[i];
			Vertex tempUv = texture[uvIndex - 1];
			realTexture.push_back(tempUv);
		}

		for (unsigned int i = 0; i < normalIndices.size(); i++) {
			unsigned int normalIndex = normalIndices[i];
			Vertex tempNormal = normal[normalIndex - 1];
			realNormal.push_back(tempNormal);
		}
		*/
	}

	vector<Vertex> vertex;
	vector<Vertex> normal;
	vector<Vertex> texture;

	vector<Vertex> skeleton;

	vector<int> vertexIndices;
	vector<int> uvIndices;
	vector<int> normalIndices;

	vector<Vertex> realVertex;
	vector<Vertex> realNormal;
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
		else if (strcmp(lineHeader, "vn") == 0) {
			Vertex tempVertex;
			tempVertex.X = x / scale;
			tempVertex.Y = y / scale;
			tempVertex.Z = z / scale;
			normal.push_back(tempVertex);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			unsigned int vertexIndex[4], uvIndex[4], normalIndex[4];
			int matches = sscanf(line, "%s %d/%d %d/%d %d/%d\n", lineHeader, &vertexIndex[0], &uvIndex[0], &vertexIndex[1], &uvIndex[1], &vertexIndex[2], &uvIndex[2]);
			if (matches != 7) {
				printf("File can't be read by our simple parser : ( Try exporting with other options\n");
				return;
			}
	
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);

			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			/*
			uvIndices.push_back(uvIndex[3]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
			normalIndices.push_back(normalIndex[3]);
			*/
		}
	}

	void parsing(const char* line) {
		float x, y, z;
		char lineHeader[128];
		sscanf(line, "%s %f %f %f", lineHeader, &x, &y, &z);
		if (strcmp(lineHeader, "v") == 0) {
			Vertex tempVertex;
			tempVertex.X = x / scale;
			tempVertex.Y = -y / scale;
			tempVertex.Z = z / scale;
			realVertex.push_back(tempVertex);
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
			tempVertex.X = x / scale;
			tempVertex.Y = y / scale;
			realTexture.push_back(tempVertex);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			Vertex tempVertex;
			tempVertex.X = x / scale;
			tempVertex.Y = y / scale;
			tempVertex.Z = z / scale;
			realNormal.push_back(tempVertex);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[4], uvIndex[4], normalIndex[4];
			int matches = sscanf(line, "%d %d %d %d\n", &vertexIndex[0], &vertexIndex[1], &vertexIndex[2], &vertexIndex[3]);
			if (matches != 4) {
				printf("File can't be read by our simple parser : ( Try exporting with other options\n");
				return;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			vertexIndices.push_back(vertexIndex[3]);
			//uvIndices.push_back(uvIndex[0]);
			//uvIndices.push_back(uvIndex[1]);
			//uvIndices.push_back(uvIndex[2]);
			//normalIndices.push_back(normalIndex[0]);
			//normalIndices.push_back(normalIndex[1]);
			//normalIndices.push_back(normalIndex[2]);
			//normalIndices.push_back(normalIndex[3]);
		}
	}


};

/*
	FILE* file;
	file = fopen("junyong10.obj", "r");

	float x, y, z;

	while (1) {
		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.

		// else : parse lineHeader
		if (strcmp(lineHeader, "v") == 0) {
			fscanf(file, "%f %f %f\n", &x, &y, &z);
			Vertex tempVertex;
			tempVertex.X = x / scale;
			tempVertex.Y = -y / scale;
			tempVertex.Z = z / scale;
			vertex.push_back(tempVertex);
		}
		else if (strcmp(lineHeader, "b") == 0) {
			fscanf(file, "%f %f %f\n", &x, &y, &z);
			Vertex tempVertex;
			tempVertex.X = x / scale;
			tempVertex.Y = -y / scale;
			tempVertex.Z = z / scale;
			skeleton.push_back(tempVertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			fscanf(file, "%f %f\n", &x, &y);
			Vertex tempVertex;
			tempVertex.X = x / scale;
			tempVertex.Y = y / scale;
			texture.push_back(tempVertex);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			fscanf(file, "%f %f %f\n", &x, &y, &z);
			Vertex tempVertex;
			tempVertex.X = x / scale;
			tempVertex.Y = y / scale;
			tempVertex.Z = z / scale;
			normal.push_back(tempVertex);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[4], uvIndex[4], normalIndex[4];
			//int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			//int matches = fscanf(file, "%d//%d %d//%d %d//%d %d//%d\n", &vertexIndex[0], &normalIndex[0], &vertexIndex[1], &normalIndex[1], &vertexIndex[2], &normalIndex[2], &vertexIndex[3], &normalIndex[3]);
			int matches = fscanf(file, "%d %d %d %d\n", &vertexIndex[0], &vertexIndex[1], &vertexIndex[2], &vertexIndex[3]);
			if (matches != 4) {
				printf("File can't be read by our simple parser : ( Try exporting with other options\n");
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			vertexIndices.push_back(vertexIndex[3]);
			//uvIndices.push_back(uvIndex[0]);
			//uvIndices.push_back(uvIndex[1]);
			//uvIndices.push_back(uvIndex[2]);
			//normalIndices.push_back(normalIndex[0]);
			//normalIndices.push_back(normalIndex[1]);
			//normalIndices.push_back(normalIndex[2]);
			//normalIndices.push_back(normalIndex[3]);
		}
	}

	for (unsigned int i = 0; i < vertexIndices.size(); i++) {
		unsigned int vertexIndex = vertexIndices[i];
		Vertex tempVertex = vertex[vertexIndex - 1];
		realVertex.push_back(tempVertex);

		//unsigned int normalIndex = normalIndices[i];
		//Vertex tempNormal = normal[normalIndex - 1];
		//realNormal.push_back(tempNormal);

		//unsigned int uvIndex = uvIndices[i];
		//Vertex tempUv = vertex[uvIndex - 1];
		//realVertex.push_back(tempUv);
	}

	fclose(file);
			*/