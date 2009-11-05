#include <InteractionUtilities.h>
#include <Particle.h>
#include <Random.h>
#include <Vec3.h>

#include <chimp/interaction/filter/Elastic.h>

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/array.hpp>

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <utility>


int main( int argc, char* argv[] )
{
  // Start the rng.
  RandomGenerator rng( 41u );

  // Volume of the domain.
  const double cellVolumeCubicMeters = 1.e-9;

  // Particle species and their initial specifications.
  std::map< std::string, ParticleInputSpec > particleInitialConditions;
  Vec3 zeroVelocity( 0. );
  double gasTemperatureInKelvin = 300.;

  // Add some species.
  for ( int i = 1; i < argc; i += 2 ) {
    ParticleInputSpec inputSpec =
      { boost::lexical_cast< std::size_t >( argv[i+1] ), gasTemperatureInKelvin, zeroVelocity };
    particleInitialConditions[ boost::lexical_cast< std::string >( argv[i] ) ] = inputSpec;
  }

  // Initialize the default database.
  InteractionDatabase myDatabase;

  // Add all species types to the database.
  typedef std::pair< std::string, ParticleInputSpec > MapReturnPair;
  BOOST_FOREACH( const MapReturnPair& species, particleInitialConditions ) {
    myDatabase.addParticleType( species.first );
  }

  // Throw away all interactions except elastic reactions.
//  myDatabase.filter = boost::make_shared< chimp::interaction::filter::Elastic >();

  // Finalize the database and do housekeeping.
  myDatabase.initBinaryInteractions();
  myDatabase.xmlDb.close();

  // Now make up some particles. We use the same indexing as the particle database to ease bookkeeping.
  typedef std::vector< Particle > AllParticlesOfOneSpecies;
  std::vector< AllParticlesOfOneSpecies > speciesSets( myDatabase.getProps().size() );
  for ( int i = 0; i < myDatabase.getProps().size(); ++i ) {
    // Find the pertinent particle spec.
    std::string foo = myDatabase[i].name::value;
    ParticleInputSpec spec = particleInitialConditions[ myDatabase[i].name::value ];

    // Resize the particle container for this species.
    speciesSets[i].resize( spec.count );

    // Give the particles some random velocities.
    BOOST_FOREACH( Particle& particle, speciesSets[i] ) {
      particle.velocity =
        randomVelocityFromMaxwellian( rng, spec.temperatureInKelvin, myDatabase[i].mass::value );
    }
  }

  // Make the interaction list that has the max cross section * speed product for all pair
  // interactions. This will be used to determine the number of particle pairs to be collided at
  // each time step.
  std::vector< Interaction > interactions;
  std::cout << "BEGIN INTERACTION TABE" << std::endl;
  BOOST_FOREACH( const InteractionDatabase::Set& reactionBranches, myDatabase.getInteractions() ) {

    if ( reactionBranches.rhs.size() == 0 ) continue;

    Interaction interaction;
    interaction.reactantIndicies[0] = reactionBranches.lhs.A.species;
    interaction.reactantIndicies[1] = reactionBranches.lhs.B.species;
    interaction.reactionBranches = &reactionBranches;

    const InteractionDatabase::Properties& species1 = myDatabase[ interaction.reactantIndicies[0] ];
    const InteractionDatabase::Properties& species2 = myDatabase[ interaction.reactantIndicies[1] ];

    const double temperature1 = particleInitialConditions[ species1.name::value ].temperatureInKelvin;
    const double temperature2 = particleInitialConditions[ species2.name::value ].temperatureInKelvin;

    const double reducedMass = species1.mass::value * species2.mass::value /
      ( species1.mass::value + species2.mass::value );

    double maxRelativeSpeed =
      estimateMaxRelativeSpeed( temperature1, temperature2, reducedMass, 3.0 );
    interaction.maxProductOfCrossSectionAndSpeed =
      reactionBranches.findMaxSigmaVProduct( maxRelativeSpeed );

    interactions.push_back( interaction );

    // Write out some info for the reaction.
    std::cout << "  Reactions of "
      << species1.name::value << " with "
      << species2.name::value << std::endl;
    BOOST_FOREACH( const InteractionDatabase::Set::Equation& eq, reactionBranches.rhs ) {
      eq.print( std::cout << "    ", myDatabase ) << std::endl;
    }
  }
  std::cout << "END  INTERACTION  TABLE" << std::endl;

  // Loop over time steps.
  const double beginTime = 0.;
  const double endTime = 10.e-9;
  const double deltaTime = 0.1e-9; // 0.1 ns
  UniformRandomSize particleSelector( rng, boost::uniform_int< std::size_t >( 0, 1 ) );
  for ( double time = beginTime; time < endTime; time += deltaTime ) {

    // Update particle positions.
    BOOST_FOREACH( AllParticlesOfOneSpecies& species, speciesSets ) {
      BOOST_FOREACH( Particle& particle, species ) {
        // v = v + E * dt;
        // x = x + v * dt;
      }
    }

    // Collide a few particles. This is done by looping over all reactions, choosing a few pairs of
    // the pertinent 2 species, and doing a null-collision algorithm on these pairs.
    BOOST_FOREACH( const Interaction& interaction, interactions ) {

      std::vector< std::size_t > particlesPerSpecies;
      std::size_t numberOfCollisions = 1;
      BOOST_FOREACH( const int& reactantIndex, interaction.reactantIndicies ) {
        particlesPerSpecies.push_back( speciesSets[ reactantIndex ].size() );
        numberOfCollisions *= particlesPerSpecies.back();
      }

      // TODO: fix this so that the rounding is done correctly by MC method on remainder.
      numberOfCollisions = static_cast< int >(
        numberOfCollisions * interaction.maxProductOfCrossSectionAndSpeed * deltaTime /
        cellVolumeCubicMeters );

      // Setup some dummy vectors to hold arguments.
      boost::array< ParticleProxy, 2 > reactants;
      boost::array< ParticleProxy*, 2 > reactantPointers;
      for ( int i = 0; i < 2; ++i ) {
        reactantPointers[i] = &reactants[i];
      }
      boost::array< ParticleProxy, 5 > products;

      // Loop over all of the chosen collision pairs.
      for ( std::size_t collisionPair = 0; collisionPair < numberOfCollisions; ++collisionPair ) {

        // Choose the particles randomly.
        boost::array< std::size_t, 2 > particleIndex;
        for ( int i = 0; i < 2; ++i ) {
          particleSelector.distribution() =
            boost::uniform_int< std::size_t >( 0, particlesPerSpecies[i] - 1 );
          particleIndex[i] = particleSelector();
          reactants[i].pointAt( speciesSets[ interaction.reactantIndicies[i] ][ particleIndex[i] ] );
        }

        // Set these flags to a bogus value. A more elegant method would be to use a boost::optional
        // or to resize the vector for results.
        BOOST_FOREACH( ParticleProxy& proxy, products ) {
          proxy.speciesIndex() = -1;
        }

        collideParticlePair( reactantPointers, products );

        // For elastic collisions, we just copy the velocities of the products.
        for ( int i = 0; i < 2; ++i ) {
          std::vector< Particle >& species = speciesSets[ interaction.reactantIndicies[i] ];
          if ( products[i].speciesIndex() >=0 ) {
            // The original reactant was modified, but was not consumed.
            species[ particleIndex[i] ].velocity = products[i].velocity();
          } else {
            // The reactant was consumed. Remove it.
            species.erase( species.begin() + particleIndex[i] );
          }
        }

        // Add the new particles, if there were any created.
        for ( int i = 2; i < 5; ++i ) {
          if ( products[i].speciesIndex() >= 0 ) {
            Particle newParticle;
            newParticle.velocity = products[i].velocity();
            speciesSets[ products[i].speciesIndex() ].push_back( newParticle );
          }
        }

      } // collisionPair
    } // interaction

    // Print out the temperature of each species.
    std::cout << "\nSpecies temperatures at end of time step = " << time * 1.e9 << " ns" << std::endl;
    for ( int speciesIndex = 0; speciesIndex < speciesSets.size(); ++speciesIndex ) {
      double sumOfSquareSpeeds = 0.;
      BOOST_FOREACH( const Particle& particle, speciesSets[ speciesIndex ] ) {
        sumOfSquareSpeeds += magnitudeSquared( particle.velocity );
      }
      double averageSquareSpeed = sumOfSquareSpeeds / speciesSets[ speciesIndex ].size();
      double temperature =
        myDatabase[speciesIndex].mass::value * averageSquareSpeed / ( 3. * physical::constant::si::K_B );
      std::cout << "  " << myDatabase[speciesIndex].name::value << " " << temperature << " K\n";
    }


  } // time step

  return 0;
}
