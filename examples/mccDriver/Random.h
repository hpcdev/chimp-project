#ifndef Random_h
#define Random_h

#include <physical/physical.h>

#include <boost/random/variate_generator.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/mersenne_twister.hpp>

#include <cmath>

/// Just some typedefs to make the usage of boost::random a bit cleaner.

// Mersenne Twister generator.
typedef boost::mt19937 RandomGenerator;

// A uniform random integer generator.
typedef boost::variate_generator< RandomGenerator&, boost::uniform_int<> > UniformRandomInt;

// A uniform random std::size_t generator.
typedef boost::variate_generator< RandomGenerator&, boost::uniform_int<std::size_t> > UniformRandomSize;

// A uniform random double generator.
typedef boost::variate_generator< RandomGenerator&, boost::uniform_real<> > UniformRandomDouble;

// A normal distribution with mean = 0 and sigma = 1.
typedef boost::variate_generator< RandomGenerator&, boost::normal_distribution<> > NormalRandomDouble;

// Generate a velocity from Maxwell-Boltzmann distribution.
Vec3 randomVelocityFromMaxwellian(
  RandomGenerator& rng,
  const double& temperatureKelvin,
  const double& massKg )
{
  NormalRandomDouble speedGenerator( rng, boost::normal_distribution<>() );
  Vec3 velocity;
  const double averageSpeed = sqrt( physical::constant::si::K_B * temperatureKelvin / massKg );
  for ( int i = 0; i < 3; ++i ) {
    velocity[i] = averageSpeed * speedGenerator();
  }

  return velocity;
}

#endif // Random_h
