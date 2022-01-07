# Test about Haar-wavelet-transform
This program do image analysis and synthesis using simple Harr-wavelet-transform.


Input  :
original YUV 4:0:0 8bit format image

Output :
Harr-wavelet-transform coefficient images, encode file, reconstructed image

----------------

## Usage
set input image path
``` C++
string input_file = "./Input/Lenna_256x256_yuv400_8bit.raw";
```

set encode file and output image path
``` C++
string encode_file = "./encode_file";
string output_file = "./reconstructed_image_" + to_string(width) + "x" + to_string(height) + "_yuv400_8bit.raw";
```

set input image's width & height
``` C++
int width = 256;
int height = 256;
```

set transform iteration
``` C++
int iteration = 3;
```
and run program!

Then you can get below output:

+ Harr-wavelet-transform coefficient images

  ![image](https://user-images.githubusercontent.com/26856370/148579280-b81a8658-4c3d-4cbb-b100-3d20c079619c.png)

+ reconstructed image 

  ![image](https://user-images.githubusercontent.com/26856370/148579362-727dc098-12b2-41b3-9f15-6dc1ded0c3ca.png)

+ MSE of two reconstructed images

  ![image](https://user-images.githubusercontent.com/26856370/148579064-1ef89fc6-f712-43ac-80d1-81dc8bb7d767.png)

 
