#ifndef Particle_h
#define Particle_h

#include <Vec3.h>

// A very simple particle struct. We have no need for positions here, and due to the way in which
// these are stored/manipulated, there is no need to keep a particle name or index inside the
// struct.
class Particle
{
public:
  Vec3 velocity;
};


// A proxy for use when passing particle information to/from the reaction database.
struct ParticleProxy
{
public:
  ParticleProxy() :
    mBase( NULL ), mSpeciesIndex( -1 )
  {
  }

  ParticleProxy( Particle& p ) :
    mBase( &p ), mSpeciesIndex( -1 )
  {
  }

  void pointAt( Particle& p )
  {
    mBase = &p;
  }

  const Vec3& velocity() const
  {
    return mBase->velocity;
  }

  Vec3& velocity()
  {
    return mBase->velocity;
  }

  const int& speciesIndex() const
  {
    return mSpeciesIndex;
  }

  int& speciesIndex()
  {
    return mSpeciesIndex;
  }

private:
  Particle* mBase;
  int mSpeciesIndex;
};

// Particle initial specifications.
struct ParticleInputSpec {
  std::size_t count;
  double temperatureInKelvin;
  Vec3 driftVelocity;
};

#endif // Particle_h
