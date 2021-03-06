#ifndef LIN_ALG_FNS
#define LIN_ALG_FNS

#ifndef M_PI
#define M_PI 3.14159265358979
#endif

#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <math.h>

#ifdef WIN32
#include <Eigen\Core>
#include <Eigen\Dense>
#include <Eigen\Geometry>
#else
#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#endif

#include <random>
#include <fstream>
#include <iterator>



using namespace Eigen;

template <class numType>
inline numType Dist(const std::vector<numType> &v1)
{
	numType dist = (numType)0;
	for (auto &elem : v1)
	{
		dist += pow(elem, 2);
	}
	return sqrt(dist);
}

template <class numType>
inline numType Dist(std::vector<numType> v1, const std::vector<numType> &v2)
{
	return Dist(Minus(v1, v2));
}

inline void Merge(std::vector<std::vector<double>> &incomingArray, int idx1, int end1, int idx2, int end2, int sortedColumn)
{
	std::vector<std::vector<double>> sortedArray; // initial allocation, numbers will be changed during sorting
	int firstIdx = idx1, lastIdx = end2;
	sortedArray.resize(lastIdx - firstIdx, std::vector<double>(incomingArray[0].size()));
	int i = 0;
	for (;;)
	{
		if (idx1 < end1 && idx2 < end2)
		{
			if (incomingArray[idx1][sortedColumn] <= incomingArray[idx2][sortedColumn])
			{
				sortedArray[i] = std::vector<double>(incomingArray[idx1]);
				idx1++;
			}
			else
			{
				sortedArray[i] = std::vector<double>(incomingArray[idx2]);
				idx2++;
			}
		}
		else if (idx1 < end1)
		{
			sortedArray[i] = std::vector<double>(incomingArray[idx1]);
			idx1++;
		}
		else if (idx2 < end2)
		{
			sortedArray[i] = std::vector<double>(incomingArray[idx2]);
			idx2++;
		}
		else
		{
			for (int j = 0; j < lastIdx - firstIdx; j++)
			{
				incomingArray[firstIdx + j] = std::vector<double>(sortedArray[j]);
			}
			return;
		}
		i++;
	}
}

inline void MergeSort(std::vector<std::vector<double>> &vec, int sortedColumn)
{
	int size = 1;
	int len = vec.size();
	int start1, end1, start2, end2;
	//for (auto &elem : vec)
	//{
	//	std::cout << " [";
	//	for (int i = 0; i < elem.size(); i++)
	//	{
	//		if (i == sortedColumn)
	//		{
	//			std::cout << "| " << elem[i] << " |";
	//		}
	//		else
	//		{
	//			std::cout << " " << elem[i] << " ";
	//		}
	//	}
	//	std::cout << "] ";
	//}
	//std::cout << std::endl;

	while (size < vec.size())
	{
		for (int i = 0; i < len - size; i += size * 2)
		{
			start1 = i;
			end1 = start1 + size;
			start2 = end1;
			end2 = (int)fmin(start2 + size, len);
			Merge(vec, start1, end1, start2, end2, sortedColumn);
			//for (auto &elem : vec)
			//{
			//	std::cout << " [";
			//	for (int i = 0; i < elem.size(); i++)
			//	{
			//		if (i == sortedColumn)
			//		{
			//			std::cout << "| " << elem[i] << " |";
			//		}
			//		else
			//		{
			//			std::cout << " " << elem[i] << " ";
			//		}
			//	}
			//	std::cout << "] ";
			//}
			//std::cout << std::endl;
		}
		size *= 2;
	}
}

inline Matrix3d SkewVector(const Vector3d &v)
{
	Matrix3d A;
	A << 0, -v[2], v[1],
		v[2], 0, -v[0],
		-v[1], v[0], 0;
	return A;
}

inline Vector3d BackTransform(const Vector3d &xyz, VectorXd &tVec, double timeRatio)
{
	// Separate the translation and angle-axis vectors from transformation vector
	Vector3d t = { tVec[0], tVec[1], tVec[2] }, w = { tVec[3],tVec[4], tVec[5] };
	// Separate the angle and axis of rotation matrix
	double theta = w.norm();
	Matrix3d I = Matrix3d::Identity(), wHat, R;
	if (theta > 10e-5)
	{
		w /= theta;
		// Scale the translation and angle by the time-ratio
		theta *= timeRatio;

		// Calculate the rotation matrix
		wHat = SkewVector(w);
		R = I + wHat*sin(theta) + wHat*wHat*(1 - cos(theta));
	}
	else
	{
		R = I;
	}

	t *= timeRatio;
	// Return the transformed point
	return R.transpose()*(xyz - t);
}

inline Vector3d ForwardTransform(const Vector3d &xyz, VectorXd &tVec, double timeRatio)
{
	// Separate the translation and angle-axis vectors from transformation vector
	Vector3d t = { tVec[0], tVec[1], tVec[2] }, w = { tVec[3],tVec[4], tVec[5] };
	// Separate the angle and axis of rotation matrix
	double theta = w.norm();
	Matrix3d I = Matrix3d::Identity(), wHat, R;
	if (theta > 10e-5)
	{
		w /= theta;
		// Scale the translation and angle by the time-ratio
		theta *= timeRatio;

		// Calculate the rotation matrix
		wHat = SkewVector(w);
		R = I + wHat*sin(theta) + wHat*wHat*(1 - cos(theta));
	}
	else
	{
		R = I;
	}

	t *= timeRatio;
	// Return the transformed point
	return R*xyz + t;
}

template <class numType>
std::vector<std::vector<std::vector<numType>>> OrganizePoints(std::vector<std::vector<numType>> &rawPointData)
{
	// Takes the raw pointcloud and then sorts the points into half-degree slices
	std::vector<std::vector<std::vector<numType>>> slices;
	double granularity = 0.5; // degree increment between slices
	int numSlices = (int)(360.0 / granularity);
	double theta, residual;
	slices.resize(numSlices, {});
	for (auto &elem : rawPointData)
	{
		theta = atan2(elem[1], elem[0])*180.0 / M_PI + 180.0;
		residual = fmod(theta, granularity);
		// if the point lies within 0.1deg of a slice bucket, add it to the corresponding slice list
		if (residual < 0.1) 
		{
			slices[(int)(floor(theta)/granularity)].push_back(elem);
		}
		else if (residual > 0.4)
		{
			slices[(int)(floor(theta) / granularity + 1)].push_back(elem);
		}
	}
	// sort each slice-list from bottom to top (by z-value of the points)
	for (auto &slice : slices)
	{
		MergeSort(slice, 2);
	}
	return slices;
}

template <class numType, class castType>
std::vector<std::vector<castType>> ParseBinary(numType inTypeEx, castType outTypeEx, char fileName[256])
{
	std::vector<std::vector<castType>> outputPtList;
	numType v[4];
	FILE *file;

	fopen_s(&file, fileName, "rb");

	while (fread(v, sizeof(v[0]), sizeof(v) / sizeof(v[0]), file))
	{
		outputPtList.push_back(std::vector<castType>({ (castType)v[0],(castType)v[1],(castType)v[2]}));
	}

	fclose(file);

	return outputPtList;
}

inline int MyMod(int a, int b)
{
	int r = a % b;
	return r < 0 ? r + b : r;
}

#endif