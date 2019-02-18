// ConsoleApplication_test1.cpp: 定义控制台应用程序的入口点。
//
/****************************************************xiao7@copyright_allrightreserive****************************************************
****************************************************************************************************************************************/
#include "stdafx.h"
#include <iostream>
#include<fstream>
#include<string>
#include<vector>
#include <cmath>
#include <opencv2/core/core.hpp>  
#include <opencv2/highgui/highgui.hpp>
#define PI 3.141592658
using namespace std;
using namespace cv;
//声明vec数组循环右移函数
void array_rightshift(vector<int> &, int &);

int main()
{
	Mat lena = imread("lena512color.tiff", IMREAD_COLOR);
	//读取图像行列及通道数
	int row_size = lena.rows;
	int cols_size = lena.cols;
	int alpha_size = lena.channels();
	int lena_sum = 0;
	////对图像像素求和，提取图像特征信息
	for (int k = 0; k < alpha_size; k++)
	{
		for (int i = 0; i < row_size; i++)
		{
			for (int j = 0; j < cols_size; j++)
			{
				lena_sum = lena_sum + lena.at<Vec3b>(i, j)[k];
			}
		}
	}
	double lena_info = (lena_sum % 256) * 1.0 / 255;
	//动态申请混沌数组内存
	double *x_chaos = new double[row_size * cols_size * alpha_size];
	double *y_chaos = new double[row_size * cols_size * alpha_size];
	double *z_chaos = new double[row_size * cols_size * alpha_size];
	double *w_chaos = new double[row_size * cols_size * alpha_size];
	//结合图像特征信息初始化初值
	double tempx_last = 0.1 + lena_info;
	double tempy_last = 0.1 + lena_info;
	double tempz_last = 0.1 + lena_info;
	double tempw_last = 0.1 + lena_info;
	double tempx_new, tempy_new, tempz_new, tempw_new;
	//离散采样时间
	double tt0 = 0.001;
	//需舍去的元素个数
	int num_ignore = 100000;
	//迭代混沌序列
	for (int i = 0; i < row_size * cols_size * alpha_size + num_ignore; i++)
	{
		tempx_new = tempx_last + tt0 * (24 * (tempy_last - tempx_last) + 4 * (1 + 0.02 * tempw_last * tempw_last) * tempy_last);
		tempy_new = tempy_last + tt0 * (19 * tempx_last - tempx_last * tempz_last);
		tempz_new = tempz_last + tt0 * (tempx_last * tempx_last - 9 * tempz_last);
		tempw_new = tempw_last + tt0 * tempy_last;
		//舍去前10000次迭代结果即忽略瞬态行为
		if (i >= num_ignore)
		{
			*(x_chaos + i - num_ignore) = tempx_new;
			*(y_chaos + i - num_ignore) = tempy_new;
			*(z_chaos + i - num_ignore) = tempz_new;
			*(w_chaos + i - num_ignore) = tempw_new;
		}
		tempx_last = tempx_new;
		tempy_last = tempy_new;
		tempz_last = tempz_new;
		tempw_last = tempw_new;
	}
	//对混沌序列进行处理
	double x_max = *max_element(x_chaos, x_chaos + row_size * cols_size * alpha_size - 1);
	double x_min = *min_element(x_chaos, x_chaos + row_size * cols_size * alpha_size - 1);
	double y_max = *max_element(y_chaos, y_chaos + row_size * cols_size * alpha_size - 1);
	double y_min = *min_element(y_chaos, y_chaos + row_size * cols_size * alpha_size - 1);
	for (int i = 0; i < row_size * cols_size * alpha_size; i++)
	{
		//x归一化,三角变换，取模
		*(x_chaos + i) = (*(x_chaos + i) - x_min) / (x_max - x_min);
		*(x_chaos + i) = 1.0 / PI * asin(sqrt(*(x_chaos + i)));
		*(x_chaos + i) = int(floor(pow(10, 8)*(*(x_chaos + i)))) % 256;
		*(y_chaos + i) = (*(y_chaos + i) - y_min) / (y_max - y_min);
		*(y_chaos + i) = 1.0 / PI * asin(sqrt(*(y_chaos + i)));
		*(y_chaos + i) = int(floor(pow(10, 8)*(*(y_chaos + i)))) % 256;
	}
	int count_n = 0;
	//声明、定义行操作数组与加密图像矩阵；
	vector<int> lenarow(cols_size);
	Mat encryption_lena = lena.clone();
	//申明变换位数变量
	int num_shf;

	//进行行移位与扩散
	//注意的是行变换使用的是x混沌序列
	for (int k = 0; k < alpha_size; k++)
	{
		for (int i = 0; i < row_size; i++)
		{
			for (int j = 0; j < cols_size; j++)
			{
				lenarow[j] = lena.at<Vec3b>(i, j)[k];
			}
			//强制类型转换
			num_shf = static_cast<int>(*(x_chaos + count_n));
			array_rightshift(lenarow, num_shf);
			if (i == 0)
			{
				for (int j = 0; j < cols_size; j++)
				{
					//给加密像素矩阵赋值
					int temp = lenarow[j] ^ static_cast<int>(*(x_chaos + count_n));
					encryption_lena.at<Vec3b>(i, j)[k] = temp;

					//encryption_lena.at<Vec3b>(i, j)[k] = lenarow[j];
					count_n++;
				}
			}
			else
			{
				for (int j = 0; j < cols_size; j++)
				{
					//给加密像素矩阵赋值
					int temp = lenarow[j] ^ static_cast<int>(encryption_lena.at<Vec3b>(i - 1, j)[k]);
					temp = temp ^ static_cast<int>(*(x_chaos + count_n));
					encryption_lena.at<Vec3b>(i, j)[k] = temp;

					//encryption_lena.at<Vec3b>(i, j)[k] = lenarow[j];
					count_n++;
				}
			}
		}
	}

	//定义列标号与列操作数组
	int count_m = 0;
	vector<int> lenacols(row_size);
	//进行列移位与扩散
	//注意的是列变换使用的是y混沌序列
	for (int k = 0; k < alpha_size; k++)
	{
		for (int j = 0; j < cols_size; j++)
		{
			for (int i = 0; i < row_size; i++)
			{
				lenacols[i] = encryption_lena.at<Vec3b>(i, j)[k];
			}
			//强制类型转换
			num_shf = static_cast<int>(*(y_chaos + count_m));
			array_rightshift(lenacols, num_shf);
			if (j == 0)
			{
				for (int i = 0; i < row_size; i++)
				{
					//给加密像素矩阵赋值
					int temp = lenacols[i] ^ static_cast<int>(*(y_chaos + count_m));
					encryption_lena.at<Vec3b>(i, j)[k] = temp;
					count_m++;
				}
			}
			else
			{
				for (int i = 0; i < row_size; i++)
				{
					//给加密像素矩阵赋值
					int temp = lenacols[i] ^ encryption_lena.at<Vec3b>(i, j - 1)[k];
					temp = temp ^ static_cast<int>(*(y_chaos + count_m));
					encryption_lena.at<Vec3b>(i, j)[k] = temp;
					count_m++;
				}
			}

		}
	}

	count_m--;
	//解密操作
	//进行列解密
	Mat decryption_lena = encryption_lena.clone();
	for (int k = alpha_size-1; k >= 0; k--)
	{
		for (int j = cols_size - 1; j >= 0; j--)
		{
			if (j == 0)
			{
				for (int i = row_size - 1; i >= 0; i--)
				{
					lenacols[i] = static_cast<int>(encryption_lena.at<Vec3b>(i, j)[k]) ^ static_cast<int>(*(y_chaos + count_m));
					count_m--;
				}
			}
			else
			{
				for (int i = row_size - 1; i >= 0; i--)
				{
					lenacols[i] = static_cast<int>(encryption_lena.at<Vec3b>(i, j)[k]) ^ static_cast<int>(*(y_chaos + count_m));
					lenacols[i] = lenacols[i] ^ static_cast<int>(encryption_lena.at<Vec3b>(i, j - 1)[k]);
					count_m--;
				}
			}
			int num_shf = row_size - static_cast<int>(*(y_chaos + count_m + 1));
			array_rightshift(lenacols, num_shf);
			for (int i = 0; i < row_size; i++)
			{
				decryption_lena.at<Vec3b>(i, j)[k] = lenacols[i];
			}
		}
	}

	count_n--;
	//进行行解密
	for (int k = alpha_size - 1; k >= 0; k--)
	{
		for (int i = row_size - 1; i >= 0; i--)
		{
			if (i == 0)
			{
				for (int j = cols_size - 1; j >= 0; j--)
				{
					lenarow[j] = static_cast<int>(decryption_lena.at<Vec3b>(i, j)[k]) ^ static_cast<int>(*(x_chaos + count_n));

					//lenarow[j] = decryption_lena.at<Vec3b>(i, j)[k];
					count_n--;
				}
			}
			else
			{
				for (int j = cols_size - 1; j >= 0; j--)
				{
					lenarow[j] = static_cast<int>(decryption_lena.at<Vec3b>(i, j)[k]) ^ static_cast<int>(*(x_chaos + count_n));
					lenarow[j] = lenarow[j] ^ static_cast<int>(decryption_lena.at<Vec3b>(i - 1, j)[k]);

					//lenarow[j] = decryption_lena.at<Vec3b>(i, j)[k];
					count_n--;
				}
			}
			num_shf = cols_size - static_cast<int>(*(x_chaos + count_n + 1));
			array_rightshift(lenarow, num_shf);
			for (int j = 0; j < cols_size; j++)
			{
				decryption_lena.at<Vec3b>(i, j)[k] = lenarow[j];
			}
		}
	}

	namedWindow("Lena_Original", WINDOW_AUTOSIZE);
	imshow("Lena_Original", lena);
	namedWindow("Lena_Encryption", WINDOW_AUTOSIZE);
	imshow("Lena_Encryption", encryption_lena);
	namedWindow("Lena_Decryption", WINDOW_AUTOSIZE);
	imshow("Lena_Decryption", decryption_lena);
	//delay 60000ms
	waitKey(60000);

	return 0;
}

//定义vec数组循环右移函数
void array_rightshift(vector<int> &a, int &N)
{
	int bitnum = static_cast<int>(a.size());
	int num_shift1 = N % bitnum;
	vector<int> temp(bitnum);
	for (int i = 0; i < num_shift1; i++)
	{
		temp[i] = a[bitnum - num_shift1 + i];
	}
	for (int j = 0; j < bitnum - num_shift1; j++)
	{
		temp[j + num_shift1] = a[j];
	}
	for (int k = 0; k < bitnum; k++)
	{
		a[k] = temp[k];
	}
}


