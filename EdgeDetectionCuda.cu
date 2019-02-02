#include <iostream>
#include <math.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <time.h>
#include <stdlib.h>

// For the CUDA runtime routines (prefixed with "cuda_")
#include <cuda_runtime.h>

using namespace std;

const int THREADS_PER_BLOCK = 512;

/**
 * CUDA Kernel Device code
 */ 
/*****************************************************************************/

__global__ void scaleImageCuda (int *pixels, int minpix, int maxpix, int imageSize) {
	/* blockDim.x gives the number of threads per block, combining it
	with threadIdx.x and blockIdx.x gives the index of each global
	thread in the device */
	int index = threadIdx.x * blockIdx.x * threadIdx.x;
	int value;
	/* Typical problems are not friendly multiples of blockDim.x.
	Avoid accesing data beyond the end of the arrays */
	if (index < imageSize) {
		value = round(((double)(pixels[index] - minpix) / (maxpix - minpix)) * 255);
		pixels[index] = value;
	}

    __syncthreads();
}

__global__ void edgeDetectionCuda (int *pixels, int *tempImage, int width, int height, int imageSize) {
	/* blockDim.x gives the number of threads per block, combining it
	with threadIdx.x and blockIdx.x gives the index of each global
	thread in the device */
	//int index = threadIdx.x * blockIdx.x * threadIdx.x;
	int index = (blockDim.x * blockIdx.x) + threadIdx.x;
	int x = 0, y = 0;
	int xG = 0, yG = 0;

	/* Typical problems are not friendly multiples of blockDim.x.
	Avoid accesing data beyond the end of the arrays */
	if (index < imageSize) {
		x = index % width;

		if (index != 0) {
			y = __double2int_rd((__int2double_rn(index) / __int2double_rn(width)));	
		}
		
		if (x < (width - 1) && y < (height - 1)
				&& (y > 0) && (x > 0)) {

			//index = x + (y * width)
			//Finds the horizontal gradient
			xG = (pixels[(x+1) + ((y-1) * width)]
						 + (2 * pixels[(x+1) + (y * width)])
						 + pixels[(x+1) + ((y+1) * width)]
								  - pixels[(x-1) + ((y-1) * width)]
										   - (2 * pixels[(x-1) + (y * width)])
										   - pixels[(x-1) + ((y+1) * width)]);

			//Finds the vertical gradient
			yG = (pixels[(x-1) + ((y+1) * width)]
						 + (2 * pixels[(x) + ((y + 1) * width)])
						 + pixels[(x+1) + ((y+1) * width)]
								  - pixels[(x-1) + ((y-1) * width)]
										   - (2 * pixels[(x) + ((y-1) * width)])
										   - pixels[(x+1) + ((y-1) * width)]);
			tempImage[index] = __double2int_rn(sqrt(__int2double_rn(xG * xG) + __int2double_rn(yG * yG)));

		} else {

			//Pads out of bound pixels with 0
			tempImage[index] = 0;

		}
	}

    //__syncthreads();
}

//Creating image class (base class)
class Image{

public:

	Image():
		height(0),
		width(0),
		maxPixelValue(0),
		minpix(0),
		maxpix(0),
		imageSize(0){}
	virtual ~Image(){}

	virtual void readImage(ifstream &inFile) = 0;
	virtual void writeImage(ofstream &outFile) = 0;

	void readHeader(ifstream &inFile);
	void scaleImage();
	void edgeDection();

	//Accessor methods
	int getHeight(){return height;}
	int getWidth(){return width;}
	int getMaxPixelValue(){return maxPixelValue;}

	//Mutator methods
	void setHeight(int h){height = h;}
	void setWidth(int w){width = w;}
	void setMaxPixelValue(int mpv){maxPixelValue = mpv;}

	//Member variables
protected:

	int height;
	int width;
	int maxPixelValue;
	int minpix;
	int maxpix;
	unsigned int imageSize;
	int * pixels;

	inline void findMin();
	inline void findMax();

};

//Binary image class (derived class)

class BinaryImage: public Image{

public:

	BinaryImage(){}
	~BinaryImage(){}

	void readImage(ifstream &inFile);
	void writeImage(ofstream &outFile);

};

class AsciiImage: public Image{

public:

	AsciiImage(){}
	~AsciiImage(){}

	void readImage(ifstream &infile);
	void writeImage(ofstream &outFile);

};

//Check if header contains comments
//Comments start with #
bool isComment(string comment){

	for(unsigned int i = 0; i < comment.length(); i++){

		if(comment[i] == '#') return true;

		if(!isspace(comment[i])) return false;

	}

	return true;
}

//Reads binary pixel values in image
void BinaryImage::readImage(ifstream &inFile){

	//Check if the file stream in open
	if(!inFile){

		cerr << "Could not read from file!" << endl;

		exit(1000);

	}

	//Making a temp array, and putting it on the heap
	char * byteArray = new char[imageSize + 1];

	//Read the bytes of the image, and puts data in byteArray
	inFile.read(byteArray, imageSize);

	//If reading in the data failed, return an error
	if(inFile.fail()){

		cerr << "Error: cannot read pixels." << endl;

		exit(1000);

	}

	//Set the last element in array to EOF character
	byteArray[imageSize] = '\0';

	//Put the data read from file into pixels
	pixels = (int *)malloc(imageSize * sizeof(int));
	for(unsigned int i = 0; i < imageSize; i++){

		pixels[i] = static_cast<int>
		(static_cast<unsigned char>(byteArray[i]));

	}

	//Delete the byteArray
	free(byteArray);

}

//Writes binary pixels to output file
void BinaryImage::writeImage(ofstream &outFile){

	//Check if the file stream is open
	if(!outFile){

		cerr << "Could not write to file." << endl;

		exit(1000);

	}

	//Write header
	outFile << "P5"       << " "  <<
			width         << " "  <<
			height        << " "  <<
			maxPixelValue << endl;

	//Take all pixel values from pixels and writes it to output file
	char * byteArray = new char[imageSize + 1];

	for(unsigned int i = 0; i < imageSize; i++){

		byteArray[i] = static_cast<char>(pixels[i]);

	}

	byteArray[imageSize] = '\0';

	outFile.write(byteArray, imageSize);

	if(outFile.fail()){

		cerr << "Error: error writing to file." << endl;

		exit(1000);

	}

	free(byteArray);
	free(pixels);
	//delete[] byteArray;

}

void AsciiImage::readImage(ifstream &inFile){

	//Check if the file opened properly
	if(!inFile){

		cerr << "Could not read from file." << endl;

		exit(1001);

	}

	int pixelValue;

	//Read in the Ascii values from file
	int i = 0;
	while(inFile >> pixelValue){

		pixels[i] = pixelValue;
		i++;

	}


}


void AsciiImage::writeImage(ofstream &outFile){

	//Check if file is open
	if(!outFile){

		cerr << "Could not write to file." << endl;

		exit(1001);

	}

	//Write Header
	outFile << "P2" << ' ' <<
			width << ' ' <<
			height << ' ' <<
			maxPixelValue << '\n';

	//Write the contents of pixels to the output file
	for(unsigned int i = 0; i < imageSize; i++){

		//Add a '\n' at the end of each row
		if(i % width == 0 && i != 0) outFile << '\n';

		outFile << pixels[i] << '\t';

	}

	free(pixels);
}

void Image::readHeader(ifstream &inFile){

	stringstream sStream;

	string line;

	//Check if the file opened successfully
	if(!inFile){

		cerr << "Error: Could not open file." << endl;

		exit(1002);

	}

	char readChar;

	string errorMessage = "Error: incorrect picture format.";

	getline(inFile, line);

	unsigned int lineSize = line.length();

	//After we read magic number, we read the next line and determine if it's valid
	for(unsigned int i = 0; i < lineSize; i++){

		if(!isspace(line[i])){

			cerr << errorMessage << endl;

			cerr << "Extra info after magic number." << endl;

			exit(1002);

		}

	}

	//Read through the rest of the header and skip through comments
	while(getline(inFile, line)){

		if(!(isComment(line))) break;

	}

	sStream << line;

	//Read in width.
	//If there is a problem, return error
	if(!(sStream >> width)){

		cerr << errorMessage << endl;

		cerr << "Cannot read width." << endl;

		exit(1002);

	}

	//Read in height
	//If there is a problem, return error
	if(!(sStream >> height)){

		cerr << errorMessage << endl;

		cerr << "Cannot read height." << endl;

		exit(1002);

	}

	//Check if there is extra information after width and height
	while(sStream >> readChar){

		if(!(isspace(readChar))){

			cerr << errorMessage << endl;

			cerr << "Extra info when reading height and width." << endl;

			exit(1002);

		}

	}

	//Make sure the height and width is positive
	if(width <= 0 || height <= 0){

		cerr << "Error: width and height cannot be negative" << endl;

		exit(1002);

	}

	//Check if there are any comments between height/width and maxPixelValue
	while(getline(inFile, line)){

		if(!(isComment(line))) break;

	}

	//Clear out the string stream
	sStream.str("");
	sStream.clear();

	sStream << line;

	//Read in the maxPixelValue
	if(!(sStream >> maxPixelValue)){

		cerr << errorMessage << endl;
		cerr << "Could not read maxPixelValue." << endl;

		exit(1002);

	}

	//Check if there is extra information after maxPixelValue
	while(sStream >> readChar){

		if(!(isspace(readChar))){

			cerr << errorMessage << endl;
			cerr << "Extra info after the max pixel value." << endl;

			exit(1002);

		}

	}

	if(maxPixelValue < 0 || maxPixelValue > 255){

		cerr << errorMessage << endl;
		cerr << "Invalid max pixel value." << endl;

		exit(1002);

	}

	imageSize = width * height;

}

//Finds the maxium pixel value in the image
void Image::findMax(){

	int maxVal = 0;

	for(unsigned int i = 0; i < imageSize; i++){

		if(pixels[i] > maxVal){

			maxVal = pixels[i];

		}

	}

	maxpix = maxVal;

}

//Finds the minimal pixel value of the image
void Image::findMin(){

	int minVal = 255;

	for(unsigned int i = 0; i < imageSize; i++){

		if(pixels[i] < minVal){

			minVal = pixels[i];

		}

	}

	minpix = minVal;

}

//Scales image so that the maximum pixel value is 255
void Image::scaleImage(){

	findMin();

	findMax();

	printf("OK1\n");

	int *d_pixels;
	size_t size = imageSize * sizeof(int);
    cudaError_t err = cudaSuccess;
	printf("OK2\n");

	/* Allocate memory in device */
	err = cudaMalloc((void **) &d_pixels, size);
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to allocate device vector pixels (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	printf("OK3\n");

	/* Copy data to device */
	err = cudaMemcpy(d_pixels, pixels, size, cudaMemcpyHostToDevice);
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to copy vector pixels from host to device (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	printf("OK4\n");

	/* Launch scaleImageCuda() kernel on device with N threads in N blocks */
	scaleImageCuda<<<(imageSize + (THREADS_PER_BLOCK - 1)) / THREADS_PER_BLOCK, THREADS_PER_BLOCK>>>(d_pixels, minpix, maxpix, imageSize);
    err = cudaGetLastError();
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to launch scaleImageCuda kernel (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	printf("OK5\n");

	/* Copy data to tohost device */
	err = cudaMemcpy(pixels, d_pixels, size, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to copy vector pixels from device to host (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	printf("OK6\n");

	/* Clean-up */
	err = cudaFree(d_pixels);
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to free device vector pixels (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	printf("OK7\n");

    err = cudaDeviceReset();
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to deinitialize the device! error=%s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	printf("OK8\n");

	maxPixelValue = 255;

}

//Sobel edge detection function - detects edges and draws an outline
void Image::edgeDection(){
    cudaError_t err = cudaSuccess;
	size_t size = imageSize * sizeof(int);
	/* Allocate memory in host */
	int *tempImage = (int *)malloc(size);

	int *d_pixels, *d_tempImage;
	/* Allocate memory in device */
	err = cudaMalloc((void **) &d_pixels, size);
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to allocate device array pixels (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	err = cudaMalloc((void **) &d_tempImage, size);
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to allocate device array tempImage (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
	}
	printf("alojo memoria\n");

	/* Copy data to device */
	err = cudaMemcpy(d_pixels, pixels, size, cudaMemcpyHostToDevice);
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to copy array pixels from host to device (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	err = cudaMemcpy(d_tempImage, tempImage, size, cudaMemcpyHostToDevice);
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to copy array tempImage from host to device (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	printf("copio memoria a device\n");

	/* Launch edgeDetectionCuda() kernel on device with N threads in N blocks */
	edgeDetectionCuda<<<(imageSize + (THREADS_PER_BLOCK - 1)) / THREADS_PER_BLOCK, THREADS_PER_BLOCK>>>(d_pixels, d_tempImage, width, height, imageSize);
    err = cudaGetLastError();
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to launch edgeDetectionCuda kernel (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	printf("ejecuto kernel\n");

	/* Copy data to host */ 
	err = cudaMemcpy(tempImage, d_tempImage, size, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to copy array tempImage from device to host (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	printf("copio memoria a host\n");

	/* Clean-up device */
	err = cudaFree(d_tempImage);
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to free array vector tempImage (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	err = cudaFree(d_pixels);
    if (err != cudaSuccess){
        fprintf(stderr, "Failed to free array vector pixels (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
	printf("libero memoria\n");

	for(unsigned int i = 0; i < imageSize; i++){

		pixels[i] = tempImage[i];

	}
	printf("copio resultado\n");

	/* Clean-up host */
	free(tempImage);
}

bool isBinary(ifstream &inFile);

void run(char **argv);

int main(int argc, char **argv){

	if(argc != 3){

		cerr << "Usage: EdgeDetection imageName.pgm output.pgm";

		return 1;

	}

	//long start, end;

	//double total;

	//start = clock();

	run(argv);

	//end = clock();

	//total = (double)(end - start)/1000;

	//cout << "Execution time: " << total << endl;

	return 0;
}


bool isBinary(ifstream &inFile){

	char readChar = ' ';

	string errorMessage = "Error: incorrect picture format.";

	//If there is no character or the character is not equal to 'P'
	//then return an error
	if(!(inFile >> readChar) || ( readChar != 'P' )){

		cerr << errorMessage << endl;
		cerr << "P" << endl;

		exit(1002);

	}

	//If there is no character or the second character is not a 2 or 5
	//then return an error
	if(!(inFile >> readChar) || ( readChar != '2' && readChar != '5')){

		cerr << errorMessage << endl;
		cerr << readChar << endl;

		exit(1002);

	}

	if(readChar == '5') return true;

	return false;

}

void run(char **argv){

	ifstream inFile;

	inFile.open(argv[1], ios::binary | ios::in);

	ofstream outFile;

	outFile.open(argv[2], ios::binary
			            | ios::out
						| ios::trunc);

	if(isBinary(inFile)){

		BinaryImage binaryImage;

		binaryImage.readHeader(inFile);

		binaryImage.readImage(inFile);

		binaryImage.edgeDection();

		binaryImage.scaleImage();

		binaryImage.writeImage(outFile);

	}else{

		AsciiImage asciiImage;

		asciiImage.readHeader(inFile);

		asciiImage.readImage(inFile);

		asciiImage.edgeDection();

		asciiImage.scaleImage();

		asciiImage.writeImage(outFile);

	}

	inFile.close();
	outFile.close();

}


