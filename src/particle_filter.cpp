/**
 * particle_filter.cpp
 *
 * Created on: Dec 12, 2016
 * Author: Tiffany Huang
 */

#include "particle_filter.h"

#include <math.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <limits>

#include "helper_functions.h"

using std::string;
using std::vector;
using std::sin;
using std::cos;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
  /**
   * TODO: Set the number of particles. Initialize all particles to 
   *   first position (based on estimates of x, y, theta and their uncertainties
   *   from GPS) and all weights to 1. 
   * TODO: Add random Gaussian noise to each particle.
   * NOTE: Consult particle_filter.h for more information about this method 
   *   (and others in this file).
   */
  num_particles = 100;  // TODO: Set the number of particles
  std::default_random_engine gen;

  // This line creates a normal (Gaussian) distribution for x
  std::normal_distribution<double> dist_x(x, std[0]);
  std::normal_distribution<double> dist_y(y, std[1]);
  std::normal_distribution<double> dist_theta(theta, std[2]);

  for(int i = 0; i < num_particles; ++i)
  {
    Particle particle;
    particle.id = i;
    particle.x = dist_x(gen);
    particle.y = dist_y(gen);
    particle.theta = dist_theta(gen);
    particle.weight = 1;
    particles.push_back(particle);
    weights.push_back(particle.weight);
  }
  is_initialized = true;
}

void ParticleFilter::prediction(double delta_t, double std_pos[], 
                                double velocity, double yaw_rate) {
  /**
   * TODO: Add measurements to each particle and add random Gaussian noise.
   * NOTE: When adding noise you may find std::normal_distribution 
   *   and std::default_random_engine useful.
   *  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
   *  http://www.cplusplus.com/reference/random/default_random_engine/
   */
  std::default_random_engine gen;
  std::normal_distribution<double> dist_x(0, std_pos[0]);
  std::normal_distribution<double> dist_y(0, std_pos[1]);
  std::normal_distribution<double> dist_theta(0, std_pos[2]);

  const auto comp1 = velocity/yaw_rate;
  const auto comp2 = yaw_rate * delta_t; 
  for(int i = 0; i < num_particles; ++i)
  {

     //predict next pose and avoid division by zero
    if(fabs(yaw_rate)>0.001){
      particles[i].x = particles[i].x + (comp1 * (sin(particles[i].theta + comp2) - sin(particles[i].theta)));
      particles[i].y = particles[i].y + (comp1 * (cos(particles[i].theta) - cos(particles[i].theta + comp2)));
      particles[i].theta = particles[i].theta + comp2;
    } 
    else{
      particles[i].x = particles[i].x + velocity*delta_t*cos(particles[i].theta);
      particles[i].y = particles[i].y + velocity*delta_t*sin(particles[i].theta);
    }
    
    particles[i].x = particles[i].x + dist_x(gen);
    particles[i].y = particles[i].y + dist_y(gen);
    particles[i].theta = particles[i].theta + dist_theta(gen);   
  }
}

void ParticleFilter::dataAssociation(vector<LandmarkObs> predicted, 
                                     vector<LandmarkObs>& observations) {
  /**
   * TODO: Find the predicted measurement that is closest to each 
   *   observed measurement and assign the observed measurement to this 
   *   particular landmark.
   * NOTE: this method will NOT be called by the grading code. But you will 
   *   probably find it useful to implement this method and use it as a helper 
   *   during the updateWeights phase.
   */

}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
                                   const vector<LandmarkObs> &observations, 
                                   const Map &map_landmarks) {
  /**
   * TODO: Update the weights of each particle using a mult-variate Gaussian 
   *   distribution. You can read more about this distribution here: 
   *   https://en.wikipedia.org/wiki/Multivariate_normal_distribution
   * NOTE: The observations are given in the VEHICLE'S coordinate system. 
   *   Your particles are located according to the MAP'S coordinate system. 
   *   You will need to transform between the two systems. Keep in mind that
   *   this transformation requires both rotation AND translation (but no scaling).
   *   The following is a good resource for the theory:
   *   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
   *   and the following is a good resource for the actual equation to implement
   *   (look at equation 3.33) http://planning.cs.uiuc.edu/node99.html
   */
  vector<LandmarkObs> transformed_observations = observations;

  for(int i = 0; i < num_particles; ++i)
  {
    double weight = 1.0;
    for (uint16_t j = 0; j < observations.size(); ++j)
    {
      /* transformed observation to map coordinates */
      transformed_observations[j].x = particles[i].x + (cos(particles[i].theta) * observations[j].x) - (sin(particles[i].theta) * observations[j].y);
      transformed_observations[j].y = particles[i].y + (sin(particles[i].theta) * observations[j].x) + (cos(particles[i].theta) * observations[j].y);

      /* associate transformed observations to nearest landmark */
      float dist_min = std::numeric_limits<float>::max();
      auto landmark_index = 0;
      for(uint16_t k = 0; k < map_landmarks.landmark_list.size(); ++k)
      {

        auto d1 = dist(map_landmarks.landmark_list[k].x_f, map_landmarks.landmark_list[k].y_f, particles[i].x, particles[i].y);
        if (d1 > sensor_range)
          continue;


        auto d = dist(map_landmarks.landmark_list[k].x_f, map_landmarks.landmark_list[k].y_f, transformed_observations[j].x, transformed_observations[j].y);
        if(d < dist_min)
        {
          dist_min = d;
          transformed_observations[j].id = map_landmarks.landmark_list[k].id_i;
          landmark_index = k;  
        }
      }

      /* get weight for transformed observation associated with landmark */
      weight *= multiv_prob(std_landmark[0], std_landmark[1], transformed_observations[j].x, transformed_observations[j].y,
                   double(map_landmarks.landmark_list[landmark_index].x_f), double(map_landmarks.landmark_list[landmark_index].y_f));
      
    }
    particles[i].weight = weight;
    weights[i] = weight;
  }
}

void ParticleFilter::resample() {
  /**
   * TODO: Resample particles with replacement with probability proportional 
   *   to their weight. 
   * NOTE: You may find std::discrete_distribution helpful here.
   *   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
   */

  vector<Particle> resampled_particles;

	// Create a generator to be used for generating random particle index and beta value
	std::default_random_engine gen;
	
	//Generate random particle index
	std::uniform_int_distribution<int> particle_index(0, num_particles - 1);
	
	int current_index = particle_index(gen);
	
	double beta = 0.0;
	
	double max_weight_2 = 2.0 * *max_element(weights.begin(), weights.end());
	
	for (int i = 0; i < particles.size(); i++) {
		std::uniform_real_distribution<double> random_weight(0.0, max_weight_2);
		beta += random_weight(gen);

	  while (beta > weights[current_index]) {
	    beta -= weights[current_index];
	    current_index = (current_index + 1) % num_particles;
	  }
	  resampled_particles.push_back(particles[current_index]);
	}
	particles = resampled_particles;

}

void ParticleFilter::SetAssociations(Particle& particle, 
                                     const vector<int>& associations, 
                                     const vector<double>& sense_x, 
                                     const vector<double>& sense_y) {
  // particle: the particle to which assign each listed association, 
  //   and association's (x,y) world coordinates mapping
  // associations: The landmark id that goes along with each listed association
  // sense_x: the associations x mapping already converted to world coordinates
  // sense_y: the associations y mapping already converted to world coordinates
  particle.associations= associations;
  particle.sense_x = sense_x;
  particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best) {
  vector<int> v = best.associations;
  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<int>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}

string ParticleFilter::getSenseCoord(Particle best, string coord) {
  vector<double> v;

  if (coord == "X") {
    v = best.sense_x;
  } else {
    v = best.sense_y;
  }

  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<float>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}