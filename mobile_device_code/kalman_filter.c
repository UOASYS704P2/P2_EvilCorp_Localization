#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int kalmanfilter(int arg[10]) {
	//initial values for the kalman filter
	float x_est_last = 0;
	float P_last = 0;
	//the noise in the system
	float Q = 0.022; //Process Noise Covariance Q
	float R = 0.717; //Measurement Noise Covariance R

	float K;
	float P;
	float P_temp;
	float x_temp_est;
	float x_est;
	float z_measured[180]; //the 'noisy' value we measured
	float z_real = -61; //the ideal value we wish to measure
	int i = 0;
	srand(0);
	float total = 0;
	float x_est_average;

	int xy = 0;

	char * line = NULL;
	size_t len = 0;
	ssize_t read;

//	FILE *fp;
//	fp = fopen("D:\\eclipse-workspace\\simple\\input.txt", "r");
//	if (fp == NULL) {
//		printf("Open file failure !");
//		exit(1);
//	}

//	for (int i = 0; i < 180 && (read = getline(&line, &len, fp) != -1); i++) {
//		z_measured[i] = atoi(line);
////	    printf("%s", line);
////		printf("z_measured[%d]: %f \r\n", i, z_measured[i]);
//	}
//
//	fclose(fp);

	//initialize with a measurement
	for (int i = 0; i < 10; i++) {

		z_measured[i] = (float)arg[i];
//		printf("\r\n arg[i] is %d \r\n" , arg[i]);
//		printf("\r\n z_measured[i] is %f \r\n" , z_measured[i]);
	}
	x_est_last = z_measured[0];

	float sum_error_kalman = 0;
	float sum_error_measure = 0;

	for (int i = 0; i < 10; i++) {
		//do a prediction
		x_temp_est = x_est_last;
		P_temp = P_last + Q;
		//calculate the Kalman gain
		K = P_temp * (1.0 / (P_temp + R));
		//measure
//        z_measured = z_real + frand()*0.09; //the real measurement plus noise
		//correct
		x_est = x_temp_est + K * (z_measured[i] - x_temp_est);
		P = (1 - K) * P_temp;
		//we have our new system
//
//		printf("Ideal    RSSI: %6.3f \n", z_real);
//		printf("Mesaured RSSI: %6.3f [diff:%.3f]\n", z_measured[i],
//				fabs(z_real - z_measured[i]));
//		printf("Kalman   RSSI: %6.3f [diff:%.3f]\n", x_est,
//				fabs(z_real - x_est));

		sum_error_kalman += fabs(z_real - x_est);
		sum_error_measure += fabs(z_real - z_measured[i]);

		//update our last's
		P_last = P;
		x_est_last = x_est;
		total += x_est_last;
	}
	x_est_average = total / 10;
//
//	printf("\r\n x_est_average:  %f \r\n", x_est_average);
//	printf("Total error if using kalman filter: %f\n", sum_error_kalman);
//	printf("Reduction in error: %d%% \n",
//			100 - (int) ((sum_error_kalman / sum_error_measure) * 100));

	xy = ceil(x_est_average);
//	printf("\r\nxy is %d \r\n", xy);
	return (int)x_est_average;
}
