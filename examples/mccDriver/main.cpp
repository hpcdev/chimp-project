#include <InteractionUtilities.h>
#include <Particle.h>
#include <Random.h>
#include <Vec3.h>

#include <chimp/interaction/filter/Elastic.h>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/array.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <numeric>
#include <string>
#include <utility>


void showUsage();

void printConsoleData(
  const InteractionDatabase& database,
  const std::vector< double >& speciesTemperatures,
  double timeSeconds,
  std::size_t collisionCount,
  std::size_t particleCount,
  bool initialConditionsOnly = false );

int main( int argc, char* argv[] )
{
  if ( argc != 2 ) {
    showUsage();
    return 0;
  }

  // Start the rng.
  RandomGenerator rng( 41u );
  UniformRandomSize sizeTypeRng( rng, boost::uniform_int< std::size_t >( 0,
    std::numeric_limits< std::size_t >::max() ) );

  // Parse the input file.
  std::ifstream input( argv[1] );
  std::string line;

  // The interaction database file.
  std::string xmlDatabaseFile;
  input >> xmlDatabaseFile; getline( input, line );
  InteractionDatabase myDatabase( xmlDatabaseFile );

  // Time and time step.
  double deltaTime, endTime, outputDeltaTime;
  input >> deltaTime; getline( input, line );
  input >> endTime; getline( input, line );

  // Output info.
  input >> outputDeltaTime; getline( input, line );
  std::size_t numberOfOutputBins;
  input >> numberOfOutputBins; getline( input, line );

    // Volume of the domain.
  double cellVolumeCubicMeters;
  input >> cellVolumeCubicMeters; getline( input, line );

  // Particle species, counts, and temperatures.
  std::multimap< std::string, ParticleInputSpec > particleInitialConditions;
  while ( true ) {
    std::string speciesName;
    std::size_t particleCount;
    double particleTemperature;
    Vec3 commonVelocity;
    input >> speciesName >> particleCount >> particleTemperature >> commonVelocity;
    if ( ! input.good() ) {
      break;
    } else {
      getline( input, line );
    }

    ParticleInputSpec inputSpec = { particleCount, particleTemperature, commonVelocity };
    particleInitialConditions.insert( std::make_pair( speciesName, inputSpec ) );
  }

  input.close();

  // Add all species types to the database.
  typedef std::multimap< std::string, ParticleInputSpec >::const_iterator ConstSpecIterator;
  for ( ConstSpecIterator speciesIterator = particleInitialConditions.begin();
    speciesIterator != particleInitialConditions.end();
    speciesIterator = particleInitialConditions.upper_bound( speciesIterator->first ) ) {
    myDatabase.addParticleType( speciesIterator->first );
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

    // Find the pertinent particle specs.
    std::string speciesName = myDatabase[i].name::value;

    for ( ConstSpecIterator speciesIterator = particleInitialConditions.lower_bound( speciesName );
      speciesIterator != particleInitialConditions.upper_bound( speciesName ); ++speciesIterator ) {

      const ParticleInputSpec& spec = speciesIterator->second;

      // Give the particles some random velocities plus the common velocity.
      for ( std::size_t particleIndex = 0; particleIndex < spec.count; ++particleIndex ) {
        Particle particle;
        particle.velocity =
          randomVelocityFromMaxwellian( rng, spec.temperatureInKelvin, myDatabase[i].mass::value );
        particle.velocity += spec.driftVelocity;
        speciesSets[i].push_back( particle );
      }
    }
  }

  // Write some info for the reaction set.
  std::cout << "BEGIN INTERACTION TABLE" << std::endl;
  BOOST_FOREACH( const InteractionDatabase::Set& reactionBranches, myDatabase.getInteractions() ) {

    if ( reactionBranches.rhs.size() == 0 ) continue;

    std::cout << "  Reactions of "
      << myDatabase[ reactionBranches.lhs.A.species ].name::value << " with "
      << myDatabase[ reactionBranches.lhs.B.species ].name::value << std::endl;
    BOOST_FOREACH( const InteractionDatabase::Set::Equation& eq, reactionBranches.rhs ) {
      eq.print( std::cout << "    ", myDatabase ) << std::endl;
    }
  }
  std::cout << "END  INTERACTION  TABLE" << std::endl;

  // Loop over time steps.
  const double beginTime = 0.;
  for ( double time = beginTime; time < endTime; time += deltaTime ) {

    // Update particle positions, max speeds, and temperatures.
    std::vector< double > maxSpeedMetersPerSecond;
    std::vector< double > speciesTemperatures;
    std::size_t collisionCount = 0;
    std::size_t particleCount = 0;
    for ( int speciesIndex = 0; speciesIndex < speciesSets.size(); ++speciesIndex ) {
      maxSpeedMetersPerSecond.push_back( 0. );
      speciesTemperatures.push_back( 0. );
      BOOST_FOREACH( Particle& particle, speciesSets[ speciesIndex ] ) {
        // v = v + E * dt;
        // x = x + v * dt;
        double particleSpeedSquared = magnitudeSquared( particle.velocity );
        maxSpeedMetersPerSecond.back() =
          std::max( maxSpeedMetersPerSecond.back(), sqrt( particleSpeedSquared ) );
        speciesTemperatures.back() += particleSpeedSquared;
        ++particleCount;
      }
      speciesTemperatures.back() *= 0.5 * myDatabase[ speciesIndex ].mass::value; // kinetic energy
      speciesTemperatures.back() /= 1.5 * physical::constant::si::K_B * speciesSets[ speciesIndex ].size();
    }

    if ( time == beginTime ) {
      printConsoleData( myDatabase, speciesTemperatures, 0.0, 0, particleCount, true );
    }

    // Collide a few particles. This is done by looping over all reactions, choosing a few pairs of
    // the pertinent 2 species, and doing a null-collision algorithm on these pairs.
    BOOST_FOREACH( const InteractionDatabase::Set& reactionBranches, myDatabase.getInteractions() ) {

      if ( reactionBranches.rhs.size() == 0 ) continue;

      std::vector< std::size_t > particlesPerSpecies;
      std::vector< int > reactantIndices;
      reactantIndices.push_back( reactionBranches.lhs.A.species );
      reactantIndices.push_back( reactionBranches.lhs.B.species );
      std::size_t numberOfCollisions = 1;
      BOOST_FOREACH( const int& reactantIndex, reactantIndices ) {
        particlesPerSpecies.push_back( speciesSets[ reactantIndex ].size() );
        numberOfCollisions *= particlesPerSpecies.back();
      }

      // Round the number of collisions based on the remainder using MC method.
      double maxRelativeSpeed = std::accumulate( maxSpeedMetersPerSecond.begin(),
        maxSpeedMetersPerSecond.end(), 0. );
      double maxSigmaSpeedProduct = reactionBranches.findMaxSigmaVProduct( maxRelativeSpeed );
      double fractionalCollisions =
        numberOfCollisions * maxSigmaSpeedProduct * deltaTime / cellVolumeCubicMeters;
      numberOfCollisions = static_cast< std::size_t >( std::floor( fractionalCollisions ) );
      double partialCollision = fractionalCollisions - numberOfCollisions;
      UniformRandomDouble remainderSelector( rng, boost::uniform_real<>( 0., 1. ) );
      if ( remainderSelector() < partialCollision ) {
        ++numberOfCollisions;
      }

      // Setup some dummy vectors to hold arguments.
      std::vector< InteractionDatabase::options::Particle > reactants( 2 );
      std::vector< const InteractionDatabase::options::Particle* > reactantPointers( 2 );
      for ( int i = 0; i < 2; ++i ) {
        reactantPointers[i] = &reactants[i];
      }

      // Loop over all of the chosen collision pairs.
      for ( std::size_t collisionPair = 0; collisionPair < numberOfCollisions; ++collisionPair ) {

        // Choose the particles randomly, making sure we don't pick the same particle twice if this
        // is a like-pair collision.
        boost::array< std::size_t, 2 > particleIndex;
        particleIndex[0] = sizeTypeRng() % particlesPerSpecies[0];
        particleIndex[1] = sizeTypeRng() % particlesPerSpecies[1];
        if ( reactantIndices[0] == reactantIndices[1] ) {
          while ( particleIndex[1] == particleIndex[0] ) {
            particleIndex[1] = sizeTypeRng() % particlesPerSpecies[1];
          }
        }

        // Copy over the velocities.
        for ( int i = 0; i < 2; ++i ) {
          for ( int j = 0; j < 3; ++j )
            reactants[i].v[j] =
              speciesSets[ reactantIndices[i] ][ particleIndex[i] ].velocity[j];
        }

        double relativeSpeed = magnitude( reactants[0].v - reactants[1].v );
        if ( relativeSpeed == 0.0 ) {
          continue;
        }

        // Find which branch to take using null collision method.
        std::pair< int, double > path =
          reactionBranches.calculateOutPath( maxSigmaSpeedProduct, relativeSpeed );

        // Skip it if there is no collision.
        if ( path.first < 0 ) {
          continue;
        }

        const InteractionDatabase::Set::Equation& selectedPath = reactionBranches.rhs[ path.first ];

        InteractionDatabase::Interaction::ParticleParam defaultParam;
        defaultParam.is_set = false; // because it doesn't have a default constructor yet.
        std::vector< InteractionDatabase::Interaction::ParticleParam > products( 5, defaultParam );
        selectedPath.interaction->interact( reactantPointers, products );
        ++collisionCount;

        // For elastic collisions, we just copy the velocities of the products.
        for ( int i = 0; i < 2; ++i ) {
          std::vector< Particle >& species = speciesSets[ reactantIndices[i] ];
          if ( products[i].is_set ) {
            // The original reactant was modified, but was not consumed.
            for ( int j = 0; j < 3; ++j ) {
              species[ particleIndex[i] ].velocity[j] = products[i].particle.v[j];
            }
          } else {
            // The reactant was consumed. Remove it.
            species.erase( species.begin() + particleIndex[i] );
          }
        }

        // Add the new particles if there were any created.
        for ( int i = 2; i < 5; ++i ) {
          if ( products[i].is_set ) {
            Particle newParticle;
            for ( int j = 0; j < 3; ++j ) {
              newParticle.velocity[j] = products[i].particle.v[j];
            }
            speciesSets[ selectedPath.products[i-2].species ].push_back( newParticle );
          }
        }

      } // collisionPair
    } // interaction

    // Print the status.
    printConsoleData( myDatabase, speciesTemperatures, time, collisionCount, particleCount );

  } // time step

  return 0;
}

void showUsage()
{
  std::cout << "Usage: QuickPicConsole inputFile"
    << std::endl;
}

// Print out some info.
void printConsoleData(
  const InteractionDatabase& database,
  const std::vector< double >& speciesTemperatures,
  double timeSeconds,
  std::size_t collisionCount,
  std::size_t particleCount,
  bool initialConditionsOnly )
{
  if ( initialConditionsOnly ) {
    std::cout << boost::format( "  Initial conditions:       " );
  } else {
    std::cout << boost::format( "%7.2f ns, collided %5.3f %%" ) % ( timeSeconds * 1.e9 ) %
      ( 100. * collisionCount / static_cast< double >( particleCount ) );
  }
  for ( int speciesIndex = 0; speciesIndex < speciesTemperatures.size(); ++speciesIndex ) {
    std::cout << "  " << database[ speciesIndex ].name::value <<
      boost::format( " %6.1f K" ) % speciesTemperatures[ speciesIndex ];
  }
  std::cout << std::endl;
}
