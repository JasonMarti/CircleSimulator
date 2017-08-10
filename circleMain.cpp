/*
Jason Martin
Last Modified: 8/4/2017
Purpose:This program takes a number of coordinates and the x and y of each coordinate and finds the smallest circle that encricles at least 50% of the points
Return: returns the coordinates of the center and radius of the circle 
*/

#include <iostream>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <cmath>
#include <iomanip>
#include <pthread.h>
#include <algorithm>
#include <fstream>

#define DEFAULT_THREADS_COUNT 8

using namespace std;

//***Struct definitions***
pthread_mutex_t outputMutex;
pthread_mutex_t solutionCheckMutex;

//used to hold the attributes of the circle
struct circleAttr
{
    double x;
    double y;
    double radius;
};

//points and relevant information
struct pointCoordinates
{

    int id;
    double x;
    double y;
};

//used to hold the arguments necessary for thread calculations
struct runnerArgs
{
    int id;
    vector<pointCoordinates> coords;  //copy of the locations for all of the coordinates
    double x;                         //x value of the start of the circle
    double y;                         //y value of the start of the circle
    vector<circleAttr> *validCircles; //pointer to the vector that holds all of the successful circles
};

//***function prototypes***

/*
PURPOSE: Takes a base number and calculates (base!)
PARAMETERS: Takes an int base to have the opperation applied to
RETURN: returns base! as an int
*/
int factorial(int base);

/*
PURPOSE: Takes a Struct of runnerArgs in the form of a voidpointer, finds the smallest circle that encompasses at least 50% of the circle
PARAMETERS: void pointer to a Struct of runnerArgs
RETURN: none
*/
void *circleSimRunner(void *args);

/*
PURPOSE: Takes two structs containing a center point and another point and returns the distance between the two
PARAMETERS: first param is a pointCoordinates corresponding to the center, second is a pointCoordinates 
RETURN: double for the distance
*/
double distanceCalc(pointCoordinates center, pointCoordinates point);

/*
PURPOSE: takes a vector of distances of different points and finds the radius of the smallest circle that encompases the median distance
PARAMETERS: takes a vector of doubles with distances
RETURN: returns a double with the value of the radius
*/
double smallestCircle(vector<double> distance);

//***MAIN***
int main(int argc, char **argv)
{

    //setting up start up variables with default settings
    int NUM_THREADS = DEFAULT_THREADS_COUNT; //default number of threads used in calculation
    double GRANULARITY = .01;                //default granularity for search space delta
    bool readFile = false;                   //default to interactive input

    //support setup variables
    int argvFileIterator = 0;             //used to keep track of where the file name is in argv
    vector<pointCoordinates> coordinates; //used to hold all of the coordinates

    //variables for bounds of the space
    double xDistance = 0.0;
    double yDistance = 0.0;

    //interpreting commandline arguments and making changes
    if (argc != 1)
    {
        cout << "Starting Execution with default settings..." << endl;
    }
    else
    {
        for (int i = 0; i < argc - 1; i++)
        {
            if (argv[i][0] == '-')
            {
                switch (argv[i][1])
                {
                case 104: //-h
                    if (strcmp(argv[i], "-help"))
                    {
                        cout << "usage: \n\t -t <num threads> \tChange the number of threads used, default 8.\n\t -help \tDisplays this menu. \n\t-f <filename> \t filename for a file with coordinates" << endl;
                    }
                    break;
                case 116: //-t
                    if (argc < i + 2)
                    {
                        cout << "Unable to parse arguments." << endl;
                        return -1;
                    }
                    NUM_THREADS = atoi(argv[i + 1]);
                    break;
                case 102: //-f
                    if (argc < i + 2)
                    {
                        cout << "Unable to parse arguments." << endl;
                        return -1;
                    }
                    argvFileIterator = i + 1;
                    readFile = true;
                    break;
                };
            }
        }
    }

    //taking iteractive input from the user.
    if (!readFile)
    {
        //variables for holding user input

        int numbOfCoords = 0;
        string userInputString = "";
        const char *coordinateInput = NULL;

        //taking input from the user for number of coordinates
        cout << "enter the number of coordinates.\n>>";
        getline(cin, userInputString);
        numbOfCoords = atoi(userInputString.c_str());

        //taking input from the user for coordinates of each of the points
        for (int i = 0; i < numbOfCoords; i++)
        {

            cout << "please enter the coordinates for point " << i << " as  <x.x> <y.y>\n>>";

            getline(cin, userInputString);
            char *pEnd;
            coordinateInput = userInputString.c_str();

            if (coordinateInput != NULL)
            {
                pointCoordinates insertStruct;
                //believe in the user for sake of time, puts a safety 0.0 if nothing is there
                //TODO don't add duplicate values, statement to the user about rentering coordinates.
                insertStruct.x = (strtod(coordinateInput, &pEnd));
                insertStruct.y = (strtod(pEnd, NULL));
                insertStruct.id = i;
                //coorecting values of 0 to 0.0 instead of close to 0
                if (insertStruct.x < 0.000001 && insertStruct.x > -0.000001)
                {
                    insertStruct.x = 0.0;
                }
                if (insertStruct.y < 0.000001 && insertStruct.y > -0.000001)
                {
                    insertStruct.y = 0.0;
                }

                //adding the coordinate to the vector
                coordinates.push_back(insertStruct);
            }
        }
    }
    else
    {

        ifstream fin;
        
        fin.open(argv[argvFileIterator]);
        if(fin.fail())
        {
            cout << "unable to read input file " << argv[argvFileIterator] << "."; 
        }
        
    }
    //setting up the number of threads needed by calculating the number of points to be examined by finding the highest x and y and the lowest x and y and checking all points inbetween.
    //possible speed up with a search heristic by starting with points and moving out radially
    double minX, minY, maxX, maxY;
    minX = minY = maxX = maxY = 0.0;
    cout << "maxX: " << maxX << "|minX: " << minX << "|maxY: " << maxY << "|minY: " << minY << endl;

    //finding maximums in the coordinates vector
    for (int i = 0; i < coordinates.size(); i++)
    {
        //setting starting values to first assigned in cases where the minimum is greater than 0 or the maximum is less than 0
        if (i == 0)
        {
            maxX = minX = coordinates[i].x;
            maxY = minY = coordinates[i].y;
            cout << "max/minX in first iteration: " << maxX << " " << minX << endl;
            continue;
        }

        cout << "coord[i].x: " << coordinates[i].x << " |coord[i].y: " << coordinates[i].y << endl;
        if (coordinates[i].x >= maxX)
        {
            maxX = coordinates[i].x;
        }
        if (coordinates[i].x <= minX)
        {
            cout << "minx was " << minX << " setting it to be " << coordinates[i].x << endl;
            minX = coordinates[i].x;
        }
        if (coordinates[i].y >= maxY)
        {
            maxY = coordinates[i].y;
        }
        if (coordinates[i].y <= minY)
        {
            minY = coordinates[i].y;
        }
    }

    cout << "maxX: " << maxX << "|minX: " << minX << "|maxY: " << maxY << "|minY: " << minY << endl;
    //sanitize the maximum to prevent 0 values in NUM_THREADS
    if (minX == 0 || maxX == 0 || minY == 0 || maxY == 0)
    {
        //cases for when an x extreme is 0
        if (minX == 0)
        {

            if (maxX == minX)
            {
                xDistance = GRANULARITY;
            }
            else
            {
                xDistance = maxX - minX;
            }
        }

        if (maxX == 0)
        {
            xDistance = maxX - minX;
        }

        //case for when both x extrema are non-zero
        if (maxX != 0 && minX != 0)
        {
            cout << "case for when both x extrema are non-zero" << endl;
            xDistance = maxX - minX;
        }

        //cases for when a y extreme is 0
        if (minY == 0)
        {

            if (maxY == minY)
            {
                yDistance = GRANULARITY;
            }
            else
            {
                yDistance = maxY - minY;
            }
        }

        if (maxY == 0)
        {
            yDistance = maxY - minY;
        }

        //case for when both y extrema are non-zero
        if (minY != 0 && maxY != 0)
        {
            yDistance = maxY - minY;
        }
    }

    //case for when all extrema are non-zero
    else
    {
        xDistance = maxX - minX;
        yDistance = maxY - minY;
    }

    //making sure points in a line on the lowerbounds will be counted
    if (xDistance < GRANULARITY)
    {
        xDistance = GRANULARITY;
    }
    if (yDistance < GRANULARITY)
    {
        yDistance = GRANULARITY;
    }
    //Debug Remove
    cout << "xD: " << xDistance << " yD: " << yDistance << endl;

    //calculate the needed number of threads

    //vector for the thread arguments
    vector<runnerArgs> args;

    //vector to hold solution circles
    vector<circleAttr> solutions;

    //initalize the arguments vector
    for (int i = 0; i < NUM_THREADS; i++)
    {

        args.push_back(runnerArgs());
    }

    //initalizing threads with default attributes
    pthread_t tid[NUM_THREADS];
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    //variable for traversal of the tid and args arrays and sequential id for the thread
    int createIterator = 0;
    int joinIterator = 0;
    int activeThreads = 0;
    int operationCount = 0;

    cout << "using " << NUM_THREADS << " Threads." << endl;

    //calling each thread for calcuations
    for (double i = minX; i <= maxX; i += GRANULARITY)
    {
        for (double j = minY; j <= maxY; j += GRANULARITY)
        {
            if (activeThreads == NUM_THREADS)
            {
                pthread_join(tid[joinIterator], NULL);
                /*  cout << "joinIterator: " << joinIterator << endl; */
                if (joinIterator < NUM_THREADS - 1)
                {
                    joinIterator++;
                }
                else
                {
                    joinIterator = 0;
                }
                pthread_mutex_lock(&outputMutex);
                cout << "thread: " << operationCount - NUM_THREADS << " joined." << endl;
                pthread_mutex_unlock(&outputMutex);
                activeThreads--;
            }

            //filling thread arguments

            pthread_mutex_lock(&outputMutex);
            args[createIterator].x = i;
            cout << "\nargs[createIterator].x: " << args[createIterator].x << endl;
            args[createIterator].y = j;
            cout << "args[createIterator].y: " << args[createIterator].y << endl;
            args[createIterator].id = operationCount;
            cout << "args[createIterator].id: " << args[createIterator].id << endl;
            cout << "tid: " << tid[createIterator] << endl;
            cout << "createIterator: " << createIterator << endl
                 << endl;
            args[createIterator].coords = coordinates;
            args[createIterator].validCircles = &solutions;
            operationCount++;
            pthread_mutex_unlock(&outputMutex);

            //creating threads
            /* REMOVE pthread_mutex_lock(&outputMutex);
            cout << "running operation: " << operationCount << " on thread: " << tid[createIterator] << endl;
            
            pthread_mutex_unlock(&outputMutex); */
            pthread_create(&tid[createIterator], &attr, circleSimRunner, &args[createIterator]);

            activeThreads++;

            /* pthread_mutex_lock(&outputMutex);
            cout << "current activeThreads: " << activeThreads << "." << endl;
            pthread_mutex_unlock(&outputMutex); */
            if (createIterator < NUM_THREADS - 1)
            {
                createIterator++;
            }
            else
            {
                createIterator = 0;
            }
        }
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(tid[i], NULL);
    }
    int end = solutions.size() - 1;
    cout << "solution: " << solutions[end].x << "," << solutions[end].y << " | " << solutions[end].radius << endl;

    return 0;
}

int factorial(int base)
{
    //no protection for overflow currently
    int factorial = 1;
    for (int i = 1; i <= base; i++)
    {
        factorial *= i;
    }
    return factorial;
}

void *circleSimRunner(void *args)
{

    struct runnerArgs *localArgs = (runnerArgs *)args;
    pthread_mutex_lock(&outputMutex);
    cout << "in thread: " << localArgs->id << " at (" << localArgs->x << "," << localArgs->y << ")." << endl;
    pthread_mutex_unlock(&outputMutex);
    //number of threads to find distances

    const int numbDistanceCalcs = localArgs->coords.size();

    vector<double> distance;
    double radiusSolution = 0.0;

    //parameter structs for distance
    struct pointCoordinates center;
    struct pointCoordinates point;
    center.x = localArgs->x;
    center.y = localArgs->y;

    for (int i = 0; i < numbDistanceCalcs; i++)
    {
        point.x = localArgs->coords[i].x;
        point.y = localArgs->coords[i].y;
        distance.push_back(distanceCalc(center, point));
    }

    struct circleAttr solutionCircle;
    solutionCircle.x = localArgs->x;
    solutionCircle.y = localArgs->y;
    solutionCircle.radius = smallestCircle(distance);
    pthread_mutex_lock(&solutionCheckMutex);
    if (localArgs->validCircles->size() > 0)
    {
        if (localArgs->validCircles->operator[](localArgs->validCircles->size() - 1).radius > solutionCircle.radius)
        {
            localArgs->validCircles->push_back(solutionCircle);
        }
    }
    else
    {
        localArgs->validCircles->push_back(solutionCircle);
    }
    pthread_mutex_unlock(&solutionCheckMutex);
    /* REMOVE pthread_mutex_lock(&outputMutex);
    cout << "thread " << localArgs->id << "|: number of distances: " << distance.size() << "|: each distance: ";
    for(int i = 0; i < distance.size(); i++)
    {
         
        cout << distance[i] << " ";
    }
    cout << endl;
    pthread_mutex_unlock(&outputMutex);
 */
    pthread_exit(0);
}

double distanceCalc(pointCoordinates center, pointCoordinates point)
{
    //variable to hold the final distance
    double distance;
    distance = sqrt(pow((center.x - point.x), 2) + pow((center.y - point.y), 2));

    return distance;
}

double smallestCircle(vector<double> distance)
{
    //int for storing the furthest point to be included.
    int middle = 0;

    /* REMOVE   pthread_mutex_lock(&outputMutex);
    cout << "distance: ";
    for(int i = 0; i < distance.size(); i++)
    {
        cout <<" " << i <<  ":" << distance[i] << " ";
    }
    \
    cout << "distance after: ";
    for(int i = 0; i < distance.size(); i++)
    {
        cout <<" " << i <<  ":" << distance[i] << " ";
    }
    pthread_mutex_unlock(&outputMutex); */
    //sort the distances
    sort(distance.begin(), distance.end());

    //if even then just take the middle value
    if (distance.size() % 2 == 0)
    {
        middle = (distance.size() - 1) / 2;
    }
    else
    {
        middle = distance.size() / 2;
    }

    return distance[middle];
}