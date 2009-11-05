#ifndef InteractionUtilities_h
#define InteractionUtilities_h

#include <Particle.h>

#ifndef   PARTICLEDB_XML
#  define PARTICLEDB_XML  "particledb.xml"
#endif
#include <chimp/RuntimeDB.h>
#include <physical/physical.h>

#include <boost/array.hpp>

#include <cmath>

typedef chimp::RuntimeDB< > InteractionDatabase;

/// An abbreviated interaction spec.
struct Interaction {
  int reactantIndicies[2];
  double maxProductOfCrossSectionAndSpeed;
  const InteractionDatabase::Set* reactionBranches;
};

/// Estimate the maximum relative speed of particle collisions for a given pair.
double estimateMaxRelativeSpeed(
  const double& temperature1,
  const double& temperature2,
  const double& reducedMass,
  const double& standardDeviations )
{
  using physical::constant::si::K_B;
  double temperature = std::max( temperature1, temperature2 );
  double meanSpeed = std::sqrt( 1.5 * K_B * temperature / reducedMass );
  return standardDeviations * meanSpeed;
}

/// Use the interaction database to do the null-collision for a pair of particles. ParticleType must
/// have a velocity() and speciesIndex() for read/write of these values.
template< typename ParticleType >
void collideParticlePair(
  const boost::array< ParticleType*, 2 >& reactants,
  boost::array< ParticleType, 5 >& products )
{
  // For now, just do elastic collisions.
  for ( int i = 0; i < 2; ++i ) {
    products[i].speciesIndex() = 0;
    products[i].velocity() = Vec3( 0. );
  }
}

#endif // InteractionUtilities_h
