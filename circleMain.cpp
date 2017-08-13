/*
Jason Martin
Last Modified: 8/4/2017
Purpose: This program takes a number of coordinates and the x and y of each coordinate and finds the smallest circle that encricles at least 50% of the points
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
#include <time.h>


#define DEFAULT_THREADS_COUNT 8

using namespace std;

//***Struct definitions***
pthread_mutex_t outputMutex;
pthread_mutex_t solutionCheckMutex;
pthread_mutex_t taskMutex;

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
    vector<pointCoordinates> coords;     //copy of the locations for all of the coordinates
    vector<circleAttr> *validCircles;    //pointer to the vector that holds all of the successful circles
    vector<pointCoordinates> *pointList; //point to the list of all points to check
    int *currentTaskIterator;            //pointer to an int that represents the current point
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
    //starting timer
    //TODO: get a higher resolution timer
    clock_t time1 = clock();
    time_t time2;
    time(&time2);
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
    if (argc == 1)
    {
        cout << "Starting Execution with default settings..." << endl;
    }
    else
    {
        cout << "Starting Execution..." << endl;

        for (int i = 0; i < argc; i++)
        {

            //checking for modifiers
            if (argv[i][0] == '-')
            {
                switch (argv[i][1])
                {
                case 102: //-f for filename
                    if (argc < i + 2 || argv[i + 1][0] == '-')
                    {
                        cout << "Unable to parse arguments." << endl;
                        return -1;
                    }
                    argvFileIterator = i + 1;
                    readFile = true;
                    break;

                case 103: //-g for granularity
                    if (argc < i + 2)
                    {
                        cout << "Unable to parse arguments." << endl;
                        return -1;
                    }
                    GRANULARITY = strtod(argv[i + 1], NULL);

                    break;
                case 104: //-h for help

                    if (!(strcmp(argv[i], "-help")))
                    {
                        cout << "usage: \n\t -t <num threads> \t:Change the number of threads used, default 8.\n\t -f <filename> \t\t:filename for a file with coordinates\n\t -help \t\t\t:Displays this menu. "
                             << "\n\t -g <Granularity> \t:changes the resolution of coordinates, .01 default." << endl;
                    }
                    return 0;
                    break;
                case 116: //-t to edit the number of threads used
                    if (argc < i + 2)
                    {
                        cout << "Unable to parse arguments." << endl;
                        return -1;
                    }
                    if (atoi(argv[i + 1]) > 255 || atoi(argv[i + 1]) < 1)
                    {
                        NUM_THREADS = DEFAULT_THREADS_COUNT;
                    }
                    else
                    {
                        NUM_THREADS = atoi(argv[i + 1]);
                    }
                    break;

                default: //unknown arguments
                    cout << "unable to parse arguments" << endl;
                    return -1;
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

        //taking input from the user for number of coordinates
        cout << "enter the number of coordinates.\n>:";
        getline(cin, userInputString);
        numbOfCoords = atoi(userInputString.c_str());

        //taking input from the user for coordinates of each of the points
        for (int i = 0; i < numbOfCoords; i++)
        {

            cout << "please enter the coordinates for point " << i << " as  <x.x> <y.y>\n>:";

            getline(cin, userInputString);
            char *pEnd;
            userInputString.c_str();

            if (userInputString.c_str() != NULL)
            {
                pointCoordinates insertStruct;
                //believe in the user for sake of time, puts a safety 0.0 if nothing is there
                insertStruct.x = (strtod(userInputString.c_str(), &pEnd));
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

                //adding the coordinate to the coordinate vector
                coordinates.push_back(insertStruct);
            }
        }
    }
    //reading a list of points from a file
    else
    {

        ifstream fin;
        string fileInput = "";
        int coordCount = 0;
        fin.open(argv[argvFileIterator]);
        if (fin.fail())
        {
            cout << "Error unable to read input file, exiting." << argv[argvFileIterator] << ".";
            return -1;
        }
        else
        {
            //struct to hold values to put into the coordinate vector
            pointCoordinates insertStruct;
            char *pEnd;
            while (!fin.eof())
            {
                //file IO
                getline(fin, fileInput);
                if (fileInput == "")
                {
                    continue;
                }
                insertStruct.x = (strtod(fileInput.c_str(), &pEnd));
                insertStruct.y = (strtod(pEnd, NULL));
                insertStruct.id = coordCount;
                coordCount++;

                //cleaning up input
                if (insertStruct.x < 0.000001 && insertStruct.x > -0.000001)
                {
                    insertStruct.x = 0.0;
                }
                if (insertStruct.y < 0.000001 && insertStruct.y > -0.000001)
                {
                    insertStruct.y = 0.0;
                }

                //adding coordinates to the coordinate vector
                coordinates.push_back(insertStruct);
            }
        }
    }
    //setting up the number of calculations needed by calculating the number of points to be examined by finding the highest x and y and the lowest x and y and checking all points inbetween.
    //possible speed up with a search heristic by starting with points and moving out radially
    double minX, minY, maxX, maxY;
    minX = minY = maxX = maxY = 0.0;

    //finding maximums in the coordinates vector
    for (unsigned int i = 0; i < coordinates.size(); i++)
    {
        //setting starting values to first assigned in cases where the minimum is greater than 0 or the maximum is less than 0
        if (i == 0)
        {
            maxX = minX = coordinates[i].x;
            maxY = minY = coordinates[i].y;
            continue;
        }

        //checking values after starting values have been set
        //checking for new max in the x direction
        if (coordinates[i].x >= maxX)
        {
            maxX = coordinates[i].x;
        }
        //checking for new min in the x direction
        if (coordinates[i].x <= minX)
        {
            minX = coordinates[i].x;
        }
        //checking for new max in the y direction
        if (coordinates[i].y >= maxY)
        {
            maxY = coordinates[i].y;
        }
        //checking for new min in the y direction
        if (coordinates[i].y <= minY)
        {
            minY = coordinates[i].y;
        }
    }

    //sanitize the maximum to prevent skipping
    if (minX == 0 || maxX == 0 || minY == 0 || maxY == 0)
    {

        //case for when both x extrema are non-zero
        if (maxX != 0 && minX != 0)
        {
            xDistance = maxX - minX;
        }

        //case for when both y extrema are non-zero
        if (minY != 0 && maxY != 0)
        {
            yDistance = maxY - minY;
        }

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
    }

    //case for when all extrema are non-zero
    else
    {
        xDistance = maxX - minX;
        yDistance = maxY - minY;
    }

    //making sure if points are all in a line along an axis it will still try to find a circle
    if (xDistance < GRANULARITY)
    {
        xDistance = GRANULARITY;
    }
    if (yDistance < GRANULARITY)
    {
        yDistance = GRANULARITY;
    }

    //variables for the thread arguments
    vector<runnerArgs> args;             //vector for the thread arguments
    vector<circleAttr> solutions;        //vector to hold solution circles
    vector<pointCoordinates> pointsList; //vector to hold all of the locations to try
    int currentTask = 0;                 //int for the location of the next point to check

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
    struct pointCoordinates pointListInserter;

    //filling the vector with each point to check
    for (double i = minX; i <= maxX; i += GRANULARITY)
    {
        for (double j = minY; j <= maxY; j += GRANULARITY)
        {
            pointListInserter.x = i;
            pointListInserter.y = j;
            pointsList.push_back(pointListInserter);
        }
    }

    //set up arguments and call threads
    for (int i = 0; i < NUM_THREADS; i++)
    {
        args[i].coords = coordinates;               //coordinates vector
        args[i].validCircles = &solutions;          //solutions vector
        args[i].id = i;                             //local thread id
        args[i].pointList = &pointsList;            //task list vector
        args[i].currentTaskIterator = &currentTask; //current iterator for pointList

        pthread_create(&tid[i], &attr, circleSimRunner, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(tid[i], NULL);
    }
    int end = solutions.size() - 1;
    time1 = clock() - time1;
    time_t time3;
    time(&time3);
    cout << "time taken: " << time3 - time2 << "seconds. "<< endl;
    cout << "cpu time: " << time1 << " ticks (" << ((double)time1)/CLOCKS_PER_SEC << " seconds)." << endl;
    cout << "checked " << currentTask << " locations." << endl;
    cout << "solution: " << solutions[end].x << "," << solutions[end].y << " | " << solutions[end].radius << endl;

    return 0;
}

int factorial(int base)
{
    int factorial = 1;
    for (int i = 1; i <= base; i++)
    {
        factorial *= i;
    }
    return factorial;
}

void *circleSimRunner(void *args)
{
    //converting parameter to useable localArgs
    struct runnerArgs *localArgs = (runnerArgs *)args;

    //number of calculations to find distances
    const int numbDistanceCalcs = localArgs->coords.size();
    vector<double> distance;

    //structs for distance
    struct pointCoordinates center;
    struct pointCoordinates point;
    while (true)
    {
        
        //parameter structs for distance
        pthread_mutex_lock(&taskMutex);
        //checking
        if ((unsigned)*localArgs->currentTaskIterator >= localArgs->pointList->size())
        {
            break;
        }
        else
        {
            center.x = localArgs->pointList->operator[](*localArgs->currentTaskIterator).x;
            
            center.y = localArgs->pointList->operator[](*localArgs->currentTaskIterator).y;
            
            *localArgs->currentTaskIterator += 1;
        }
        pthread_mutex_unlock(&taskMutex);
        while(distance.size() > 0)
        {
            distance.pop_back();
        }
        //for collecting the distances from the center to each point in the distance

        for (int i = 0; i < numbDistanceCalcs; i++)
        {
            point.x = localArgs->coords[i].x;
            point.y = localArgs->coords[i].y;
            distance.push_back(distanceCalc(center, point));
        }

        //creating and filling the solution for the point
        struct circleAttr solutionCircle;
        solutionCircle.x = center.x;
        solutionCircle.y = center.y;
        solutionCircle.radius = smallestCircle(distance);
        

        //lock the mutex so the vector isn't changed while its being read
        pthread_mutex_lock(&solutionCheckMutex);
        if (localArgs->validCircles->size() > 0)
        {
            
            
            //if there is already a value in the vector, check to see if its larger or smaller than the current one
            if (localArgs->validCircles->operator[](localArgs->validCircles->size() - 1).radius > solutionCircle.radius)
            {
                
                //add the smallest to the end
                localArgs->validCircles->push_back(solutionCircle);
            }
        }
        else
        {
            localArgs->validCircles->push_back(solutionCircle);
        }
        pthread_mutex_unlock(&solutionCheckMutex);
        
    }
    pthread_mutex_unlock(&taskMutex);
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