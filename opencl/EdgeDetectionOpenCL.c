#include <iostream>
#include <math.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <time.h>
#include <stdlib.h>
#include <CL/cl.h>
#include <sys/types.h>
#include "err_code.h"

using namespace std;

#define MAX_SOURCE_SIZE (0x100000)

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
	void scaleImage(int blocks, int threadsPerblock);
	void edgeDection(int blocks, int threadsPerblock);

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
void Image::scaleImage(int blocks, int threadsPerblock){

	findMin();

	findMax();

	size_t size = imageSize * sizeof(int);

	/******************************************************************************/
	/* declare opencl variables */
	cl_device_id device_id = NULL;
	cl_context context = NULL;
	cl_command_queue command_queue = NULL;
	cl_mem d_pixels = NULL;
	cl_program program = NULL;
	cl_kernel kernel = NULL;
	cl_platform_id platform_id = NULL;
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;
	cl_int ret;

	/******************************************************************************/
	/* open kernel */
	FILE *fp;
	char fileName[] = "./EdgeDetectionOpenCL.cl";
	char *source_str;
	size_t source_size;

	/* Load the source code containing the kernel*/
	fp = fopen(fileName, "r");
	if (!fp) {
		fprintf(stderr, "Failed to load kernel.\n");
		exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);

	/******************************************************************************/
	/* create objects */

	/* Get Platform and Device Info */
	ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);

	/* Create OpenCL context */
	context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
	checkError(ret, "Creating context");

	/* Create Command Queue */
	command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
	checkError(ret, "Creating queue");

	/* Create Memory Buffer */
	d_pixels = clCreateBuffer(context, CL_MEM_READ_WRITE, size, &pixels, &ret);
	checkError(ret, "Creating buffer d_pixels");

	//print kernel
	//printf("\n%s\n%i bytes\n", source_str, (int)source_size); fflush(stdout);
	/******************************************************************************/

	/* create build program */

	/* Create Kernel Program from the source */
	program = clCreateProgramWithSource(context, 1, (const char **)&source_str,
	(const size_t *)&source_size, &ret);
	checkError(ret, "Creating program");

	/* Build Kernel Program */
	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		size_t len;
		char buffer[2048];

		printf("Error: Failed to build program executable!\n%s\n", err_code(ret));
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
		printf("%s\n", buffer);
		//return EXIT_FAILURE;
		exit(1);
	}
	/* Create OpenCL Kernel */
	kernel = clCreateKernel(program, "scaleImageOpenCL", &ret);
	checkError(ret, "Creating kernel");

	/* Set OpenCL Kernel Parameters */
	ret = clSetKernelArg(kernel, 0, size, (void *)&d_pixels);
	checkError(ret, "Setting kernel arguments");
	ret = clSetKernelArg(kernel, 1, sizeof(int), &minpix);
	checkError(ret, "Setting kernel arguments");
	ret = clSetKernelArg(kernel, 2, sizeof(int), &maxpix);
	checkError(ret, "Setting kernel arguments");
	ret = clSetKernelArg(kernel, 3, sizeof(int), &imageSize);
	checkError(ret, "Setting kernel arguments");

	//clEnqueueWriteBuffer(command_queue, pi, CL_TRUE, 0, 1, &h_pi, 0, NULL, NULL);
	size_t global_work_size = blocks * threadsPerblock;
	size_t local_work_size = threadsPerblock;
	cl_uint work_dim = 1;
	/* Execute OpenCL Kernel */
	//ret = clEnqueueTask(command_queue, kernel, 0, NULL,NULL);  //single work item
	ret = clEnqueueNDRangeKernel(command_queue, kernel, work_dim,
	0, &global_work_size, &local_work_size, 0, NULL, NULL);
	checkError(ret, "Enqueueing kernel");
	ret = clFinish(command_queue);
	checkError(ret, "Waiting for commands to finish");
	/******************************************************************************/
	/* Copy results from the memory buffer */
	ret = clEnqueueReadBuffer(command_queue, d_pixels, CL_TRUE, 0, size, pixels, 0, NULL, NULL);
	checkError(ret, "Creating program");

	/* Finalization */
	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(d_pixels);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);

	free(source_str);

	maxPixelValue = 255;
}

//Sobel edge detection function - detects edges and draws an outline
void Image::edgeDection(int blocks, int threadsPerblock){
	int x = 0, y = 0;

	int xG = 0, yG = 0;

	int * tempImage = new int[imageSize];

	int cantidadPrint = 0;

	for(unsigned int i = 0; i < imageSize; i++){

		x = i % width;

		if(i != 0 && x == 0){
			y = (int) ((double)i / (double)width);
		}

		if(x < (width - 1) && y < (height - 1)
				&& (y > 0) && (x > 0)){

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

			//newPixel = sqrt(xG^2 + yG^2)
			tempImage[i] = sqrt((xG * xG) + (yG * yG));

		}else{

			//Pads out of bound pixels with 0
			tempImage[i] = 0;

		}

	}

	for(unsigned int i = 0; i < imageSize; i++){

		pixels[i] = tempImage[i];

	}
}

bool isBinary(ifstream &inFile);

void run(char **argv);

int main(int argc, char **argv){

	if(argc != 5){

		cerr << "Usage: EdgeDetection imageName.pgm output.pgm blocks threadperblock";

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

	int blocks = strtol(argv[3], NULL, 10);
	int threadsPerblock = strtol(argv[4], NULL, 10);
	printf("blocks=%d\n", blocks);
	printf("threadsPerblock=%d\n", threadsPerblock);

	if(isBinary(inFile)){

		BinaryImage binaryImage;

		binaryImage.readHeader(inFile);

		binaryImage.readImage(inFile);

		binaryImage.edgeDection(blocks,threadsPerblock);

		binaryImage.scaleImage(blocks,threadsPerblock);

		binaryImage.writeImage(outFile);

	}else{

		AsciiImage asciiImage;

		asciiImage.readHeader(inFile);

		asciiImage.readImage(inFile);

		asciiImage.edgeDection(blocks,threadsPerblock);

		asciiImage.scaleImage(blocks,threadsPerblock);

		asciiImage.writeImage(outFile);

	}

	inFile.close();
	outFile.close();

}
