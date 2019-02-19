#include <iostream>
#include <math.h>
#include <fstream>
#include <sstream>
#include <time.h>
#include <stdlib.h>
#include <cstdlib>
#include <mpi.h>

using namespace std;

#define MASTER_RANK 0
#define NOT_ENOUGH_PROCESSES_NUM_ERROR 1
#define MASTER_TAG 1
#define WORKER_TAG 2

//Creating image class (base class)
class Image{

public:

	Image():
		height(0),
		width(0),
		maxPixelValue(0),
		minpix(0),
		maxpix(0),
		imageSize(0),
		tasks(0),
		processRank(0){}
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
	int getTasks(){return tasks;}
	int getProcessRank(){return processRank;}

	//Mutator methods
	void setHeight(int h){height = h;}
	void setWidth(int w){width = w;}
	void setMaxPixelValue(int mpv){maxPixelValue = mpv;}
	void setTasks(int t){tasks = t;}
	void setProcessRank(int pr){processRank = pr;}

	//Member variables
protected:

	int height;
	int width;
	int maxPixelValue;
	int minpix;
	int maxpix;
	unsigned int imageSize;
	int * pixels;
	int tasks;
	int processRank;

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
	//printf("scaleImage\n");

	// Array that will hold the scaled pixels
	int * scaledPixels;

	// Number of rows per worker
	int workers, pixelsPerWorker;

	// Smaller arrays that will be held on each separate process
	int * p_pixels;
	int * p_scaledPixels;

	// We choose process rank 0 to be the root, or master,
	// which will be used to initialize the full arrays.
	workers = tasks;
	if (processRank == MASTER_RANK) {
		findMin();
		findMax();
		scaledPixels = (int *) malloc(sizeof(int) * imageSize);
		pixelsPerWorker = imageSize / workers;
	}

	// Determine how many elements each process will work on
	// In this simple version, the number of processes needs to
	// divide evenly into the number of rows (image height)
	//printf("workers=%d\n", workers);

	if (processRank == MASTER_RANK) {
		// Broadcasting values to all proceses
		MPI_Bcast(&minpix, 1, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);
		MPI_Bcast(&maxpix, 1, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);
		MPI_Bcast(&imageSize, 1, MPI_UNSIGNED, MASTER_RANK, MPI_COMM_WORLD);
		MPI_Bcast(&pixelsPerWorker, 1, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);
	} else {
		// Receiving values from all proceses
		MPI_Bcast(&minpix, 1, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);
		MPI_Bcast(&maxpix, 1, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);
		MPI_Bcast(&imageSize, 1, MPI_UNSIGNED, MASTER_RANK, MPI_COMM_WORLD);
		MPI_Bcast(&pixelsPerWorker, 1, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);
	}

	// All processes take part in the calculations concurrently

	//printf("pixelsPerWorker=%d in processRank=%d\n", pixelsPerWorker, processRank);
	//printf("imageSize=%d in processRank=%d\n", imageSize, processRank);
	//printf("minpix=%d in processRank=%d\n", minpix, processRank);
	//printf("maxpix=%d in processRank=%d\n", maxpix, processRank);

	// Initialize the smaller subsections of the larger array
	p_pixels = (int *) malloc(sizeof(int) * pixelsPerWorker);
	p_scaledPixels = (int *) malloc(sizeof(int) * pixelsPerWorker);

	// Scattering array pixels from MASTER node out to the other nodes
	MPI_Scatter(pixels, pixelsPerWorker, MPI_INT, p_pixels, pixelsPerWorker, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);

	// Do the calculations
	for (int i = 0; i < pixelsPerWorker; i++) {
		double calc = (double)(p_pixels[i] - minpix) / (maxpix - minpix);
		int newPixelValue = round(calc * 255);
		p_scaledPixels[i] = newPixelValue;
	}

	// MASTER node gathering array scaledPixels from the workers
	MPI_Gather(p_scaledPixels, pixelsPerWorker, MPI_INT, scaledPixels, pixelsPerWorker, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);

	// All concurrent processes are finished once they all communicate
	// data back to the master via the gather function.

	// Master process gets to here only when it has been able to gather from all processes
	if (processRank == MASTER_RANK) {
		//int maximo = 0;
		for (unsigned int i = 0; i < imageSize; i++) {
			/*double calc2 = (double)(pixels[i] - minpix) / (maxpix - minpix);
			int newPixelValue2 = round(calc2 * 255);
			if (newPixelValue2 > maximo) {
				maximo = newPixelValue2;
			}
			if (newPixelValue2 != scaledPixels[i]) {
				printf("Valor diferente, pos=%d, valor mpi=%d, valor serial=%d\n", i, p_scaledPixels[i], newPixelValue2);
			}*/
			pixels[i] = scaledPixels[i];
		}
		//printf("maximo=%d\n", maximo);
		//printf("imageSize=%d VS cantidadEnCero=%d\n", imageSize, cantidadEnCero);
	}

	// clean up memory
	if (processRank == MASTER_RANK)  {
		free(scaledPixels);
	}
	free(p_pixels);
	free(p_scaledPixels);

	maxPixelValue = 255;

}

//Sobel edge detection function - detects edges and draws an outline
void Image::edgeDection(){

	int workers, rowsPerWorker, remainder;
	int rows, messageTag, processId;
	MPI_Status status;

	// Arrays that will be held on each separate process
	int * p_tempImage;

    /*if (processRank == MASTER_RANK) {
        printf("b casting array\n");
        MPI_Bcast(&pixels, MSG_LENGTH, MPI_INT, root, MPI_COMM_WORLD);
        printf("b casted array\n");
     } else {
        MPI_Bcast(message, MSG_LENGTH, MPI_INT, root, MPI_COMM_WORLD);
        MPI_Comm_rank(MPI_COMM_WORLD, &iam);
        printf("\nnode %d %s ", iam, message);                        
        MPI_Get_processor_name(processor_name, &namelen);
        printf("processor %s", processor_name); fflush(stdout);
    }*/


	int start, end;
	if (processRank == MASTER_RANK) {
		//printf("in master\n");
		workers = tasks - 1;
		rowsPerWorker = height / workers;
		remainder = height % workers;
		//printf("division=%d\n", rowsPerWorker);
		//printf("remainder=%d\n", remainder);

		messageTag = MASTER_TAG;
		for (processId = 1; processId <= workers; processId++) {
			rows = processId <= remainder ? rowsPerWorker + 1 : rowsPerWorker;
			start = width * ((processId - 1) * rows);
			end = width * (processId * rows);

			MPI_Send(&width, 1, MPI_INT, processId, messageTag, MPI_COMM_WORLD);
			//printf("sent width=%d, processId=%d\n", width, processId);
			MPI_Send(&height, 1, MPI_INT, processId, messageTag, MPI_COMM_WORLD);
			//printf("sent height=%d, processId=%d\n", height, processId);
			MPI_Send(&rows, 1, MPI_INT, processId, messageTag, MPI_COMM_WORLD);
			//printf("sent rows=%d, processId=%d\n", rows, processId);
			MPI_Send(&start, 1, MPI_INT, processId, messageTag, MPI_COMM_WORLD);
			//printf("sent start=%d, processId=%d\n", start, processId);
			MPI_Send(&end, 1, MPI_INT, processId, messageTag, MPI_COMM_WORLD);
			//printf("sent end=%d, processId=%d\n", end, processId);
			MPI_Send((void *)pixels, imageSize, MPI_INT, processId, messageTag, MPI_COMM_WORLD);
			//printf("sent pixels, processId=%d\n", processId);
		}

		messageTag = WORKER_TAG;
		workers = tasks - 1;
		
		for (processId = 1; processId <= workers; processId++) {
			rows = processId <= remainder ? rowsPerWorker + 1 : rowsPerWorker;
			start = width * ((processId - 1) * rows);
			end = width * (processId * rows);

			//printf("workers=%d\n", workers);
			//printf("receiving results processId=%d, size=%d\n", processId, (end - start));
			MPI_Recv(&start, 1, MPI_INT, processId, WORKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Recv(&end, 1, MPI_INT, processId, WORKER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//printf("received start=%d, end=%d for processId=%d\n", start, end, processId);
			p_tempImage = (int *)malloc((end - start) * sizeof(int));
			MPI_Recv((void *)p_tempImage, (end - start), MPI_INT, processId, messageTag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//printf("received results processId=%d\n", processId);

			//printf("reasigning to pixels\n");
			for (unsigned int i = 0; i < (end - start); i++) {
				pixels[i + start] = p_tempImage[i];
			}
			free(p_tempImage);
		}

	} 
	if (processRank != MASTER_RANK) {
		//printf("in process=%d\n", processRank);
		// Receive the values
		messageTag = MASTER_TAG;
		MPI_Recv(&width, 1, MPI_INT, MASTER_RANK, messageTag, MPI_COMM_WORLD, &status);
		//printf("receiving width=%d, processRank=%d\n", width, processRank);
		MPI_Recv(&height, 1, MPI_INT, MASTER_RANK, messageTag, MPI_COMM_WORLD, &status);
		//printf("receiving height=%d, processRank=%d\n", height, processRank);
		MPI_Recv(&rows, 1, MPI_INT, MASTER_RANK, messageTag, MPI_COMM_WORLD, &status);
		//printf("receiving rows=%d, processRank=%d\n", rows, processRank);
		MPI_Recv(&start, 1, MPI_INT, MASTER_RANK, messageTag, MPI_COMM_WORLD, &status);
		//printf("receiving start=%d, processRank=%d\n", start, processRank);
		MPI_Recv(&end, 1, MPI_INT, MASTER_RANK, messageTag, MPI_COMM_WORLD, &status);
		//printf("receiving end=%d, processRank=%d\n", end, processRank);
		//printf("going to allocate on slave size=%d, processRank=%d\n", (width * height), processRank);
		pixels = (int *)malloc((width * height) * sizeof(int));
		//printf("pixels allocated on slave, processRank=%d\n", processRank);
		MPI_Recv((void *)pixels, (width * height), MPI_INT, MASTER_RANK, messageTag, MPI_COMM_WORLD, &status);
		//printf("receiving pixels, processRank=%d\n", processRank);

		p_tempImage = (int *) malloc(sizeof(int) * (rows * width));
		// Do the calculations
		int x = 0, i;
		int y = (processRank - 1) * rows;

		int xG = 0, yG = 0;
		
		//printf("starting calculations rows=%d, start=%d, end=%d, y=%d, processRank=%d\n", rows, start, end, y, processRank);
		
		for (i = start; i < end; i++) {

			x = i % width;
			if (i != 0 && x == 0) {
				y++;
			}

			if(x < (width - 1) && y < (height - 1)
					&& (y > 0) && (x > 0)){
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

				p_tempImage[i - start] = sqrt((xG * xG) + (yG * yG));

			} else {
				//Pads out of bound pixels with 0
				p_tempImage[i - start] = 0;
			}
		}

		//printf("finishing calculations on processRank=%d, returning results\n", processRank);

		messageTag = WORKER_TAG;
		MPI_Send(&start, 1, MPI_INT, MASTER_RANK, WORKER_TAG, MPI_COMM_WORLD);
		MPI_Send(&end, 1, MPI_INT, MASTER_RANK, messageTag, MPI_COMM_WORLD);
		MPI_Send((void *)p_tempImage, rows * width, MPI_INT, MASTER_RANK, messageTag, MPI_COMM_WORLD);
		free(p_tempImage);
		free(pixels);

		//printf("returned result for processRank=%dn", processRank);
	}

}

bool isBinary(ifstream &inFile);

void run(char **argv, int tasks, int processRank);

int main(int argc, char **argv){

	if(argc != 3){

		cerr << "Usage: EdgeDetection imageName.pgm output.pgm";

		return 1;

	}

	int tasks, processRank;

	/* Initialize the message passing system, get the number of nodes,
	and find out which node we are. */
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &tasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &processRank);
	//printf("impriendo proceso == %d\n", processRank);

	if (tasks < 2) {
		MPI_Abort(MPI_COMM_WORLD, NOT_ENOUGH_PROCESSES_NUM_ERROR);
	}

	run(argv, tasks, processRank);

	MPI_Finalize();

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

void run(char **argv, int tasks, int processRank){

	ifstream inFile;

	inFile.open(argv[1], ios::binary | ios::in);

	ofstream outFile;

	outFile.open(argv[2], ios::binary
			            | ios::out
						| ios::trunc);

	// Only scaleImage and edgeDection operations are parallelized
	// The rest operations run on the master to avoin inconsistencies

	if(isBinary(inFile)){

		BinaryImage binaryImage;
		binaryImage.setTasks(tasks);
		binaryImage.setProcessRank(processRank);

		if (processRank == MASTER_RANK) {
			//printf("va a leer la imagen, processRank=%d\n", processRank);
			binaryImage.readHeader(inFile);
			binaryImage.readImage(inFile);
		}

		MPI_Barrier(MPI_COMM_WORLD);
		//printf("va allamar edgeDection, processRank=%d\n", processRank);
		binaryImage.edgeDection();
		MPI_Barrier(MPI_COMM_WORLD);
		binaryImage.scaleImage();
		MPI_Barrier(MPI_COMM_WORLD);

		if (processRank == MASTER_RANK) {
			//printf("va a escribir la imagen, processRank=%d\n", processRank);
			binaryImage.writeImage(outFile);
		}

	}else{

		AsciiImage asciiImage;
		asciiImage.setTasks(tasks);
		asciiImage.setProcessRank(processRank);

		if (processRank == MASTER_RANK) {
			asciiImage.readHeader(inFile);
			asciiImage.readImage(inFile);
		}

		MPI_Barrier(MPI_COMM_WORLD);
		asciiImage.edgeDection();
		MPI_Barrier(MPI_COMM_WORLD);
		asciiImage.scaleImage();
		MPI_Barrier(MPI_COMM_WORLD);

		if (processRank == MASTER_RANK) {
			asciiImage.writeImage(outFile);
		}
	}

	inFile.close();
	outFile.close();

}


