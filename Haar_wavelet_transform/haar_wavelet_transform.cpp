/*
 * Program's process:
 *  input image -> Haar wavelet transform -> Quantization
 *              -> Inverse quantization   -> Inverse Haar wavelet transform
 *   -> reconstructed image
 *
 *  You can check encode & decode process using Haar wavelet transform.
 *  This program is valid for YUV 4:0:0 8bit format images.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
using namespace std;

void Read_YUV400(int, int, string, char*);
void Write_YUV400(int, int, string, char*);
void Encode_YUV400(int, int, int, unsigned char*, string);
void Decode_YUV400(int, int, int, unsigned char*, string);
void Haar_Wavelet_transform(int, int, double**);
void Inverse_Haar_Wavelet_transform(int, int, int, double**);
void Quantization(int, int, int, double**, string);
void Dequantization(int, int, int, unsigned char*, double**);
double GetMSE(int, int, unsigned char*, unsigned char*);

int main()
{
	/* set input image's width & height and transform iteration */
	int width = 256;
	int height = 256;
	int iteration = 3;

	/* set file name */
	string input_file = "./Input/Lenna_256x256_yuv400_8bit.raw";
	string encode_file = "./encode_file";
	string output_file = "./reconstructed_image_" + to_string(width) + "x" + to_string(height) + "_yuv400_8bit.raw";

	/* original image */
	unsigned char* Y = new unsigned char[width * height];
	/* encoded data */
	unsigned char* Y_code = new unsigned char[width * height];
	/* reconstructed image */
	unsigned char* Y_R = new unsigned char[width * height];

	/* Read YUV 4:0:0 image */
	Read_YUV400(width, height, input_file, (char*)Y);

	/* Encode YUV 4:0:0 image with Haar Wavelet transform  */
	Encode_YUV400(width, height, iteration, Y, encode_file);

	/* Read encoded file */
	Read_YUV400(width, height, encode_file, (char*)Y_code);

	/* Decode encoded file with inverse Haar Wavelet transform  */
	Decode_YUV400(width, height, iteration, Y_code, output_file);

	/* Read reconstructed file */
	Read_YUV400(width, height, output_file, (char*)Y_R);

	/* Get MSE */
	printf("MSE : %f\n", GetMSE(width, height, Y, Y_R));

	delete[]Y, Y_code, Y_R;
}


/* Read Y data of YUV 4:0:0 image */
void Read_YUV400(int width, int height, /* image width & height */
				 string input_file,     /* file name to read    */
				 char* Y)               /* array to save data   */
{
	/* open image file */
	ifstream fin(input_file.c_str(), ios_base::in | ios_base::binary);
	if (!fin.is_open()) {
		printf("Error : Input file open fail");
		exit(1);
	}
	/* read image file */
	fin.read(Y, height * width);
	/* close image file */
	fin.close();
}
/* Write Y data of YUV 4:0:0 image */
void Write_YUV400(int width, int height,  /* image width & height   */
				  string output_file,     /* file name to write     */
				  char* Y)                /* array saved write data */
{
	/* create image file */
	ofstream fout(output_file.c_str(), ios::out | ios::binary);
	if (!fout.is_open()) {
		printf("Error : Output file create fail");
		exit(1);
	}
	/* write image file */
	fout.write(Y, width * height);
	/* close image file */
	fout.close();
}
/* Get MSE */
double GetMSE(int width, int height,  /* image width & height */
			  unsigned char* Y,       /* original image       */
			  unsigned char* Y_R)     /* reconstructed image  */
{
	double mse = 0;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			mse += pow(Y[i + width + j] - Y_R[i + width + j], 2);
		}
	}
	return mse / (height * width);
}

/* Encode YUV 4:0:0 image with Haar Wavelet transform  */
void Encode_YUV400(int width, int height,  /* image width & height        */
				   int iteration,          /* transform result image data */
				   unsigned char* Y,	   /* image data before transform */
				   string encode_file)     /* encode file name            */
{
	/* Copy original data to 2D double array for transform */
	double** coefficient = new double* [height];
	for (int i = 0; i < height; i++)
		coefficient[i] = new double[width];
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
			coefficient[i][j] = Y[i * width + j];
	}

	for (int k = 0; k < iteration; k++)
	{
		int transform_width = width / pow(2, k);
		int transform_height = height / pow(2, k);

		/* Do (i+1) step Haar Wavelet transform */
		Haar_Wavelet_transform(transform_width, transform_height, coefficient);

		/* Write (i+1) step transform coefficient to image file */
		unsigned char* normalized_coefficient = new unsigned char[width * height];
		for (int m = 0; m < height; m++)
		{
			for (int n = 0; n < width; n++)
			{
				if (m < transform_height / 2 && n < transform_width / 2) 
					normalized_coefficient[m * width + n] = (unsigned char)coefficient[m][n];
				else 
					normalized_coefficient[m * width + n] = (unsigned char)(coefficient[m][n] + 127.5);
			}
		}
		string coefficient_file = "./" + to_string(k + 1) + "_transform_coefficient_" + to_string(width) + "x" + to_string(height) + "_yuv400_8bit.raw";
		Write_YUV400(width, height, coefficient_file, (char*)normalized_coefficient);
		delete[]normalized_coefficient;
	}

	/* Quantize coefficient data and Write encoded file */
	Quantization(width, height, iteration, coefficient, encode_file);

	for (int i = 0; i < height; i++)
		delete[]coefficient[i];
	delete[]coefficient;
}

/* Do Haar Wavelet transform about YUV 4:0:0 image */
void Haar_Wavelet_transform(int N, int M,         /* transform size width & height  */
							double** coefficient) /* transform coeffieient array    */
{
	/* 2D array to save horizontal low & high pass filtered data */
	double** LPF_hor = new double* [M];
	double** HPF_hor = new double* [M];
	for (int i = 0; i < M; i++)
	{
		LPF_hor[i] = new double[N / 2];
		HPF_hor[i] = new double[N / 2];
	}

	/* Do horizontal low & high pass filtering :
	 * LPF = { 1/2,  1/2 }, HPF = { 1/2, -1/2 }
	 */
	for (int i = 0; i < M; i++)
	{
		for (int j = 0; j < N / 2; j++)
		{
			LPF_hor[i][j] = (coefficient[i][2 * j] + coefficient[i][2 * j + 1]) / 2;
			HPF_hor[i][j] = (coefficient[i][2 * j] - coefficient[i][2 * j + 1]) / 2;
		}
	}

	/* Do vertical low & high pass filtering :
	 * LPF = { 1/2,  1/2 }, HPF = { 1/2, -1/2 }
	 */
	for (int i = 0; i < M / 2; i++)
	{
		for (int j = 0; j < N / 2; j++)
		{
			coefficient[i        ][j        ] = (LPF_hor[2 * i][j] + LPF_hor[2 * i + 1][j]) / 2;  // LL
			coefficient[i + M / 2][j        ] = (LPF_hor[2 * i][j] - LPF_hor[2 * i + 1][j]) / 2;  // LH
			coefficient[i        ][j + N / 2] = (HPF_hor[2 * i][j] + HPF_hor[2 * i + 1][j]) / 2;  // HL
			coefficient[i + M / 2][j + N / 2] = (HPF_hor[2 * i][j] - HPF_hor[2 * i + 1][j]) / 2;  // HH
		}
	}

	for (int i = 0; i < M; i++)
	{
		delete[]LPF_hor[i];
		delete[]HPF_hor[i];
	}
	delete[]LPF_hor, HPF_hor;
}

/* Quantize Haar Wavelet transform's coefficient data and Write encode file */
void Quantization(int width, int height,  /* image width & height        */
				  int iteration,		  /* transform iteration         */
				  double** coefficient,   /* transform coeffieient array */
				  string encode_file)     /* encode file name            */
{
	int Lwidth = width / pow(2, iteration);
	int Lheight = height / pow(2, iteration);

	/* array to save quantized data */
	unsigned char* Y_quan = new unsigned char[width * height];
	int idx = 0;
	/* Quantize LL block to 8bits */
	for (int i = 0; i < Lheight; i++)
	{
		for (int j = 0; j < Lwidth; j++)
			Y_quan[idx++] = (unsigned char)coefficient[i][j];
	}
	/* Quantize LH, HL, HH blocks to 7, 6, ... bits */
	for (int k = 1; k <= iteration; k++)
	{
		/* Set quantization bit */
		int quan_bits = 8 - k;
		int quan_step = 256 / pow(2, quan_bits);
		int block_width = Lwidth * pow(2, k - 1);
		int block_height = Lheight * pow(2, k - 1);

		for (int i = 0; i < block_height; i++)
		{
			for (int j = 0; j < block_width; j++)
			{
				// quantize LH block
				Y_quan[idx++] = (unsigned char)(coefficient[i + block_height][j              ] + 127.5) / quan_step;
				// quantize HL block
				Y_quan[idx++] = (unsigned char)(coefficient[i               ][j + block_width] + 127.5) / quan_step;
				// quantize HH block
				Y_quan[idx++] = (unsigned char)(coefficient[i + block_height][j + block_width] + 127.5) / quan_step;
			}
		}
	}
	
	/* create encode file */
	ofstream fout(encode_file.c_str(), ios::out | ios::binary);
	if (!fout.is_open()) {
		printf("Error : Output file create fail");
		exit(1);
	}
	/* encode LL block */
	fout.write((char*)Y_quan, Lwidth * Lheight);
	idx = Lwidth * Lheight;
	/* encode LH, HL, HH blocks */
	unsigned char buf = 0;
	int buf_remains = 8;
	for (int k = 1; k <= iteration; k++)
	{
		/* Set quantization bit */
		int quan_bits = 8 - k;
		int block_size = (Lwidth * pow(2, k - 1)) * (Lheight * pow(2, k - 1));
		for (int i = 0; i < 3 * block_size; i++)
		{
			unsigned char temp = Y_quan[idx++];
			if (buf_remains < quan_bits) {
				buf = buf | (temp >> (quan_bits - buf_remains));
				fout.write((char*)&buf, 1);
				buf = temp << (8 - (quan_bits - buf_remains));
				buf_remains = 8 - (quan_bits - buf_remains);
			}
			else {
				buf = buf | (temp << (buf_remains - quan_bits));
				buf_remains -= quan_bits;
			}
		}
	}
	fout.write((char*)&buf, 1);
	fout.close();

	delete[]Y_quan;
}


/* Decode encoded file with inverse Haar Wavelet transform  */
void Decode_YUV400(int width, int height,  /* image width & height    */
				   int iteration,		   /* transform iteration     */
				   unsigned char* Y_code,  /* encode data             */
				   string output_file)     /* reconstructed file name */
{
	double** coefficient = new double* [height];
	for (int i = 0; i < height; i++)
		coefficient[i] = new double[width];

	/* Decode & Dequantize encoded file and reconstuct coefficient */
	Dequantization(width, height, iteration, Y_code, coefficient);

	/* Do inverse Haar Wavelet transform */
	Inverse_Haar_Wavelet_transform(width, height, iteration, coefficient);

	/* Write resconstructed image */
	unsigned char* Y_R = new unsigned char[width * height];
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
			Y_R[i * width + j] = (unsigned char)coefficient[i][j];
	}
	Write_YUV400(width, height, output_file, (char*)Y_R);

	for (int i = 0; i < height; i++)
		delete[]coefficient[i];
	delete[]coefficient, Y_R;
}

/* Decode & Dequantize encoded file and reconstuct coefficient */
void Dequantization(int width, int height,  /* image width & height     */
					int iteration,		    /* transform iteration      */
					unsigned char* Y_code,  /* encode data              */
					double** coefficient)   /* reconstucted coefficient */
{
	int Lwidth = width / pow(2, iteration);
	int Lheight = height / pow(2, iteration);
	int idx = 0;

	/* decode and dequantize LL block */
	for (int i = 0; i < Lheight; i++)
	{
		for (int j = 0; j < Lwidth; j++)
			coefficient[i][j] = (double)Y_code[idx++];
	}

	/* decode and dequantize LH, HL, HH blocks */
	unsigned char buf = 0;
	int valid_bits = 8;
	for (int k = 1; k <= iteration; k++)
	{
		/* Set quantization bit */
		int quan_bits = 8 - k;
		int quan_step = 256 / pow(2, quan_bits);
		int block_width = Lwidth * pow(2, k - 1);
		int block_height = Lheight * pow(2, k - 1);
		for (int i = 0; i < block_height; i++)
		{
			for (int j = 0; j < block_width; j++)
			{
				// decode and dequantize LH block
				if (valid_bits < quan_bits) {
					buf = (Y_code[idx] << (8 - valid_bits)) >> (8 - quan_bits);
					buf = buf | (Y_code[++idx] >> (8 - (quan_bits - valid_bits)));
					valid_bits = 8 - (quan_bits - valid_bits);
				}
				else {
					buf = (Y_code[idx] << (8 - valid_bits)) >> (8 - quan_bits);
					valid_bits -= quan_bits;
				}
				coefficient[i + block_height][j] = (double)buf * quan_step + (quan_step / 2) - 127.5;

				// decode and dequantize HL block 
				if (valid_bits < quan_bits) {
					buf = (Y_code[idx] << (8 - valid_bits)) >> (8 - quan_bits);
					buf = buf | (Y_code[++idx] >> (8 - (quan_bits - valid_bits)));
					valid_bits = 8 - (quan_bits - valid_bits);
				}
				else {
					buf = (Y_code[idx] << (8 - valid_bits)) >> (8 - quan_bits);
					valid_bits -= quan_bits;
				}
				coefficient[i][j + block_width] = (double)buf * quan_step + (quan_step / 2) - 127.5;

				// decode and dequantize HH block
				if (valid_bits < quan_bits) {
					buf = (Y_code[idx] << (8 - valid_bits)) >> (8 - quan_bits);
					buf = buf | (Y_code[++idx] >> (8 - (quan_bits - valid_bits)));
					valid_bits = 8 - (quan_bits - valid_bits);
				}
				else {
					buf = (Y_code[idx] << (8 - valid_bits)) >> (8 - quan_bits);
					valid_bits -= quan_bits;
				}
				coefficient[i + block_height][j + block_width] = (double)buf * quan_step + (quan_step / 2) - 127.5;
			}
		}
	}
}

/* Do inverse Haar Wavelet transform */
void Inverse_Haar_Wavelet_transform(int width, int height,  /* image width & height     */
									int iteration,		    /* transform iteration      */
									double** coefficient)   /* reconstucted coefficient */
{
	for (int k = iteration; k > 0; k--)
	{
		int Lwidth = width / pow(2, k);
		int Lheight = height / pow(2, k);

		/* vertical interpolation */
		double** LPF_hor = new double* [Lheight * 2];
		double** HPF_hor = new double* [Lheight * 2];
		for (int i = 0; i < Lheight * 2; i++)
		{
			LPF_hor[i] = new double[Lwidth];
			HPF_hor[i] = new double[Lwidth];
		}
		for (int i = 0; i < Lheight; i++)
		{
			for (int j = 0; j < Lwidth; j++)
			{
				LPF_hor[2 * i    ][j] = coefficient[i][j] + coefficient[i + Lheight][j];
				LPF_hor[2 * i + 1][j] = coefficient[i][j] - coefficient[i + Lheight][j];
				HPF_hor[2 * i    ][j] = coefficient[i][j + Lwidth] + coefficient[i + Lheight][j + Lwidth];
				HPF_hor[2 * i + 1][j] = coefficient[i][j + Lwidth] - coefficient[i + Lheight][j + Lwidth];
			}
		}

		/* horizontal interpolation */
		for (int i = 0; i < Lheight * 2; i++)
		{
			for (int j = 0; j < Lwidth; j++)
			{
				coefficient[i][2 * j    ] = LPF_hor[i][j] + HPF_hor[i][j];
				coefficient[i][2 * j + 1] = LPF_hor[i][j] - HPF_hor[i][j];
			}
		}

		for (int i = 0; i < Lheight * 2; i++)
		{
			delete[]LPF_hor[i];
			delete[]HPF_hor[i];
		}
		delete[]LPF_hor, HPF_hor;
	}
}
