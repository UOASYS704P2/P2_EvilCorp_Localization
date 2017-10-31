#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_int.h"
#include "radio.h"
#include "token.h"
#include "retargetserial.h"
#include "main.h"
#include "appl_ver.h"
#include "native_gecko.h"
#include "aat.h"
#include "thunderboard/util.h"
#include "thunderboard/board.h"
#include "thunderboard/bmp.h"
#include "thunderboard/si7021.h"
#include "thunderboard/si7210.h"
#include "thunderboard/si1133.h"
#include "thunderboard/ccs811.h"
#include "thunderboard/mic.h"
#include "app.h"
#include "radio.h"
#include "radio_ble.h"
#include <math.h>
#define MIC_SAMPLE_RATE            1000
#define MIC_SAMPLE_BUFFER_SIZE     512
#define IMU_RAD_TO_DEG_FACTOR      (360.0 / (2.0 * 3.14159265358979323) )
static uint16_t micSampleBuffer[MIC_SAMPLE_BUFFER_SIZE];
static void     init                (bool radio);
static void     readTokens          (void);
uint16_t RADIO_xoTune = 344;



// Initialise global variables
static uint8_t *i_data;
float oneMetreRSSI[10];
float N = 0.8;
int xCoordinates[10]; // Stores the X coordinate of each iBeacon
int yCoordinates[10]; // Stores the Y coordinate of each iBeacon
uint16_t location[2];
bool impossibleData;
int secondClosest;
int prevClosest;
bool initialised = false;
int rssiStorage[30][10];
float d1, d2, d3; // distance of current location from beacon1, beacon2, beacon3
int closestBeacon[3]; // Element zero contains the closest, element one the second-closest and element two the third-closest
int xmin;
int xmax;
int ymin;
int ymax;

// Identify ranges of blocked areas (e.g. location can't be on top of a desk)
int b1xmin = 0;
int b1xmax = 50;
int b1ymin = 0;
int b1ymax = 540;
int b2xmin = 0;
int b2xmax = 450;
int b2ymin = 460;
int b2ymax = 540;
int b3xmin = 320;
int b3xmax = 1050;
int b3ymin = 0;
int b3ymax = 290;
int b4xmin = 0;
int b4xmax = 530;
int b4ymin = 700;
int b4ymax = 850;
int b5xmin = 0;
int b5xmax = 530;
int b5ymin = 1020;
int b5ymax = 1160;
int b6xmin = 0;
int b6xmax = 530;
int b6ymin = 1320;
int b6ymax = 1470;
int b7xmin = 0;
int b7xmax = 530;
int b7ymin = 1660;
int b7ymax = 1810;
int b8xmin = 1020;
int b8xmax = 1050;
int b8ymin = 930;
int b8ymax = 1900;
int b9xmin = 600;
int b9xmax = 900;
int b9ymin = 1150;
int b9ymax = 1320;
int b10xmin = 780;
int b10xmax = 1050;
int b10ymin = 1610;
int b10ymax = 1900;
int b11xmin = 750;
int b11xmax = 1050;
int b11ymin = 510;
int b11ymax = 670;




// This function stores RSSI values corresponding to each minor value and calculates averages
int smooth_average(int8_t rssi, int index)
{
	float average = 0; // An average value of zero means that not enough values have been collected, and will be ignored
	for(int i=0; i<30; i++)
	{
		if (rssiStorage[i][index] == 0 || i == 29) // store new value in the first empty cell in the row corresponding to the iBeacon index, or if all cells are full, shift them all along and put new value in the last one
		{
			int total = 0;
			if (i == 29)  // All cells in this row have been filled
			{
				for(int j=0; j<30; j++) // Shift values down
				{
					if (j<29) {rssiStorage[j][index] = rssiStorage[j+1][index];}
				}
			}
			rssiStorage[i][index] = rssi; // Add new value as last element
			for(int j=0; j<i+1; j++)
			{
				total += rssiStorage[j][index]; // Add up all the values
			}
			average = total/(i+1); // Calculate average
			break; // break out of for loop once the value has been stored
		}
	}
	return (int)average;
}



// This function takes an RSSI value and converts it into distance
float rssi_to_distance(float rssi, int index, float zOrientation)
{
	// Determine the orientation of a line between a person and the beacon
	// Position of person is considered the origin, so coordinates of beacon are relative to person
	double xCoordinate = xCoordinates[index] - location[0];
	double yCoordinate = yCoordinates[index] - location[1];
	// Convert to polar coordinates to get orientation
	double beaconOrientation = atan2(yCoordinate, xCoordinate);
	// atan returns an answer in radians - convert to degrees
	beaconOrientation = beaconOrientation * 180 / 3.14159265358979323;
	// atan function considers pointing along x axis to be zero degrees; subtract ninety to shift orientation so that pointing towards windows is zero
	beaconOrientation -= 90;
	// Make orientation positive
	if (beaconOrientation < 0) {beaconOrientation += 360;}

	// Signal is considered blocked if the person holding the board is turned more than 90 degrees away from the beacon (the signal will have to pass through the person)
    if (abs(beaconOrientation - zOrientation) > 90) {rssi += 1;} // corrects for 1dB loss of signal strength when passing through person

    // Calculate distance
	float interimCalculation;
	interimCalculation = (oneMetreRSSI[index] - rssi) / (10*N);
	float distance = pow(10, interimCalculation); // this gives the distance in metres
	distance = distance * 100; // this gives the distance in centimetres
	return distance;
}



// This function compares the distance from the mobile device to each beacon, and saves the indexes of the closest three beacons to an array
void closest_beacons(float distance[11])
{
	distance[10] = 2195; // extra 'fake' beacon distance used to reset closestBeacon values - set to longest possible distance (diagonal line across room)
	closestBeacon[0] = 10;
    closestBeacon[1] = 10;
    closestBeacon[2] = 10;
    for(int i=0; i<10; i++)
    {
  	  if (distance[i] != 0) // An RSSI value of 0 means that the RSSI value hasn't been updated yet
  	  {
  		  if (distance[i] < distance[closestBeacon[2]]) // this iBeacon is closer than the one that is currently believed to be third closest
      	  {
      		  if (distance[i] < distance[closestBeacon[1]]) // it is also closer than the one that is currently believed to be second closest
      		  {
      			  if (distance[i] < distance[closestBeacon[0]]) // it is also closer than the one that is currently believed to be closest
      			  {
      				  // This beacon replaces the one at index zero, which moves down
      	    		  closestBeacon[2] = closestBeacon[1];
      	    		  closestBeacon[1] = closestBeacon[0];
      	    		  closestBeacon[0] = i;
      			  }
      			  else
      			  {
      				  // This beacon replaces the one at index one, which moves down
      	    		  closestBeacon[2] = closestBeacon[1];
      	    		  closestBeacon[1] = i;
      			  }
      		  }
      		  else
      		  {
  	    		  closestBeacon[2] = i; // This beacon replaces the one at index two
      		  }
      	  }
  	  }
    }
    return;
}



// Corners, walls and machinery cause reflections that give unusual RSSI values and alter the distances calculated.
// We have made note of which areas have unusual reflections, and when the mobile device is close to a beacon in a 'reflection area',
// this function will tweak some one-metre-rssi values to account for the reflections
void adjust_for_reflections(float zOrientation, float distance[11])
{
	  if (closestBeacon[0] == 6)
	  {
		  if (distance[6] < 400) // correct for reflections in beacon 11's corner
		  {
			  oneMetreRSSI[0] = -61.13;
			  oneMetreRSSI[3] = -55;
			  oneMetreRSSI[4] = -67.57;
			  oneMetreRSSI[5] = -55;
		  }
		  else
		  {
			  oneMetreRSSI[0] = -61.13;
			  oneMetreRSSI[3] = -57;
			  oneMetreRSSI[4] = -66.57;
			  oneMetreRSSI[5] = -65;
		  }
	  }
	  else if (closestBeacon[0] == 5) // correct for reflections in beacon 10's corner
	  {
		  oneMetreRSSI[0] = -63.13;
		  oneMetreRSSI[3] = -64;
		  oneMetreRSSI[4] = -66.57;
		  oneMetreRSSI[5] = -65;
	  }
	  else
	  {
		  oneMetreRSSI[0] = -61.13;
		  oneMetreRSSI[3] = -64;
		  oneMetreRSSI[4] = -66.57;
		  oneMetreRSSI[5] = -65;
	  }
}



// Depending on the two closest beacons (third closest may be used instead if first and second closest don't match a possible pair), select a sub-area to search
void sub_area(float distance[11])
{
	impossibleData = false; // if neither second nor third closest beacon match any pairs that the first closest beacon could form, this data is too noisy and should be ignored
	if (closestBeacon[0] == 0)
	{
		if (closestBeacon[1] == 7 || closestBeacon[1] == 5 || closestBeacon[1] == 9) {secondClosest = closestBeacon[1];}
		else if (closestBeacon[2] == 7 || closestBeacon[2] == 5 || closestBeacon[2] == 9) {secondClosest = closestBeacon[2];}
		else {impossibleData = true; return;}
		if (secondClosest == 7)
		{
			xmin = 500;
			xmax = 800;
			ymin = 1500;
			ymax = 1800;
		}
		else if (secondClosest == 5)
		{
			xmin = 700;
			xmax = 1050;
			ymin = 1500;
			ymax = 1900;
		}
		else if (secondClosest == 9)
		{
			xmin = 500;
			xmax = 900;
			ymin = 1600;
			ymax = 1900;
		}
	}
	else if (closestBeacon[0] == 1)
	{
		if (closestBeacon[1] == 4 || closestBeacon[1] == 8 || closestBeacon[1] == 7 || closestBeacon[1] == 5) {secondClosest = closestBeacon[1];}
		else if (closestBeacon[2] == 4 || closestBeacon[2] == 8 || closestBeacon[2] == 7 || closestBeacon[2] == 5) {secondClosest = closestBeacon[2];}
		else {impossibleData = true; return;}
		if (secondClosest == 4)
		{
			xmin = 800;
			xmax = 1050;
			ymin = 300;
			ymax = 600;
		}
		else if (secondClosest == 8)
		{
			// if 13 and 6 are close, 11 or 10 or 5 cannot be close and the RSSI values must be bad; set 9 as the third-closest instead
			if (closestBeacon[1] == 6) {closestBeacon[1] == 4;}
			else if (closestBeacon[2] == 6) {closestBeacon[2] == 4;}
			else if (closestBeacon[1] == 5) {closestBeacon[1] == 4;}
			else if (closestBeacon[2] == 5) {closestBeacon[2] == 4;}
			else if (closestBeacon[1] == 0) {closestBeacon[1] == 4;}
			else if (closestBeacon[2] == 0) {closestBeacon[2] == 4;}
			xmin = 800;
			xmax = 1050;
			ymin = 500;
			ymax = 1100;
		}
		else if (secondClosest == 7)
		{
			xmin = 800;
			xmax = 900;
			ymin = 1000;
			ymax = 1200;
		}
		else if (secondClosest == 5)
		{
			if (closestBeacon[1] == 6 || closestBeacon[2] == 6) {impossibleData = true; return;}
			xmin = 800;
			xmax = 1050;
			ymin = 900;
			ymax = 1200;
		}
	}
	else if (closestBeacon[0] == 2)
	{
		if (closestBeacon[1] == 7 || closestBeacon[1] == 9) {secondClosest = closestBeacon[1];}
		else if (closestBeacon[2] == 7 || closestBeacon[2] == 9) {secondClosest = closestBeacon[2];}
		else {impossibleData = true; return;}
		if (closestBeacon[1] == 8) {closestBeacon[1] = 9;}
		else if (closestBeacon[2] == 8) {closestBeacon[2] = 9;}
		else if (closestBeacon[1] == 4) {closestBeacon[1] = 9;}
		else if (closestBeacon[2] == 4) {closestBeacon[2] = 9;}
		if (secondClosest == 7)
		{
			xmin = 100;
			xmax = 400;
			ymin = 1200;
			ymax = 1500;
		}
		else if (secondClosest == 9)
		{
			xmin = 0;
			xmax = 400;
			ymin = 1400;
			ymax = 1900;
		}
	}
	else if (closestBeacon[0] == 3)
	{
		if (closestBeacon[1] == 6 || closestBeacon[1] == 4 || closestBeacon[1] == 8 || closestBeacon[1] == 7) {secondClosest = closestBeacon[1];}
		else if (closestBeacon[2] == 6 || closestBeacon[2] == 4 || closestBeacon[2] == 8 || closestBeacon[2] == 7) {secondClosest = closestBeacon[2];}
		else {impossibleData = true; return;}
		if (secondClosest == 6)
		{
			xmin = 0;
			xmax = 200;
			ymin = 400;
			ymax = 700;
		}
		else if (secondClosest == 4)
		{
			xmin = 100;
			xmax = 300;
			ymin = 400;
			ymax = 600;
		}
		else if (secondClosest == 8)
		{
			xmin = 0;
			xmax = 400;
			ymin = 500;
			ymax = 1100;
		}
		else if (secondClosest == 7)
		{
			xmin = 100;
			xmax = 400;
			ymin = 1000;
			ymax = 1300;
		}
	}
	else if (closestBeacon[0] == 4)
	{
		if (closestBeacon[1] == 6 || closestBeacon[1] == 3 || closestBeacon[1] == 8 || closestBeacon[1] == 1) {secondClosest = closestBeacon[1];}
		else if (closestBeacon[2] == 6 || closestBeacon[2] == 3 || closestBeacon[2] == 8 || closestBeacon[2] == 1) {secondClosest = closestBeacon[2];}
		else {impossibleData = true; return;}
		if (secondClosest == 6)
		{
			xmin = 100;
			xmax = 800;
			ymin = 300;
			ymax = 500;
		}
		else if (secondClosest == 3)
		{
			xmin = 100;
			xmax = 300;
			ymin = 400;
			ymax = 600;
		}
		else if (secondClosest == 8)
		{
			xmin = 200;
			xmax = 1000;
			ymin = 300;
			ymax = 600;
		}
		else if (secondClosest == 1)
		{
			xmin = 800;
			xmax = 1050;
			ymin = 300;
			ymax = 600;
		}
	}
	else if (closestBeacon[0] == 5)
	{
		if (closestBeacon[1] == 1 || closestBeacon[1] == 7 || closestBeacon[1] == 0) {secondClosest = closestBeacon[1];}
		else if (closestBeacon[2] == 1 || closestBeacon[2] == 7 || closestBeacon[2] == 0) {secondClosest = closestBeacon[2];}
		else {impossibleData = true; return;}
		if (secondClosest == 1)
		{
			xmin = 800;
			xmax = 1050;
			ymin = 1100;
			ymax = 1400;
		}
		else if (secondClosest == 7)
		{
			if (closestBeacon[1] == 6) {closestBeacon[1] = 0;}
			if (closestBeacon[2] == 6) {closestBeacon[2] = 0;}
			xmin = 800;
			xmax = 1050;
			ymin = 1100;
			ymax = 1600;
		}
		else if (secondClosest == 0)
		{
			xmin = 800;
			xmax = 1050;
			ymin = 1400;
			ymax = 1900;
		}
	}
	else if (closestBeacon[0] == 6)
	{
		// Ignore impossible data (beacon 7 being close to beacon 11)
		if (closestBeacon[1] == 2 || closestBeacon[2] == 2 || closestBeacon[1] == 5 || closestBeacon[2] == 5) {impossibleData = true; return;}
		if (closestBeacon[1] == 4 || closestBeacon[1] == 3) {secondClosest = closestBeacon[1];}
		else if (closestBeacon[2] == 4 || closestBeacon[2] == 3) {secondClosest = closestBeacon[2];}
		else {impossibleData = true; return;}
		if (secondClosest == 4)
		{
			if (distance[6] < 300)
			{
				xmin = 0;
				xmax = 300;
				ymin = 0;
				ymax = 300;
			}
			else
			{
				xmin = 0;
				xmax = 400;
				ymin = 0;
				ymax = 500;
			}
		}
		else if (secondClosest == 3)
		{
			xmin = 0;
			xmax = 200;
			ymin = 300;
			ymax = 500;
		}
	}
	else if (closestBeacon[0] == 7)
	{
		if (closestBeacon[1] == 8 || closestBeacon[1] == 3 || closestBeacon[1] == 1 || closestBeacon[1] == 5 || closestBeacon[1] == 0 || closestBeacon[1] == 9 || closestBeacon[1] == 2) {secondClosest = closestBeacon[1];}
		else if (closestBeacon[2] == 8 || closestBeacon[2] == 3 || closestBeacon[2] == 1 || closestBeacon[2] == 5 || closestBeacon[2] == 0 || closestBeacon[2] == 9 || closestBeacon[2] == 2) {secondClosest = closestBeacon[2];}
		else {impossibleData = true; return;}
		if (secondClosest == 8)
		{
			xmin = 300;
			xmax = 800;
			ymin = 1000;
			ymax = 1300;
		}
		else if (secondClosest == 3)
		{
			xmin = 200;
			xmax = 400;
			ymin = 1000;
			ymax = 1300;
		}
		else if (secondClosest == 1)
		{
			xmin = 700;
			xmax = 900;
			ymin = 1000;
			ymax = 1200;
		}
		else if (secondClosest == 5)
		{
			xmin = 500;
			xmax = 900;
			ymin = 1100;
			ymax = 1600;
		}
		else if (secondClosest == 0)
		{
			xmin = 400;
			xmax = 800;
			ymin = 1300;
			ymax = 1700;
		}
		else if (secondClosest == 9)
		{
			xmin = 300;
			xmax = 500;
			ymin = 1400;
			ymax = 1700;
		}
		else if (secondClosest == 2)
		{
			xmin = 200;
			xmax = 500;
			ymin = 1200;
			ymax = 1500;
		}
	}
	else if (closestBeacon[0] == 8)
	{
		if (closestBeacon[1] == 4 || closestBeacon[1] == 3 || closestBeacon[1] == 1 || closestBeacon[1] == 7) {secondClosest = closestBeacon[1];}
		else if (closestBeacon[2] == 4 || closestBeacon[2] == 3 || closestBeacon[2] == 1 || closestBeacon[2] == 7) {secondClosest = closestBeacon[2];}
		else {impossibleData = true; return;}
		if (secondClosest == 4)
		{
			// if 13 and 9 are close, 11 or 5 cannot be close and the RSSI values must be bad; set 6 as the third-closest instead
			if (closestBeacon[1] == 6) {closestBeacon[1] == 1;}
			else if (closestBeacon[2] == 6) {closestBeacon[2] == 1;}
			else if (closestBeacon[1] == 0) {closestBeacon[1] == 1;}
			else if (closestBeacon[2] == 0) {closestBeacon[2] == 1;}
			xmin = 200;
			xmax = 900;
			ymin = 500;
			ymax = 800;
		}
		else if (secondClosest == 3)
		{
			xmin = 200;
			xmax = 600;
			ymin = 500;
			ymax = 1100;
		}
		else if (secondClosest == 1)
		{
			// if 13 and 6 are close, 11 or 10 or 5 cannot be close and the RSSI values must be bad; set 9 as the third-closest instead
			if (closestBeacon[1] == 6) {closestBeacon[1] == 4;}
			else if (closestBeacon[2] == 6) {closestBeacon[2] == 4;}
			else if (closestBeacon[1] == 5) {closestBeacon[1] == 4;}
			else if (closestBeacon[2] == 5) {closestBeacon[2] == 4;}
			else if (closestBeacon[1] == 0) {closestBeacon[1] == 4;}
			else if (closestBeacon[2] == 0) {closestBeacon[2] == 4;}
			xmin = 500;
			xmax = 900;
			ymin = 500;
			ymax = 1100;
		}
		else if (secondClosest == 7)
		{
			xmin = 300;
			xmax = 800;
			ymin = 500;
			ymax = 1100;
		}
	}
	else if (closestBeacon[0] == 9)
	{
		if (closestBeacon[1] == 7 || closestBeacon[1] == 0 || closestBeacon[1] == 2) {secondClosest = closestBeacon[1];}
		else if (closestBeacon[2] == 7 || closestBeacon[2] == 0 || closestBeacon[2] == 2) {secondClosest = closestBeacon[2];}
		else {impossibleData = true; return;}
		if (secondClosest == 7)
		{
			xmin = 300;
			xmax = 500;
			ymin = 1500;
			ymax = 1700;
		}
		else if (secondClosest == 0)
		{
			xmin = 300;
			xmax = 600;
			ymin = 1600;
			ymax = 1900;
		}
		else if (secondClosest == 2)
		{
			xmin = 0;
			xmax = 400;
			ymin = 1500;
			ymax = 1900;
		}
	}
	// 13 and 6 and 9, in any order, form a trio with their own region (as for these three beacons using only the two closest doesn't restrict the search enough)
	if ((closestBeacon[0] == 8 || closestBeacon[1] == 8 || closestBeacon[2] == 8) && (closestBeacon[0] == 4 || closestBeacon[1] == 4 || closestBeacon[2] == 4) && (closestBeacon[0] == 1 || closestBeacon[1] == 1 || closestBeacon[2] == 1))
	{
		xmin = 600;
		xmax = 1050;
		ymin = 300;
		ymax = 800;
	}
}



// This function takes distances and the indexes of the closest beacons and implements trilateration to determine location (x and y coordinates, in metres)
void trilateration(float distance[11], uint16_t location[2])
{

	// ignore any results with the 'fake beacon' 15 (used for resetting closest beacon values) as a closest beacon
	if (closestBeacon[0] == 10 || closestBeacon[1] == 10 || closestBeacon[2] == 10) {impossibleData = true; return;}

	if (!initialised) {prevClosest = closestBeacon[0];} // the first closest beacon calculated is always allowed

	// Determine the sub-area that should be searched (this reduces computation)
	sub_area(distance);

	// Skip trilateration for data that is impossible (e.g. closest beacon 11, second-closest 14), instead keep previous location estimate until a new average is ready
	if (impossibleData)
	{
		return;
	}

	// Update previous closest value
	prevClosest = closestBeacon[0];

	// Initialise variables to use in equations for the sake of readability
	int xB1, xB2, xB3; // x coordinates of beacon1, beacon2, beacon3
	int yB1, yB2, yB3; // y coordinates of beacon1, beacon2, beacon3

	// Assign values from arrays
	xB1 = xCoordinates[closestBeacon[0]];
	xB2 = xCoordinates[closestBeacon[1]];
	xB3 = xCoordinates[closestBeacon[2]];
	yB1 = yCoordinates[closestBeacon[0]];
	yB2 = yCoordinates[closestBeacon[1]];
	yB3 = yCoordinates[closestBeacon[2]];
	d1 = distance[closestBeacon[0]];
	d2 = distance[closestBeacon[1]];

	// Correct incorrect data by making the third-closest beacon closer if the second-closest is incorrect
	if (secondClosest != closestBeacon[1]) {d3 = distance[closestBeacon[1]];}
	else {d3 = distance[closestBeacon[2]];}

	// Iterate through the sub-area specified above, with a resolution of ten centimetres, to determine which point has the smallest error
	float bestError = 1446000000; // largest possible error (can't initialise to zero as we are looking for the smallest number)
	float bestX;
	float bestY;
	// Iterate through x coordinates in sub-area
	for(int x=xmin; x<=xmax; x+=10)
	{
		// Iterate through y coordinates in sub-area
		for(int y=ymin; y<=ymax; y+=10)
		{
			// Don't do calculations if this is a blocked area
			if (!((x>=b1xmin && x<=b1xmax && y>=b1ymin && y<=b1ymax) || (x>=b2xmin && x<=b2xmax && y>=b2ymin && y<=b2ymax) || (x>=b3xmin && x<=b3xmax && y>=b3ymin && y<=b3ymax) || (x>=b4xmin && x<=b4xmax && y>=b4ymin && y<=b4ymax) || (x>=b5xmin && x<=b5xmax && y>=b5ymin && y<=b5ymax) || (x>=b6xmin && x<=b6xmax && y>=b6ymin && y<=b6ymax) || (x>=b7xmin && x<=b7xmax && y>=b7ymin && y<=b7ymax) || (x>=b8xmin && x<=b8xmax && y>=b8ymin && y<=b8ymax) || (x>=b9xmin && x<=b9xmax && y>=b9ymin && y<=b9ymax) || (x>=b10xmin && x<=b10xmax && y>=b10ymin && y<=b10ymax) || (x>=b11xmin && x<=b11xmax && y>=b11ymin && y<=b11ymax)))
			{
				// Calculate error
				float e1 = abs(pow(x-xB1,2) + pow(y-yB1,2) - pow(d1,2));
				float e2 = abs(pow(x-xB2,2) + pow(y-yB2,2) - pow(d2,2));
				float e3 = abs(pow(x-xB3,2) + pow(y-yB3,2) - pow(d3,2));
				float error = e1 + e2 + e3;
				// If error has improved, these are the new best coordinates
				if(error < bestError)
				{
					bestError = error;
					bestX = x;
					bestY = y;
				}
			}
		}
	}

	// Return the results to the location array
	location[0] = (uint16_t)bestX;
	location[1] = (uint16_t)bestY;
}





int main(void)
{
  CHIP_Init();
  readTokens();
  init(true);

  printf("\r\n\r\n#### Thunderboard Sense BLE application - %d.%d.%d build %d ####\r\n",
         APP_VERSION_MAJOR,
         APP_VERSION_MINOR,
         APP_VERSION_PATCH,
         APP_VERSION_BUILD
         );

  // Enable IMU
  BOARD_imuEnable(true);
  BOARD_imuEnableIRQ(true);
  IMU_init();
  float sampleRate = 100;
  IMU_config(sampleRate);
  int threshold_low = 500; // ignore orientation changes below 5 degrees



  // Initialise variables
  int16_t ovec[3];
  ovec[0] = 0;
  ovec[1] = 0;
  ovec[2] = 0;
  int16_t prevOvec[3];
  uint8 ovecZero_partOne, ovecZero_partTwo, ovecOne_partOne, ovecOne_partTwo, ovecTwo_partOne, ovecTwo_partTwo;
  uint8 xCoordinate_partOne, xCoordinate_partTwo, yCoordinate_partOne, yCoordinate_partTwo;
  uint8 rssi;
  uint8 minor_partOne, minor_partTwo;
  uint8 negativeZero = 0;
  uint8 negativeOne = 0;
  uint8 negativeTwo = 0;
  int _conn_handle;
  int RSSI[10]; // Array to store RSSI value from each iBbeacon
  float distance[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // Array contains the distance of each of the three closest iBeacons
  bool calculationsReady = false;
  int numCalculations = 0;
  float xOrientation, yOrientation, zOrientation;
  int drift[3];
  drift[0] = 0;
  drift[1] = 0;
  drift[2] = 0;

  // Initialise iBeacon X and Y coordinate values
  xCoordinates[0] = 750;
  xCoordinates[1] = 1057;
  xCoordinates[2] = 0;
  xCoordinates[3] = 0;
  xCoordinates[4] = 589;
  xCoordinates[5] = 1057;
  xCoordinates[6] = 58;
  xCoordinates[7] = 602;
  xCoordinates[8] = 602;
  xCoordinates[9] = 363;
  yCoordinates[0] = 1796;
  yCoordinates[1] = 826;
  yCoordinates[2] = 1568;
  yCoordinates[3] = 935;
  yCoordinates[4] = 285;
  yCoordinates[5] = 1460;
  yCoordinates[6] = 0;
  yCoordinates[7] = 1358;
  yCoordinates[8] = 775;
  yCoordinates[9] = 1900;

  // Initialise one metre RSSI values
  oneMetreRSSI[0] = -61.13;
  oneMetreRSSI[1] = -64.67;
  oneMetreRSSI[2] = -60.57;
  oneMetreRSSI[3] = -64;
  oneMetreRSSI[4] = -66.57;
  oneMetreRSSI[5] = -65;
  oneMetreRSSI[6] = -66.67;
  oneMetreRSSI[7] = -59.7;
  oneMetreRSSI[8] = -61.97;
  oneMetreRSSI[9] = -61.07;




  while (1) {

	  // Determine which beacons are closest
	  closest_beacons(distance);

	  // adjust for reflections
	  adjust_for_reflections(zOrientation, distance);


	  // Detect bluetooth events
      struct gecko_cmd_packet* evt;
      evt = gecko_wait_event();

      // React to bluetooth events
      switch (BGLIB_MSG_ID(evt->header)) {

        case gecko_evt_system_boot_id:
        	gecko_cmd_hardware_set_soft_timer(7500, 1, 0);
        	gecko_cmd_le_connection_get_rssi(_conn_handle);
        	gecko_cmd_le_gap_set_scan_parameters(0x0004,0x0064,0);
        	gecko_cmd_le_gap_discover(2);
        	printf("\n\r");
        	printf("++++boot++++\r\n");
        	gecko_cmd_le_gap_set_mode(le_gap_user_data, le_gap_undirected_connectable);
        	break;


        case gecko_evt_le_connection_opened_id:
        	_conn_handle = evt->data.evt_le_connection_opened.connection;
        	printf("++connected!!++\r\n");
        	break;

        case gecko_evt_le_connection_rssi_id:
            printf("++++connection_rssi_evt++++");
            printf("\n\r");
            struct gecko_msg_le_connection_rssi_evt_t *pStatus;
            pStatus = &(evt->data.evt_le_connection_rssi);
        	uint8_t rssi_id;
            rssi_id = pStatus->rssi;
            printf("%d",rssi_id);
            printf("\n\r");
            break;

		// This event is activated whenever a read request is received through a bluetooth connection
		case gecko_evt_gatt_server_user_read_request_id:
		{
			uint16 characteristic = evt->data.evt_gatt_server_user_read_request.characteristic;
			uint8 att_errorcode = 0;
			uint8 value_len = 13;
			uint8 value_data[10];
			value_data[0] = ovecZero_partOne;
			value_data[1] = ovecZero_partTwo;
			value_data[2] = ovecOne_partOne;
			value_data[3] = ovecOne_partTwo;
			value_data[4] = ovecTwo_partOne;
			value_data[5] = ovecTwo_partTwo;
			value_data[6] = xCoordinate_partOne;
			value_data[7] = xCoordinate_partTwo;
			value_data[8] = yCoordinate_partOne;
			value_data[9] = yCoordinate_partTwo;
			value_data[10] = negativeZero;
			value_data[11] = negativeOne;
			value_data[12] = negativeTwo;
			struct gecko_msg_gatt_server_send_user_read_response_rsp_t* response;
			response = gecko_cmd_gatt_server_send_user_read_response(_conn_handle, characteristic, att_errorcode, value_len, &value_data);
		}
			break;

        // Get RSSI data
        case gecko_evt_le_gap_scan_response_id:
            {
            	i_data = &evt->data.evt_le_gap_scan_response.data.data;

				struct gecko_msg_le_gap_scan_response_evt_t *pStatus;
				pStatus = &(evt->data.evt_le_gap_scan_response);
				int8_t _rssi = pStatus->rssi;

				// If the major value indicates that the signal comes from an iBeacon, process the RSSI value
				if(i_data[25] == 0x03 && i_data[26] == 0x87)
				{
					// Minor values start at 5, whereas array indexes start at 0
					int index = i_data[28]-5;

					// Apply averaging (if not enough values have been collected, average = 0 and will be ignored)
					int avg = smooth_average(_rssi, index);

					// Save the RSSI average and corresponding distance to the index array that matches the minor of the beacon it comes from
					if (avg != 0)
					{
						RSSI[index] = avg;
						distance[index] = rssi_to_distance(avg, index, zOrientation);
						numCalculations++;
						if (numCalculations > 20) {calculationsReady = true;}
					}

				}

			    if (calculationsReady)
			    {

			        // Use trilateration localisation
			        trilateration(distance, location);

			        initialised = true;

			        if (!impossibleData)
			        {
			        	printf("\nThe two closest iBeacons are: %d %d \n\r", (closestBeacon[0]+5), (secondClosest+5));
			        	printf("The minor values of the closest iBeacons are: %d %d %d \n\r", (closestBeacon[0]+5), (closestBeacon[1]+5), (closestBeacon[2])+5);
			        	printf("The corresponding RSSI values are: %d %d %d \n\r", (int)RSSI[closestBeacon[0]], (int)RSSI[closestBeacon[1]], (int)RSSI[closestBeacon[2]]);
			        	printf("The corresponding distances (in centimetres) are: %d %d %d \n\r", (int)d1, (int)d2, (int)d3);
			        	printf("Orientation: %d \n\r", (int)zOrientation);
			        	printf("X coordinate: %d, Y coordinate: %d\n\r", location[0], location[1]);
			        	// Update characteristic for the base station to read
          			    xCoordinate_partOne = (location[0]>>8) & 0xff;
  					    xCoordinate_partTwo = location[0] & 0xff;
  					    yCoordinate_partOne = (location[1]>>8) & 0xff;
  					    yCoordinate_partTwo = location[1] & 0xff;
			        }

			        calculationsReady = false;
			        numCalculations = 0;
			    }

            }
            break;

	    // Software timer event
        case gecko_evt_hardware_soft_timer_id:
        	switch (evt->data.evt_hardware_soft_timer.handle)
        	{
        		case 1:
        		{

        			  // Save previous values
        			  prevOvec[0] = ovec[0];
        			  prevOvec[1] = ovec[1];
        			  prevOvec[2] = ovec[2];

        			  // Get IMU orientation data
        			  IMU_update();
        			  IMU_orientationGet(ovec);

        			  // Threshold filter to remove drift
        			  for (int i=0; i<3; i++)
        			  {
            			  int difference = ovec[i] - prevOvec[i];
            			  if (abs(difference) < threshold_low)
            			  {
            				  drift[i] += difference;
            			  }
            			  if (drift[i] > 36000) {drift[i] -= 36000;}
            			  else if (drift[i] < -36000) {drift[i] += 360000;}
        			  }

        			  xOrientation = (ovec[0]-drift[0])/100;
        			  yOrientation = (ovec[1]-drift[1])/100;
        			  zOrientation = (ovec[2]-drift[2])/100;

        			  // Make zOrientation positive for easier comparison with beacon orientations
        			  if (zOrientation < 0) {zOrientation += 360;}
//        			  printf("Orientation: %d %d %d \n\r", (int)xOrientation, (int)yOrientation, (int)zOrientation);


        	    	  ovecZero_partOne = (((int)(xOrientation))>>8) & 0xff;
        			  ovecZero_partTwo = ((int)(xOrientation)) & 0xff;
        			  ovecOne_partOne = (((int)(yOrientation))>>8) & 0xff;
        			  ovecOne_partTwo = ((int)(yOrientation)) & 0xff;
        			  ovecTwo_partOne = (((int)(zOrientation))>>8) & 0xff;
        			  ovecTwo_partTwo = ((int)(zOrientation)) & 0xff;
        		}

        		break;

        		default:
        			break;
        	}
        	break;

        default:
        	break;
        }
  }
}












/**************************************************************************/
/* Thunderboard Sense function definitions                                */
/**************************************************************************/

void MAIN_initSensors()
{
  uint8_t bmpDeviceId;
  uint32_t status;

  SI7021_init();
  SI1133_init();
  BMP_init(&bmpDeviceId);
  printf("Pressure sensor: %s detected\r\n",
         bmpDeviceId == BMP_DEVICE_ID_BMP280 ? "BMP280" : "BMP180");

  status = SI7210_init();
  printf("SI7210 init status: %x\r\n", (unsigned int)status);
  if ( status == SI7210_OK ) {
    SI7210_suspend();
  }

  if ( UTIL_isLowPower() == false ) {
    CCS811_init();
    status = CCS811_startApplication();
    if ( status == CCS811_OK ) {
      status = CCS811_setMeasureMode(CCS811_MEASURE_MODE_DRIVE_MODE_10SEC);
    }
    printf("CCS811 init status: %x\r\n", (unsigned int)status);
  }

  MIC_init(MIC_SAMPLE_RATE, micSampleBuffer, MIC_SAMPLE_BUFFER_SIZE);

  BOARD_rgbledSetRawColor(0, 0, 0);

  return;
}

void MAIN_deInitSensors()
{
  SI7021_deInit();
  SI7210_deInit();
  SI1133_deInit();
  BMP_deInit();
  BOARD_envSensEnable(false);

  if ( UTIL_isLowPower() == false ) {
    CCS811_deInit();
  }

  MIC_deInit();

  BOARD_ledSet(0);
  BOARD_rgbledSetRawColor(0, 0, 0);
  BOARD_rgbledEnable(false, 0xFF);

  return;
}

#define RADIO_XO_TUNE_VALUE 344
void init(bool radio)
{
  uint8_t  supplyType;
  float    supplyVoltage;
  float    supplyIR;
  uint8_t  major, minor, patch, hwRev;
  uint32_t id;

  /**************************************************************************/
  /* Module init                                                            */
  /**************************************************************************/
  UTIL_init();
  BOARD_init();

  id = BOARD_picGetDeviceId();
  BOARD_picGetFwRevision(&major, &minor, &patch);
  hwRev = BOARD_picGetHwRevision();

  printf("\r\n");
  printf("PIC device id    : %08Xh '%c%c%c%c'\r\n", (unsigned int)id,
         (int)id, (int)(id >> 8), (int)(id >> 16), (int)(id >> 24));
  printf("PIC firmware rev : %dv%dp%d\r\n", major, minor, patch);
  printf("PIC hardware rev : %c%.2d\r\n", 'A' + (hwRev >> 4), (hwRev & 0xf) );

  UTIL_supplyProbe();
  UTIL_supplyGetCharacteristics(&supplyType, &supplyVoltage, &supplyIR);

  printf("\r\n");
  printf("Supply voltage : %.3f\r\n", supplyVoltage);
  printf("Supply IR      : %.3f\r\n", supplyIR);
  printf("Supply type    : ");
  if ( supplyType == UTIL_SUPPLY_TYPE_USB ) {
    printf("USB\r\n");
  } else if ( supplyType == UTIL_SUPPLY_TYPE_AA ) {
    printf("Dual AA batteries\r\n");
  } else if ( supplyType == UTIL_SUPPLY_TYPE_AAA ) {
    printf("Dual AAA batteries\r\n");
  } else if ( supplyType == UTIL_SUPPLY_TYPE_CR2032 ) {
    printf("CR2032\r\n");
  } else {
    printf("Unknown\r\n");
  }

  /**************************************************************************/
  /* System clock and timer init                                            */
  /**************************************************************************/
  if ( radio ) {
    RADIO_bleStackInit();
  } else {
    CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
  }

  /* Re-initialize serial port and UTIL which depend on the HF clock frequency */
  RETARGET_SerialInit();
  UTIL_init();
  BOARD_init();

  /* In low power mode, sensors are enabled and disabled when entering/leaving connected mode */
  if ( !UTIL_isLowPower() ) {
    MAIN_initSensors();
  }

  GPIO_PinModeSet(gpioPortD, 14, gpioModeInput, 0);
  GPIO_PinModeSet(gpioPortD, 15, gpioModeInput, 0);

  return;
}

void readTokens(void)
{
  /*uint8_t t8;*/
  uint16_t t16;
  /*uint32_t t32;*/

  /* Dump tokens */
  t16 = TOKEN_getU16(SB_RADIO_CTUNE);
  if ( t16 != 0xFFFF ) {
    RADIO_xoTune = t16;
    printf("\r\nSB_RADIO_CTUNE = %d\r\n", t16);
  }
  t16 = TOKEN_getU16(SB_RADIO_CHANNEL);
  if ( t16 != 0xFFFF ) {
    printf("SB_RADIO_CHANNEL = %d\r\n", t16);
  }
  t16 = TOKEN_getU16(SB_RADIO_OUTPUT_POWER);
  if ( t16 != 0xFFFF ) {
    printf("SB_RADIO_OUTPUT_POWER = %d\r\n", t16);
  }
  printf("\r\n");

  return;
}
