#include "FusionEKF.h"
#include "tools.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/* * Constructor.  */
FusionEKF::FusionEKF() 
{
    is_initialized_ = false;
    previous_timestamp_ = 0;

    // Measurement covariance matrix - laser
    R_laser_ = MatrixXd(2, 2);
    R_laser_ << 0.0225, 0,
                0, 0.0225;

    // Measurement covariance matrix - radar
    R_radar_ = MatrixXd(3, 3);
    R_radar_ << 0.09, 0, 0,
                0, 0.0009, 0,
                0, 0, 0.09;

    // Measurement Matrix (lesson 5, section 10)
    H_laser_ = MatrixXd(2, 4);
    H_laser_ << 1, 0, 0, 0,
                0, 1, 0, 0;

    // Place holder for Jacobian
    Hj_ = MatrixXd(3, 4);

    /**
    * TODO: Finish initializing the FusionEKF. Set the process and measurement noises
    */

    // State covariance matrix
    ekf_.P_ = MatrixXd(4, 4);
    ekf_.P_ << 1, 0, 0, 0,
               0, 1, 0, 0,
               0, 0, 1000, 0,
               0, 0, 0, 1000;

    // Initial transition matrix
    ekf_.F_ = MatrixXd(4, 4);
    ekf_.F_ << 1, 0, 1, 0,
               0, 1, 0, 1,
               0, 0, 1, 0,
               0, 0, 0, 1;
}


/** * Destructor.  */
FusionEKF::~FusionEKF() {}


void FusionEKF::ProcessMeasurement(const MeasurementPackage &measurement_pack) 
{
    /*****************************************************************************
     *  Initialization
     ****************************************************************************/
    if (!is_initialized_) {
        /**
        TODO:
          * Initialize the state ekf_.x_ with the first measurement.
          * Create the covariance matrix.
          * Remember: you'll need to convert radar from polar to cartesian coordinates.
        */
        //cout << "Initializing..." << endl;

        // first measurement
        ekf_.x_ = VectorXd(4);

        //TODO: Tweak the last two values here to get the best RMSE
        ekf_.x_ << 1, 1, 0.8, 0.8;

        if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
            // Convert radar from polar to cartesian coordinates and initialize state
            float rho = measurement_pack.raw_measurements_[0];
            float phi = measurement_pack.raw_measurements_[1];

            ekf_.x_(0) = rho*cos(phi); // x
            ekf_.x_(1) = rho*sin(phi); // y

        } else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
            // Lidar is already in cartesian, so just initialize state
            ekf_.x_(0) = measurement_pack.raw_measurements_(0); // x
            ekf_.x_(1) = measurement_pack.raw_measurements_(1); // y
        }

        // Initialise F with dt = 0
        ekf_.F_ = MatrixXd(4, 4);
        ekf_.F_ << 1, 0, 0, 0,
                   0, 1, 0, 0,
                   0, 0, 1, 0,
                   0, 0, 0, 1;

        previous_timestamp_ = measurement_pack.timestamp_;

        // done initializing, no need to predict or update
        is_initialized_ = true;

        //cout << "Done!" << endl;
        return;
    }

    /*****************************************************************************
     *  Prediction
     ****************************************************************************/

    /**
     TODO:
       * Update the state transition matrix F according to the new elapsed time.
        - Time is measured in seconds.
       * Update the process noise covariance matrix.
       * Use noise_ax = 9 and noise_ay = 9 for your Q matrix.
     */

    float dt = (measurement_pack.timestamp_ - previous_timestamp_) / 1000000.0; // dt in seconds
    previous_timestamp_ = measurement_pack.timestamp_;

    float dt_2 = dt * dt;
    float dt_3 = dt_2 * dt;
    float dt_4 = dt_3 * dt;

    // Modify the F matrix so that the time is integrated (Lesson 5 Section 8)
    ekf_.F_(0, 2) = dt;
    ekf_.F_(1, 3) = dt;

    // Acceleration noise components - Lesson 5, Section 13, provided as 9 and 9
    // TODO: Tweak the best values here, e.g. 3**2, 5**2
    float noise_ax = 9;
    float noise_ay = 9;

    // Set the process covarianve matrix Q (Lesson 5, Section 9)
    ekf_.Q_ = MatrixXd(4, 4);
    ekf_.Q_ <<    (dt_4/4) * noise_ax,  0, (dt_3/2) * noise_ax, 0,
               0, (dt_4/4) * noise_ay,  0, (dt_3/2) * noise_ay,
                  (dt_3/2) * noise_ax,  0,   (dt_2) * noise_ax, 0,
               0, (dt_3/2) * noise_ay,  0,   (dt_2) * noise_ay;
    
    ekf_.Predict();

    /*****************************************************************************
     *  Update
     ****************************************************************************/

    /**
     TODO:
      * Use the sensor type to perform the update step.
      * Update the state and covariance matrices.
    */

    if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
        // Radar updates

        // set ekf_.H_ by setting to Hj which is the calculated jacobian (use the equation provided in tools.h)
        ekf_.H_ = tools.CalculateJacobian(ekf_.x_);

        // set ekf_.R_ by using the R_radar_
        ekf_.R_ = R_radar_;

        ekf_.UpdateEKF(measurement_pack.raw_measurements_);

    } else {
        // Laser updates
        ekf_.H_ = H_laser_;
        ekf_.R_ = R_laser_;
        ekf_.Update(measurement_pack.raw_measurements_);
    }

    // print the output
    //cout << "x_ = " << ekf_.x_ << endl;
    //cout << "P_ = " << ekf_.P_ << endl;
    //cout << "px = " << ekf_.x_[0] << "; py = " << ekf_.x_[1] << "; Px = " << ekf_.P_(0, 0) << "; Py = " << ekf_.P_(1, 1) << endl;
}

