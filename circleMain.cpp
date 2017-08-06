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

//***MAIN***
int main()
{

    //granularity constant for incremental step size
    const double GRANULARITY = .01;

    //variables for holding user input
    vector<pointCoordinates> coordinates;
    int numbOfCoords = 0;
    string userInputString = "";
    string inputString;
    const char *coordinateInput = NULL;

    //variables for bounds of the space
    double xDistance = 0.0;
    double yDistance = 0.0;

    //taking input from the user for number of coordinates
    cout << "enter the number of coordinates.\n>>";
    getline(cin, userInputString);
    numbOfCoords = atoi(userInputString.c_str());

    //number of threads, change to a commandline argument eventually
    cout << "Enter number of threads: " << endl;
    int temp = 8;
    getline(cin, userInputString);
    if(atoi(userInputString.c_str()) > 0)
    {
        temp = atoi(userInputString.c_str());
    }
    const int NUM_THREADS = temp; 

    //taking input from the user for coordinates of each of the points
    for (int i = 0; i < numbOfCoords; i++)
    {

        cout << "please enter the coordinates for point " << i << " as  <x.x> <y.y>\n>>";

        getline(cin, inputString);
        char *pEnd;
        coordinateInput = inputString.c_str();

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

    cout << "making " << NUM_THREADS << " Threads." << endl;

    //calling each thread for calcuations
    for (double i = minX; i < maxX; i += GRANULARITY)
    {
        for (double j = minY; j < maxY; j += GRANULARITY)
        {
            if (activeThreads == NUM_THREADS)
            {
                pthread_join(tid[joinIterator], NULL);
                cout << "joinIterator: " << joinIterator << endl;
                if (joinIterator < NUM_THREADS)
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

            args[createIterator].x = i;
            args[createIterator].y = j;
            args[createIterator].id = operationCount;
            operationCount++;

            //creating threads
            pthread_create(&tid[createIterator], &attr, circleSimRunner, &args[createIterator]);
            pthread_mutex_lock(&outputMutex);
            cout << "creating thread: " << operationCount << endl;
            activeThreads++;
            cout << "current activeThreads: " << activeThreads << "." << endl;
            pthread_mutex_unlock(&outputMutex);
            if (createIterator < NUM_THREADS)
            {
                createIterator++;
            }
            else
            {
                createIterator = 0;
            }
            cout << "looking for segfault*****************" << endl;
        }
    }

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
    // UNCOMMENT cout << "in thread: " << localArgs->id << " at (" << localArgs->x << "," << localArgs->y << ")." << endl;
    pthread_mutex_unlock(&outputMutex);
    //number of threads to find distances
    const int numbDistanceCalcs = localArgs->coords.size() - 1;

    vector<double> distance;

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

    //create largest circle possible, terminate if houses less than 50% and move the circle either
    //find the slope

    //if not possible anywhere move to the next furthest point
    //if there is just one solution present it, if mutiple, present smallest in area
    //starting from the center move along until
    //move circle along the diagonal in .01 increments
    pthread_exit(0);
}

double distanceCalc(pointCoordinates center, pointCoordinates point)
{
    //variable to hold the final distance
    double distance;
    distance = sqrt(pow((center.x - point.x), 2) + pow((center.y - point.y), 2));

    return distance;
}