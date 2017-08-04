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

//used to hold the attributes of the circle
struct circleAttr
{
    double x;
    double y;
    double radius;
};

//used to hold the arguments necessary for thread calculations
struct runnerArgs
{
    vector<double> pointCoordinates; //copy of the locations for all of the coordinates
    int point1; //first of the pair of points examined by the thread
    int point2; //second of the pair of points
    vector<circleAttr> *validCircles; //pointer to the vector that holds all of the successful circles
};

//***function prototypes***

/*
PURPOSE: Takes a base number and calculates (base!)
PARAMETERS: Takes an int base to have the opperation applied to
RETURN: returns the value of (base!)
*/
int factorial(int base);

/*
PURPOSE: Takes a Struct of runnerArgs in the form of a voidpointer, finds the smallest circle that encompasses at least 50% of the circle
PARAMETERS:
RETURN:
*/
void *circleSimRunner (void* args);

//***MAIN***
int main()
{

    //variables for holding user input
    vector<double> coordinates;
    int numbOfCoords = 0;
    string userInputNumbOfCoords = "";
    string inputString;
    const char *coordinateInput = NULL;

    

    //taking input from the user for number of coordinates
    cout << "enter the number of coordinates.\n>>";
    getline(cin, userInputNumbOfCoords);
    numbOfCoords = atoi(userInputNumbOfCoords.c_str());

    //taking input from the user for coordinates of each of the points
    for (int i = 0; i < numbOfCoords; i++)
    {

        cout << "please enter the coordinates for point " << i << " as  <x.x> <y.y>\n>>";

        getline(cin, inputString);
        char *pEnd;
        coordinateInput = inputString.c_str();

        if (coordinateInput != NULL)
        {
            //believe in the user for sake of time, puts a safety 0.0 if nothing is there
            //TODO don't add duplicate values, statement to the user about rentering coordinates.
            coordinates.push_back(strtod(coordinateInput, &pEnd));
            coordinates.push_back(strtod(pEnd, NULL));
        }
    }

    //setting up the number of threads needed by calculating the combination of numbofcoords in pairs
    const int NUM_THREADS = (int)((factorial(numbOfCoords)) / (pow(2, (numbOfCoords / 2)) * factorial(numbOfCoords / 2)) - numbOfCoords);

    //vector for the thread arguments
    vector<runnerArgs> args;
    
    //initalize the vector
    for(int i = 0; i < NUM_THREADS; i++)
    {

        args.push_back(runnerArgs());

    }


    //initalizing threads with default attributes
    pthread_t tid[NUM_THREADS];
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    //assigning values to thread arguments vector
    //point 1 and point 2 is the same as point 2 and point 1 so each combination is visited once.
    int iterator = 0;
    for (int i = 0; i < numbOfCoords; i++)
    {
        
        for (int j = i; j < numbOfCoords; j++)
        {
            if (i == j)
                continue;
            else
            {

                cout << "i: " << i << " | " << " j: " << j << endl;
                args[iterator].point1 = i;
                args[iterator].point2 = j;
                args[iterator].pointCoordinates = coordinates;
            }
        }
    }

    //calling each thread for calcuations
    for (int i = 0; i < NUM_THREADS; i++)
    {
        cout << "creating thread: " << i << "." << endl;
        pthread_create(&tid[i],&attr, circleSimRunner, &args[i]);
    }


    //do not continue until all threads have finished
    for(int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(tid[i],NULL);
    }

    
    

    //repeat until all points are exhausted
    //if no solution at the end rip no solution

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

    //create largest circle possible, terminate if houses less than 50% and move the circle either
    //find the slope

    //if not possible anywhere move to the next furthest point
    //if there is just one solution present it, if mutiple, present smallest in area
    //starting from the center move along until
    //move circle along the diagonal in .01 increments
    pthread_exit(0);
}