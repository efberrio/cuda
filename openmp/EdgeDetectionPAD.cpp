#include <iostream>
#include <math.h>
#include <fstream>
#include <sstream>
#include <time.h>
#include <stdlib.h>
#include <omp.h>

#define PAD 8

using namespace std;

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
	void edgeDetection(int numThreads);

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
	int ** pixels;

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
	char * byteArray = (char *)malloc((imageSize + 1) * sizeof(char));
	//char * byteArray = new char[imageSize + 1];

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
	pixels = (int **)malloc(imageSize * sizeof(int*));
	for (int j = 0; j < imageSize; j++) {
		pixels[j] = (int *)malloc(PAD * sizeof(int));
	}
	
	for(unsigned int i = 0; i < imageSize; i++){

		pixels[i][0] = static_cast<int>
		(static_cast<unsigned char>(byteArray[i]));

	}

	//Delete the byteArray
	free(byteArray);
	//delete[] byteArray;

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

		byteArray[i] = static_cast<char>(pixels[i][0]);

	}

	byteArray[imageSize] = '\0';

	outFile.write(byteArray, imageSize);

	if(outFile.fail()){

		cerr << "Error: error writing to file." << endl;

		exit(1000);

	}

	free(byteArray);
	for (int j = 0; j < imageSize; j++) {
		free(pixels[j]);
	}
	free(pixels);

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

		pixels[i][0] = pixelValue;
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

		outFile << pixels[i][0] << '\t';

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

		if(pixels[i][0] > maxVal){

			maxVal = pixels[i][0];

		}

	}

	maxpix = maxVal;

}

//Finds the minimal pixel value of the image
void Image::findMin(){

	int minVal = 255;

	for(unsigned int i = 0; i < imageSize; i++){

		if(pixels[i][0] < minVal){

			minVal = pixels[i][0];

		}

	}

	minpix = minVal;

}

//Scales image so that the maximum pixel value is 255
void Image::scaleImage(){

	double calc = 0.0;

	int newPixelValue = 0;

	findMin();

	findMax();

	#pragma omp parallel for
	for(int i = 0; i < imageSize; i++){

		calc = (double)(pixels[i][0] - minpix) / (maxpix - minpix);

		newPixelValue = round(calc * 255);

		pixels[i][0] = newPixelValue;

	}

	maxPixelValue = 255;

}

//Sobel edge detection function - detects edges and draws an outline
void Image::edgeDetection(int numThreads){
	
	/*printf("Number of threads=%d\n", numThreads);
	printf("imageSize=%d\n", imageSize);
	printf("width=%d\n", width);
	printf("height=%d\n", height);*/
	
	int rowsPerThread = height / numThreads;
	int rowsForLastThread = height - (rowsPerThread * (numThreads - 1));

	/*printf("rowsPerThread=%d\n", rowsPerThread);
	printf("rowsForLastThread=%d\n", rowsForLastThread);*/
	
	int ** tempImage = (int **)malloc(imageSize * sizeof(int*));
	for (int j = 0; j < imageSize; j++) {
		tempImage[j] = (int *)malloc(PAD * sizeof(int));
	}
	
	#pragma omp parallel num_threads(numThreads)
	{
		unsigned int i;
		
		int threadId = omp_get_thread_num();
		
		int start, end, relativeHeight;
		
		start = width * (threadId * rowsPerThread);

		if (threadId < numThreads - 1) {
			end = width * ((threadId + 1) * rowsPerThread);
			relativeHeight = rowsPerThread;
		} else {
			end = imageSize;
			relativeHeight = rowsForLastThread;
		}
		
		int x = 0;
		int y = threadId * rowsPerThread;

		int xG = 0, yG = 0;
		
		/*printf("threadId=%d\n", threadId);
		printf("start=%d\n", start);
		printf("end=%d\n", end);
		printf("y=%d\n", y);*/
		
		for(i = start; i < end; i++){

			x = i % width;

			if(i != 0 && x == 0){

				y++;

			}

			if(x < (width - 1) && y < (height - 1)
					&& (y > 0) && (x > 0)){

				//index = x + (y * width)
				//Finds the horizontal gradient
				xG = (pixels[(x+1) + ((y-1) * width)][0]
							 + (2 * pixels[(x+1) + (y * width)][0])
							 + pixels[(x+1) + ((y+1) * width)][0]
									  - pixels[(x-1) + ((y-1) * width)][0]
											   - (2 * pixels[(x-1) + (y * width)][0])
											   - pixels[(x-1) + ((y+1) * width)][0]);


				//Finds the vertical gradient
				yG = (pixels[(x-1) + ((y+1) * width)][0]
							 + (2 * pixels[(x) + ((y + 1) * width)][0])
							 + pixels[(x+1) + ((y+1) * width)][0]
									  - pixels[(x-1) + ((y-1) * width)][0]
											   - (2 * pixels[(x) + ((y-1) * width)][0])
											   - pixels[(x+1) + ((y-1) * width)][0]);

				//newPixel = sqrt(xG^2 + yG^2)
				tempImage[i][0] = sqrt((xG * xG) + (yG * yG));

			}else{

				//Pads out of bound pixels with 0
				tempImage[i][0] = 0;

			}

		}
			
	}

	for(unsigned int i = 0; i < imageSize; i++){

		pixels[i][0] = tempImage[i][0];

	}
	
	free(tempImage);


}



bool isBinary(ifstream &inFile);

void run(char **argv);

int main(int argc, char **argv){

	if(argc != 4){

		cerr << "Usage: EdgeDetection imageName.pgm output.pgm threads";

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
						
	int numThreads = atoi(argv[3]);

	if(isBinary(inFile)){

		BinaryImage binaryImage;

		binaryImage.readHeader(inFile);

		binaryImage.readImage(inFile);

		binaryImage.edgeDetection(numThreads);

		binaryImage.scaleImage();

		binaryImage.writeImage(outFile);

	}else{

		AsciiImage asciiImage;

		asciiImage.readHeader(inFile);

		asciiImage.readImage(inFile);

		asciiImage.edgeDetection(numThreads);

		asciiImage.scaleImage();

		asciiImage.writeImage(outFile);
		
	}

	inFile.close();
	outFile.close();

}


